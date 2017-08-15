/****************************************************************************
*
*  Copyright (c) 2001-2008 Broadcom Corporation
*
*  This program is the proprietary software of Broadcom Corporation and/or
*  its licensors, and may only be used, duplicated, modified or distributed
*  pursuant to the terms and conditions of a separate, written license
*  agreement executed between you and Broadcom (an "Authorized License").
*  Except as set forth in an Authorized License, Broadcom grants no license
*  (express or implied), right to use, or waiver of any kind with respect to
*  the Software, and Broadcom expressly reserves all rights in and to the
*  Software and all intellectual property rights therein.  IF YOU HAVE NO
*  AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
*  AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE
*  SOFTWARE.
*
*  Except as expressly set forth in the Authorized License,
*
*  1.     This program, including its structure, sequence and organization,
*  constitutes the valuable trade secrets of Broadcom, and you shall use all
*  reasonable efforts to protect the confidentiality thereof, and to use this
*  information only in connection with your use of Broadcom integrated circuit
*  products.
*
*  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
*  "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
*  OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*  RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
*  IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
*  A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
*  ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME
*  THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
*
*  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
*  OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*  INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
*  RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
*  HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
*  EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
*  WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
*  FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*
****************************************************************************
*    Filename: pcm63268.c
*
****************************************************************************
*
*    Description:  PCM device driver for BCM63268
*
****************************************************************************/

/* ---- Include Files ---------------------------------------- */

/* 
** Always include this file before any other include files
** to avoid conflicts with remapped printf command 
*/
#include <kernelMainWrapper.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
#include <linux/dmapool.h>
#endif

#include <xchgAssert.h> 
#include <xdrvTypes.h>
#include <xdrvLog.h>
#include <bosSleep.h>
#include <xchg_dma.h>
#include <xchg_pcm.h>
#include "pcm63268.h"
#include "dma63268.h"
#include <bcmChip.h>

#ifdef PCM_WIDEBAND
/* Wideband sampling. Note: These timslot assignments 
 * are valid when a 2.048Mhz Pcm clk is used in conjunction 
 * with Si3216 slic. If the slic or pcm clk is changed these 
 * timeslot allocations must be changed */
#define P_TSALLOC_CHAN0         0 // FXS0 Sample0 on chan0( TS0 )
#define P_TSALLOC_CHAN1         8 // FXS0 Sample1 on chan1( TS8 )
#define P_TSALLOC_CHAN2         1 // FXS1 Sample0 on chan2( TS1 )
#define P_TSALLOC_CHAN3         9 // FXS1 Sample1 on chan3( TS9 )
#elif defined PCM_WIDEBAND_DECT_SITEL
#define P_TSALLOC_CHAN0        0 // Dect Wideband channel 0 (16-bit consist of 2 8-bit little endian)
#define P_TSALLOC_CHAN1        1 // Dect Wideband channel 1 (16-bit consist of 2 8-bit little endian)
#define P_TSALLOC_CHAN2        4 // FXS 0 to TS 4  (16-bit linear)
#define P_TSALLOC_CHAN3        5 // FXS 1 to TS 5  (16-bit linear)
#define P_TSALLOC_CHAN4        6 // FXO to TS 6    (16-bit linear)
#else
#define P_TSALLOC_CHAN0         0 // chan0 to TS 0
#define P_TSALLOC_CHAN1         1 // chan1 to TS 1
#define P_TSALLOC_CHAN2         2 // chan2 to TS 2
#endif

BCM63268_IUDMA_DESC *dmaRxDesc_c[PCM63268_NUM_DMA_CHANNELS*DMA_RX_NUM_BUFFERS];
BCM63268_IUDMA_DESC *dmaTxDesc_c[PCM63268_NUM_DMA_CHANNELS*DMA_TX_NUM_BUFFERS];

PCM63268_DMA_BUFFER dmaRxData[PCM63268_NUM_DMA_CHANNELS*DMA_RX_NUM_BUFFERS];
PCM63268_DMA_BUFFER dmaTxData[PCM63268_NUM_DMA_CHANNELS*DMA_TX_NUM_BUFFERS];

static XDRV_UINT8 PCM_SPECIAL_MODE;

static int  pcm63268_PLLInit( void );
static int  pcm63268_PLLDeInit( void );
static int  pcm63268_iudmaInit(void);
static int  pcm63268_iudmaDeinit(void);
static int  pcm63268_dmaDescrInit(void);
static int  pcm63268_dmaDescrDeinit(void);
static int  pcm63268_regInit( PCM_CHANNEL_TIMESLOT_MAP* chanTsMap );
static int  pcm63268_regDeinit( void );
static void pcm63268_timeslotAlloc( int chnum, int ts);

/* DMA descriptor or DMA data handle */
typedef dma_addr_t      DMA_HANDLE;

/* DMA Channel info (simplex - one way) for easy access to DMA channels */
typedef struct PCM63268_DMA_SIMPLEX_CHANNEL_INFO
{
   int                  iuChanNum;     /* IUDMA channel number assosiated with this APM channel */
   BCM63268_IUDMA_DESC   *dmaDescp;     /* Where are the descriptors for this channel? */
   DMA_HANDLE           dmaDescHandle;
   PCM63268_DMA_BUFFER   *dmaDatap;     /* Where are the DMA buffers to store/fetch the data to/from for each channel */
   DMA_HANDLE           dmaBufHandle;
}  PCM63268_DMA_SIMPLEX_CHANNEL_INFO;

/* DMA Channel info (duplex - both ways) for easy access to DMA channels */
typedef struct PCM63268_DMA_DUPLEX_CHANNEL_INFO
{
   PCM63268_DMA_SIMPLEX_CHANNEL_INFO rxChan;  /* RX channel */
   PCM63268_DMA_SIMPLEX_CHANNEL_INFO txChan;  /* TX channel */
} PCM63268_DMA_DUPLEX_CHANNEL_INFO;

/* Initialized DMA channel info */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
extern struct dma_pool *pcm_dma_pool;
#endif
static PCM63268_DMA_DUPLEX_CHANNEL_INFO gPcmDmaInfo[PCM_NUM_DMA_CHANNELS] =
{
   {
      /* RX Channel 0 */
      {
         BCM_IUDMA_PCM_RX,
         NULL,
         0,
         NULL,
         0
       },

      /* TX Channel 0 */
      {
         BCM_IUDMA_PCM_TX,
         NULL,
         0,
         NULL,
         0
      }
   }
};                          

