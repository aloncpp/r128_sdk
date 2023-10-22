/**
  components/arecord_component.h

  OpenMAX audio_record control component. This component implements a recorder that
  record audio PCM streams and produces a single output stream.

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


#ifndef _OMX_AUDIO_RECORD_COMPONENT_H_
#define _OMX_AUDIO_RECORD_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <string.h>


#include "omx_base_audio_port.h"
#include "omx_base_sink.h"
#include "OMXComponentLoader.h"


#define RECORD_COMP_NAME "OMX.audio.record"
#define RECORD_COMP_ROLE "audio.record"

#define AREOCRD_MAX_PORTS   1 // Maximum number of ports supported by the record. 1 output port
#define AREOCRD_BUF_SIZE 1920 //16kHz/3ch/16bit-20ms

/** OMX_AUDIO_REOCRD_OUTPUTPORT_INDEX is the index of any output port for the derived components
 */
#define OMX_AUDIO_REOCRD_OUTPUTPORT_INDEX 0

#if defined(CONFIG_ARCH_DSP)
#define RECORD_NAME "capture"
#else
#define RECORD_NAME "amp"
#endif

extern OMX_ERRORTYPE omx_audio_record_component_setup(OmxLoaderComponentType *OmxComponents);


/** Audio record port structure.
  */
DERIVEDCLASS(omx_audio_record_component_PortType, omx_base_audio_PortType)
#define omx_audio_record_component_PortType_FIELDS omx_base_audio_PortType_FIELDS \
  /** @param pAudioPcmMode Referece to OMX_AUDIO_PARAM_PCMMODETYPE structure*/  \
  OMX_AUDIO_PARAM_PCMMODETYPE pAudioPcmMode;
ENDCLASS(omx_audio_record_component_PortType)

/** Twoport component private structure.
* see the define above
*/
DERIVEDCLASS(omx_audio_record_component_PrivateType, omx_base_sink_PrivateType)
#define omx_audio_record_component_PrivateType_FIELDS omx_base_sink_PrivateType_FIELDS \
	OMX_PTR		ar; \
	OMX_U32 	bytes_perframe; \
	OMX_TICKS	timestamp;
ENDCLASS(omx_audio_record_component_PrivateType)


OMX_ERRORTYPE omx_audio_record_component_setup(OmxLoaderComponentType *OmxComponents);

/* Component private entry points declaration */
OMX_ERRORTYPE omx_audio_record_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_audio_record_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);


void omx_audio_record_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* pOutBuffer);

OMX_ERRORTYPE omx_audio_record_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_audio_record_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_audio_record_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_audio_record_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_audio_record_component_SetState(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_U32 dest_state);

/** This is the central function for component processing, overridden for audio mixer. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_audio_record_BufferMgmtFunction (void* param);

#endif
