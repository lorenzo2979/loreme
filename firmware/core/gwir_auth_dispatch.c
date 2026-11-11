#include "gwir_auth_dispatch.h"

int gwir_auth_dispatch(gwir_auth_session *rx, gwir_auth_session *tx,
                       gwir_engine *engine, const uint8_t *request, size_t req_size,
                       uint8_t *scratch, size_t scratch_cap,
                       uint8_t *response, size_t response_cap,
                       size_t *response_len) {
    if (response_len) *response_len=0u;
    /* Sufficient capacity must be reserved BEFORE any side effect (TX/ACK).
       No processing of unsigned packets or on undersized response storage. */
    if (!rx || !tx || !engine || !request || !scratch || !response ||
        !response_len || scratch_cap<GWIR_MAX_MESSAGE ||
        response_cap<GWIR_AUTH_MAX_PACKET ||
        rx==tx || scratch==response || request==response || request==scratch)
        return GWIR_AUTH_REJECT;
    const uint8_t *inner=NULL;
    size_t inner_len=0u;
    int status=gwir_auth_unwrap(rx,GWIR_AUTH_TO_DEVICE,request,req_size,
                                &inner,&inner_len);
    if (status != GWIR_AUTH_OK) return status;
    size_t unsigned_len=0u;
    if (gwir_engine_process(engine,inner,inner_len,scratch,scratch_cap,
                            &unsigned_len)) return GWIR_AUTH_REJECT;
    /* Signing failure must NEVER cause unsigned response to be sent.
       The caller must not repeat a TX command merely because an ACK was lost. */
    status=gwir_auth_wrap(tx,GWIR_AUTH_FROM_DEVICE,scratch,unsigned_len,
                          response,response_cap,response_len);
    return status;
}
