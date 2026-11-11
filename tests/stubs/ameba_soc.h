#ifndef GWIR_TEST_AMEBA_SOC_H
#define GWIR_TEST_AMEBA_SOC_H
/* Build-only shim for testing vendor API signatures. It is NOT the SDK. */
#include <stdint.h>
typedef uint32_t u32;
typedef struct { uint32_t _mock; } IR_TypeDef;
extern IR_TypeDef gwir_ir_test_device;
#define IR_DEV (&gwir_ir_test_device)
#define IR_MODE_TX 0u
#define TRUE 1u
#define FALSE 0u
#define ENABLE 1u
u32 IR_GetTxFIFOFreeLen(IR_TypeDef *ir);
void IR_SendBuf(IR_TypeDef *ir, u32 *words, u32 count, u32 final);
void IR_Cmd(IR_TypeDef *ir, u32 mode, u32 state);
u32 IR_FSMRunning(IR_TypeDef *ir);
u32 IR_GetRxDataLen(IR_TypeDef *ir);
u32 IR_ReceiveData(IR_TypeDef *ir);
#endif