void printRegs( void )
{
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM: Interrupt Masks"));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "---------------"));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "IrqMask                    = 0x%016llX ", (unsigned long long)(PERF->IrqControl[0].IrqMask)));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "IrqMask1                   = 0x%016llX ", (unsigned long long)(PERF->IrqControl[1].IrqMask)));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[0].intMask = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[0].intMask));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[1].intMask = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[1].intMask));

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM: Interrupt Status"));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "-----------------"));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "IrqStatus                  = 0x%016llX ", (unsigned long long)(PERF->IrqControl[0].IrqStatus)));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "IrqStatus1                 = 0x%016llX ", (unsigned long long)(PERF->IrqControl[1].IrqStatus)));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[0].intStat = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[0].intStat));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[1].intStat = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[1].intStat));

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl1         = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl1));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl2         = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl2));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl3         = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl3));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl4         = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl4));
   
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_int_pending       = 0x%08X ", (unsigned int)PCM->pcm_int_pending));                  
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_int_mask          = 0x%08X ", (unsigned int)PCM->pcm_int_mask   ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_ctrl              = 0x%08X ", (unsigned int)PCM->pcm_ctrl       ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM->pcm_chan_ctrl         = 0x%08X ", (unsigned int)PCM->pcm_chan_ctrl  ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->regs.ctrlConfig = 0x%08X ", (unsigned int)PCM_IUDMA->regs.ctrlConfig));
   
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[0].maxBurst                  = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[0].maxBurst                 ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[0].config                    = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[0].config                   ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[0].baseDescPointer          = 0x%08X ", (unsigned int)PCM_IUDMA->stram[0].baseDescPointer         ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[0].stateBytesDoneRingOffset = 0x%08X ", (unsigned int)PCM_IUDMA->stram[0].stateBytesDoneRingOffset));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[0].flagsLengthStatus        = 0x%08X ", (unsigned int)PCM_IUDMA->stram[0].flagsLengthStatus       ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[0].currentBufferPointer     = 0x%08X ", (unsigned int)PCM_IUDMA->stram[0].currentBufferPointer    ));

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[1].maxBurst                  = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[1].maxBurst                 ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->ctrl[1].config                    = 0x%08X ", (unsigned int)PCM_IUDMA->ctrl[1].config                   ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[1].baseDescPointer          = 0x%08X ", (unsigned int)PCM_IUDMA->stram[1].baseDescPointer         ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[1].stateBytesDoneRingOffset = 0x%08X ", (unsigned int)PCM_IUDMA->stram[1].stateBytesDoneRingOffset));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[1].flagsLengthStatus        = 0x%08X ", (unsigned int)PCM_IUDMA->stram[1].flagsLengthStatus       ));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM_IUDMA->stram[1].currentBufferPointer     = 0x%08X ", (unsigned int)PCM_IUDMA->stram[1].currentBufferPointer    ));
}

/*****************************************************************************
*
*  FUNCTION: pcm63268_init
*
*  PURPOSE:
*      Intialize 63268 PCM Module
*
*  PARAMETERS: channelMask - Mask to enable specific TX/RX pcm timeslots
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/

int pcm63268_init( PCM_CHANNEL_TIMESLOT_MAP* chanTsMap, XDRV_UINT32 channelMask, XDRV_UINT8 supportMode )
{  
   PCM_SPECIAL_MODE = supportMode;
   
   /* Enable clock to pcm block */
   PERF->blkEnables |= PCM_CLK_EN;

   /* Soft reset the PCM block */
   PERF->softResetB &= ~SOFT_RST_PCM;

   /* Sleep briefly to recover from the reset */
   bosSleep(50);

   /* Clear the reset of the PCM block */
   PERF->softResetB |= SOFT_RST_PCM;

   /* First reset the PLL for PCM clock to avoid glitch */
   pcm63268_PLLDeInit();

   bosSleep(10);

   /* Initialize PCM clock */
   pcm63268_PLLInit();

   bosSleep(10);

   /* Initialize PCM registers */
   pcm63268_regInit( chanTsMap );

   pcm63268_dmaDescrInit();

   pcm63268_iudmaInit();

   PCM->pcm_ctrl |= PCM_ENABLE;    

   PCM->pcm_chan_ctrl |= channelMask;

   bosSleep(100);

   printRegs();

   return ( 0 );
}


/*****************************************************************************
*
*  FUNCTION: pcm63268_deinit
*
*  PURPOSE:
*      Deintialize 63268 PCM Module
*
*  PARAMETERS:
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
int pcm63268_deinit( void )
{
   PCM->pcm_chan_ctrl = 0;

   PCM->pcm_ctrl &= ~PCM_ENABLE;
   
   PCM->pcm_ctrl = 0;
   
   PCM->pcm_int_pending |=  PCM->pcm_int_pending;

   pcm63268_iudmaDeinit();

   pcm63268_dmaDescrDeinit();

   pcm63268_regDeinit();

   pcm63268_PLLDeInit();
   
   printRegs();
      
   PERF->blkEnables &= ~PCM_CLK_EN;

   return ( 0 );
}

/*****************************************************************************
*
*  FUNCTION: pcm63268_PLLInit
*
*  PURPOSE:
*      Set up PLL registers needed for starting PCM clock.
*
*  PARAMETERS:
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int pcm63268_PLLInit( void )
{
retry_lock:
   /* Power up PLL */
   PCM->pcm_pll_ctrl1 &= ~(PCM_PLL_PWRDN | PCM_PLL_PWRDN_CH1 | PCM_PLL_REFCMP_PWRDN ); 

