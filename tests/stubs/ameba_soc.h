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

/* Realtek IRQ API mock; distinct test-only flags, NOT hardware bits. */
#define DISABLE 0u
#define IR_RX_FIFO_FULL_INT_STATUS  (1u << 0)
#define IR_RX_FIFO_LEVEL_INT_STATUS (1u << 1)
#define IR_RX_CNT_OF_INT_STATUS     (1u << 2)
#define IR_RX_FIFO_OF_INT_STATUS    (1u << 3)
#define IR_RX_CNT_THR_INT_STATUS    (1u << 4)
#define IR_RX_FIFO_ERROR_INT_STATUS (1u << 5)
#define IR_RX_INT_ALL_MASK 0x3fu
#define IR_RX_FIFO_FULL_INT_CLR    IR_RX_FIFO_FULL_INT_STATUS
#define IR_RX_FIFO_LEVEL_INT_CLR   IR_RX_FIFO_LEVEL_INT_STATUS
#define IR_RX_CNT_OF_INT_CLR       IR_RX_CNT_OF_INT_STATUS
#define IR_RX_FIFO_OF_INT_CLR      IR_RX_FIFO_OF_INT_STATUS
#define IR_RX_CNT_THR_INT_CLR      IR_RX_CNT_THR_INT_STATUS
#define IR_RX_FIFO_ERROR_INT_CLR   IR_RX_FIFO_ERROR_INT_STATUS
u32 IR_GetINTStatus(IR_TypeDef *ir);
u32 IR_GetIMR(IR_TypeDef *ir);
void IR_MaskINTConfig(IR_TypeDef *ir, u32 mask, u32 state);
void IR_ClearINTPendingBit(IR_TypeDef *ir, u32 mask);
void IR_ClearRxFIFO(IR_TypeDef *ir);
#endif
