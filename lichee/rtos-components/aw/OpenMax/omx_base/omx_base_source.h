/**
  src/base/omx_base_source.h

  OpenMAX base source component. This component does not perform any multimedia
  processing. It derives from base component and contains a single port. It can be used
  as base class for source components.

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


#ifndef _OMX_BASE_SOURCE_COMPONENT_H_
#define _OMX_BASE_SOURCE_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include "omx_base_component.h"

#define OMX_BASE_SOURCE_INPUTPORT_INDEX    0     /* The index of the input port for the derived components */
#define OMX_BASE_SOURCE_CLOCKPORT_INDEX    1     /* The index of the clock port for the derived components */
#define OMX_BASE_SOURCE_INPUTPORT_INDEX_1  1     /* The index of the 2nd input port for the derived components */

/** OMX_BASE_SOURCE_ALLPORT_INDEX as the standard specifies, the -1 value for port index is used to point to all the ports
 */
#define OMX_BASE_SOURCE_ALLPORT_INDEX -1

/** base sink component private structure.
 */
DERIVEDCLASS(omx_base_source_PrivateType, omx_base_component_PrivateType)
#define omx_base_source_PrivateType_FIELDS omx_base_component_PrivateType_FIELDS \
  /** @param BufferMgmtCallback function pointer for algorithm callback */ \
  void (*BufferMgmtCallback)(OMX_COMPONENTTYPE* openmaxStandComp, OMX_BUFFERHEADERTYPE* inputbuffer);
ENDCLASS(omx_base_source_PrivateType)

/** Base source contructor
 */
OMX_ERRORTYPE omx_base_source_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);

/** The base source destructor. It simply calls the base destructor
 */
OMX_ERRORTYPE omx_base_source_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

/** This is the central function for component processing. It
 * is executed in a separate thread, is synchronized with
 * semaphores at each port, those are released each time a new buffer
 * is available on the given port.
 */
void* omx_base_source_BufferMgmtFunction(void* param);

void* omx_base_source_twoport_BufferMgmtFunction (void* param);

#endif
