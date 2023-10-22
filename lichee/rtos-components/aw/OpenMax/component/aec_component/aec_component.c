/**
  src/components/aechocancel_component.c

  OpenMAX audio echocancel control component. This component implements a echocancel that
  echo cancel audio PCM streams and produces a single output stream.

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

#include "aec_component.h"
#include <OMX_Audio.h>

#if defined (CONFIG_COMPONENTS_PROCESS_AEC)
#include "echo_cancellation.h"
#endif

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
OMX_ERRORTYPE omx_audio_echocancel_component_setup(OmxLoaderComponentType *OmxComponents) {

	omx_debug("In %s \n",__func__);

	/** component 1 - volume component */
	OmxComponents->componentVersion.s.nVersionMajor = 1;
	OmxComponents->componentVersion.s.nVersionMinor = 1;
	OmxComponents->componentVersion.s.nRevision = 1;
	OmxComponents->componentVersion.s.nStep = 1;

	strncpy(OmxComponents->name, ECHO_CANCEL_COMP_NAME, OMX_STRINGNAME_SIZE);

	OmxComponents->name_specific_length = NAME_SPECIFIC_NUM;
	OmxComponents->constructor = omx_audio_echocancel_component_Constructor;

	strncpy(OmxComponents->name_specific[0], ECHO_CANCEL_COMP_NAME, OMX_STRINGNAME_SIZE);
	strncpy(OmxComponents->role_specific[0], ECHO_CANCEL_COMP_ROLE, OMX_STRINGNAME_SIZE);

	omx_debug("Out of %s \n",__func__);
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_audio_echocancel_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private;
  omx_audio_echocancel_component_PortType *pPort;// *inPort;
  OMX_U32 i;

  if (!openmaxStandComp->pComponentPrivate) {
    omx_debug("In %s, allocating component\n",__func__);
    openmaxStandComp->pComponentPrivate = omx_alloc(sizeof(omx_audio_echocancel_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    memset(openmaxStandComp->pComponentPrivate, 0x00, sizeof(omx_audio_echocancel_component_PrivateType));
  } else {
    omx_debug("In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
  }

  omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_echocancel_component_Private->ports = NULL;

  /** Calling base echocancel constructor */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  /*Assuming 1 input port and 1 output port*/
  omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = AECHOCANCEL_MAX_PORTS;

  /** Allocate Ports and call port constructor. */
  if (omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_audio_echocancel_component_Private->ports) {
    omx_audio_echocancel_component_Private->ports = omx_alloc(omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
    if (!omx_audio_echocancel_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }

    memset(omx_audio_echocancel_component_Private->ports, 0x00, omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));

    for (i=0; i < omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      omx_audio_echocancel_component_Private->ports[i] = omx_alloc(sizeof(omx_audio_echocancel_component_PortType));
      if (!omx_audio_echocancel_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
      memset(omx_audio_echocancel_component_Private->ports[i], 0x00, sizeof(omx_audio_echocancel_component_PortType));
    }
  }

  /* construct all input ports */
	for(i = 0; i < omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1; i++) {
		base_audio_port_Constructor(openmaxStandComp, &omx_audio_echocancel_component_Private->ports[i], i, OMX_TRUE);
	}

	/* construct one output port */
	base_audio_port_Constructor(openmaxStandComp, &omx_audio_echocancel_component_Private->ports[omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1], omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1, OMX_FALSE);

	/** Domain specific section for the ports. */
	for(i = 0;  i< omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
		pPort = (omx_audio_echocancel_component_PortType *) omx_audio_echocancel_component_Private->ports[i];

		pPort->sPortParam.nBufferSize = AEC_BUF_SIZE;
		pPort->sPortParam.nBufferCountActual = 8;

		setHeader(&pPort->sAudioPcmMode,sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
		pPort->sAudioPcmMode.nPortIndex = i;
		pPort->sAudioPcmMode.nChannels = 3;
		pPort->sAudioPcmMode.eNumData = OMX_NumericalDataSigned;
		pPort->sAudioPcmMode.eEndian = OMX_EndianLittle;
		pPort->sAudioPcmMode.bInterleaved = OMX_TRUE;
		pPort->sAudioPcmMode.nBitPerSample = 16;
		pPort->sAudioPcmMode.nSamplingRate = 16000;
		pPort->sAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;

		setHeader(&pPort->sAudioEcType,sizeof(OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE));
		pPort->sAudioEcType.nPortIndex = i;
		pPort->sAudioEcType.eEchoCancelation = OMX_AUDIO_EchoCanOff;

		setHeader(&pPort->sAecParams,sizeof(OMX_OTHER_PARAM_AECTYPE));
		pPort->sAecParams.nPortIndex = i;
		pPort->sAecParams.pAecprm = NULL;

  }

  omx_audio_echocancel_component_Private->destructor = omx_audio_echocancel_component_Destructor;
  openmaxStandComp->SetParameter = omx_audio_echocancel_component_SetParameter;
  openmaxStandComp->GetParameter = omx_audio_echocancel_component_GetParameter;
  openmaxStandComp->GetConfig = omx_audio_echocancel_component_GetConfig;
  openmaxStandComp->SetConfig = omx_audio_echocancel_component_SetConfig;
  omx_audio_echocancel_component_Private->BufferMgmtCallback = omx_audio_echocancel_component_BufferMgmtCallback;
  omx_audio_echocancel_component_Private->BufferMgmtFunction = omx_audio_echocancel_BufferMgmtFunction;
  omx_audio_echocancel_component_Private->DoStateSet = omx_audio_echocancel_component_SetState;

  return err;
}


/** The destructor
  */
OMX_ERRORTYPE omx_audio_echocancel_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;

  omx_debug("In %s\n", __func__);
  /* frees port/s */
  if (omx_audio_echocancel_component_Private->ports) {
    for (i=0; i < omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      if(omx_audio_echocancel_component_Private->ports[i])
        omx_audio_echocancel_component_Private->ports[i]->PortDestructor(omx_audio_echocancel_component_Private->ports[i]);
    }
    omx_free(omx_audio_echocancel_component_Private->ports);
    omx_audio_echocancel_component_Private->ports = NULL;
  }

  omx_base_filter_Destructor(openmaxStandComp);

  omx_debug("Out of %s\n", __func__);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE omx_audio_echocancel_component_init(OMX_COMPONENTTYPE* openmaxStandComp)
{
	OMX_S32 ret = 0;
	omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
	omx_audio_echocancel_component_PortType* pPort;

	uint32_t rate;
	uint32_t channels;
	uint8_t bitwidth;

	pPort = (omx_audio_echocancel_component_PortType*)omx_audio_echocancel_component_Private->ports[OMX_AUDIO_ECHOCANCEL_INPUTPORT_INDEX];

	channels = pPort->sAudioPcmMode.nChannels;
	bitwidth = pPort->sAudioPcmMode.nBitPerSample;
	rate     = pPort->sAudioPcmMode.nSamplingRate;

	if (channels == 0 || bitwidth== 0 || rate== 0 || pPort->sAecParams.pAecprm == NULL) {
		omx_err("input param error, rate %d, ch %d, width %d pAecprm is NULL\n", \
				rate, channels, bitwidth);
		ret = OMX_ErrorBadParameter;
		return ret;
	}

	omx_audio_echocancel_component_Private->uBytesPerSample = bitwidth * channels / 8;

	omx_info("bits %ld, ch %ld, rate %ld, bytes_perframe %ld\n", \
					pPort->sAudioPcmMode.nBitPerSample, pPort->sAudioPcmMode.nChannels, \
					pPort->sAudioPcmMode.nSamplingRate, omx_audio_echocancel_component_Private->uBytesPerSample);

	/* init aec alth */
#if (defined CONFIG_COMPONENTS_PROCESS_AEC) && ((defined CONFIG_XTENSA_HIFI4) || (defined CONFIG_XTENSA_HIFI5))

	AecConfig sConfig;

	memcpy(&sConfig, (AecConfig *)(pPort->sAecParams.pAecprm), sizeof(AecConfig));

	if (pPort->sAudioEcType.eEchoCancelation) {
		ret = WebRtcAec_Create(&omx_audio_echocancel_component_Private->pAecmInst);
		if (ret < 0) {
			omx_err("Aec_Create error\n");
			ret = OMX_ErrorBadParameter;
			return ret;
		}

		ret = WebRtcAec_Init(omx_audio_echocancel_component_Private->pAecmInst, rate, rate);
		if (ret < 0) {
			omx_err("Aec_Init error\n");
			ret = OMX_ErrorBadParameter;
			return ret;
		}

		ret = WebRtcAec_set_config(omx_audio_echocancel_component_Private->pAecmInst, sConfig);
		if (ret < 0) {
			omx_err("Aec_set_config error\n");
			ret = OMX_ErrorBadParameter;
			return ret;
		}
	}
#endif
	return ret;
}

static void omx_audio_echocancel_component_exit(OMX_COMPONENTTYPE* openmaxStandComp)
{
	omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
	omx_audio_echocancel_component_PortType* pPort;

	pPort = (omx_audio_echocancel_component_PortType*)omx_audio_echocancel_component_Private->ports[OMX_AUDIO_ECHOCANCEL_INPUTPORT_INDEX];

	if (omx_audio_echocancel_component_Private->pAecmInst == NULL)
		return;

#if (defined CONFIG_COMPONENTS_PROCESS_AEC) && ((defined CONFIG_XTENSA_HIFI4) || (defined CONFIG_XTENSA_HIFI5))
	/* exit aec alth */
	if (pPort->sAudioEcType.eEchoCancelation) {
		WebRtcAec_Free(omx_audio_echocancel_component_Private->pAecmInst);
	}
#endif
	omx_audio_echocancel_component_Private->pAecmInst = NULL;
	omx_audio_echocancel_component_Private->uBytesPerSample = 0;

}

/** This function is used to process the input buffer and provide one output buffer.
  *
  *  InBuffer must be 160 or 80 samples, and has 2 channels.
  *
  *  InBuffer nFilledLen = samples * channels * nBitPerSample / 8
  *
  *  e.g 16000hz, 16bit, 2 channel
  *
  *	 InBuffer nFilledLen = 160 * 2 * 2 = 640 bytes or  80 * 2 * 2 = 320 bytes
  *  The duration time is 10ms or 5ms
  *
  *  e.g InBuffer has 320 samples.
  *  first 160 samples is mic in data, last 160 samples is ref data.
  *
  *  InBuffer data arrangement
  *
  *  LLLLLLLLLLLLLLLLLLLLLLLL RRRRRRRRRRRRRRRRRRRRRRR
  *        160 samples               160 samples
  *
  *  OutBuffer
  *  LLLLLLLLLLLLLLLLLLLLLLLL
  *        160 samples
  */
void omx_audio_echocancel_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInBuffer, OMX_BUFFERHEADERTYPE* pOutBuffer) {
  omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_echocancel_component_PortType* pPort;

#if (defined CONFIG_COMPONENTS_PROCESS_AEC) && ((defined CONFIG_XTENSA_HIFI4) || (defined CONFIG_XTENSA_HIFI5))
  OMX_S32 ret = 0;
  OMX_S16 nSampleCount = 0;
  OMX_S16 nSampleOffset= 0;
#endif

  if (pInBuffer == NULL || pOutBuffer == NULL)
	  return;

  pOutBuffer->nFilledLen = 0;

  if (pInBuffer->pBuffer == NULL || pOutBuffer->pBuffer == NULL) {
	  omx_err("inbuffer:%p, pBuffer:%p!!",
		  pInBuffer->pBuffer, pOutBuffer->pBuffer);
	  omx_err("err param!");
	  return;
  }

  if (pInBuffer->nFilledLen == 0) {
	  omx_info("nFilledLen = 0, return!\n");
	  return;
  }

  pPort = (omx_audio_echocancel_component_PortType*)omx_audio_echocancel_component_Private->ports[pInBuffer->nInputPortIndex];

  pOutBuffer->nFlags = pInBuffer->nFlags;
  pOutBuffer->nOffset = pInBuffer->nOffset;
  pOutBuffer->nTimeStamp = pInBuffer->nTimeStamp;

#if (defined CONFIG_COMPONENTS_PROCESS_AEC) && ((defined CONFIG_XTENSA_HIFI4) || (defined CONFIG_XTENSA_HIFI5))

  pOutBuffer->nFilledLen = pInBuffer->nFilledLen / pPort->sAudioPcmMode.nChannels;

  nSampleCount = pInBuffer->nFilledLen / omx_audio_echocancel_component_Private->uBytesPerSample;

  nSampleOffset = pInBuffer->nFilledLen / pPort->sAudioPcmMode.nChannels;


  if (nSampleCount != NN && nSampleCount != NN / 2) {
	  omx_err("nFrameCount %d is invaild, return!\n", nSampleCount);
	  return;
  }


  if (pPort->sAudioEcType.eEchoCancelation) {

	  ret = WebRtcAec_BufferFarend(omx_audio_echocancel_component_Private->pAecmInst, \
		     (OMX_S16 *)(pInBuffer->pBuffer + pInBuffer->nOffset + nSampleOffset), nSampleCount);
	  if (ret < 0)  {
		  omx_err("Aec BufferFarend failed, return!\n");
		  return;
	  }

	  ret = WebRtcAec_Process(omx_audio_echocancel_component_Private->pAecmInst, (OMX_S16 *)(pInBuffer->pBuffer + pInBuffer->nOffset), \
		     NULL, (OMX_S16 *)(pOutBuffer->pBuffer + pOutBuffer->nOffset), NULL, nSampleCount, 0, 0);
	  if (ret < 0)  {
		  omx_err("Aec Process failed, return!\n");
		  return;
	  }
  }
  else {

	  memcpy(pOutBuffer->pBuffer + pOutBuffer->nOffset, pInBuffer->pBuffer + pInBuffer->nOffset,
		  pOutBuffer->nFilledLen);
  }

#else

  pOutBuffer->nFilledLen = pInBuffer->nFilledLen;
  memcpy(pOutBuffer->pBuffer + pOutBuffer->nOffset, pInBuffer->pBuffer + pInBuffer->nOffset,
	  pOutBuffer->nFilledLen);

#endif

  pInBuffer->nFilledLen = 0;
  if (pInBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
	  omx_err("echocancel_buf_handle get the end of stream\n");
	  pOutBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
  }

}

/** setting configurations */
OMX_ERRORTYPE omx_audio_echocancel_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE* pAec;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_echocancel_component_PortType * pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {

    case OMX_IndexConfigAudioEchoCancelation :
      pAec = (OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE*) pComponentConfigStructure;

      if (pAec->nPortIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[pAec->nPortIndex];
        omx_debug("Port %i Enable=%d\n",(int)pAec->nPortIndex,(int)pAec->eEchoCancelation);
        memcpy(&pPort->sAudioEcType, pAec, sizeof(OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;

    default: // delegate to superclass
      err = omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_echocancel_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE  *pAec;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_echocancel_component_PrivateType *omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
  omx_audio_echocancel_component_PortType    *pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {

    case OMX_IndexConfigAudioEchoCancelation :
      pAec = (OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE*) pComponentConfigStructure;
      if (pAec->nPortIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[pAec->nPortIndex];
		omx_debug("Port %i Enable=%d\n",(int)pAec->nPortIndex,(int)pAec->eEchoCancelation);
        memcpy(pAec, &pPort->sAudioEcType, sizeof(OMX_AUDIO_CONFIG_ECHOCANCELATIONTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }

      break;

    default :
      err = omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_echocancel_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_U32                         portIndex;
  OMX_OTHER_PARAM_AECTYPE	      *pAecParams = NULL;
  OMX_AUDIO_PARAM_PCMMODETYPE	  *pAudioPcmMode = NULL;
  omx_audio_echocancel_component_PortType *port;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
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
      if (portIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[portIndex];
        memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_audio_echocancel_component_Private->state != OMX_StateLoaded && omx_audio_echocancel_component_Private->state != OMX_StateWaitForResources) {
        omx_debug("In %s Incorrect State=%x lineno=%d\n",__func__,omx_audio_echocancel_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, ECHO_CANCEL_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;

	case OMX_IndexVendorParamAEC:
		pAecParams = (OMX_OTHER_PARAM_AECTYPE *)ComponentParameterStructure;
		portIndex = pAecParams->nPortIndex;
		 err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAecParams, sizeof(OMX_OTHER_PARAM_AECTYPE));
		if(err!=OMX_ErrorNone) {
			omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
			break;
		}
		if (portIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
			port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[portIndex];
			memcpy(&port->sAecParams, pAecParams, sizeof(OMX_OTHER_PARAM_AECTYPE));
		} else {
			err = OMX_ErrorBadPortIndex;
		}
	break;

  	case OMX_IndexParamAudioPcm:

      pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      portIndex = pAudioPcmMode->nPortIndex;
      err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      if(err != OMX_ErrorNone) {
        omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
        break;
      }
      if (portIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[portIndex];
        memcpy(&port->sAudioPcmMode, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }

	break;
    default:
      err = omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_echocancel_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE     *pAudioPcmMode;
  OMX_OTHER_PARAM_AECTYPE	      *pAecParams = NULL;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  omx_audio_echocancel_component_PortType    *port;
  OMX_COMPONENTTYPE                     *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_audio_echocancel_component_PrivateType *omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;

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
      memcpy(ComponentParameterStructure, &omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioPortFormat->nPortIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[pAudioPortFormat->nPortIndex];
        memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;

	case OMX_IndexVendorParamAEC:
      pAecParams = (OMX_OTHER_PARAM_AECTYPE*)ComponentParameterStructure;
	  if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_OTHER_PARAM_AECTYPE))) != OMX_ErrorNone) {
		break;
	  }
	  if (pAecParams->nPortIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[pAecParams->nPortIndex];
        memcpy(pAecParams, &port->sAecParams, sizeof(OMX_OTHER_PARAM_AECTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;

    case OMX_IndexParamAudioPcm:
      pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (pAudioPcmMode->nPortIndex <= omx_audio_echocancel_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_audio_echocancel_component_PortType *)omx_audio_echocancel_component_Private->ports[pAudioPcmMode->nPortIndex];
        memcpy(pAudioPcmMode, &port->sAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, ECHO_CANCEL_COMP_ROLE);
      break;
    default:
      err = omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_audio_echocancel_component_SetState(OMX_COMPONENTTYPE* openmaxStandComp,
	OMX_U32 dest_state)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	/* Check which structure we are being fed and make control its header */
	omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_STATETYPE pre_state;

	omx_debug(" %p, %ld\n", openmaxStandComp, dest_state);

	if (dest_state == OMX_StateExecuting && (omx_audio_echocancel_component_Private->state == OMX_StateIdle ||
		omx_audio_echocancel_component_Private->state == OMX_StatePause)) {
		if (omx_audio_echocancel_component_Private->pAecmInst != NULL) {
			omx_debug("Device not closed while entering StateIdle");
			omx_audio_echocancel_component_exit(openmaxStandComp);
		}
		omx_debug("audio echocancel init\n");
		ret = omx_audio_echocancel_component_init(openmaxStandComp);
		if (OMX_ErrorNone != ret)
			return ret;
	}

	pre_state = omx_audio_echocancel_component_Private->state;
	ret = omx_base_component_DoStateSet(openmaxStandComp, dest_state);
	if (dest_state == OMX_StatePause && pre_state == OMX_StateExecuting) {
		omx_debug("audio echocancel exit\n");
		omx_audio_echocancel_component_exit(openmaxStandComp);
	}
	if (dest_state == OMX_StateIdle &&
		(pre_state == OMX_StateExecuting || pre_state == OMX_StatePause)) {
		omx_debug("audio echocancel exit\n");
		omx_audio_echocancel_component_exit(openmaxStandComp);
	}
	return ret;

}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_audio_echocancel_BufferMgmtFunction (void* param) {

    OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
    omx_audio_echocancel_component_PrivateType* omx_audio_echocancel_component_Private = (omx_audio_echocancel_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_base_PortType *pInPort=(omx_base_PortType *)omx_audio_echocancel_component_Private->ports[OMX_AUDIO_ECHOCANCEL_INPUTPORT_INDEX];
    omx_base_PortType *pOutPort=(omx_base_PortType *)omx_audio_echocancel_component_Private->ports[OMX_AUDIO_ECHOCANCEL_OUTPUTPORT_INDEX];
    omx_sem_t* pInputSem = pInPort->pBufferSem;
    omx_sem_t* pOutputSem = pOutPort->pBufferSem;
    queue_t* pInputQueue = pInPort->pBufferQueue;
    queue_t* pOutputQueue = pOutPort->pBufferQueue;
    OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
    OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
    OMX_BOOL isInputBufferNeeded=OMX_TRUE,isOutputBufferNeeded=OMX_TRUE;
    int inBufExchanged=0,outBufExchanged=0;

    /* checks if the component is in a state able to receive buffers */
    while(omx_audio_echocancel_component_Private->state == OMX_StateIdle || omx_audio_echocancel_component_Private->state == OMX_StateExecuting ||  omx_audio_echocancel_component_Private->state == OMX_StatePause ||
            omx_audio_echocancel_component_Private->transientState == OMX_TransStateLoadedToIdle) {

        /*Wait till the ports are being flushed*/
        omx_thread_mutex_lock(&omx_audio_echocancel_component_Private->flush_mutex);
        while( PORT_IS_BEING_FLUSHED(pInPort) ||
                PORT_IS_BEING_FLUSHED(pOutPort)) {
            omx_thread_mutex_unlock(&omx_audio_echocancel_component_Private->flush_mutex);

            omx_debug("In %s 1 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
                  __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

            if(isOutputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pOutPort)) {
                pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
                outBufExchanged--;
                pOutputBuffer=NULL;
                isOutputBufferNeeded=OMX_TRUE;
                omx_debug("Ports are flushing,so returning output buffer\n");
            }

            if(isInputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pInPort)) {
                pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
                inBufExchanged--;
                pInputBuffer=NULL;
                isInputBufferNeeded=OMX_TRUE;
                omx_debug("Ports are flushing,so returning input buffer\n");
            }

            omx_debug("In %s 2 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
                  __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

            omx_sem_up(omx_audio_echocancel_component_Private->flush_all_condition);
            omx_sem_down(omx_audio_echocancel_component_Private->flush_condition);
            omx_thread_mutex_lock(&omx_audio_echocancel_component_Private->flush_mutex);
        }
        omx_thread_mutex_unlock(&omx_audio_echocancel_component_Private->flush_mutex);

        /*No buffer to process. So wait here*/
        if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
                (omx_audio_echocancel_component_Private->state != OMX_StateLoaded && omx_audio_echocancel_component_Private->state != OMX_StateInvalid)) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_audio_echocancel_component_Private->bMgmtSem);

        }
        if(omx_audio_echocancel_component_Private->state == OMX_StateLoaded || omx_audio_echocancel_component_Private->state == OMX_StateInvalid) {
            omx_debug("In %s Buffer Management Thread is exiting\n",__func__);
            break;
        }
        if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
                (omx_audio_echocancel_component_Private->state != OMX_StateLoaded && omx_audio_echocancel_component_Private->state != OMX_StateInvalid) &&
                !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_audio_echocancel_component_Private->bMgmtSem);

        }
        if(omx_audio_echocancel_component_Private->state == OMX_StateLoaded || omx_audio_echocancel_component_Private->state == OMX_StateInvalid) {
            omx_debug("In %s Buffer Management Thread is exiting\n",__func__);
            break;
        }

        omx_debug("Waiting for input buffer semval=%d in %s\n",pInputSem->semval, __func__);
        if(pInputSem->semval>0 && isInputBufferNeeded==OMX_TRUE ) {
            omx_sem_down(pInputSem);
            if(pInputQueue->nelem>0) {
                inBufExchanged++;
                isInputBufferNeeded=OMX_FALSE;
                pInputBuffer = dequeue(pInputQueue);
                if(pInputBuffer == NULL) {
				   omx_info("In %s Had NULL output buffer!!\n",__func__);
				   int ret;
				   ret = omx_sem_timed_down(omx_audio_echocancel_component_Private->bMgmtSem, 1000);
				   if (ret < 0) {
						if (ret == ETIMEDOUT) {
							omx_err("wait timeout!");
						} else {
							omx_err("wait error %d!", ret);
						}
					}
				   continue;
			   }
            }
        }
        /*When we have input buffer to process then get one output buffer*/
        if(pOutputSem->semval>0 && isOutputBufferNeeded==OMX_TRUE) {
            omx_sem_down(pOutputSem);
            if(pOutputQueue->nelem>0) {
                outBufExchanged++;
                isOutputBufferNeeded=OMX_FALSE;
                pOutputBuffer = dequeue(pOutputQueue);
                if(pOutputBuffer == NULL) {
                    omx_info("Had NULL output buffer!! op is=%d,iq=%d\n",pOutputSem->semval,pOutputQueue->nelem);
					omx_sem_down(omx_audio_echocancel_component_Private->bMgmtSem);
					continue;

                }
            }
        }

        if(isInputBufferNeeded==OMX_FALSE) {
            if(pInputBuffer->hMarkTargetComponent != NULL) {
                if((OMX_COMPONENTTYPE*)pInputBuffer->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
                    /*Clear the mark and generate an event*/
                    (*(omx_audio_echocancel_component_Private->callbacks->EventHandler))
                    (openmaxStandComp,
                     omx_audio_echocancel_component_Private->callbackData,
                     OMX_EventMark, /* The command was completed */
                     1, /* The commands was a OMX_CommandStateSet */
                     0, /* The state has been changed in message->messageParam2 */
                     pInputBuffer->pMarkData);
                } else {
                    /*If this is not the target component then pass the mark*/
                    omx_audio_echocancel_component_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
                    omx_audio_echocancel_component_Private->pMark.pMarkData            = pInputBuffer->pMarkData;
                }
                pInputBuffer->hMarkTargetComponent = NULL;
            }
        }

        if(isInputBufferNeeded==OMX_FALSE && isOutputBufferNeeded==OMX_FALSE) {

            if(omx_audio_echocancel_component_Private->pMark.hMarkTargetComponent != NULL) {
                pOutputBuffer->hMarkTargetComponent = omx_audio_echocancel_component_Private->pMark.hMarkTargetComponent;
                pOutputBuffer->pMarkData            = omx_audio_echocancel_component_Private->pMark.pMarkData;
                omx_audio_echocancel_component_Private->pMark.hMarkTargetComponent = NULL;
                omx_audio_echocancel_component_Private->pMark.pMarkData            = NULL;
            }

            pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
                omx_debug("Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags = pInputBuffer->nFlags;
                pInputBuffer->nFlags = 0;
            }

            if(omx_audio_echocancel_component_Private->state == OMX_StateExecuting)  {
                if (omx_audio_echocancel_component_Private->BufferMgmtCallback && pInputBuffer->nFilledLen > 0) {
                    (*(omx_audio_echocancel_component_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, pOutputBuffer);
                } else {
                    /*It no buffer management call back the explicitly consume input buffer*/
                    pInputBuffer->nFilledLen = 0;
                }
            } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                omx_debug("In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_audio_echocancel_component_Private->state);
            } else {
                pInputBuffer->nFilledLen = 0;
            }

            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) {
                omx_debug("Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags=pInputBuffer->nFlags;
                pInputBuffer->nFlags=0;
                (*(omx_audio_echocancel_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_audio_echocancel_component_Private->callbackData,
                 OMX_EventBufferFlag, /* The command was completed */
                 1, /* The commands was a OMX_CommandStateSet */
                 pOutputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
                 NULL);
                omx_audio_echocancel_component_Private->bIsEOSReached = OMX_TRUE;
            }
            if(omx_audio_echocancel_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                /*Waiting at paused state*/
                omx_sem_wait(omx_audio_echocancel_component_Private->bStateSem);
            }

            /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
            if((pOutputBuffer->nFilledLen != 0) || ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) || (omx_audio_echocancel_component_Private->bIsEOSReached == OMX_TRUE)) {
                pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
                outBufExchanged--;
                pOutputBuffer=NULL;
                isOutputBufferNeeded=OMX_TRUE;
            }
        }

        if(omx_audio_echocancel_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            /*Waiting at paused state*/
            omx_sem_wait(omx_audio_echocancel_component_Private->bStateSem);
        }

        /*Input Buffer has been completely consumed. So, return input buffer*/
        if((isInputBufferNeeded == OMX_FALSE) && (pInputBuffer->nFilledLen==0)) {
            pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
            inBufExchanged--;
            pInputBuffer=NULL;
            isInputBufferNeeded=OMX_TRUE;
        }
    }
    omx_debug("Out of %s of component %p\n", __func__, openmaxStandComp);
    return NULL;
}

