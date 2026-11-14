/* GW006 CP40: a SINGLE serialized STA socket owner for raw GWAS and browser
 * HTTP GWAS. This is a review-only board port: no key, pin or flash is
 * provisioned by this source. Never expose unauthenticated IR commands.
 */
#include "gwir_amebad_auth_server.h"
#include "../core/gwir_web_rpc.h"
#include "../core/gwir_web_http_post.h"
#include "../core/gwir_dashboard_http.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#if defined(GWIR_USE_LWIP)
#include <lwip/sockets.h>
#define GWIR_AUTH_CLOSE lwip_close
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#define GWIR_AUTH_CLOSE close
#endif

/* Exactly one listener/task is permitted. No allocation on a small FreeRTOS
 * stack. Both buffers are owned by the single blocking GWIR-AUTH task. */
static uint8_t http_request[GWIR_WEB_HTTP_HEADER_MAX + GWIR_AUTH_MAX_PACKET];
static uint8_t http_signed_answer[GWIR_AUTH_MAX_PACKET];

static int allowed(const gwir_amebad_auth_server_config *c)
{
    return c && c->owner && c->owner->initialized &&
        c->owner->rx.active && c->owner->tx.active &&
        c->stop && !*c->stop && c->check_sta &&
        c->sta_ip_network_order && c->sta_ip_network_order != UINT32_MAX &&
        c->port && c->check_sta(c->ctx, c->sta_ip_network_order) == 1;
}
static int set_deadline(int fd, int which, int ms)
{
#if defined(GWIR_USE_LWIP) && defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    return setsockopt(fd, SOL_SOCKET, which, (const char *)&ms, sizeof(ms));
#else
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return setsockopt(fd, SOL_SOCKET, which, (const char *)&tv, sizeof(tv));
#endif
}
typedef struct {
    int fd;
    const gwir_amebad_auth_server_config *cfg;
} writer_ctx;
static int send_whole_signed(void *context, const uint8_t *data, size_t len)
{
    writer_ctx *w = (writer_ctx *)context;
    size_t pos = 0u;
    if (!data || !len || len > GWIR_AUTH_MAX_PACKET) return -1;
    while (pos < len) {
        int count;
        if (!allowed(w->cfg)) return -1;
        count = (int)send(w->fd, (const char *)data + pos, len - pos, 0);
        if (count <= 0 || (size_t)count > len - pos) return -1;
        pos += (size_t)count;
    }
    return 0;
}
static int send_some(void *context, const uint8_t *data, size_t n)
{
    writer_ctx *w = (writer_ctx *)context;
    if (!data || !n || !allowed(w->cfg)) return -1;
    return (int)send(w->fd, (const char *)data, n, 0);
}
/* Bind Host/Origin to the exact IPv4:port of the authenticated STA listener.
 * There are no wildcards, public DNS names or cross-origin requests here. */
static int local_authority(const gwir_amebad_auth_server_config *cfg,
                           char *out, size_t cap)
{
    const uint32_t host = ntohl(cfg->sta_ip_network_order);
    int n = snprintf(out, cap, "%lu.%lu.%lu.%lu:%u",
        (unsigned long)((host >> 24u) & 0xffu),
        (unsigned long)((host >> 16u) & 0xffu),
        (unsigned long)((host >> 8u) & 0xffu),
        (unsigned long)(host & 0xffu), (unsigned)cfg->port);
    return (n > 0 && (size_t)n < cap) ? 0 : -1;
}
static int header_end(const uint8_t *buf, size_t n, size_t *out)
{
    for (size_t i = 3u; i < n; ++i) {
        if (buf[i-3u]=='\r' && buf[i-2u]=='\n' &&
            buf[i-1u]=='\r' && buf[i]=='\n') {
            *out = i + 1u;
            return 1;
        }
    }
    return 0;
}
/* Static GET page contains no secrets. Its CSP presently forbids fetch:
 * it is only a shell until the WebCrypto UI is implemented separately.
 * All state-changing commands MUST use signed POST; no raw public endpoint.
 */
