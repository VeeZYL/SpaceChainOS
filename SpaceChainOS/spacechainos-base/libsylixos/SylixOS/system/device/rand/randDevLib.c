/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: randDevLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 10 �� 31 ��
**
** ��        ��: UNIX ����������豸.

** BUG:
2012.11.08  __randRead() �� iSeedInit ����Ϊ static ���ͱ���.
2013.12.12  ֻ�� LW_IRQ_FLAG_SAMPLE_RAND ���жϲŸ������������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"                                /*  ��Ҫ���ļ�ϵͳʱ��          */
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0
#include "randDevLib.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static spinlock_t       _G_slRandLock;                                  /*  �ж���Ϣ������              */
static struct timespec  _G_tvLastInt;                                   /*  ���һ���ж�ʱ���          */
static INT64            _G_i64IntCounter;                               /*  �жϼ�����                  */
static LW_OBJECT_HANDLE _G_hRandSelMutex;                               /*  select list mutex           */
/*********************************************************************************************************
** ��������: __randInterHook
** ��������: ������������жϹ��Ӻ��� (�˺������˳��ж�ʱ������, �������Ա����ӳ��ж���Ӧʱ��)
** �䡡��  : ulVector  �ж�����
**           ulNesting ��ǰǶ�ײ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __randInterHook (ULONG  ulVector, ULONG  ulNesting)
{
    INTREG  iregInterLevel;

    (VOID)ulNesting;
    
    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_SAMPLE_RAND) {         /*  ��Ҫ�������������          */
        LW_SPIN_LOCK_QUICK(&_G_slRandLock, &iregInterLevel);
        _G_tvLastInt = _K_tvTODCurrent;
        _G_i64IntCounter++;
        LW_SPIN_UNLOCK_QUICK(&_G_slRandLock, iregInterLevel);
    }
}
/*********************************************************************************************************
** ��������: __randInit
** ��������: �����������������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __randInit (VOID)
{
    LW_SPIN_INIT(&_G_slRandLock);
    
    if (_G_hRandSelMutex == LW_OBJECT_HANDLE_INVALID) {
        _G_hRandSelMutex = API_SemaphoreMCreate("randsel_lock", LW_PRIO_DEF_CEILING, 
                                                LW_OPTION_WAIT_PRIORITY | 
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __randOpen
** ��������: ����������� open
** �䡡��  : pranddev      �豸
**           pcName        ����
**           iFlags        �򿪱�־
**           iMode         ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LONG  __randOpen (PLW_RAND_DEV  pranddev, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    PLW_RAND_FIL  prandfil;
    
    prandfil = (PLW_RAND_FIL)__SHEAP_ALLOC(sizeof(LW_RAND_FIL));
    if (prandfil == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    prandfil->RANDFIL_pranddev = pranddev;
    prandfil->RANDFIL_selwulList.SELWUL_hListLock     = _G_hRandSelMutex;
    prandfil->RANDFIL_selwulList.SELWUL_ulWakeCounter = 0;
    prandfil->RANDFIL_selwulList.SELWUL_plineHeader   = LW_NULL;

    if (LW_DEV_INC_USE_COUNT(&pranddev->RANDDEV_devhdr) == 1) {
        API_SystemHookAdd(__randInterHook, LW_OPTION_CPU_INT_EXIT);     /*  �����ж�hook                */
    }
    
    return  ((LONG)prandfil);
}
/*********************************************************************************************************
** ��������: __randClose
** ��������: ����������� close
** �䡡��  : prandfil      �ļ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __randClose (PLW_RAND_FIL  prandfil)
{
    PLW_RAND_DEV  pranddev = prandfil->RANDFIL_pranddev;

    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELREAD);
    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELWRITE);
    SEL_WAKE_UP_ALL(&prandfil->RANDFIL_selwulList, SELEXCEPT);

    if (LW_DEV_DEC_USE_COUNT(&pranddev->RANDDEV_devhdr) == 0) {
        API_SystemHookDelete(__randInterHook, LW_OPTION_CPU_INT_EXIT);  /*  ɾ���ж�hook                */
    }
    
    __SHEAP_FREE(prandfil);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __randRead
