##################################################################################
#
# Copyright � 2008 Broadcom Corporation
#
# This program  is the  proprietary software  of Broadcom  Corporation and/or  its
# licensors, and may only be used, duplicated, modified or distributed pursuant to
# the  terms and  conditions of  a separate,  written license  agreement executed
# between you and Broadcom (an "Authorized  License").  Except as set forth in  an
# Authorized License, Broadcom  grants no license  (express or implied),  right to
# use, or waiver of any kind with respect to the Software, and Broadcom  expressly
# reserves all rights in and to the Software and all intellectual property  rights
# therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE  THIS
# SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE  ALL
# USE OF THE SOFTWARE.
#
# Except as expressly set forth in the Authorized License,
#
# 1.      This  program,  including  its  structure,  sequence  and  organization,
# constitutes  the valuable  trade secrets  of Broadcom,  and you  shall use  all
# reasonable  efforts to  protect the  confidentiality thereof,  and to  use this
# information only  in connection  with your  use of  Broadcom integrated  circuit
# products.
#
# 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS  IS"
# AND  WITH  ALL  FAULTS  AND  BROADCOM  MAKES  NO  PROMISES,  REPRESENTATIONS  OR
# WARRANTIES, EITHER EXPRESS,  IMPLIED, STATUTORY, OR  OTHERWISE, WITH RESPECT  TO
# THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF
# TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE,  LACK
# OF  VIRUSES,  ACCURACY OR  COMPLETENESS,  QUIET ENJOYMENT,  QUIET  POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE  OR
# PERFORMANCE OF THE SOFTWARE.
#
# 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
# LICENSORS BE  LIABLE FOR  (i) CONSEQUENTIAL,  INCIDENTAL, SPECIAL,  INDIRECT, OR
# EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF  OR IN ANY WAY RELATING TO  YOUR USE
# OF OR INABILITY  TO USE THE  SOFTWARE EVEN IF  BROADCOM HAS BEEN  ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY
# PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
# SHALL APPLY  NOTWITHSTANDING ANY  FAILURE OF  ESSENTIAL PURPOSE  OF ANY  LIMITED
# REMEDY.
#
##################################################################################
#
#                  This makefile is used to build the CCTK module.
#
##################################################################################

ifeq ($(NATT_ROOT),)
$(error "NATT_ROOT hasn't been set")
endif

ifeq ($(M5T_SIPUA_ROOT),)
$(error "M5T_SIPUA_ROOT hasn't been set")
endif

ifeq ($(EXPAT_ROOT),)
$(error "EXPAT_ROOT hasn't been set")
endif
#
# This should match the target name.
#
export CCTK_BUILD_CONFIG_INCLUDE   := $(CCTK_CONFIG_INCLUDE)
XCHG_C_GLOBAL_INCLUDE              += $(XCHG_PUBLIC_INC_assert)  \
                                      $(XCHG_PUBLIC_INC_blog)    \
                                      $(XCHG_PUBLIC_INC_bos)     \
                                      $(XCHG_PUBLIC_INC_str)
XCHG_C_GLOBAL_INCLUDE     += $(CCTK_ROOT)/cctk/cfginc              \
                             $(CCTK_ROOT)/cctk/inc                 \
                             $(NATT_ROOT)/stacks/cfginc             \
                             $(NATT_ROOT)/stacks/ice/inc            \
                             $(NATT_ROOT)/stacks/stunc/inc          \
                             $(NATT_ROOT)/stacks/stunc/stun/inc     \
                             $(M5T_SIPUA_ROOT)/CustomConfig              \
                             $(M5T_SIPUA_ROOT)/M5TSce                    \
                             $(M5T_SIPUA_ROOT)/M5TSdp                    \
                             $(M5T_SIPUA_ROOT)/M5TSipUa                  \
                             $(M5T_SIPUA_ROOT)/M5TFramework              \
                             $(M5T_SIPUA_ROOT)/M5TFramework/Custom/Bos/BosSingleton \
                             $(M5T_SIPUA_ROOT)/ThirdParty/regex          \
                             $(EXPAT_ROOT)/expat/lib
export CCTK_BUILD_IMPORTED_INCLUDE := $(XCHG_C_GLOBAL_INCLUDE)
include $(CCTK_ROOT)/cctk/build/xchg/cctk.mk

XCHG_LIB_BASE     := cctk

CCTK_MODULE_ROOT  := $(CCTK_DIR_$(XCHG_LIB_BASE))


#
# We use notdir and vpath so that the object files don't wind up in
# nested subdirectories
#
XCHG_SRC_FILES_$(XCHG_LIB_BASE) += $(CCTK_BUILD_SRC_FILES)

XCHG_SRC_FILES += $(XCHG_SRC_FILES_$(XCHG_LIB_BASE))

vpath %.c $(CCTK_BUILD_SRC_DIRS)
vpath %.cpp $(CCTK_BUILD_SRC_DIRS)


#
# Add in private directories that are only needed when compiling this library.
# These must be absolute paths in order to support non-recursive makefiles.
#
XCHG_C_LOCAL_INCLUDE_$(XCHG_LIB_BASE) := $(CCTK_BUILD_CONFIG_INCLUDE) \
                                         $(CCTK_BUILD_LOCAL_INCLUDE)

#
# Add in private defs that are only needed when compiling this library.
#
XCHG_C_LOCAL_DEFS_$(XCHG_LIB_BASE) := $(CCTK_BUILD_PRIVATE_DEFS)
ifeq ($(XCHG_OS),eCos)
   XCHG_C_LOCAL_DEFS_$(XCHG_LIB_BASE) += BOS_OS_ECOS MXD_OS_BOS_ECOS
endif

#
# To set a module specific compiler option, create a target specific make
# variable which sets the option and depends on the module target name.
# For example, to enable compiler optimizations for this module:
#
#     $(XCHG_LIB_BASE): XCHG_C_OPTIMIZE_OPT := $(XCHG_C_OPTIMIZE_OPT_ENABLE)
#
# or, to disable optimizations:
#
#     $(XCHG_LIB_BASE): XCHG_C_OPTIMIZE_OPT := $(XCHG_C_OPTIMIZE_OPT_DISABLE)

