#include "gwir_web_http_post.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const char valid[] =
    "POST /api/gwir HTTP/1.1\r\n"
    "Host: 192.168.1.7:8080\r\n"
    "Origin: http://192.168.1.7:8080\r\n"
    "Content-Type: application/octet-stream\r\n"
    "Content-Length: 88\r\n"
    "\r\n";
static void reject(const char *s) {
    size_t n=99;
    assert(gwir_web_http_post_header((const uint8_t*)s,strlen(s),
           "192.168.1.7:8080",&n)!=0 && n==0);
}
int main(void) {
    size_t len=0;
    assert(gwir_web_http_post_header((const uint8_t*)valid,strlen(valid),
        "192.168.1.7:8080",&len)==0 && len==88u);
    reject("GET /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nContent-Length: 88\r\nContent-Type: application/octet-stream\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nOrigin: http://evil.invalid\r\nContent-Type: application/octet-stream\r\nContent-Length: 88\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nOrigin: http://192.168.1.7:8080\r\nContent-Type: application/octet-stream\r\nContent-Length: 88\r\nContent-Length: 88\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nOrigin: http://192.168.1.7:8080\r\nContent-Type: application/octet-stream\r\nTransfer-Encoding: chunked\r\nContent-Length: 88\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nOrigin: http://192.168.1.7:8080\r\nContent-Type: application/octet-stream\r\nContent-Length: 99999999999999999999\r\n\r\n");
    reject("POST /api/gwir HTTP/1.1\r\nHost: 192.168.1.7:8080\r\nOrigin: http://192.168.1.7:8080\r\nContent-Type: application/octet-stream\r\nContent-Length: 88\r\nX-Test: hi\n\r\n");
    puts("GW006_WEB_HTTP_POST_STRICT_HEADER=PASS; NO NETWORK/FLASH");
    return 0;
}
