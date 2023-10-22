/**
  src/components/dump_component.h

  OpenMAX dumps component. This component implements a dump filter that
  dump audio PCM streams and produces a single output stream.

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


#ifndef _OMX_DUMP_COMPONENT_H_
#define _OMX_DUMP_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <string.h>
#include <stdio.h>

#include "omx_base_audio_port.h"
#include "omx_base_filter.h"
#include "OMXComponentLoader.h"


#define DUMP_COMP_NAME "OMX.dump"
#define DUMP_COMP_ROLE "dump"

#define DUMP_MAX_PORTS   2 // Maximum number of ports supported by the dump. 1 input and 1 output
#define DUMP_BUF_SIZE 1920 //16kHz/3ch/16bit-20ms
/** OMX_BASE_FILTER_INPUTPORT_INDEX is the index of any input port for the derived components
 */
#define OMX_DUMP_INPUTPORT_INDEX 0

/** OMX_BASE_FILTER_OUTPUTPORT_INDEX is the index of any output port for the derived components
 */
#define OMX_DUMP_OUTPUTPORT_INDEX 1

extern OMX_ERRORTYPE omx_dump_component_setup(OmxLoaderComponentType *OmxComponents);

typedef struct  rpdata_arg_dump{
	OMX_S8  type[OMX_STRINGNAME_SIZE];
	OMX_S8  name[OMX_STRINGNAME_SIZE];
	OMX_S32 dir;
}rpdata_arg_dump;

/** dump port structure.
  */
DERIVEDCLASS(omx_dump_component_PortType, omx_base_audio_PortType)
#define omx_dump_component_PortType_FIELDS omx_base_audio_PortType_FIELDS \
  /** @param pAudioPcmMode Referece to OMX_AUDIO_PARAM_PCMMODETYPE structure*/  \
  OMX_AUDIO_PARAM_PCMMODETYPE	       pAudioPcmMode; \
  OMX_OTHER_PARAM_DUMPTYPE	   		   pDumpParam; /**< Domain specific (other) OpenMAX port parameter */
ENDCLASS(omx_dump_component_PortType)

/** Twoport component private structure.
* see the define above
*/
DERIVEDCLASS(omx_dump_component_PrivateType, omx_base_filter_PrivateType)
#define omx_dump_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
	FILE 	    *dumpfile; \
	OMX_S32 	 dump_forward_port; \
	OMX_PTR  	 rpd; \
	OMX_PTR  	 buffer; \
	OMX_S8 	     rpdata_type[OMX_STRINGNAME_SIZE]; \
	OMX_S8 	     rpdata_name[OMX_STRINGNAME_SIZE]; \
	OMX_S8 	     pathname[OMX_STRINGNAME_SIZE];

ENDCLASS(omx_dump_component_PrivateType)


OMX_ERRORTYPE omx_dump_component_setup(OmxLoaderComponentType *OmxComponents);

/* Component private entry points declaration */
OMX_ERRORTYPE omx_dump_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_dump_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);


void omx_dump_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* inputbuffer,
  OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_dump_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_dump_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_dump_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_dump_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_dump_component_SetState(
	OMX_COMPONENTTYPE* openmaxStandComp,
	OMX_U32 dest_state);

/** This is the central function for component processing, overridden for audio mixer. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_dump_BufferMgmtFunction (void* param);

#endif
