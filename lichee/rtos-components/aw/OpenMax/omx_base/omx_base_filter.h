/**
  src/base/omx_base_filter.h

  OpenMAX Base Filter component. This component does not perform any multimedia
  processing. It derives from base component and contains two ports. It can be used
  as base class for codec and filter components.

* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#ifndef __OMX_BASE_FILTER_H__
#define __OMX_BASE_FILTER_H__


#include "omx_base_component.h"

/** OMX_BASE_FILTER_INPUTPORT_INDEX is the index of any input port for the derived components
 */
#define OMX_BASE_FILTER_INPUTPORT_INDEX 0

/** OMX_BASE_FILTER_OUTPUTPORT_INDEX is the index of any output port for the derived components
 */
#define OMX_BASE_FILTER_OUTPUTPORT_INDEX 1

/** OMX_BASE_FILTER_ALLPORT_INDEX as the standard specifies, the -1 value for port index is used to point to all the ports
 */
#define OMX_BASE_FILTER_ALLPORT_INDEX -1

/** Base Filter component private structure.
 */
DERIVEDCLASS(omx_base_filter_PrivateType, omx_base_component_PrivateType)
#define omx_base_filter_PrivateType_FIELDS omx_base_component_PrivateType_FIELDS \
  /** @param pPendingOutputBuffer pending Output Buffer pointer */ \
  OMX_BUFFERHEADERTYPE* pPendingOutputBuffer; \
  /** @param BufferMgmtCallback function pointer for algorithm callback */ \
  void (*BufferMgmtCallback)(OMX_COMPONENTTYPE* openmaxStandComp, OMX_BUFFERHEADERTYPE* inputbuffer, OMX_BUFFERHEADERTYPE* outputbuffer);
ENDCLASS(omx_base_filter_PrivateType)

/**
 * @brief The base filter contructor for the OpenMAX ST components
 *
 * @param openmaxStandComp the ST component to be initialized
 * @param cComponentName the OpenMAX string that describes the component
 *
 * @return OMX_ErrorInsufficientResources if a memory allocation fails
 */
OSCL_IMPORT_REF OMX_ERRORTYPE omx_base_filter_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);

/** @brief the base filter destructor for ST OpenMAX components
 *
 * @param openmaxStandComp the ST OpenMAX component to be disposed
 */
OSCL_IMPORT_REF OMX_ERRORTYPE omx_base_filter_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

/** This is the central function for component processing. It
 * is executed in a separate thread, is synchronized with
 * semaphores at each port, those are released each time a new buffer
 * is available on the given port.
 */
void* omx_base_filter_BufferMgmtFunction(void* param);


#endif
