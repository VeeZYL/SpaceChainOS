;/*********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: mipsContextAsm.S
;**
;** ��   ��   ��: Ryan.Xin (�Ž���)
;**
;** �ļ���������: 2015 �� 09 �� 01 ��
;**
;** ��        ��: MIPS ��ϵ�����������л�.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "mipsContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archGetGP)
    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  ��� _gp
;*********************************************************************************************************/

FUNC_DEF(archGetGP)
    LA      V0 , _gp
    JR      RA
    NOP
    FUNC_END(archGetGP)

;/*********************************************************************************************************
;  ��ǰ CPU ��ʼ���ж�����ִ��
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      T0 , 0(A0)                                                  ;/*  ��ȡ�������ջջ����ַ      */
    LW      SP , 0(T0)                                                  ;/*  ��ջָ�븳ֵ                */

    RESTORE_REGS                                                        ;/*  ���мĴ�����ջ              */

    .set    pop
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  �߳��л�
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SAVE_REGS                                                           ;/*  ���мĴ�����ջ              */

    LW      T0 , 0(A0)                                                  ;/*  stack current = SP          */
    SW      SP , 0(T0)

#if LW_CFG_SMP_EN > 0
    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    JAL     _SchedSafeStack                                             ;/*  _SchedSafeStack();          */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    MOV     SP , V0
#endif

    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    JAL     _SchedSwp                                                   ;/*  _SchedSwp();                */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    ADDU    SP , 32

    LW      T0 , 0(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  ���мĴ�����ջ              */

    .set    pop
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  Э���л�
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    SAVE_REGS                                                           ;/*  ���мĴ�����ջ              */

    LW      T0 , 8(A0)                                                  ;/*  stack current = SP          */
    SW      SP , 0(T0)

    SUBU    SP , 32
    SW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    JAL     _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    NOP
    LW      A0 , 28(SP)                                                 ;/*  ��ǰ CPU ��Ϣ��ջ           */
    ADDU    SP , 32

    LW      T0 , 8(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  ���мĴ�����ջ              */

    .set    pop
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  �жϷ���ʱ, �߳�װ��
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    LW      T0 , 0(A0)                                                  ;/*  SP = stack current          */
    LW      SP , 0(T0)

    RESTORE_REGS                                                        ;/*  ���мĴ�����ջ              */

    .set    pop
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  �ź������ķ���
;  ����Ϊ��Ҫ���صĶ�ջջ��λ��
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    .set    push
    .set    noat
    .set    noreorder
    .set    volatile

    MOV     SP , A0                                                     ;/*  ���صĶ�ջָ��              */

    RESTORE_REGS                                                        ;/*  ���мĴ�����ջ              */

    .set    pop
    FUNC_END(archSigCtxLoad)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/