#ifdef NTR_SUPPORT
   /* Settings *without* DECT, without ISI, without ZSI.  Just plain old SLAC with NTR. */
         PCM->pcm_pll_ctrl1    = 0x1C311114; // Set P1=1, P2=1, Ndiv=49, M1div=20, using 20Mhz for PCM PLL => 49.152Mhz  (w/ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0x0226E978; // VCO<1600Mhz, Ndiv_mode=MFB, NDIV_FRAC=0x26E978 => 49.152MHz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x00000015; // Set reference clock to 20Mhz XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000028; // 63268 Turn on PCM PLL CH2, M2DIV=40 => 24.576MHz
         PCM->pcm_msif_intf    = 0x00000000; // 63268 PCM Control: Set to regular PCM mode
         PCM->pcm_pll_ch3_ctrl = 0x00000078; // 63268 Turn on PCM PLL CH3, M3DIV=240 => 8.192MHz
#else
   if ( PCM_SPECIAL_MODE & MODE_ISI )
   {  /* Settings with ISI */  
      if ( PCM_SPECIAL_MODE & MODE_DECT )
      {
         PCM->pcm_pll_ctrl1    = 0x1C80111B; // Set P1=1, P2=1, Ndiv=128, M1div=27, using DECT XTAL for PCM PLL => 49.152Mhz  (w/o ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0xC0000000; // VCO<1600Mhz, Ndiv_mode=N-mode, NDIV_FRAC=0 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x01000015; // Set reference clock to DECT XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000036; // 63268 Turn on PCM PLL CH2, M2DIV=54 => 24.576
         PCM->pcm_msif_intf    = 0x00000001; // 63268 PCM Control: Set to regular PCM mode
      }
      else
      {
         PCM->pcm_pll_ctrl1    = 0x1C621128; // Set P1=1, P2=1, Ndiv=98, M1div=40, using 20Mhz for PCM PLL => 49.152Mhz  (w/ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0x124DD2F1; // VCO>=1600Mhz, Ndiv_mode=MFB, NDIV_FRAC=0x4DD2F1 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x38000700;  
         PCM->pcm_pll_ctrl4    = 0x00000015; // Set reference clock to 20Mhz XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000050; // 63268 Turn on PCM PLL CH2, M2DIV=80 => 24.576
         PCM->pcm_msif_intf    = 0x00000001; // 63268 MSIF Control: Turn on MSIF
      }
   }
   else if( PCM_SPECIAL_MODE & MODE_ZSI)
   {  /* Settings with ZSI */
      PERF->blkEnables |= (HS_SPI_CLK_EN|PCM_CLK_EN);
      PERF->softResetB |= (SOFT_RST_PCM|SOFT_RST_SPI);
      
      MISC->miscIddqCtrl &= ~MISC_IDDQ_CTRL_PCM;

      if ( PCM_SPECIAL_MODE & MODE_DECT )
      {
         PCM->pcm_pll_ctrl1    = 0x1C80111B; // Set P1=1, P2=1, Ndiv=128, M1div=27, using DECT XTAL for PCM PLL => 49.152Mhz  (w/o ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0xC0000000; // VCO<1600Mhz, Ndiv_mode=N-mode, NDIV_FRAC=0 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x01000015; // Set reference clock to DECT XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000036; // 63268 Turn on PCM PLL CH2, M2DIV=54 => 24.576
         PCM->pcm_msif_intf    = 0x00000002; // 63268 PCM Control: Set to regular PCM mode
         /* To operate in BCM63268 ZSI A0 mode */
         //PCM->pcm_pll_ch2_ctrl |= (0x1 << 16); 
         /* To operate in BCM63268 ZSI C0 mode */
         PCM->pcm_pll_ch2_ctrl &= ~(0x1 << 16);    
      }
      else
      {
         PCM->pcm_pll_ctrl1    = 0x1C311114; // Set P1=1, P2=1, Ndiv=49, M1div=20, using 20Mhz for PCM PLL => 49.152Mhz  (w/ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0x0226E978; // VCO<1600Mhz, Ndiv_mode=MFB, NDIV_FRAC=0x26E978 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x00000015; // Set reference clock to 20Mhz XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000028; // 63268 Turn on PCM PLL CH2, M2DIV=40 => 24.576     
         PCM->pcm_msif_intf    = 0x00000002; // 63268 ZDS Control: Turn on ZDS
         /* To operate in BCM63268 ZSI A0 mode */
         //PCM->pcm_pll_ch2_ctrl |= (0x1 << 16); 
         /* To operate in BCM63268 ZSI C0 mode */
         PCM->pcm_pll_ch2_ctrl &= ~(0x1 << 16);       
      }
   }
   else
   {     /* Yes DECT, No ISI, No ZSI, No NTR */
      if ( PCM_SPECIAL_MODE & MODE_DECT )
      {
         PCM->pcm_pll_ctrl1    = 0x1C80111B; // Set P1=1, P2=1, Ndiv=128, M1div=27, using DECT XTAL for PCM PLL => 49.152Mhz  (w/o ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0xC0000000; // VCO<1600Mhz, Ndiv_mode=N-mode, NDIV_FRAC=0 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x01000015; // Set reference clock to DECT XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000036; // 63268 Turn on PCM PLL CH2, M2DIV=54 => 24.576
         PCM->pcm_msif_intf    = 0x00000000; // 63268 PCM Control: Set to regular PCM mode
      }
      else
      {  /* No DECT, No ISI, No ZSI, No NTR */
         PCM->pcm_pll_ctrl1    = 0x1C311114; // Set P1=1, P2=1, Ndiv=49, M1div=20, using 20Mhz for PCM PLL => 49.152Mhz  (w/ndiv_frac)
         PCM->pcm_pll_ctrl2    = 0x0226E978; // VCO<1600Mhz, Ndiv_mode=MFB, NDIV_FRAC=0x26E978 => 49.152Mhz
         PCM->pcm_pll_ctrl3    = 0x210064C0;  
         PCM->pcm_pll_ctrl4    = 0x00000015; // Set reference clock to 20Mhz XTAL
         PCM->pcm_pll_ch2_ctrl = 0x00000028; // 63268 Turn on PCM PLL CH2, M2DIV=40 => 24.576
         PCM->pcm_msif_intf    = 0x00000000; // 63268 PCM Control: Set to regular PCM mode
      } 
   }

