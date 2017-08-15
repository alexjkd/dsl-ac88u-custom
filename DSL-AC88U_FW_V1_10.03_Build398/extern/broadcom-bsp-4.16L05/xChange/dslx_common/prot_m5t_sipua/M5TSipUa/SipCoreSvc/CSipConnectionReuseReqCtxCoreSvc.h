//==============================================================================
//==============================================================================
//
//   Copyright(c) 2008 Media5 Corporation ("Media5")
//
//  NOTICE:
//   This document contains information that is confidential and proprietary to
//   Media5.
//
//   Media5 reserves all rights to this document as well as to the Intellectual
//   Property of the document and the technology and know-how that it includes
//   and represents.
//
//   This publication cannot be reproduced, neither in whole nor in part, in any
//   form whatsoever without prior written approval by Media5.
//
//   Media5 reserves the right to revise this publication and make changes at
//   any time and without the obligation to notify any person and/or entity of
//   such revisions and/or changes.
//
//==============================================================================
//==============================================================================
#ifndef MXG_CSIPCONNECTIONREUSEREQCTXCORESVC_H
//M5T_INTERNAL_USE_BEGIN
#define MXG_CSIPCONNECTIONREUSEREQCTXCORESVC_H

//==============================================================================
//====  INCLUDES + FORWARD DECLARATIONS  =======================================
//==============================================================================

//-- M5T Global definitions
#ifndef MXG_MXCONFIG_H
#include "Config/MxConfig.h"
#endif

#ifndef MXG_SIPSTACKCFG_H
#include "Config/SipStackCfg.h"
#endif

#ifndef MXD_SIPSTACK_ENABLE_SIP_CONNECTION_REUSE_SVC_SUPPORT
#error You must define \
MXD_SIPSTACK_ENABLE_SIP_CONNECTION_REUSE_SVC_SUPPORT to be able to use \
this class.
#endif

//-- Data Members

//-- Interface Realized and/or Parent


#ifndef MXG_CSIPREQCTXCORESVC_H
#include "SipCore/CSipReqCtxCoreSvc.h"
#endif

MX_NAMESPACE_START(MXD_GNS)

//-- Forward Declarations
class CSipConnectionReuseSvc;

//==============================================================================
//====  CONSTANTS + DEFINES  ===================================================
//==============================================================================

//==============================================================================
//====  NEW TYPE DEFINITIONS  ==================================================
//==============================================================================

//==============================================================================
//== Class: CSipConnectionReuseReqCtxCoreSvc
//==============================================================================
//
// Description:
//   This class implements the ISipReqCtxCoreSvc interface which is used
//   by the ISipRequestContext to interact with the core services.
//   It is obtained by the request context through the ISipCoreSvc
//   interface of the CSipConnectionReuseSvc.
//
//   This sets the peer and local addresses when they come from the first
//   incoming request.
//
// Location:
//   SipCoreSvc/CSipConnectionReuseReqCtxCoreSvc.h
//
//==============================================================================
class CSipConnectionReuseReqCtxCoreSvc : public CSipReqCtxCoreSvc
{
//-- Friend Declarations

//-- Published Interface
public:

    //-- << Constructors / Destructors / Operators >>

    // Summary:
    //  Constructor.
    CSipConnectionReuseReqCtxCoreSvc(IN CSipConnectionReuseSvc* pSipConnectionReuseSvc,
                                     IN bool bAddRef = true);

    // Summary:
    //  Destructor.
    virtual ~CSipConnectionReuseReqCtxCoreSvc();

// Hidden Methods
private:
    //-- << CSipReqCtxCoreSvc methods >>

        // Summary:
    //  Updates rPacket with the resolved target.
    virtual mxt_result UpdatePacket(IN ISipRequestContext& rRequestContext,
                                    INOUT CSipPacket& rPacket);

    // Summary:
    //  Informs the request context core service of a received packet.
    virtual void OnPacketReceived(IN    ISipRequestContext& rRequestContext,
                                  IN    const CSipPacket&   rPacket,
                                  INOUT CSipCoreEventList&  rEventList);

    // Summary:
    //  Add a reference on this request context core service.
    virtual unsigned int AddRcsRef();

    // Summary:
    //  Remove a reference on this request context core service.
    virtual unsigned int ReleaseRcsRef();

//-- Hidden Methods
protected:
private:

    //-- << Deactivated Constructors / Destructors / Operators >>

    // GCC 2.7.2 does not like empty private copy constructors (when building in debug).
#if !defined(MXD_COMPILER_GNU_GCC) || \
    (defined(MXD_COMPILER_GNU_GCC) && MXD_COMPILER_GNU_GCC > MXD_COMPILER_GNU_GCC_2_7)

    // Summary:
    //  Copy Constructor.
    CSipConnectionReuseReqCtxCoreSvc(IN const CSipConnectionReuseReqCtxCoreSvc& rSrc);

    // Summary:
    //  Assignment Operator.
    CSipConnectionReuseReqCtxCoreSvc& operator=(IN const CSipConnectionReuseReqCtxCoreSvc& rSrc);

#endif

// Friends declarations

//-- Hidden Data Members
protected:
private:

    CSipConnectionReuseSvc* m_pSipConnectionReuseSvc;

    unsigned int m_uRefCount;
};

MX_NAMESPACE_END(MXD_GNS)
//M5T_INTERNAL_USE_END
#endif //-- #ifndef MXG_CSIPCONNECTIONREUSEREQCTXCORESVC_H
