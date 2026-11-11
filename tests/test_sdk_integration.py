#!/usr/bin/env python3
"""Host-only SDK project injection test using a deliberately incomplete mock SDK.
A mock SDK validates paths and Makefile integration, NOT a genuine native build.
"""
import pathlib
import os
import shutil
import subprocess
import sys
import tempfile

PROJ = pathlib.Path(__file__).resolve().parents[1]
TOOL = PROJ / 'scripts/integrate_amebad_sdk.py'

AM = '''#ifndef STUB_AMEBA_SOC_H
#define STUB_AMEBA_SOC_H
#include <stdint.h>
typedef uint32_t u32;
typedef struct { uint32_t mock; } IR_TypeDef;
typedef struct { uint32_t IR_Mode, IR_Freq, IR_DutyCycle, IR_RxFIFOThrLevel, IR_RxCntThrType, IR_RxCntThr; } IR_InitTypeDef;
#define IR_MODE_RX (1u<<31)
#define IR_RX_COUNT_LOW_LEVEL 0u
#define portTICK_PERIOD_MS 1u
u32 xTaskGetTickCount(void);
#define IR_DEV ((IR_TypeDef*)0)
#define IR_MODE_TX 0u
#define TRUE 1u
#define FALSE 0u
#define ENABLE 1u
#define DBG_8195A(...) do {} while (0)
u32 IR_GetTxFIFOFreeLen(IR_TypeDef *ir);
void IR_SendBuf(IR_TypeDef *ir, u32 *words, u32 count, u32 final);
void IR_Cmd(IR_TypeDef *ir, u32 mode, u32 state);
u32 IR_FSMRunning(IR_TypeDef *ir);
u32 IR_GetRxDataLen(IR_TypeDef *ir);
u32 IR_ReceiveData(IR_TypeDef *ir);
u32 IR_GetINTStatus(IR_TypeDef *ir);
u32 IR_GetIMR(IR_TypeDef *ir);
void IR_MaskINTConfig(IR_TypeDef *ir, u32 mask, u32 state);
void IR_ClearINTPendingBit(IR_TypeDef *ir, u32 mask);
void IR_ClearRxFIFO(IR_TypeDef *ir);
#define DISABLE 0u
#define IR_RX_FIFO_FULL_INT_STATUS (1u<<0)
#define IR_RX_FIFO_LEVEL_INT_STATUS (1u<<1)
#define IR_RX_CNT_OF_INT_STATUS (1u<<2)
#define IR_RX_FIFO_OF_INT_STATUS (1u<<3)
#define IR_RX_CNT_THR_INT_STATUS (1u<<4)
#define IR_RX_FIFO_ERROR_INT_STATUS (1u<<5)
#define IR_RX_INT_ALL_MASK 0x3fu
#define IR_RX_FIFO_FULL_INT_CLR IR_RX_FIFO_FULL_INT_STATUS
#define IR_RX_FIFO_LEVEL_INT_CLR IR_RX_FIFO_LEVEL_INT_STATUS
#define IR_RX_CNT_OF_INT_CLR IR_RX_CNT_OF_INT_STATUS
#define IR_RX_FIFO_OF_INT_CLR IR_RX_FIFO_OF_INT_STATUS
#define IR_RX_CNT_THR_INT_CLR IR_RX_CNT_THR_INT_STATUS
#define IR_RX_FIFO_ERROR_INT_CLR IR_RX_FIFO_ERROR_INT_STATUS
void IR_StructInit(IR_InitTypeDef *cfg);
void IR_Init(IR_TypeDef *ir, IR_InitTypeDef *cfg);
void IR_ClearTxFIFO(IR_TypeDef *ir);
#endif
'''

MAKE = '''include $(MAKE_INCLUDE_GEN)
DIR = $(SRCDIR)
MODULE_IFLAGS = -I$(DIR)./
vpath %.c $(DIR)
CSRC = $(DIR)/main.c
OBJS = $(notdir $(CSRC:.c=.o))
all: CORE_TARGETS COPY_RAM_OBJS
CORE_TARGETS: $(OBJS)
clean: CLEAN_OBJS
'''
INC = '''SRCDIR = $(CURDIR)
MAKE_INCLUDE_GEN = $(CURDIR)/../../GCC-RELEASE/project_hp/asdk/Makefile.include.gen
GLOBAL_CFLAGS = -std=c11 -Wall -Wextra -Werror -I$(CURDIR)/include -c
CC = gcc
COPY_RAM_OBJS:
\t@true
CLEAN_OBJS:
\t@rm -f *.o
%.o: %.c
\t$(CC) $(GLOBAL_CFLAGS) $(MODULE_IFLAGS) $< -o $@
'''