#endif /* NTR_SUPPORT */

   /* Remove Analog Reset */
   PCM->pcm_pll_ctrl1 &= ~PCM_PLL_ARESET;

   /* Clear clk 16 reset */
   PCM->pcm_pll_ctrl1 &= ~PCM_CLK16_RESET;

   bosSleep(10);

   if ( (PCM->pcm_pll_stat & PCM_PLL_LOCK) != PCM_PLL_LOCK )
   {
      XDRV_LOG_ERROR((XDRV_LOG_MOD_PCM, "ERROR: PLL didn't lock to programmed output frequency 0x%08x", (unsigned int)PCM->pcm_pll_stat));
      goto retry_lock;  
   }
   else
   {
      /* Remove Digital Reset */
      PCM->pcm_pll_ctrl1 &= ~PCM_PLL_DRESET;
   }

   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PLL init completed. PLL registers set to:"));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl1       = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl1));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl2       = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl2));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl3       = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl3));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ctrl4       = 0x%08X ", (unsigned int)PCM->pcm_pll_ctrl4));
   /* New additional registers */
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ch2_ctrl    = 0x%08X ", (unsigned int)PCM->pcm_pll_ch2_ctrl));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_msif_intf       = 0x%08X ", (unsigned int)PCM->pcm_msif_intf));
   XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "PCM->pcm_pll_ch3_ctrl    = 0x%08X ", (unsigned int)PCM->pcm_pll_ch3_ctrl));

   return(0);
}

static int pcm63268_PLLDeInit( void )
{
   
   /* Apply clock 16 reset */
   PCM->pcm_pll_ctrl1 |= PCM_CLK16_RESET;      
   
   /* Analog Reset */
   PCM->pcm_pll_ctrl1 |= PCM_PLL_ARESET;
   
   /* Digital Reset */
   PCM->pcm_pll_ctrl1 |= PCM_PLL_DRESET;

   /* Power down PLL */
   PCM->pcm_pll_ctrl1 |= (PCM_PLL_PWRDN | PCM_PLL_PWRDN_CH1 | PCM_PLL_REFCMP_PWRDN );    

   bosSleep(10);
         
   return(0);
}

/*****************************************************************************
*
*  FUNCTION: pcm63268_regInit
*
*  PURPOSE:
*      Intialize 63268 PCM Module Registers
*
*  PARAMETERS:
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int pcm63268_regInit( PCM_CHANNEL_TIMESLOT_MAP* chanTsMap )
{
   XDRV_UINT32 i;
   XDRV_UINT32 msif_enabled = 0;

  
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Set up PCM registers"));

   if ( PCM_SPECIAL_MODE & MODE_ISI )
   {
      if( (PCM->pcm_msif_intf & 0x1) == 1 )
      {
         msif_enabled = 1;
      }      
   }
  // PCM Control
   if( msif_enabled == 1)
   {
      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "MSIF Enabled!!!"));
      PCM->pcm_ctrl = (  PCM_FS_TRIG
		                   | PCM_CLOCK_INV    
		                   | PCM_DATA_OFF

#if defined( NTR_SUPPORT )
                         | PCM_NTR_ENABLE
#endif /* NTR_SUPPORT */

#ifndef PCM_ALAW                   
#  ifndef PCM_ULAW
                         | PCM_DATA_16_8
#  endif
#endif            
                         | ( 7 << PCM_CLOCK_SEL_SHIFT  )
                         |  PCM_BITS_PER_FRAME_256 
                      );
   }               
   else  /* msif_enabled == 0 */
   {
      PCM->pcm_ctrl = (  PCM_FS_TRIG
		                   | PCM_CLOCK_INV
		                   | PCM_DATA_OFF

#if defined( NTR_SUPPORT )
                         | PCM_NTR_ENABLE
#endif /* NTR_SUPPORT */

#ifndef PCM_ALAW                   
#  ifndef PCM_ULAW
                         | PCM_DATA_16_8
#  endif
#endif            
                         | ( 2 << PCM_CLOCK_SEL_SHIFT  )
                         | PCM_BITS_PER_FRAME_256 
                       );
   }               
   PCM->pcm_chan_ctrl = 0;

   // clear time slot allocation table
   for (i=0; i < PCM_MAX_TIMESLOT_REGS; i++)
   {
      PCM->pcm_slot_alloc_tbl[i] = 0;      
   }

   for ( i = 0; i < PCM_MAX_CHANNELS; i++ )
   {
      if ( chanTsMap[i].txTimeslot == PCM_TIMESLOT_INVALID )
      {
         break;
      }
      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "PCM: Channel/Line %d Timeslot %d ", (int)i, (int)chanTsMap[i].txTimeslot ));
      pcm63268_timeslotAlloc( i, chanTsMap[i].txTimeslot );
   }

   /* Clear overflow/underflow PCM interrupts at the PCM level */
   PCM->pcm_int_pending |=  PCM_TX_UNDERFLOW | PCM_RX_OVERFLOW;

   /* Disable PCM over/underflow interrupts, DSP utilizes PCM DMA interrupts only */
   PCM->pcm_int_mask &=  ~(PCM_TX_UNDERFLOW | PCM_RX_OVERFLOW);

  return( 0 );
}


static int pcm63268_regDeinit( void )
{
   XDRV_UINT32 i;
  
   // clear time slot allocation table
   for (i=0; i < PCM_MAX_TIMESLOT_REGS; i++)
   {
      PCM->pcm_slot_alloc_tbl[i] = 0;      
   }

   return( 0 );
}


