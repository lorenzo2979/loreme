/* Cross-language GWCD test: binary written by browser JS, parsed by the
 * exact pinned C production reader. Runs on host, not on the GW006 board. */
#include "gwir_gwcd_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    uint8_t data[GWIR_GWCD_MAX_SIZE];
    size_t len;
} memrecord;
static int read_bytes(void *context,uint32_t off,void *out,size_t count){
    const memrecord *r=(const memrecord*)context;
    if((size_t)off>r->len||count>r->len-(size_t)off)return -1;
    memcpy(out,r->data+off,count);return 0;
}
int main(int argc,char **argv){
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");
    assert(f);
    memrecord *r=calloc(1,sizeof(*r));
    assert(r);
    r->len=fread(r->data,1,sizeof(r->data),f);
    assert(feof(f));
    assert(fclose(f)==0);
    assert(r->len==41040u);
    assert(gwir_gwcd_worst_case_bytes()==96u*41040u);
    gwir_gwcd_index idx;
    assert(gwir_gwcd_scan(read_bytes,r,(uint32_t)r->len,95u,&idx)==0);
    assert(idx.button==95u&&idx.frame_count==5u&&idx.serialized_bytes==41040u);
    for(unsigned i=0;i<5;i++){
        assert(idx.frame[i].pulse_count==2048u);
        uint32_t *p=calloc(2048u,sizeof(uint32_t));assert(p);
        assert(gwir_gwcd_read_frame(read_bytes,r,&idx,(uint8_t)i,p,2048u)==0);
        assert(p[0]==1000u&&p[2047]==3047u);
        free(p);
    }
    r->data[20]^=1u;
    assert(gwir_gwcd_scan(read_bytes,r,(uint32_t)r->len,95u,&idx)==-3);
    r->data[20]^=1u;
    assert(gwir_gwcd_scan(read_bytes,r,(uint32_t)r->len,94u,&idx)==-4);
    assert(gwir_gwcd_scan(read_bytes,r,(uint32_t)r->len-1u,95u,&idx)!=0);
    free(r);
    puts("GW006_JS_GWCD_TO_PINNED_C_READER=PASS; 5 x 2048; NO FLASH");
    return 0;
}
