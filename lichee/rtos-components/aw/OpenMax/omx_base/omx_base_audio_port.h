/**
  src/base/omx_base_audio_port.h
    
  Base Audio Port class for OpenMAX ports to be used in derived components.
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


#ifndef __OMX_BASE_AUDIO_PORT_H__
#define __OMX_BASE_AUDIO_PORT_H__

#include "omx_classmagic.h"
#include "omx_base_port.h"

/**
 * @brief the base audio domain structure that describes each port. 
 * 
 * The data structure is derived from base port class and contain audio 
 * domain specific parameters.
 * Other elements can be added in the derived components structures. 
 */

DERIVEDCLASS(omx_base_audio_PortType, omx_base_PortType)
#define omx_base_audio_PortType_FIELDS omx_base_PortType_FIELDS \
  /** @param sAudioParam Domain specific (audio) OpenMAX port parameter */ \
  OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioParam; 
ENDCLASS(omx_base_audio_PortType)

/** 
  * @brief the base contructor for the generic OpenMAX ST Audio port
  * 
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the component.
  * It takes care of constructing the instance of the port and 
  * every object needed by the base port.
  *
  * @param openmaxStandComp pointer to the Handle of the component
  * @param openmaxStandPort the ST port to be initialized
  * @param nPortIndex Index of the port to be constructed
  * @param isInput specifices if the port is an input or an output
  * 
  * @return OMX_ErrorInsufficientResources if a memory allocation fails
  */

OMX_ERRORTYPE base_audio_port_Constructor(
  OMX_COMPONENTTYPE *openmaxStandComp,
  omx_base_PortType **openmaxStandPort,
  OMX_U32 nPortIndex, 
  OMX_BOOL isInput);

/** 
  * @brief the base audio port destructor for the generic OpenMAX ST Audio port
  * 
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  * 
  * @param openmaxStandPort the ST port to be destructed
  * 
  * @return OMX_ErrorNone 
  */


OMX_ERRORTYPE base_audio_port_Destructor(
  omx_base_PortType *openmaxStandPort);

#endif