/*
*****************************************************************************
** FUNCTION:   initDmaDescriptors
**
** PURPOSE:    Initialize the DMA descriptor rings.
**
** PARAMETERS: sampleRate - sampling rate in kHz (8 or 16k) 
**             packetRate - the rate (in ms) at which APM generates packets/interrupts 
** 
** RETURNS:    none
**
*****************************************************************************
*/
static int pcm63268_dmaDescrInit( void )
{
   XDRV_UINT8                       i,chnum;
   volatile BCM63268_IUDMA_DESC      *dmaTxDesc;
   volatile BCM63268_IUDMA_DESC      *dmaRxDesc;
   volatile BCM63268_IUDMA_DESC      *dmaTxDesc_c;
   volatile BCM63268_IUDMA_DESC      *dmaRxDesc_c;
   volatile PCM63268_DMA_BUFFER      *dmaRxData;
   volatile PCM63268_DMA_BUFFER      *dmaTxData;
   PCM63268_DMA_SIMPLEX_CHANNEL_INFO *dmaChanInfop;
   volatile XDRV_UINT32 *dmaDatap;
   unsigned                         bufferSizeBytes = 0;
   XDRV_UINT32                      rxDescriptorArea;
   XDRV_UINT32                      txDescriptorArea;
   XDRV_UINT32                      rxBufferArea;
   XDRV_UINT32                      txBufferArea;

   rxDescriptorArea = sizeof(BCM63268_IUDMA_DESC) * PCM63268_NUM_DMA_CHANNELS * DMA_RX_NUM_BUFFERS + sizeof(XDRV_UINT8) * BCM63268_DMA_UBUS_BYTES;
   txDescriptorArea = sizeof(BCM63268_IUDMA_DESC) * PCM63268_NUM_DMA_CHANNELS * DMA_TX_NUM_BUFFERS + sizeof(XDRV_UINT8) * BCM63268_DMA_UBUS_BYTES;

   /* Allocate receive and transmit descriptors */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   dmaRxDesc_c = dma_pool_alloc( pcm_dma_pool, GFP_KERNEL, &gPcmDmaInfo[0].rxChan.dmaDescHandle );
   dmaTxDesc_c = dma_pool_alloc( pcm_dma_pool, GFP_KERNEL, &gPcmDmaInfo[0].txChan.dmaDescHandle );
#else
   dmaRxDesc_c = dma_alloc_coherent( NULL, rxDescriptorArea, &gPcmDmaInfo[0].rxChan.dmaDescHandle, GFP_KERNEL );
   dmaTxDesc_c = dma_alloc_coherent( NULL, txDescriptorArea, &gPcmDmaInfo[0].txChan.dmaDescHandle, GFP_KERNEL );
#endif
   if ( (dmaRxDesc_c == NULL) || (dmaTxDesc_c == NULL) )
   {
      XDRV_LOG_ERROR((XDRV_LOG_MOD_PCM, "Could not allocate memory for DMA descriptors !!! "));
      return( -1 );
     }

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "rxDescriptorArea       = %d ", (int)rxDescriptorArea));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "txDescriptorArea       = %d ", (int)txDescriptorArea));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Unaligned dmaRxDesc_c  = 0x%08X ", (unsigned int)dmaRxDesc_c));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Unaligned dmaTxDesc_c  = 0x%08X ", (unsigned int)dmaTxDesc_c));


   /* Fill in the global PCM DMA structure with the allocated pointers to descriptors */
   for (i=0; i < PCM63268_NUM_DMA_CHANNELS; i++)
   {
      /* Align DMA descriptior to 64Bit Ubus */
      dmaRxDesc_c = (volatile BCM63268_IUDMA_DESC *) ALIGN_DATA( (unsigned int) dmaRxDesc_c, BCM63268_DMA_UBUS_BYTES );
      dmaTxDesc_c = (volatile BCM63268_IUDMA_DESC *) ALIGN_DATA( (unsigned int) dmaTxDesc_c, BCM63268_DMA_UBUS_BYTES );
      
      gPcmDmaInfo[i].rxChan.dmaDescp = (BCM63268_IUDMA_DESC*)(dmaRxDesc_c + i * DMA_RX_NUM_BUFFERS);
      gPcmDmaInfo[i].txChan.dmaDescp = (BCM63268_IUDMA_DESC*)(dmaTxDesc_c + i * DMA_TX_NUM_BUFFERS);
   }

   /* Clear the descriptors */
   memset( (void *) CacheToNonCacheVirtAddress( dmaRxDesc_c ), 0, rxDescriptorArea);
   memset( (void *) CacheToNonCacheVirtAddress( dmaTxDesc_c ), 0, txDescriptorArea);

   /* Allocate data buffers */
   rxBufferArea = sizeof(XDRV_UINT32) * (PCM63268_DMA_MAX_BUFFER_SIZE + DMA_PADDING_WORD32) * PCM63268_NUM_DMA_CHANNELS * DMA_RX_NUM_BUFFERS;
   txBufferArea = sizeof(XDRV_UINT32) * (PCM63268_DMA_MAX_BUFFER_SIZE + DMA_PADDING_WORD32) * PCM63268_NUM_DMA_CHANNELS * DMA_TX_NUM_BUFFERS;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   dmaRxData = dma_pool_alloc( pcm_dma_pool, GFP_KERNEL, &gPcmDmaInfo[0].rxChan.dmaBufHandle );
   dmaTxData = dma_pool_alloc( pcm_dma_pool, GFP_KERNEL, &gPcmDmaInfo[0].txChan.dmaBufHandle );
#else
   dmaRxData = dma_alloc_coherent( NULL, rxBufferArea, &gPcmDmaInfo[0].rxChan.dmaBufHandle, GFP_KERNEL );
   dmaTxData = dma_alloc_coherent( NULL, txBufferArea, &gPcmDmaInfo[0].txChan.dmaBufHandle, GFP_KERNEL );
