/**
  components/arecord_component.c

  OpenMAX audio record control component. This component implements a recorder that
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

#include <omx_base_audio_port.h>
#include "OMX_Base.h"
#ifdef CONFIG_COMPONENTS_AW_AUDIO_SYSTEM
#include <AudioSystem.h>
#endif

#include "arecord_component.h"
#include <OMX_Audio.h>


/** @brief The library entry point. It must have the same name for each
  * library of the components loaded by the ST static component loader.
  *
  * This function fills the version, the component name and if existing also the roles
  * and the specific names for each role. This base function is only an explanation.
  * For each library it must be implemented, and it must fill data of any component
  * in the library
  *
  * @param OmxComponents pointer to an array of components descriptors.If NULL, the
  * function will return only the number of components contained in the library
  *
  * @return number of components contained in the library
  */
OMX_ERRORTYPE omx_audio_record_component_setup(OmxLoaderComponentType *OmxComponents) {

	omx_debug("In %s \n",__func__);


	OmxComponents->componentVersion.s.nVersionMajor = 1;
	OmxComponents->componentVersion.s.nVersionMinor = 1;
	OmxComponents->componentVersion.s.nRevision = 1;
	OmxComponents->componentVersion.s.nStep = 1;

	strncpy(OmxComponents->name, RECORD_COMP_NAME, OMX_STRINGNAME_SIZE);

	OmxComponents->name_specific_length = NAME_SPECIFIC_NUM;
	OmxComponents->constructor = omx_audio_record_component_Constructor;

	strncpy(OmxComponents->name_specific[0], RECORD_COMP_NAME, OMX_STRINGNAME_SIZE);
	strncpy(OmxComponents->role_specific[0], RECORD_COMP_ROLE, OMX_STRINGNAME_SIZE);

	omx_debug("Out of %s \n",__func__);
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_audio_record_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_audio_record_component_PrivateType* omx_audio_record_component_Private;
  omx_audio_record_component_PortType *pPort;// *inPort;
  OMX_U32 i;

  if (!openmaxStandComp->pComponentPrivate) {
    omx_debug("In %s, allocating component\n",__func__);
    openmaxStandComp->pComponentPrivate = omx_alloc(sizeof(omx_audio_record_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    memset(openmaxStandComp->pComponentPrivate, 0x00, sizeof(omx_audio_record_component_PrivateType));
  } else {
    omx_debug("In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
  }

  omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;

  /** Calling base sink constructor */
  err = omx_base_sink_Constructor(openmaxStandComp, cComponentName);

  /*Assuming 1 output port port*/
  omx_audio_record_component_Private->ports = NULL;
  omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = AREOCRD_MAX_PORTS;

  omx_debug("In %s, priv %p, nPorts %p \n",__func__ , omx_audio_record_component_Private, \
  	&omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts);

  /** Allocate Ports and call port constructor. */
  if (omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_audio_record_component_Private->ports) {
    omx_debug("In %s, allocating ports\n",__func__);
    omx_audio_record_component_Private->ports = omx_alloc(omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_audio_record_component_PortType *));
    if (!omx_audio_record_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }

    memset(omx_audio_record_component_Private->ports, 0x00, omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_audio_record_component_PortType *));
    for (i=0; i < omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      omx_audio_record_component_Private->ports[i] = omx_alloc(sizeof(omx_audio_record_component_PortType));
      if (!omx_audio_record_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
      memset(omx_audio_record_component_Private->ports[i], 0x00, sizeof(omx_audio_record_component_PortType));
	  omx_debug("In %s, allocating ports addr %p\n",__func__,omx_audio_record_component_Private->ports[i]);
    }
  }

  /* construct output ports */
	err = base_audio_port_Constructor(openmaxStandComp, &omx_audio_record_component_Private->ports[0], 0, OMX_FALSE);
	if (err != OMX_ErrorNone) {
		return OMX_ErrorInsufficientResources;
	}

	/** Domain specific section for the ports. */
	for(i = 0;  i< omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
		pPort = (omx_audio_record_component_PortType *) omx_audio_record_component_Private->ports[i];

		pPort->sPortParam.nBufferSize = AREOCRD_BUF_SIZE;
		pPort->sPortParam.nBufferCountActual = 8;

		setHeader(&pPort->pAudioPcmMode,sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
		pPort->pAudioPcmMode.nPortIndex = i;
		pPort->pAudioPcmMode.nChannels = 3;
		pPort->pAudioPcmMode.eNumData = OMX_NumericalDataSigned;
		pPort->pAudioPcmMode.eEndian = OMX_EndianLittle;
		pPort->pAudioPcmMode.bInterleaved = OMX_TRUE;
		pPort->pAudioPcmMode.nBitPerSample = 16;
		pPort->pAudioPcmMode.nSamplingRate = 16000;
		pPort->pAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;	

  }

  omx_audio_record_component_Private->destructor = omx_audio_record_component_Destructor;
  openmaxStandComp->SetParameter = omx_audio_record_component_SetParameter;
  openmaxStandComp->GetParameter = omx_audio_record_component_GetParameter;
  openmaxStandComp->GetConfig = omx_audio_record_component_GetConfig;
  openmaxStandComp->SetConfig = omx_audio_record_component_SetConfig;
  omx_audio_record_component_Private->BufferMgmtCallback = omx_audio_record_component_BufferMgmtCallback;
  omx_audio_record_component_Private->BufferMgmtFunction = omx_audio_record_BufferMgmtFunction;
  omx_audio_record_component_Private->DoStateSet = omx_audio_record_component_SetState;

  omx_debug("Out %s, nPorts %ld port0 %p\n",__func__ ,omx_audio_record_component_Private->sPortTypesParam[0].nPorts, \
  	omx_audio_record_component_Private->ports[0]);

  return err;
}


/** The destructor
  */
OMX_ERRORTYPE omx_audio_record_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;

  omx_debug("In %s\n", __func__);
  /* frees port/s */
  if (omx_audio_record_component_Private->ports) {
    for (i=0; i < omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      if(omx_audio_record_component_Private->ports[i])
        omx_audio_record_component_Private->ports[i]->PortDestructor(omx_audio_record_component_Private->ports[i]);
    }
    omx_free(omx_audio_record_component_Private->ports);
    omx_audio_record_component_Private->ports = NULL;
  }

  omx_base_sink_Destructor(openmaxStandComp);

  //omx_debug("Out of %s\n", __func__);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE omx_audio_record_component_init(OMX_COMPONENTTYPE* openmaxStandComp)
{
	OMX_S32 ret = 0;
	omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
	omx_audio_record_component_PortType* pPort;
	tAudioRecord *pAudioRecord = NULL;
	uint32_t rate;
	uint32_t channels;
	uint8_t bitwidth;

	pPort = (omx_audio_record_component_PortType*)omx_audio_record_component_Private->ports[OMX_AUDIO_REOCRD_OUTPUTPORT_INDEX];

	channels = pPort->pAudioPcmMode.nChannels;
	bitwidth = pPort->pAudioPcmMode.nBitPerSample;
	rate     = pPort->pAudioPcmMode.nSamplingRate;
	if (channels == 0 || bitwidth== 0 || rate== 0) {
		omx_err("input param error, rate %d, ch %d, width %d\n", \
				rate, channels, bitwidth);
		ret = OMX_ErrorBadParameter;
		return ret;
	}

	omx_audio_record_component_Private->bytes_perframe = bitwidth * channels / 8;

	omx_debug("audio record create name %s\n", RECORD_NAME);
	pAudioRecord = AudioRecordCreate(RECORD_NAME);
	if (!pAudioRecord) {
		omx_err("ar create failed\n");
		ret = OMX_ErrorResourcesLost;
		return ret;
	}

	ret = AudioRecordSetup(pAudioRecord, rate, channels, bitwidth);

	omx_audio_record_component_Private->ar = (void *)pAudioRecord;

	return ret;
}

static void omx_audio_record_component_exit(OMX_COMPONENTTYPE* openmaxStandComp)
{
	omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
	tAudioRecord *pAudioRecord = (tAudioRecord *)omx_audio_record_component_Private->ar;
	omx_debug("In\n");

	if (pAudioRecord == NULL)
		return;

	AudioRecordStop(pAudioRecord);
	AudioRecordDestroy(pAudioRecord);

	pAudioRecord = NULL;

}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_audio_record_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pOutBuffer) {

  omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  tAudioRecord *pAudioRecord = (tAudioRecord *)omx_audio_record_component_Private->ar;
  int ret = 0;

  omx_audio_record_component_PortType* pPort;

  pPort = (omx_audio_record_component_PortType*)omx_audio_record_component_Private->ports[pOutBuffer->nInputPortIndex];

  if (pAudioRecord == NULL
	  || pOutBuffer == NULL
	  || pOutBuffer->nAllocLen == 0
	  || pOutBuffer->pBuffer == NULL) {
	  omx_err("ar :%p, inbuffer:%p, nAllocLen:%ld, pBuffer:%p!!",
		  pAudioRecord,
		  pOutBuffer, pOutBuffer->nAllocLen, pOutBuffer->pBuffer);
	  omx_err("err param!");
	  return;
  }

  pOutBuffer->nOffset = 0;
  pOutBuffer->nFlags = 0;

  ret = AudioRecordRead(pAudioRecord, pOutBuffer->pBuffer, pOutBuffer->nAllocLen);
  if (ret <= 0) {
	  omx_err("audio read data error ret %d", ret);
	  pOutBuffer->nFilledLen = 0;
	  return;
  }

  pOutBuffer->nFilledLen = pOutBuffer->nAllocLen;

  omx_audio_record_component_Private->timestamp += (pOutBuffer->nFilledLen >> 1) / pPort->pAudioPcmMode.nChannels;;
  omx_debug("read audio buffer len %ld", pOutBuffer->nAllocLen);
}

/** setting configurations */
OMX_ERRORTYPE omx_audio_record_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  //OMX_AUDIO_CONFIG_VOLUMETYPE* pVolume;
//  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  //omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  //omx_audio_record_component_PortType * pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
  	/*
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if(pVolume->sVolume.nValue > 100) {
        err =  OMX_ErrorBadParameter;
        break;
      }

      if (pVolume->nPortIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[pVolume->nPortIndex];
        omx_debug("Port %i Gain=%d\n",(int)pVolume->nPortIndex,(int)pVolume->sVolume.nValue);
        memcpy(&pPort->sVolume, pVolume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
     */
    default: // delegate to superclass
      err = omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_record_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {
  //OMX_AUDIO_CONFIG_VOLUMETYPE           *pVolume;
  //OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  //omx_audio_record_component_PrivateType *omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  //omx_audio_record_component_PortType    *pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
  	/*
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if (pVolume->nPortIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[pVolume->nPortIndex];
        memcpy(pVolume,&pPort->sVolume,sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
     */
    default :
      err = omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_record_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_U32                         portIndex;
  OMX_AUDIO_PARAM_PCMMODETYPE *pAudioParams = NULL;
  omx_audio_record_component_PortType *port;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }

  omx_debug("   Setting parameter %i\n", nParamIndex);
  switch(nParamIndex) {
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      portIndex = pAudioPortFormat->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      if(err!=OMX_ErrorNone) {
        omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      if (portIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[portIndex];
        memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_audio_record_component_Private->state != OMX_StateLoaded && omx_audio_record_component_Private->state != OMX_StateWaitForResources) {
        omx_debug("In %s Incorrect State=%x lineno=%d\n",__func__,omx_audio_record_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, RECORD_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;
  	case OMX_IndexParamAudioPcm:
	  pAudioParams = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      portIndex = pAudioParams->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      if(err != OMX_ErrorNone) {
        omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      if (portIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[portIndex];
        memcpy(&port->pAudioPcmMode, pAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
	break;
    default:
      err = omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_record_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE     *pAudioPcmMode;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  omx_audio_record_component_PortType    *port;
  OMX_COMPONENTTYPE                     *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_record_component_PrivateType *omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  omx_debug("   Getting parameter %i\n", nParamIndex);
  /* Check which structure we are being fed and fill its header */
  switch(nParamIndex) {
    case OMX_IndexParamAudioInit:
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
        break;
      }
      memcpy(ComponentParameterStructure, &omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioPortFormat->nPortIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[pAudioPortFormat->nPortIndex];
        memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamAudioPcm:
      pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (pAudioPcmMode->nPortIndex <= omx_audio_record_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_record_component_PortType *)omx_audio_record_component_Private->ports[pAudioPcmMode->nPortIndex];
        memcpy(pAudioPcmMode, &port->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, RECORD_COMP_ROLE);
      break;
    default:
      err = omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_record_component_SetState(OMX_COMPONENTTYPE* openmaxStandComp,
	OMX_U32 dest_state)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	/* Check which structure we are being fed and make control its header */
	omx_audio_record_component_PrivateType* omx_audio_record_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_STATETYPE pre_state;

	omx_debug(" %p, %ld\n", openmaxStandComp, dest_state);

	if (dest_state == OMX_StateExecuting && (omx_audio_record_component_Private->state == OMX_StateIdle ||
		omx_audio_record_component_Private->state == OMX_StatePause)) {
		if (omx_audio_record_component_Private->ar != NULL) {
			omx_debug("Device not closed while entering StateIdle");
			omx_audio_record_component_exit(openmaxStandComp);
		}
		omx_audio_record_component_Private->timestamp = -1;
		omx_debug("audio record init\n");
		ret = omx_audio_record_component_init(openmaxStandComp);
		if (OMX_ErrorNone != ret)
			return ret;
	}

	pre_state = omx_audio_record_component_Private->state;
	ret = omx_base_component_DoStateSet(openmaxStandComp, dest_state);
	if (dest_state == OMX_StatePause && pre_state == OMX_StateExecuting) {
		omx_debug("audio record exit\n");
		omx_audio_record_component_exit(openmaxStandComp);
	}
	if (dest_state == OMX_StateIdle &&
		(pre_state == OMX_StateExecuting || pre_state == OMX_StatePause)) {
		omx_debug("audio record exit\n");
		omx_audio_record_component_exit(openmaxStandComp);
	}
	return ret;

}

/** This is the central function for component processing,overridden for audio record. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_audio_record_BufferMgmtFunction (void* param) {

  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_audio_record_component_PrivateType* omx_audio_record_component_Private =
  	(omx_audio_record_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  omx_base_PortType* pOutPort = (omx_base_PortType*)omx_audio_record_component_Private->ports[OMX_AUDIO_REOCRD_OUTPUTPORT_INDEX];

  omx_sem_t* pOutputSem = pOutPort->pBufferSem;
  queue_t* pOutputQueue = pOutPort->pBufferQueue;
  OMX_BUFFERHEADERTYPE* pOutputBuffer = NULL;
  OMX_COMPONENTTYPE* target_component;
  OMX_BOOL isOutputBufferNeeded = OMX_TRUE;
  int outBufExchanged = 0;

  omx_debug("In %s\n", __func__);
  while(omx_audio_record_component_Private->state == OMX_StateIdle || omx_audio_record_component_Private->state == OMX_StateExecuting ||  omx_audio_record_component_Private->state == OMX_StatePause ||
    omx_audio_record_component_Private->transientState == OMX_TransStateLoadedToIdle) {

    /*Wait till the ports are being flushed*/
	omx_thread_mutex_lock(&omx_audio_record_component_Private->flush_mutex);
	while( PORT_IS_BEING_FLUSHED(pOutPort)) {
	   omx_thread_mutex_unlock(&omx_audio_record_component_Private->flush_mutex);

	   if(isOutputBufferNeeded == OMX_FALSE) {
		   pOutPort->ReturnBufferFunction(pOutPort, pOutputBuffer);
		   outBufExchanged--;
		   pOutputBuffer = NULL;
		   isOutputBufferNeeded = OMX_TRUE;
		   omx_debug("Ports are flushing,so returning output buffer\n");
	   }
	   omx_debug("In %s signalling flush all condition \n", __func__);

	   omx_sem_up(omx_audio_record_component_Private->flush_all_condition);
	   omx_sem_down(omx_audio_record_component_Private->flush_condition);
	   omx_thread_mutex_lock(&omx_audio_record_component_Private->flush_mutex);
	}
	omx_thread_mutex_unlock(&omx_audio_record_component_Private->flush_mutex);

	/*No buffer to process. So wait here*/
	if((isOutputBufferNeeded == OMX_TRUE && pOutputSem->semval == 0) &&
			(omx_audio_record_component_Private->state != OMX_StateLoaded && omx_audio_record_component_Private->state != OMX_StateInvalid) &&
        PORT_IS_ENABLED(pOutPort) && !PORT_IS_BEING_FLUSHED(pOutPort)) {
		omx_debug("Waiting for output buffer \n");
		omx_sem_down(omx_audio_record_component_Private->bMgmtSem);
	}

    if(omx_audio_record_component_Private->state == OMX_StateLoaded || omx_audio_record_component_Private->state == OMX_StateInvalid) {
      omx_debug("In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

	omx_debug("Waiting for output buffer semval=%d \n",pOutputSem->semval);
	if(pOutputSem->semval > 0 && isOutputBufferNeeded == OMX_TRUE && PORT_IS_ENABLED(pOutPort)) {
	   omx_sem_down(pOutputSem);
	   if(pOutputQueue->nelem>0) {
		   outBufExchanged++;
		   isOutputBufferNeeded = OMX_FALSE;
		   pOutputBuffer = dequeue(pOutputQueue);
		   if(pOutputBuffer == NULL) {
			   omx_debug("In %s Had NULL output buffer!!\n",__func__);
			   omx_sem_down(omx_audio_record_component_Private->bMgmtSem);
			   continue;
		   }
	   }
	}

	if(isOutputBufferNeeded == OMX_FALSE) {
		if((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
			pOutputBuffer->nFlags = 0;
		}

		if(omx_audio_record_component_Private->pMark.hMarkTargetComponent != NULL) {
			pOutputBuffer->hMarkTargetComponent = omx_audio_record_component_Private->pMark.hMarkTargetComponent;
			pOutputBuffer->pMarkData			= omx_audio_record_component_Private->pMark.pMarkData;
			omx_audio_record_component_Private->pMark.hMarkTargetComponent = NULL;
			omx_audio_record_component_Private->pMark.pMarkData			  = NULL;
		}

		target_component = (OMX_COMPONENTTYPE*)pOutputBuffer->hMarkTargetComponent;
		if(target_component == (OMX_COMPONENTTYPE *)openmaxStandComp) {
			/*Clear the mark and generate an event*/
			(*(omx_audio_record_component_Private->callbacks->EventHandler))
			(openmaxStandComp,
			 omx_audio_record_component_Private->callbackData,
			 OMX_EventMark, /* The command was completed */
			 1, /* The commands was a OMX_CommandStateSet */
			 0, /* The state has been changed in message->messageParam2 */
			 pOutputBuffer->pMarkData);
		} else if(pOutputBuffer->hMarkTargetComponent != NULL) {
			/*If this is not the target component then pass the mark*/
			omx_debug("Pass Mark. This is a sink!!\n");
		}

		if(omx_audio_record_component_Private->state == OMX_StateExecuting)	{
			if (omx_audio_record_component_Private->BufferMgmtCallback && pOutputBuffer->nFilledLen == 0) {
				(*(omx_audio_record_component_Private->BufferMgmtCallback))(openmaxStandComp, pOutputBuffer);
			} else {
				/*It no buffer management call back then don't produce any output buffer*/
				pOutputBuffer->nFilledLen = 0;
			}
		} else {
			if(OMX_TransStateExecutingToIdle == omx_audio_record_component_Private->transientState ||
				   OMX_TransStatePauseToIdle == omx_audio_record_component_Private->transientState) {
	              pOutputBuffer->nFilledLen = 0;
	        }
			omx_debug("In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_audio_record_component_Private->state);
		}
		if(omx_audio_record_component_Private->state == OMX_StatePause && !PORT_IS_BEING_FLUSHED(pOutPort)) {
			/*Waiting at paused state*/
			omx_sem_wait(omx_audio_record_component_Private->bStateSem);
		}

		if((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
			omx_debug("Detected EOS flags in output buffer\n");

			(*(omx_audio_record_component_Private->callbacks->EventHandler))
			(openmaxStandComp,
			 omx_audio_record_component_Private->callbackData,
			 OMX_EventBufferFlag, /* The command was completed */
			 0, /* The commands was a OMX_CommandStateSet */
			 pOutputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
			 NULL);
			//pOutputBuffer->nFlags = 0;
			omx_audio_record_component_Private->bIsEOSReached = OMX_TRUE;
		}

		/*Output Buffer has been produced or EOS. So, return output buffer and get new buffer*/
		if((pOutputBuffer->nFilledLen != 0) || ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) ==OMX_BUFFERFLAG_EOS) || (omx_audio_record_component_Private->bIsEOSReached == OMX_TRUE)) {
			if((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
				omx_debug("In %s nFlags=%x Name=%s \n", __func__, (int)pOutputBuffer->nFlags, omx_audio_record_component_Private->name);
			pOutPort->ReturnBufferFunction(pOutPort, pOutputBuffer);
			outBufExchanged--;
			pOutputBuffer = NULL;
			isOutputBufferNeeded = OMX_TRUE;
		}
	}

    omx_debug("Output buffer arrived\n");

  }
  omx_debug("Exiting Buffer Management Thread\n");
  return NULL;
}

