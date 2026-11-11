/* Linux/POSIX loopback-only protocol test harness. NOT target firmware. */
#include "gwir_engine.h"
#include "gwir_stream.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static gwir_engine engine;
static uint8_t response[GWIR_MAX_MESSAGE];
static uint32_t mock_rx[4]={9000,4500,560,560};
static int tx(void *ctx, uint32_t hz, uint16_t duty, const uint32_t *d, uint16_t n, uint32_t gap) {
    (void)ctx;(void)gap;
    return !(hz==38000 && duty==333 && n==4 && d[0]==9000 && d[1]==4500);
}
static int rx_on(void *ctx) {(void)ctx;return 0;}
static int rx_off(void *ctx) {(void)ctx;return 0;}
struct session {int fd;};
static int send_all(int fd, const uint8_t *data, size_t size) {
    size_t sent=0;
    while (sent<size) {
        ssize_t n=send(fd,data+sent,size-sent,0);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) return -1;
        sent+=(size_t)n;
    }
    return 0;
}
static int dispatch(void *ctx, const uint8_t *message, size_t len) {
    struct session *s=(struct session*)ctx;
    gwir_message m;
    if (gwir_decode(message,len,&m)) return -1;
    size_t out_size=0;
    if (gwir_engine_process(&engine,message,len,response,sizeof(response),&out_size)) return -1;
    /* Simulate a known IR frame arriving after the receiver was armed. */
    gwir_message reply;
    if (gwir_decode(response,out_size,&reply)) return -1;
    if (m.command==GWIR_RX_ARM && reply.command==GWIR_STATUS &&
        reply.payload_len==1 && reply.payload[0]==GWIR_OK &&
        gwir_engine_capture(&engine,38000,333,mock_rx,4,25000)) return -1;
    return send_all(s->fd,response,out_size);
}
int main(void) {
    int sock=socket(AF_INET,SOCK_STREAM,0);assert(sock>=0);
    struct sockaddr_in local={0};
    local.sin_family=AF_INET; local.sin_port=0;
    local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(sock,(struct sockaddr*)&local,sizeof(local))==0);
    socklen_t namelen=sizeof(local);
    assert(getsockname(sock,(struct sockaddr*)&local,&namelen)==0);
    assert(listen(sock,1)==0);
    printf("%u\n",(unsigned)ntohs(local.sin_port)); fflush(stdout);
    gwir_ir_hal hal={tx,rx_on,rx_off,NULL};
    gwir_engine_init(&engine,hal);
    for (int connection=0;connection<2;connection++) {
        int client=accept(sock,NULL,NULL); assert(client>=0);
        struct session session={client};
        static gwir_stream stream;
        gwir_stream_init(&stream);
        uint8_t buffer[1024];
        for (;;) {
            ssize_t n=recv(client,buffer,sizeof(buffer),0);
            if (n<0 && errno==EINTR) continue;
            if (n<=0) break;
            if (gwir_stream_feed(&stream,buffer,(size_t)n,dispatch,&session)<0) break;
        }
        close(client);
    }
    close(sock);
    return 0;
}
