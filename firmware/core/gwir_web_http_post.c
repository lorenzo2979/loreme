#include "gwir_web_http_post.h"
#include <string.h>
#include <limits.h>

static int eq_ci(const uint8_t *s, size_t len, const char *word)
{
    size_t i = 0u;
    while (i < len && word[i]) {
        uint8_t a = s[i], b = (uint8_t)word[i];
        if (a >= 'A' && a <= 'Z') a = (uint8_t)(a+32u);
        if (b >= 'A' && b <= 'Z') b = (uint8_t)(b+32u);
        if (a != b) return 0;
        ++i;
    }
    return i == len && word[i] == 0;
}
static int exact(const uint8_t *p, size_t n, const char *literal)
{
    return strlen(literal) == n && memcmp(p, literal, n) == 0;
}
static int find_crlf(const uint8_t *v, size_t size, size_t at, size_t *end)
{
    for (size_t i = at; i + 1u < size; ++i) {
        if (v[i] == '\r' && v[i+1u] == '\n') {
            *end = i;
            return 1;
        }
        if (v[i] == '\r' || v[i] == '\n') return 0;
    }
    return 0;
}
static int parse_positive_length(const uint8_t *p, size_t n, size_t *out)
{
    size_t total = 0u;
    if (!n || (n > 1u && p[0] == '0')) return 0;
    for (size_t i = 0u; i < n; i++) {
        if (p[i] < '0' || p[i] > '9') return 0;
        size_t digit = (size_t)(p[i]-'0');
        if (total > (GWIR_AUTH_MAX_PACKET-digit)/10u) return 0;
        total = total*10u + digit;
    }
    if (total < GWIR_AUTH_HEADER_BYTES + GWIR_HEADER_SIZE + GWIR_AUTH_TAG_BYTES ||
        total > GWIR_AUTH_MAX_PACKET) return 0;
    *out = total;
    return 1;
}
int gwir_web_http_post_header(const uint8_t *h, size_t n,
                             const char *authority, size_t *body_bytes)
{
    static const char method[] = "POST /api/gwir HTTP/1.1";
    size_t at = 0u, end, value_start, value_end, lines = 0u;
    size_t length = 0u;
    unsigned host = 0u, origin = 0u, ctype = 0u, clen = 0u;
    size_t authority_n = 0u;
    if (body_bytes) *body_bytes = 0u;
    if (!h || !authority || !body_bytes || n > GWIR_WEB_HTTP_HEADER_MAX ||
        n < sizeof(method)+2u+4u ||
        h[n-4u] != '\r' || h[n-3u] != '\n' ||
        h[n-2u] != '\r' || h[n-1u] != '\n') return -1;
    while (authority_n <= 80u && authority[authority_n]) {
        if ((uint8_t)authority[authority_n] <= 32u ||
            (uint8_t)authority[authority_n] >= 127u ||
            authority[authority_n] == '/' || authority[authority_n] == '@')
            return -1;
        authority_n++;
    }
    if (authority_n == 0u || authority_n > 80u) return -1;
    if (!find_crlf(h, n, at, &end) || !exact(h, end, method)) return -1;
    at = end + 2u;
    while (at < n-2u) {
        size_t colon = at;
        if (++lines > 24u || !find_crlf(h,n,at,&end) || end == at)
            return -1;
        for (size_t i = at; i < end; ++i) {
            if (h[i] < 32u || h[i] == 127u) return -1;
        }
        while (colon < end && h[colon] != ':') ++colon;
        if (colon == end || colon == at) return -1;
        value_start = colon + 1u;
        while (value_start < end && h[value_start] == ' ') ++value_start;
        value_end = end;
        while (value_end > value_start && h[value_end - 1u] == ' ')
            --value_end;
        if (eq_ci(h+at,colon-at,"Host")) {
            if (++host != 1u || value_end-value_start != authority_n ||
                memcmp(h+value_start,authority,authority_n)) return -1;
        } else if (eq_ci(h+at,colon-at,"Origin")) {
            if (++origin != 1u || value_end-value_start != 7u+authority_n ||
                memcmp(h+value_start,"http://",7u) ||
                memcmp(h+value_start+7u,authority,authority_n)) return -1;
        } else if (eq_ci(h+at,colon-at,"Content-Type")) {
            if (++ctype != 1u || !exact(h+value_start,
                value_end-value_start,"application/octet-stream")) return -1;
        } else if (eq_ci(h+at,colon-at,"Content-Length")) {
            if (++clen != 1u || !parse_positive_length(h+value_start,
                value_end-value_start,&length)) return -1;
        } else if (eq_ci(h+at,colon-at,"Transfer-Encoding") ||
                   eq_ci(h+at,colon-at,"Expect") ||
                   eq_ci(h+at,colon-at,"Content-Encoding") ||
                   eq_ci(h+at,colon-at,"Upgrade") ||
                   eq_ci(h+at,colon-at,"Authorization") ||
                   eq_ci(h+at,colon-at,"Cookie") ||
                   eq_ci(h+at,colon-at,"Proxy-Connection")) {
            return -1;
        }
        at = end + 2u;
    }
    if (at != n-2u || host != 1u || origin != 1u ||
        ctype != 1u || clen != 1u) return -1;
    *body_bytes = length;
    return 0;
}
