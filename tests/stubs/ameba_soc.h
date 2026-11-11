#ifndef GWIR_TEST_AMEBA_SOC_H
#define GWIR_TEST_AMEBA_SOC_H
/* Build-only shim for testing vendor API signatures. It is NOT the SDK. */
#include <stdint.h>
typedef uint32_t u32;
#define IR_MODE_RX (1u << 31)
#define IR_RX_COUNT_LOW_LEVEL 0u
#define portTICK_PERIOD_MS 1u
u32 xTaskGetTickCount(void);
typedef struct { u32 IR_Mode, IR_Freq, IR_DutyCycle, IR_RxFIFOThrLevel, IR_RxCntThrType, IR_RxCntThr; } IR_InitTypeDef;
void IR_StructInit(IR_InitTypeDef *c);
typedef struct { uint32_t _mock; } IR_TypeDef;
void IR_Init(IR_TypeDef *ir, IR_InitTypeDef *c);
void IR_ClearTxFIFO(IR_TypeDef *ir);
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

/* Realtek IRQ API mock; values are distinct flags, not real chip register bits. */
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