def call(tool,root,mode):
    return subprocess.run([sys.executable,str(tool),'--sdk',str(root),mode],capture_output=True,text=True)

with tempfile.TemporaryDirectory(prefix='gw006_sdk_mock_') as td:
    root=pathlib.Path(td)
    project=root/'project/realtek_amebaD_va0_example'
    main=project/'src/src_hp/main.c'
    mf=project/'src/src_hp/Makefile'
    inc=project/'GCC-RELEASE/project_hp/asdk/Makefile.include.gen'
    hdr=root/'component/soc/realtek/amebad/fwlib/include/rtl8721d_ir.h'
    driver=root/'component/soc/realtek/amebad/fwlib/ram_common/rtl8721d_ir.c'
    for path in (main,mf,inc,hdr,driver): path.parent.mkdir(parents=True,exist_ok=True)
    (main.parent/'include').mkdir(exist_ok=True)
    (main.parent/'include/ameba_soc.h').write_text(AM)
    main.write_text('#include "ameba_soc.h"\nint main(void) {\n\tvTaskStartScheduler();\n}\n')
    # Provide SDK mock scheduler prototype for -Werror
    (main.parent/'include/ameba_soc.h').write_text(AM.replace('void IR_ClearTxFIFO(IR_TypeDef *ir);', 'void IR_ClearTxFIFO(IR_TypeDef *ir);\nvoid vTaskStartScheduler(void);'))
    mf.write_text(MAKE)
    inc.write_text(INC)
    hdr.write_text('#define SDK_REALTEK_IR_HEADER 1\n')
    driver.write_text('/* stub vendor source */\n')
    originals=(main.read_bytes(),mf.read_bytes())
    dry=call(TOOL,root,'--dry-run'); assert dry.returncode==0,dry.stderr
    assert originals==(main.read_bytes(),mf.read_bytes()), 'Dry run modified files'
    first=call(TOOL,root,'--apply');assert first.returncode==0,first.stderr
    assert 'GWIR_REV9_START' in main.read_text()
    assert main.read_text().find('gwir_sdk_integration_banner();') < main.read_text().find('vTaskStartScheduler();')
    assert 'gwir_amebad_rx_vendor.c' in mf.read_text()
    after=(main.read_bytes(),mf.read_bytes())
    twice=call(TOOL,root,'--apply');assert twice.returncode==0,twice.stderr
    assert after==(main.read_bytes(),mf.read_bytes()), 'Integration not idempotent'
    env=os.environ.copy(); env['MAKE_INCLUDE_GEN']=str(inc); env['SRCDIR']=str(main.parent)
    make=subprocess.run(['make','-C',str(main.parent),'all'],env=env,capture_output=True,text=True)
    assert make.returncode==0,make.stdout+'\n'+make.stderr
    assert len(list(main.parent.glob('*.o')))==14,'One of 14 SDK objects was not compiled'
    remove=call(TOOL,root,'--remove');assert remove.returncode==0,remove.stderr
    if originals!=(main.read_bytes(),mf.read_bytes()):
        print('Expected:',[repr(x) for x in originals]);print('Actual:',[repr(x) for x in (main.read_bytes(),mf.read_bytes())]);raise AssertionError('Removal did not restore original source')
    assert not (project/'src/gwir_sdk').exists(), 'Removal did not remove copied source'
    # conflict detection: refuse overwrite of user's unowned file
    (project/'src/gwir_sdk/core').mkdir(parents=True)
    (project/'src/gwir_sdk/core/gwir_protocol.c').write_text('user own data')
    before=(main.read_bytes(),mf.read_bytes())
    conflict=call(TOOL,root,'--apply');assert conflict.returncode!=0
    assert before==(main.read_bytes(),mf.read_bytes()), 'Collision corrupted SDK project'
    print('PASS: dry-run, apply, idempotence, 14 mocked-SDK objects, rollback, collision refusal')