#endif
   if ( (dmaRxData == NULL) || (dmaTxData == NULL ) )
   {
      XDRV_LOG_ERROR((XDRV_LOG_MOD_PCM, "Could not allocate memory for DMA data buffers !!! "));
      return( -1 );
   }

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "rxBufferArea          = %d ", (int)rxBufferArea));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "txBufferArea          = %d ", (int)txBufferArea));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Unaligned dmaRxData   = 0x%08X ", (unsigned int)dmaRxData));
   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Unaligned dmaTxData   = 0x%08X ", (unsigned int)dmaTxData));

   /* Fill in the global PCM DMA structure with the allocated pointers to descriptors */
   for (i=0; i < PCM63268_NUM_DMA_CHANNELS; i++)
     {
      gPcmDmaInfo[i].rxChan.dmaDatap = (PCM63268_DMA_BUFFER*)(dmaRxData + i * DMA_RX_NUM_BUFFERS);
      gPcmDmaInfo[i].txChan.dmaDatap = (PCM63268_DMA_BUFFER*)(dmaTxData + i * DMA_TX_NUM_BUFFERS);
   }

   /* Reset the data buffers */
   memset( (void*) dmaRxData, 0, rxBufferArea);
   memset( (void*) dmaTxData, 0, txBufferArea);

   for (chnum=0; chnum<PCM63268_NUM_DMA_CHANNELS; chnum++)
   {
      /* Get the pointer to TX DMA channel info */
      dmaChanInfop=&gPcmDmaInfo[chnum].txChan;

      /* Get the pointer to Tx descriptors for this channel. Convert to non-cached addresses */
      dmaTxDesc = (volatile BCM63268_IUDMA_DESC *)CacheToNonCacheVirtAddress((XDRV_UINT32)dmaChanInfop->dmaDescp);

      /* Init Tx DMA descriptor table */
      for (i=0; i<DMA_TX_NUM_BUFFERS; i++)
      {
         /* TX descriptor buffers should be enough to hold packetRate ms worth of sampleRate data */
         bufferSizeBytes = PCM63268_DMA_MAX_BUFFER_SIZE_BYTES;

         /* Each buffer is a complete packet */
         dmaTxDesc[i].flagsLengthStatus.status = BCM63268_IUDMA_DESC_STATUS_SOP |
                                                 BCM63268_IUDMA_DESC_STATUS_EOP;

         if( i == 0 )
         {
            /* First buffer - prime the TX fifo */
            dmaTxDesc[i].flagsLengthStatus.status |= BCM63268_IUDMA_DESC_STATUS_OWN;
         
            /* Prime with packetRate + 1ms for processing head room from the start of the ISR
            to allow us time to provide another packetRate worth of data to the TX APM DMA. */
            bufferSizeBytes = PCM63268_DMA_MAX_BUFFER_SIZE_BYTES;
         }

         if( i == ( DMA_TX_NUM_BUFFERS - 1 ) )
         {
            /* Last buffer - set the WRAP bit */
            dmaTxDesc[i].flagsLengthStatus.status |= BCM63268_IUDMA_DESC_STATUS_WRAP;
         }

         /* Check if we can accomodate the desired buffer size */
         if (bufferSizeBytes <= PCM63268_DMA_MAX_BUFFER_SIZE_BYTES)
         {
            dmaTxDesc[i].flagsLengthStatus.length = bufferSizeBytes;
         }
         else
         {
            /* Sorry, the allocated buffer size is smaller than what
            ** you want the buffer size configured to, so I can not do this.
            ** You can either increase the compile time buffer size (PCM63268_DMA_MAX_BUFFER_SIZE),
            ** or decrease the buffer size via packetRate/sampleRate */
            XDRV_LOG_ERROR((XDRV_LOG_MOD_PCM, "XDRV_PCM: ERROR - Can not accomodate configured buffer size (%d) ", bufferSizeBytes));
            return ( -1 );
         }


         /* Cache operations (invalidate and write-back) are performed on these
         ** buffers before and after DMA operations. Since cache operations
         ** operate on the entire cache-line, it is necessary to make the
         ** buffers aligned on the cache-line size (16-bytes for the 3368).
         ** Failure to do so may result in corruption of data that shares the
         ** same cache-line as the DMA buffer. */
         dmaDatap = dmaChanInfop->dmaDatap[i].dmaBuff;

         dmaDatap = (volatile XDRV_UINT32 *) ALIGN_DATA( (unsigned int) dmaDatap, DCACHE_LINE_SIZE_BYTES );

         /*  Use physical address */
         dmaTxDesc[i].addr.addr32 = CacheVirtToPhyAddress( (XDRV_UINT32)dmaDatap );

         XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Aligned Tx Desc (0x%08X): chan %d, buf %d, sts 0x%x, len %d, bufp 0x%x ",
                     (unsigned int)&dmaTxDesc[i], chnum, i, dmaTxDesc[i].flagsLengthStatus.status,  dmaTxDesc[i].flagsLengthStatus.length,  (unsigned int)dmaTxDesc[i].addr.addr32 ));

      }

      /* Get the pointer to RX DMA channel info */
      dmaChanInfop=&gPcmDmaInfo[chnum].rxChan;

      /* Get the pointer to Rx descriptors for this channel. Convert to non-cached addresses */
      dmaRxDesc = (volatile BCM63268_IUDMA_DESC *)CacheToNonCacheVirtAddress((XDRV_UINT32)dmaChanInfop->dmaDescp);

      /* Init Rx DMA descriptor table */
      for( i = 0; i < DMA_RX_NUM_BUFFERS; i++ )
      {
         /* Give ownership of all descriptors to RX DMA */
         dmaRxDesc[i].flagsLengthStatus.status = BCM63268_IUDMA_DESC_STATUS_OWN;

         if( i == ( DMA_RX_NUM_BUFFERS - 1 ) )
         {
            /* Last descriptor - set the wrap bit */
            dmaRxDesc[i].flagsLengthStatus.status |= BCM63268_IUDMA_DESC_STATUS_WRAP;
         }

         /* RX descriptor buffers should be enough to hold packetRate ms worth of sampleRate data */
         bufferSizeBytes = PCM63268_DMA_MAX_BUFFER_SIZE_BYTES;

         /* Buffer size in bytes */
         dmaRxDesc[i].flagsLengthStatus.length = bufferSizeBytes;

         /* Cache operations (invalidate and write-back) are performed on these
         ** buffers before and after DMA operations. Since cache operations
         ** operate on the entire cache-line, it is necessary to make the
         ** buffers aligned on the cache-line size (16-bytes for the 63268).
         ** Failure to do so may result in corruption of data that shares the
         ** same cache-line as the DMA buffer. */
         dmaDatap = dmaChanInfop->dmaDatap[i].dmaBuff;
         dmaDatap = (volatile XDRV_UINT32 *) ALIGN_DATA( (unsigned int) dmaDatap, DCACHE_LINE_SIZE_BYTES );

         /* Use physical address */
         dmaRxDesc[i].addr.addr32 = CacheVirtToPhyAddress( (XDRV_UINT32)dmaDatap );

         XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Aligned Rx Desc (0x%08X): chan %d, buf %d, sts 0x%x, len %d, bufp 0x%x ",
                     (unsigned int)&dmaRxDesc[i], chnum, i, dmaRxDesc[i].flagsLengthStatus.status, dmaRxDesc[i].flagsLengthStatus.length, (unsigned int)dmaRxDesc[i].addr.addr32 ));

      }
   }

   return ( 0 );
}