static int handle_web_get(int fd, const gwir_amebad_auth_server_config *cfg,
                          const uint8_t prefix[4])
{
    writer_ctx w = {fd, cfg};
    gwir_dash_http_connection req;
    uint8_t buf[256];
    int status;
    gwir_dash_http_init(&req);
    status = gwir_dash_http_feed(&req, prefix, 4u);
    for (unsigned i=0u; i<8u && status == 0 && allowed(cfg); ++i) {
        if (cfg->pump_ir && cfg->owner->engine->rx_armed) return -1;
        int n = (int)recv(fd, (char *)buf, sizeof(buf), 0);
        if (n <= 0) return -1;
        status = gwir_dash_http_feed(&req, buf, (size_t)n);
    }
    if (status != 1 || !allowed(cfg)) return -1;
    return gwir_dash_http_respond(&req, send_some, &w);
}
/* One signed POST per HTTP request, directly on GWIR-AUTH's serialized task.
 * The HTTP server never calls the unguarded IR engine. */
static int handle_web_post(int fd, const gwir_amebad_auth_server_config *cfg,
                           const uint8_t prefix[4])
{
    writer_ctx w = {fd, cfg};
    char authority[64], header[256];
    size_t used = 4u, hdr = 0u, body_size = 0u, reply_bytes = 0u;
    size_t total = 0u;
    int n;
    if (local_authority(cfg, authority, sizeof(authority)) != 0 ||
        (cfg->pump_ir && cfg->owner->engine->rx_armed)) return -1;
    memcpy(http_request, prefix, 4u);
    for (unsigned i=0u; i<80u && allowed(cfg); ++i) {
        if (header_end(http_request,used,&hdr)) {
            if (hdr > GWIR_WEB_HTTP_HEADER_MAX ||
                gwir_web_http_post_header(http_request,hdr,authority,
                                          &body_size) != 0) return -1;
            total = hdr + body_size;
            if (used > total || total > sizeof(http_request)) return -1;
            if (used == total) break;
        } else if (used >= GWIR_WEB_HTTP_HEADER_MAX) return -1;
        size_t cap = sizeof(http_request)-used;
        if (total && cap > total-used) cap = total-used;
        if (cap > 256u) cap = 256u;
        if (!cap) return -1;
        n = (int)recv(fd,(char *)http_request+used,cap,0);
        if (n <= 0 || (size_t)n > cap) return -1;
        used += (size_t)n;
    }
    if (!total || used != total || !allowed(cfg)) return -1;
    if (gwir_web_rpc_process(cfg->owner,http_request+hdr,body_size,
                             http_signed_answer,sizeof(http_signed_answer),
                             &reply_bytes) != 0) return -1;
    n = snprintf(header,sizeof(header),
       "HTTP/1.1 200 OK\r\n"
       "Content-Type: application/octet-stream\r\n"
       "Content-Length: %lu\r\n"
       "Cache-Control: no-store\r\n"
       "X-Content-Type-Options: nosniff\r\n"
       "Access-Control-Allow-Origin: http://%s\r\n"
       "Vary: Origin\r\nConnection: close\r\n\r\n",
       (unsigned long)reply_bytes,authority);
    if (n <= 0 || (size_t)n >= sizeof(header) ||
        send_whole_signed(&w,(const uint8_t *)header,(size_t)n) != 0 ||
        send_whole_signed(&w,http_signed_answer,reply_bytes) != 0)
        return -1;
    return 0;
}
int gwir_amebad_auth_handle_client(int fd,
                                   const gwir_amebad_auth_server_config *cfg)
{
    writer_ctx w;
    uint8_t incoming[256], prefix[4];
    unsigned reads;
    int result = -1;
    size_t first = 0u;
    if (fd < 0 || !allowed(cfg) || cfg->owner->connected ||
        set_deadline(fd,SO_RCVTIMEO,cfg->pump_ir ? 20 : 500) != 0 ||
        set_deadline(fd,SO_SNDTIMEO,500) != 0) return -1;
    /* TCP may split even the 4-byte prefix. Determine framing without
     * consuming a GWAS record via an HTTP parser or resetting replay state. */
    for (unsigned i=0; i<8u && first<sizeof(prefix) && allowed(cfg); ++i) {
        if (cfg->pump_ir && cfg->owner->engine->rx_armed &&
            cfg->pump_ir(cfg->ir_ctx,128u) < 0) return -1;
        int n = (int)recv(fd,(char *)prefix+first,sizeof(prefix)-first,0);
        if (n < 0 && cfg->pump_ir &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            continue;
        if (n <= 0 || (size_t)n > sizeof(prefix)-first) return -1;
        first += (size_t)n;
    }
    if (first != sizeof(prefix)) return -1;
    if (memcmp(prefix,"GET ",4u)==0)
        return handle_web_get(fd,cfg,prefix);
    if (memcmp(prefix,"POST",4u)==0)
        return handle_web_post(fd,cfg,prefix);
    if (memcmp(prefix,"GWAS",4u)!=0) return -1;
    if (gwir_runtime_owner_connect(cfg->owner) != 0) return -1;
    w.fd=fd;
    w.cfg=cfg;
    if (gwir_runtime_owner_feed(cfg->owner,prefix,sizeof(prefix),
                                send_whole_signed,&w) < 0) goto done;
    /* Raw GWAS TCP remains supported for tools until the browser UI is
     * ready. Both transports share ONE owner, nonce and anti-replay counters. */
    for (reads=0u; reads<(cfg->pump_ir ? 256u:64u) && allowed(cfg); ++reads) {
        if (cfg->pump_ir && cfg->owner->engine->rx_armed &&
            cfg->pump_ir(cfg->ir_ctx,128u)<0) break;
        int n=(int)recv(fd,(char*)incoming,sizeof(incoming),0);
        if (n < 0 && cfg->pump_ir &&
            (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            continue;
        if (n <= 0) break;
        int fed=gwir_runtime_owner_feed(cfg->owner,incoming,(size_t)n,
                                         send_whole_signed,&w);
        if (fed<0) break;
        if (fed>0) result=0;
    }
done:
    memset(incoming,0,sizeof(incoming));
    gwir_runtime_owner_disconnect(cfg->owner);
    return result;
}
int gwir_amebad_auth_server_run(const gwir_amebad_auth_server_config *cfg)
{
    struct sockaddr_in addr;
    int listener, client, one=1;
    if (!allowed(cfg) || cfg->owner->connected) return -1;
    listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (listener<0) return -2;
    if (set_deadline(listener,SO_RCVTIMEO,cfg->pump_ir ? 20 : 500)!=0) {
        GWIR_AUTH_CLOSE(listener);
        return -3;
    }
    (void)setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,
                     (const char*)&one,sizeof(one));
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=cfg->sta_ip_network_order;
    addr.sin_port=htons(cfg->port);
    if (bind(listener,(struct sockaddr*)&addr,sizeof(addr))!=0 ||
        listen(listener,1)!=0) {
        GWIR_AUTH_CLOSE(listener);
        return -4;
    }
    while (allowed(cfg)) {
        if (cfg->pump_ir && cfg->owner->engine->rx_armed &&
            cfg->pump_ir(cfg->ir_ctx,128u)<0) break;
        client=accept(listener,NULL,NULL);
        if (client<0) continue;
        if (allowed(cfg))
            (void)gwir_amebad_auth_handle_client(client,cfg);
        GWIR_AUTH_CLOSE(client);
    }
    GWIR_AUTH_CLOSE(listener);
    return 0;
}