** ��������: ����������� read
** �䡡��  : prandfil      �ļ�
**           pcBuffer      ��������
**           stMaxBytes    ��������С
** �䡡��  : ʵ�ʶ����ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __randRead (PLW_RAND_FIL  prandfil, PCHAR  pcBuffer, size_t  stMaxBytes)
{
    static INT32    iSeedInit = 1;
    
    INTREG          iregInterLevel;
    INT             i;
    INT             iTimes = stMaxBytes / 4;                            /*  һ�� 4 ���ֽ�               */
    INT             iLefts = stMaxBytes % 4;
    INT32           iSeed;
    INT64           llTmp;
    
    LW_SPIN_LOCK_QUICK(&_G_slRandLock, &iregInterLevel);
    iSeedInit += (INT32)_G_tvLastInt.tv_nsec;                           /*  �����ظ�����ȷ������ͬ      */
    LW_SPIN_UNLOCK_QUICK(&_G_slRandLock, iregInterLevel);
    
    iSeedInit = (iSeedInit >= 0) ? iSeedInit : (0 - iSeedInit);
    iSeed     = iSeedInit;
    
    for (i = 0; i < iTimes; i++) {
        llTmp   = (INT64)((INT64)iSeed * 1103515245);
        llTmp  += ((INT64)16807L ^ (_G_i64IntCounter + API_TimeGet64()));
        llTmp >>= 16;
        iSeed   = (INT32)(llTmp % __ARCH_UINT_MAX);
        iSeed   = (iSeed >= 0) ? iSeed : (0 - iSeed);
        
        *pcBuffer++ = (CHAR)(iSeed >> 24);
        *pcBuffer++ = (CHAR)(iSeed >> 8);
        *pcBuffer++ = (CHAR)(iSeed >> 16);
        *pcBuffer++ = (CHAR)(iSeed);
    }
    
    if (iLefts) {
        llTmp   = (INT64)((INT64)iSeed * 1103515245);
        llTmp  += ((INT64)16807L ^ (_G_i64IntCounter + API_TimeGet64()));
        llTmp >>= 16;
        iSeed   = (INT32)(llTmp % __ARCH_UINT_MAX);
        iSeed   = (iSeed >= 0) ? iSeed : (0 - iSeed);
        
        for (i = 0; i < iLefts; i++) {
            *pcBuffer++ = (CHAR)(iSeed >> 24);
            iSeed <<= 8;
        }
    }
    
    iSeedInit = iSeed;
    
    return  ((ssize_t)stMaxBytes);
}
/*********************************************************************************************************
** ��������: __randWrite
** ��������: ����������� write
** �䡡��  : prandfil      �ļ�
**           pcBuffer      д������
**           stNBytes      д��������
** �䡡��  : ʵ��д���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __randWrite (PLW_RAND_FIL  prandfil, CPCHAR  pcBuffer, size_t  stNBytes)
{
    _ErrorHandle(ENOSYS);
    return  (0);
}
/*********************************************************************************************************
** ��������: __randIoctl
** ��������: ����������� ioctl
** �䡡��  : prandfil      �ļ�
**           iRequest      ����
**           lArg          �������
** �䡡��  : ʵ��д���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __randIoctl (PLW_RAND_FIL  prandfil, INT  iRequest, LONG  lArg)
{
    PLW_RAND_DEV  pranddev = prandfil->RANDFIL_pranddev;
    struct stat  *pstat;
    
    switch (iRequest) {
    
    case FIOFSTATGET:                                                   /*  ����ļ�����                */
        pstat = (struct stat *)lArg;
        pstat->st_dev     = (dev_t)pranddev;
        pstat->st_ino     = (ino_t)pranddev->RANDDEV_bIsURand;
        pstat->st_mode    = 0444 | S_IFCHR;                             /*  Ĭ������                    */
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;
        pstat->st_gid     = 0;
        pstat->st_rdev    = 1;
        pstat->st_size    = 0;
        pstat->st_blksize = 0;
        pstat->st_blocks  = 0;
        pstat->st_atime   = API_RootFsTime(LW_NULL);                    /*  Ĭ��ʹ�� root fs ��׼ʱ��   */
        pstat->st_mtime   = API_RootFsTime(LW_NULL);
        pstat->st_ctime   = API_RootFsTime(LW_NULL);
        break;
        
    case FIOSELECT: {
            PLW_SEL_WAKEUPNODE pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
            SEL_WAKE_NODE_ADD(&prandfil->RANDFIL_selwulList, pselwunNode);
            if (pselwunNode->SELWUN_seltypType == SELREAD) {
                SEL_WAKE_UP(pselwunNode);                               /*  �ļ��ɶ�                    */
            }
        }
        break;
        
    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&prandfil->RANDFIL_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
/*********************************************************************************************************
  END
*********************************************************************************************************/