static int pcm63268_dmaDescrDeinit( void )
{
   XDRV_UINT8                       i;
   XDRV_UINT32                      rxDescriptorArea;
   XDRV_UINT32                      txDescriptorArea;
   XDRV_UINT32                      rxBufferArea;
   XDRV_UINT32                      txBufferArea;

   rxDescriptorArea = sizeof(BCM63268_IUDMA_DESC) * PCM63268_NUM_DMA_CHANNELS * DMA_RX_NUM_BUFFERS;
   txDescriptorArea = sizeof(BCM63268_IUDMA_DESC) * PCM63268_NUM_DMA_CHANNELS * DMA_TX_NUM_BUFFERS;

   /* Clear the descriptors */
   memset( (void *) CacheToNonCacheVirtAddress( gPcmDmaInfo[0].rxChan.dmaDescp ), 0, rxDescriptorArea);
   memset( (void *) CacheToNonCacheVirtAddress( gPcmDmaInfo[0].txChan.dmaDescp ), 0, txDescriptorArea);

   /* Free receive and transmit descriptors */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   dma_pool_free( pcm_dma_pool, gPcmDmaInfo[0].rxChan.dmaDescp, gPcmDmaInfo[0].rxChan.dmaDescHandle );
   dma_pool_free( pcm_dma_pool, gPcmDmaInfo[0].txChan.dmaDescp, gPcmDmaInfo[0].txChan.dmaDescHandle );
#else
   dma_free_coherent(NULL, rxDescriptorArea, gPcmDmaInfo[0].rxChan.dmaDescp, gPcmDmaInfo[0].rxChan.dmaDescHandle);
   dma_free_coherent(NULL, txDescriptorArea, gPcmDmaInfo[0].txChan.dmaDescp, gPcmDmaInfo[0].txChan.dmaDescHandle);
#endif

   for (i=0; i < PCM63268_NUM_DMA_CHANNELS; i++)
   {
      gPcmDmaInfo[i].rxChan.dmaDescp = NULL;
      gPcmDmaInfo[i].txChan.dmaDescp = NULL;
   }


   rxBufferArea = sizeof(XDRV_UINT32) * (PCM63268_DMA_MAX_BUFFER_SIZE + DMA_PADDING_WORD32) * PCM63268_NUM_DMA_CHANNELS * DMA_RX_NUM_BUFFERS;
   txBufferArea = sizeof(XDRV_UINT32) * (PCM63268_DMA_MAX_BUFFER_SIZE + DMA_PADDING_WORD32) * PCM63268_NUM_DMA_CHANNELS * DMA_TX_NUM_BUFFERS;

   /* Clear the data buffers */
   memset( (void*) gPcmDmaInfo[0].rxChan.dmaDatap, 0, rxBufferArea);
   memset( (void*) gPcmDmaInfo[0].txChan.dmaDatap, 0, txBufferArea);

   /* Free data buffers */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   dma_pool_free( pcm_dma_pool, gPcmDmaInfo[0].rxChan.dmaDatap, gPcmDmaInfo[0].rxChan.dmaBufHandle );
   dma_pool_free( pcm_dma_pool, gPcmDmaInfo[0].txChan.dmaDatap, gPcmDmaInfo[0].txChan.dmaBufHandle );
#else
   dma_free_coherent(NULL, rxBufferArea, gPcmDmaInfo[0].rxChan.dmaDatap, gPcmDmaInfo[0].rxChan.dmaBufHandle);
   dma_free_coherent(NULL, txBufferArea, gPcmDmaInfo[0].txChan.dmaDatap, gPcmDmaInfo[0].txChan.dmaBufHandle);
#endif

   /* Fill in the global PCM DMA structure with the allocated pointers to descriptors */
   for (i=0; i < PCM63268_NUM_DMA_CHANNELS; i++)
     {
      gPcmDmaInfo[i].rxChan.dmaDatap = NULL;
      gPcmDmaInfo[i].txChan.dmaDatap = NULL;
   }

   return ( 0 );
}


/*
*****************************************************************************
** FUNCTION:   initIudma
**
** PURPOSE:    Initialize the PCM IUDMA
**
** PARAMETERS: iuDmap       - pointer to IUDMA module
**             dmaSettingsp - pointer to DMA configuration structure
**
** RETURNS:    none
**
*****************************************************************************
*/
static int pcm63268_iudmaInit( void)
{
   int chnum;
   volatile PcmIudma* iuDmap = PCM_IUDMA;
   volatile PcmIudmaStateRam *stRamp = iuDmap->stram;

   /* Reset entire IUDMA state RAM */
   memset( (void*)stRamp, 0, BCM63268_MAX_PCM_DMA_CHANNELS * sizeof(PcmIudmaStateRam) );
         
   /* Initialize the Rx/Tx IUDMA pair for each APM channel */
   for (chnum=0; chnum<PCM63268_NUM_DMA_CHANNELS; chnum++)
   {

   /* Init RX side */
      XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "initIudma: chan %d, descBase 0x%x, descBaseIudma 0x%x",
                        gPcmDmaInfo[chnum].rxChan.iuChanNum,
                        (unsigned int)gPcmDmaInfo[chnum].rxChan.dmaDescp,
                        CacheVirtToPhyAddress(gPcmDmaInfo[chnum].rxChan.dmaDescp)
                        ));
      /* Init RX side */
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].maxBurst = 2; /* in 64-bit words */
      iuDmap->stram[gPcmDmaInfo[chnum].rxChan.iuChanNum].baseDescPointer = CacheVirtToPhyAddress(gPcmDmaInfo[chnum].rxChan.dmaDescp);
      iuDmap->stram[gPcmDmaInfo[chnum].rxChan.iuChanNum].stateBytesDoneRingOffset = 0;
      iuDmap->stram[gPcmDmaInfo[chnum].rxChan.iuChanNum].flagsLengthStatus = 0;
      iuDmap->stram[gPcmDmaInfo[chnum].rxChan.iuChanNum].currentBufferPointer = 0;
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intStat |= iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intStat;
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intMask = BCM63268_IUDMA_INTMASK_BDONE | BCM63268_IUDMA_INTMASK_NOTVLD;

      XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "initIudma: chan %d, descBase 0x%x, descBaseIudma 0x%x",
                        gPcmDmaInfo[chnum].txChan.iuChanNum,
                        (unsigned int)gPcmDmaInfo[chnum].txChan.dmaDescp,
                        CacheVirtToPhyAddress(gPcmDmaInfo[chnum].txChan.dmaDescp)
                        ));
      /* Init TX side */
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].maxBurst = 2; /* in 64-bit words */
      iuDmap->stram[gPcmDmaInfo[chnum].txChan.iuChanNum].baseDescPointer = CacheVirtToPhyAddress(gPcmDmaInfo[chnum].txChan.dmaDescp);
      iuDmap->stram[gPcmDmaInfo[chnum].txChan.iuChanNum].stateBytesDoneRingOffset = 0;
      iuDmap->stram[gPcmDmaInfo[chnum].txChan.iuChanNum].flagsLengthStatus = 0;
      iuDmap->stram[gPcmDmaInfo[chnum].txChan.iuChanNum].currentBufferPointer = 0;
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intStat |= iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intStat;
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intMask = BCM63268_IUDMA_INTMASK_BDONE | BCM63268_IUDMA_INTMASK_NOTVLD;
   }

   /* Enable the IUDMA master */
   iuDmap->regs.ctrlConfig = BCM63268_IUDMA_REGS_CTRLCONFIG_MASTER_EN;

   return ( 0 );
}


