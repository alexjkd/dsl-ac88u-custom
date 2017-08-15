/****************************************************************************
*
*  Copyright (c) 2000-2008 Broadcom
*
*  This program is the proprietary software of Broadcom and/or
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
*
*    Description:
*		This is for different country specific parameters.
*
****************************************************************************/
#ifndef SLAC_SI3226_LOCALE_H    /* support nested includes */
#define SLAC_SI3226_LOCALE_H

#include <vrgCountry.h>
#include <si3226_constants.h>
#include <xdrvSlicGains.h>

/* ---- Constants and Types ---------------------------------- */

typedef struct {
   int si3226Locale;
   short int Dlp;
   short int TxGain;
} SI3226_LOCALE_MAP;

/* ----Locale Map -------------------------------------------- */
#undef  COUNTRY_ARCHIVE_MAKE_NAME
#define COUNTRY_ARCHIVE_MAKE_NAME( country ) { SI3226_##country, FXS_##country##_Profile_Dlp, FXS_##country##_Profile_Tx_Gain },
static SI3226_LOCALE_MAP si3226LocaleMap [VRG_COUNTRY_MAX] =
{
   #include <countryArchive.h>
};
#if 0
typedef struct {
   VRG_COUNTRY vrgLocale;
   int si3226Locale;
} SI3226_LOCALE_MAP;

/* ----Locale Map -------------------------------------------- */
static SI3226_LOCALE_MAP si3226LocaleMap [] =
{
#if VRG_COUNTRY_CFG_AUSTRALIA   
   { VRG_COUNTRY_AUSTRALIA,     SI3226_AUSTRALIA    },
#endif                                         
#if VRG_COUNTRY_CFG_AUSTRIA                           
   { VRG_COUNTRY_AUSTRIA,       SI3226_AUSTRIA      },
#endif                                               
#if VRG_COUNTRY_CFG_BELGIUM                           
   { VRG_COUNTRY_BELGIUM,       SI3226_BELGIUM      },
#endif                                                
#if VRG_COUNTRY_CFG_BRAZIL                            
   { VRG_COUNTRY_BRAZIL,        SI3226_BRAZIL      },
#endif                                                
#if VRG_COUNTRY_CFG_CHILE                             
   { VRG_COUNTRY_CHILE,         SI3226_CHILE        },
#endif                                                
#if VRG_COUNTRY_CFG_CHINA                             
   { VRG_COUNTRY_CHINA,         SI3226_CHINA        },
#endif                                                
#if VRG_COUNTRY_CFG_CZECH                             
   { VRG_COUNTRY_CZECH,         SI3226_CZECH        },
#endif                                                
#if VRG_COUNTRY_CFG_DENMARK                           
   { VRG_COUNTRY_DENMARK,       SI3226_DENMARK      },
#endif                                                
#if VRG_COUNTRY_CFG_ETSI                              
   { VRG_COUNTRY_ETSI,          SI3226_ETSI         },
#endif                                                
#if VRG_COUNTRY_CFG_FINLAND                           
   { VRG_COUNTRY_FINLAND,       SI3226_FINLAND      },
#endif                                                
#if VRG_COUNTRY_CFG_FRANCE                            
   { VRG_COUNTRY_FRANCE,        SI3226_FRANCE       },
#endif                                                
#if VRG_COUNTRY_CFG_GERMANY                           
   { VRG_COUNTRY_GERMANY,       SI3226_GERMANY      },
#endif                                                
#if VRG_COUNTRY_CFG_HUNGARY     
   { VRG_COUNTRY_HUNGARY,       SI3226_HUNGARY      },
#endif
#if VRG_COUNTRY_CFG_ITALY       
   { VRG_COUNTRY_ITALY,         SI3226_ITALY        },
#endif
#if VRG_COUNTRY_CFG_JAPAN       
   { VRG_COUNTRY_JAPAN,         SI3226_JAPAN        },
#endif
#if VRG_COUNTRY_CFG_MEXICO
   { VRG_COUNTRY_MEXICO,        SI3226_MEXICO       },
#endif
#if VRG_COUNTRY_CFG_NETHERLANDS 
   { VRG_COUNTRY_NETHERLANDS,   SI3226_NETHERLANDS  },
#endif
#if VRG_COUNTRY_CFG_NEW_ZEALAND 
   { VRG_COUNTRY_NEW_ZEALAND,   SI3226_NEW_ZEALAND  },
#endif
#if VRG_COUNTRY_CFG_NORTH_AMERICA
   { VRG_COUNTRY_NORTH_AMERICA, SI3226_NORTH_AMERICA},
#endif
#if VRG_COUNTRY_CFG_POLAND
   { VRG_COUNTRY_POLAND,        SI3226_POLAND       },
#endif
#if VRG_COUNTRY_CFG_SPAIN       
   { VRG_COUNTRY_SPAIN,         SI3226_SPAIN        },
#endif
#if VRG_COUNTRY_CFG_SWEDEN      
   { VRG_COUNTRY_SWEDEN,        SI3226_SWEDEN       },
#endif
#if VRG_COUNTRY_CFG_NORWAY      
   { VRG_COUNTRY_NORWAY,        SI3226_NORWAY       },
#endif
#if VRG_COUNTRY_CFG_SWITZERLAND 
   { VRG_COUNTRY_SWITZERLAND,   SI3226_SWITZERLAND  },
#endif
#if VRG_COUNTRY_CFG_TR57        
   { VRG_COUNTRY_TR57,          SI3226_TR57         },
#endif
#if VRG_COUNTRY_CFG_UK          
   { VRG_COUNTRY_UK,            SI3226_UK           },
#endif
#if VRG_COUNTRY_CFG_TAIWAN      
   { VRG_COUNTRY_TAIWAN,        SI3226_TAIWAN       },
#endif
#if VRG_COUNTRY_CFG_UNITED_ARAB_EMIRATES      
   { VRG_COUNTRY_UNITED_ARAB_EMIRATES,        SI3226_UNITED_ARAB_EMIRATES       },
#endif
#if VRG_COUNTRY_CFG_CYPRUS
   { VRG_COUNTRY_CYPRUS,        SI3226_CYPRUS       },
#endif
   { VRG_COUNTRY_MAX,           SI3226_LOCALE_MAX   }   
};
#endif
   
#endif /* SLAC_SI3226_LOCALE_H */


