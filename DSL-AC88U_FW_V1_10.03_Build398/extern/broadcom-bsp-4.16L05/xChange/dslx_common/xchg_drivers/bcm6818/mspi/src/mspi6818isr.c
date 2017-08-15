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
*    Description:
*
****************************************************************************/

/* ---- Include Files ---------------------------------------- */

#include <bosSleep.h>
#include <bcm6818.h>
#include <bosSem.h>
#include <mspi6818.h>
#include <mspi6818defs.h>
#include <xchgAssert.h>
#include <xdrvIntCtrl.h>

#if MSPI6818_USE_INTERRUPTS
/* ---- Public Variables ------------------------------------- */
/* ---- Private Constants and Types -------------------------- */
/* ---- Private Variables ------------------------------------ */
/* ---- Private Function Prototypes -------------------------- */
/* ---- Functions -------------------------------------------- */

/*
*****************************************************************************
** FUNCTION:   mspi6818Isr
**
** PURPOSE:    MSPI interrupt service routine
**
** PARAMETERS: contextp - parm associated with the ISR
**
** RETURNS:    none
**
** NOTES:      Invocation of this routine does not mean completion of transfer.
**             If the length of the transfer is more than 64 bytes, this 
**             routine will be invoked more than once.
**
*****************************************************************************
*/
unsigned int mspi6818Isr( XDRV_INT_CTRL_ISR_PARM contextp )
{
   unsigned             indx;
   BCM_MSPI6818_DRV     *pMspiDrv;
   volatile SpiControl  *pMspiRegs;
   unsigned             numBytesToXfer;
   unsigned             numBytesTransferred;

   pMspiDrv  = &mspi6818Drv;
   pMspiRegs = pMspiDrv->p6818SpiRegs;

   /*
   ** Check if the current transfer is done
   */
   
   if( !( pMspiRegs->spiIntStatus & MMR6818_MSPI_INTSTATUS_CMNDDONE ))
   {
      XCHG_ASSERT( 0 );
   }

   /*
   ** Clear interrupt status register to clear interrupt
   */
   pMspiRegs->spiIntStatus |= MMR6818_MSPI_INTSTATUS_MASK;

   /*
   ** Retrieve the number of bytes transferred
   */
   numBytesTransferred = pMspiDrv->bytesQueued;

   /*
   ** Read out rx data
   */
   for( indx = 0; indx < numBytesTransferred; indx++ )
   {
      pMspiDrv->rxBuf[ indx + pMspiDrv->rxOffset ] = pMspiRegs->spiRxDataFifo[ indx ];
   }
   
   pMspiDrv->rxOffset += numBytesTransferred;
   pMspiDrv->bytesRemaining -= numBytesTransferred;

   /*
   ** If all bytes have been read/written
   ** Then nofify the user via the callback.
   */
   if( pMspiDrv->bytesRemaining <= 0 )
   {
      if( (pMspiDrv->rxCallbackFnc) != NULL )
      {
         (pMspiDrv->rxCallbackFnc) ( pMspiRegs->spiIntStatus );
      }

      if( (pMspiDrv->txCallbackFnc) != NULL )
      {
         (pMspiDrv->txCallbackFnc) ( pMspiRegs->spiIntStatus );
      }

      /* Release the semaphore */
      if( bosSemGive( &mspi6818IsrSem ))
      {
         XCHG_ASSERT( 0 );
      }
   }
   else
   {
      /*
      ** The MSPI can only transmit at most MSPI_NUM_QUEUE_SIZE at a time
      */
      if( pMspiDrv->bytesRemaining >= ( BCM6818_MSPI_TX_QUEUE_SIZE - 2 ))
      {
         numBytesToXfer = ( BCM6818_MSPI_TX_QUEUE_SIZE - 2 );
      }
      else
      {
         numBytesToXfer = pMspiDrv->bytesRemaining;
      }

      /*
      ** Write control byte into tx queue
      */
      pMspiRegs->spiMsgCtl = numBytesToXfer;

      /*
      ** Write tx data into tx queue
      */
      for( indx = 0; indx < numBytesToXfer; indx++ )
      {
         pMspiRegs->spiMsgData[indx] = (*(pMspiDrv->pTxBuf + indx + pMspiDrv->txOffset));
      }
      
      /* Increment data transferred */
      pMspiDrv->txOffset += numBytesToXfer;

      /* Store number of bytes queued for transfer */
      pMspiDrv->bytesQueued = numBytesToXfer;

      /* Clear command done flag and begin transmission */
      pMspiRegs->spiIntStatus |= MMR6818_MSPI_INTSTATUS_MASK;
      pMspiRegs->spiCmd |= MMR6818_MSPI_START_IMMEDIATE;
   }

   mspi6818intc_IrqEnable();

   return ( IRQ_HANDLED );
}
#endif /* MSPI6818_USE_INTERRUPTS */