/*
*****************************************************************************
** FUNCTION:   pcm63268_iudmaDeinit
**
** PURPOSE:    Deinitialize the PCM IUDMA
**
** PARAMETERS: 
**
** RETURNS:    0 on success
**
*****************************************************************************
*/
static int pcm63268_iudmaDeinit( void)
{
   int chnum;
   volatile PcmIudma* iuDmap = PCM_IUDMA;
   volatile PcmIudmaStateRam *stRamp = iuDmap->stram;
   int i=0;
   
   /* Halt individual channels first */
   for (chnum=0; chnum<PCM63268_NUM_DMA_CHANNELS; chnum++)
   {

      /* Deinit RX side */
      XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "RX DMA STATUS (1=busy 0=IDLE) : 0x%08x", (unsigned int)iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].config));
      XDRV_LOG_INFO((XDRV_LOG_MOD_PCM, "HALTING RX DMA...."));

      /* Set the burstHalt bit while clearing endma bit */
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].config = ( BCM63268_IUDMA_CONFIG_BURSTHALT );      

      while( (iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].config & BCM63268_IUDMA_CONFIG_ENDMA) != 0x00000000 )
      {
         /* Set the burstHalt bit while clearing endma bit */
         iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].config = ( BCM63268_IUDMA_CONFIG_BURSTHALT );      
         XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "RX DMA HALT Waiting, iteration:%d",i)); 
         i++;
      }
      
      i=0;
      
      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "RX DMA IDLE"));
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intMask = 0;
      iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intStat |= iuDmap->ctrl[gPcmDmaInfo[chnum].rxChan.iuChanNum].intStat;

      /* Deinit TX side */

      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "TX DMA STATUS (1=busy 0=IDLE) : 0x%08x", (unsigned int)iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].config));
      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "HALTING TX DMA...."));

      /* Set the burstHalt bit while clearing endma bit */
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].config = ( BCM63268_IUDMA_CONFIG_BURSTHALT );      

      while( (iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].config & BCM63268_IUDMA_CONFIG_ENDMA) != 0x00000000 )
      {
         /* Set the burstHalt bit while clearing endma bit */
         iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].config = ( BCM63268_IUDMA_CONFIG_BURSTHALT );               
         XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "TX DMA HALT Waiting, iteration:%d",i)); 
         i++;
      }      
      XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "TX DMA IDLE"));
               
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intMask = 0;
      iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intStat |= iuDmap->ctrl[gPcmDmaInfo[chnum].txChan.iuChanNum].intStat;
   }

   /* Disable pcm iudma controller */
   iuDmap->regs.ctrlConfig &= ~BCM63268_IUDMA_REGS_CTRLCONFIG_MASTER_EN;
   
   /* Reset entire IUDMA state RAM */
   memset( (void*)stRamp, 0, BCM63268_MAX_PCM_DMA_CHANNELS * sizeof(PcmIudmaStateRam) );

   return( 0 );
}


/*****************************************************************************
*
*  FUNCTION: pcm63268_timeslotAlloc
*
*  PURPOSE:
*      Allocate PCM timeslot in the PCM table
*
*  PARAMETERS:
*
*  RETURNS: 
*
*  NOTES:
*
*****************************************************************************/
static void pcm63268_timeslotAlloc( int chnum, int ts)
{
   int         ts_register_offset;
   int         bit_offset = 0;
   XDRV_UINT32 entry, mask;

   /* 
   ** Each PCM timeslot register contains configuration for 8 timeslots.
   ** Timeslots 0 to 7 are in the first PCM timeslot register,
   ** timeslots 8 to 15 in the second, etc. 
   */
   ts_register_offset = ts >> 3;

   switch (ts & 7)
   {
     case 0:  bit_offset = 28; break;
     case 1:  bit_offset = 24; break;
     case 2:  bit_offset = 20; break;
     case 3:  bit_offset = 16; break;
     case 4:  bit_offset = 12; break;
     case 5:  bit_offset = 8; break;
     case 6:  bit_offset = 4; break;
     case 7:  bit_offset = 0;
   }

   /* Read, modify, write for the PCM timeslot register */
   entry = PCM->pcm_slot_alloc_tbl[ts_register_offset];
   mask  = ~(0xF << bit_offset);
   entry &= mask;
   entry |= ( (PCM_TS_VALID | chnum) << bit_offset) ;

   PCM->pcm_slot_alloc_tbl[ts_register_offset] = entry;

   XDRV_LOG_DBG(( XDRV_LOG_MOD_XDRV, "%s 0x%08X: 0x%08X", __FUNCTION__, (unsigned int)&PCM->pcm_slot_alloc_tbl[ts_register_offset], (unsigned int)entry ));

   XDRV_LOG_DBG((XDRV_LOG_MOD_PCM, "Channel %d assigned to timeslot %d",chnum,ts));
}

