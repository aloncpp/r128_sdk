/**
  src/components/dump_component.c

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

#include "OMX_Base.h"

#include "dump_component.h"
#include <OMX_Audio.h>

#ifdef CONFIG_COMPONENTS_RPDATA
#include <rpdata.h>
#endif

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
#include <adb_forward.h>
#endif

/** @brief The library entry point. It must have the same name for each
  * library of the components loaded by the component loader.
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
OMX_ERRORTYPE omx_dump_component_setup(OmxLoaderComponentType *OmxComponents) {

	omx_debug("In %s \n",__func__);

	/** component 1 - volume component */
	OmxComponents->componentVersion.s.nVersionMajor = 1;
	OmxComponents->componentVersion.s.nVersionMinor = 1;
	OmxComponents->componentVersion.s.nRevision = 1;
	OmxComponents->componentVersion.s.nStep = 1;

	strncpy(OmxComponents->name, DUMP_COMP_NAME, OMX_STRINGNAME_SIZE);

	OmxComponents->name_specific_length = NAME_SPECIFIC_NUM;
	OmxComponents->constructor = omx_dump_component_Constructor;

	strncpy(OmxComponents->name_specific[0], DUMP_COMP_NAME, OMX_STRINGNAME_SIZE);
	strncpy(OmxComponents->role_specific[0], DUMP_COMP_ROLE, OMX_STRINGNAME_SIZE);

	omx_debug("Out of %s \n",__func__);
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_dump_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_dump_component_PrivateType* omx_dump_component_Private;
  omx_dump_component_PortType *pPort;
  OMX_U32 i;

  if (!openmaxStandComp->pComponentPrivate) {
    omx_debug("In %s, allocating component\n",__func__);
    openmaxStandComp->pComponentPrivate = omx_alloc(sizeof(omx_dump_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    memset(openmaxStandComp->pComponentPrivate, 0x00, sizeof(omx_dump_component_PrivateType));
  } else {
    omx_debug("In %s, Error Component %p Already Allocated\n", __func__, openmaxStandComp->pComponentPrivate);
  }

  omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
  omx_dump_component_Private->ports = NULL;

  /** Calling base filter constructor */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  /*Assuming 1 input port and 1 output port*/
  omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = DUMP_MAX_PORTS;

  /** Allocate Ports and call port constructor. */
  if (omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_dump_component_Private->ports) {
    omx_dump_component_Private->ports = omx_alloc(omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));
    if (!omx_dump_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }

    memset(omx_dump_component_Private->ports, 0x00, omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts*sizeof(omx_base_PortType *));

    for (i=0; i < omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      omx_dump_component_Private->ports[i] = omx_alloc(sizeof(omx_dump_component_PortType));
      if (!omx_dump_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
      memset(omx_dump_component_Private->ports[i], 0x00, sizeof(omx_dump_component_PortType));
    }
  }

  /* construct all input ports */
	for(i = 0; i < omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1; i++) {
		base_audio_port_Constructor(openmaxStandComp, &omx_dump_component_Private->ports[i], i, OMX_TRUE);
	}

	/* construct one output port */
	base_audio_port_Constructor(openmaxStandComp, &omx_dump_component_Private->ports[omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1], omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts - 1, OMX_FALSE);

	/** Domain specific section for the ports. */
	for(i = 0;  i< omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
		pPort = (omx_dump_component_PortType *) omx_dump_component_Private->ports[i];

		pPort->sPortParam.nBufferSize = DUMP_BUF_SIZE;
		pPort->sPortParam.nBufferCountActual = 8;
		pPort->sPortParam.nBufferCountMin	 = 1;
		pPort->sPortParam.eDomain			 = OMX_PortDomainAudio;
		pPort->sPortParam.format.other.eFormat = OMX_OTHER_FormatBinary;

		setHeader(&pPort->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
		pPort->pAudioPcmMode.nPortIndex = i;
		pPort->pAudioPcmMode.nChannels = 3;
		pPort->pAudioPcmMode.eNumData = OMX_NumericalDataSigned;
		pPort->pAudioPcmMode.eEndian = OMX_EndianLittle;
		pPort->pAudioPcmMode.bInterleaved = OMX_TRUE;
		pPort->pAudioPcmMode.nBitPerSample = 16;
		pPort->pAudioPcmMode.nSamplingRate = 16000;
		pPort->pAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;

		setHeader(&pPort->pDumpParam, sizeof(OMX_OTHER_PARAM_DUMPTYPE));
		pPort->pDumpParam.nPortIndex = i;
		pPort->pDumpParam.nForwardPort = 0;
		memset(pPort->pDumpParam.nRpdataType, 0, sizeof(pPort->pDumpParam.nRpdataType));
		memset(pPort->pDumpParam.nRpdataName, 0, sizeof(pPort->pDumpParam.nRpdataName));
		memset(pPort->pDumpParam.nPathName, 0, sizeof(pPort->pDumpParam.nPathName));

  }

  omx_dump_component_Private->destructor = omx_dump_component_Destructor;
  openmaxStandComp->SetParameter = omx_dump_component_SetParameter;
  openmaxStandComp->GetParameter = omx_dump_component_GetParameter;
  openmaxStandComp->GetConfig = omx_dump_component_GetConfig;
  openmaxStandComp->SetConfig = omx_dump_component_SetConfig;
  omx_dump_component_Private->BufferMgmtCallback = omx_dump_component_BufferMgmtCallback;
  omx_dump_component_Private->BufferMgmtFunction = omx_dump_BufferMgmtFunction;
  omx_dump_component_Private->DoStateSet = omx_dump_component_SetState;

  return err;
}


/** The destructor
  */
OMX_ERRORTYPE omx_dump_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;

  omx_debug("In %s\n", __func__);
  /* frees port/s */
  if (omx_dump_component_Private->ports) {
    for (i=0; i < omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      if(omx_dump_component_Private->ports[i])
        omx_dump_component_Private->ports[i]->PortDestructor(omx_dump_component_Private->ports[i]);
    }
    omx_free(omx_dump_component_Private->ports);
    omx_dump_component_Private->ports = NULL;
  }

  omx_base_filter_Destructor(openmaxStandComp);

  omx_debug("Out of %s\n", __func__);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE omx_dump_component_init(OMX_COMPONENTTYPE* openmaxStandComp)
{
	OMX_S32 ret = 0;
	omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
	rpdata_t *pRpdata = NULL;

	omx_debug("In \n");

	if (strlen((char *)omx_dump_component_Private->pathname) == 0 && omx_dump_component_Private->dump_forward_port == 0 &&
		(strlen((char *)omx_dump_component_Private->rpdata_type) == 0 || strlen((char *)omx_dump_component_Private->rpdata_name) == 0)) {
		omx_err("pathname and  dump_forward_port is null, please enter the name or dump_forward_port to dump\n");
		ret = OMX_ErrorBadParameter;
		return ret;
	}

#if defined(CONFIG_ARCH_DSP)

	rpdata_arg_dump targ;

	strncpy((char *)targ.type, (char *)omx_dump_component_Private->rpdata_type, sizeof(targ.type));

	strncpy((char *)targ.name, (char *)omx_dump_component_Private->rpdata_name, sizeof(targ.name));

	targ.dir = RPDATA_DIR_RV;

	pRpdata = rpdata_create(targ.dir, (char *)targ.type, (char *)targ.name, DUMP_BUF_SIZE);
	if (!pRpdata) {
		omx_err("rpdata create fail");
		ret = OMX_ErrorBadParameter;
		return ret;
	}

	omx_dump_component_Private->buffer = rpdata_buffer_addr(pRpdata);
	if (!omx_dump_component_Private->buffer) {
		omx_err("rpdata buffer addr fail");
		ret = OMX_ErrorBadParameter;
		return ret;
	}

	omx_dump_component_Private->rpd = (void *)pRpdata;

	rpdata_wait_connect(pRpdata);

#else
	/* open the pathname to dump */
	if (strlen((char *)omx_dump_component_Private->pathname) > 0) {
		omx_dump_component_Private->dumpfile = fopen(omx_dump_component_Private->pathname, "wb");
		if (!omx_dump_component_Private->dumpfile) {
			omx_err("open file %s error!\n", omx_dump_component_Private->pathname);
			ret = OMX_ErrorBadParameter;
			return ret;
		}
	}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (omx_dump_component_Private->dump_forward_port > 0) {
		ret = adb_forward_create_with_rawdata(omx_dump_component_Private->dump_forward_port);
		if (ret < 0) {
			omx_err("forward create %d error!\n", omx_dump_component_Private->dump_forward_port);
			ret = OMX_ErrorBadParameter;
			return ret;
		}
	}
#endif

#endif

	omx_debug("Out \n");

	return ret;
}

static void omx_dump_component_exit(OMX_COMPONENTTYPE* openmaxStandComp)
{
	omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
	rpdata_t *pRpdata = (rpdata_t *)omx_dump_component_Private->rpd;

	omx_debug("In \n");

	if (omx_dump_component_Private->dumpfile == NULL && omx_dump_component_Private->dump_forward_port == 0 &&
		omx_dump_component_Private->rpd == NULL) {
		omx_err("dumpfile, dump_forward_port and rpd is all null");
		return;
	}

#if defined(CONFIG_ARCH_DSP)
	if (pRpdata) {
		rpdata_destroy(pRpdata);
		pRpdata = NULL;
		omx_dump_component_Private->buffer = NULL;
	}
#else

	/* close the pathname */
	if (omx_dump_component_Private->dumpfile) {
		fflush(omx_dump_component_Private->dumpfile);
		fclose(omx_dump_component_Private->dumpfile);
		omx_dump_component_Private->dumpfile = NULL;
	}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	if (omx_dump_component_Private->dump_forward_port > 0) {
		adb_forward_end(omx_dump_component_Private->dump_forward_port);
		//adb_forward_destroy(omx_dump_component_Private->dump_forward_port);
		omx_dump_component_Private->dump_forward_port = 0;
	}
#endif

#endif

	omx_debug("Out \n");

}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_dump_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInBuffer, OMX_BUFFERHEADERTYPE* pOutBuffer) {
  omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
  rpdata_t *pRpdata = (rpdata_t *)omx_dump_component_Private->rpd;
  int ret = -1;

  if (pInBuffer == NULL || pOutBuffer == NULL)
	  return;

  pOutBuffer->nFilledLen = 0;
  if (pInBuffer->pBuffer == NULL || pOutBuffer->pBuffer == NULL || pRpdata == NULL) {
	  omx_err("inbuffer:%p, pBuffer:%p pRpdata:%p!!",
		  pInBuffer->pBuffer, pOutBuffer->pBuffer, pRpdata);
	  omx_err("err param!");
	  return;
  }
  if (pInBuffer->nFilledLen == 0){
	  omx_info("nFilledLen = 0, return!\n");
	  return;
  }

  pOutBuffer->nFlags = pInBuffer->nFlags;
  pOutBuffer->nOffset = pInBuffer->nOffset;
  pOutBuffer->nTimeStamp = pInBuffer->nTimeStamp;
  pOutBuffer->nFilledLen = pInBuffer->nFilledLen;

  memcpy(pOutBuffer->pBuffer + pOutBuffer->nOffset, pInBuffer->pBuffer + pInBuffer->nOffset,
	  pInBuffer->nFilledLen);

#if defined(CONFIG_ARCH_DSP)
	if (rpdata_is_connect(pRpdata) == 0) {
		memcpy(omx_dump_component_Private->buffer, pInBuffer->pBuffer + pInBuffer->nOffset, pInBuffer->nFilledLen);
		ret = rpdata_send(pRpdata, 0, pInBuffer->nFilledLen);
		if (ret != 0) {
			omx_err("rpdata send err, ret %d", ret);
		}
		pInBuffer->nFilledLen = 0;
		pInBuffer->nOffset = 0;
		pInBuffer->nFlags = 0;
	}
	else {
		omx_info("rpdata is not connected");
	}
#else
	if (omx_dump_component_Private->dumpfile) {
		int wlen = 0;
		wlen = fwrite(pInBuffer->pBuffer + pInBuffer->nOffset, 1, pInBuffer->nFilledLen, omx_dump_component_Private->dumpfile);
		if (wlen != pInBuffer->nFilledLen) {
			omx_err("write file error rlen %d!\n", wlen);
			pInBuffer->nFilledLen = 0;
			pInBuffer->nOffset = 0;
			pInBuffer->nFlags = OMX_BUFFERFLAG_EOS;
		}
	}

#ifdef CONFIG_COMPONENTS_USB_GADGET_ADB_FORWARD
	  if (omx_dump_component_Private->dump_forward_port) {
		  ret = adb_forward_send(omx_dump_component_Private->dump_forward_port, pInBuffer->pBuffer + pInBuffer->nOffset, pInBuffer->nFilledLen);
		  if (ret != 0) {
			  omx_err("adb forward send  error ret %d!\n", ret);
			  pInBuffer->nFilledLen = 0;
			  pInBuffer->nOffset = 0;
			  pInBuffer->nFlags = OMX_BUFFERFLAG_EOS;
		  }
	  }
#endif
	if (pInBuffer->nFilledLen) {
		pInBuffer->nFilledLen = 0;
		pInBuffer->nOffset = 0;
		pInBuffer->nFlags = 0;
	}

#endif

  if (pInBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
	  omx_err("dump handle get the end of stream\n");
	  pOutBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
  }

}

/** setting configurations */
OMX_ERRORTYPE omx_dump_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  //OMX_AUDIO_CONFIG_VOLUMETYPE* pVolume;
//  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  //omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
  //omx_dump_component_PortType * pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
  	/*
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if(pVolume->sVolume.nValue > 100) {
        err =  OMX_ErrorBadParameter;
        break;
      }

      if (pVolume->nPortIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_dump_component_PortType *)omx_dump_component_Private->ports[pVolume->nPortIndex];
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

OMX_ERRORTYPE omx_dump_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {
  //OMX_AUDIO_CONFIG_VOLUMETYPE           *pVolume;
  //OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  //omx_dump_component_PrivateType *omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
  //omx_dump_component_PortType    *pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  switch (nIndex) {
  	/*
    case OMX_IndexConfigAudioVolume :
      pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) pComponentConfigStructure;
      if (pVolume->nPortIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        pPort= (omx_dump_component_PortType *)omx_dump_component_Private->ports[pVolume->nPortIndex];
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

OMX_ERRORTYPE omx_dump_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_U32                         portIndex;
  OMX_OTHER_PARAM_DUMPTYPE	      *pDumpParams = NULL;
  OMX_AUDIO_PARAM_PCMMODETYPE 	  *pAudioParams = NULL;
  omx_dump_component_PortType *port;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
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
      if (portIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[portIndex];
        memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

      if (omx_dump_component_Private->state != OMX_StateLoaded && omx_dump_component_Private->state != OMX_StateWaitForResources) {
        omx_debug("In %s Incorrect State=%x lineno=%d\n",__func__,omx_dump_component_Private->state,__LINE__);
        return OMX_ErrorIncorrectStateOperation;
      }

      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }

      if (strcmp( (char*) pComponentRole->cRole, DUMP_COMP_ROLE)) {
        return OMX_ErrorBadParameter;
      }
      break;

  	case OMX_IndexParamAudioPcm:
		pAudioParams = (OMX_AUDIO_PARAM_PCMMODETYPE *)ComponentParameterStructure;
		portIndex = pAudioParams->nPortIndex;
		err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
		if(err!=OMX_ErrorNone) {
			omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
			break;
		}
		if (portIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        	port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[portIndex];
        	memcpy(&port->pAudioPcmMode, pAudioParams, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      	} else {
        	err = OMX_ErrorBadPortIndex;
      	}
		break;
	case OMX_IndexVendorParamDump:
		pDumpParams = (OMX_OTHER_PARAM_DUMPTYPE *)ComponentParameterStructure;
		portIndex = pDumpParams->nPortIndex;
		 err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pDumpParams, sizeof(OMX_OTHER_PARAM_DUMPTYPE));
		if(err!=OMX_ErrorNone) {
			omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
			break;
		}
		if (portIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        	port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[portIndex];
        	memcpy(&port->pDumpParam, pDumpParams, sizeof(OMX_OTHER_PARAM_DUMPTYPE));

			memcpy(omx_dump_component_Private->rpdata_type, pDumpParams->nRpdataType, sizeof(pDumpParams->nRpdataType));
			memcpy(omx_dump_component_Private->rpdata_name, pDumpParams->nRpdataName, sizeof(pDumpParams->nRpdataName));
			memcpy(omx_dump_component_Private->pathname, pDumpParams->nPathName, sizeof(pDumpParams->nPathName));
			omx_dump_component_Private->dump_forward_port = pDumpParams->nForwardPort;
      	} else {
        	err = OMX_ErrorBadPortIndex;
      	}
		break;
    default:
      err = omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_dump_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure) {

  OMX_AUDIO_PARAM_PORTFORMATTYPE  *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE     *pAudioParams;
  OMX_OTHER_PARAM_DUMPTYPE        *pDumpParams;
  OMX_PARAM_COMPONENTROLETYPE     *pComponentRole;
  OMX_ERRORTYPE                   err = OMX_ErrorNone;
  omx_dump_component_PortType    *port;
  OMX_COMPONENTTYPE                     *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_dump_component_PrivateType *omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
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
      memcpy(ComponentParameterStructure, &omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
      break;
    case OMX_IndexParamAudioPortFormat:
      pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioPortFormat->nPortIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[pAudioPortFormat->nPortIndex];
        memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexParamAudioPcm:
      pAudioParams = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone) {
        break;
      }
      if (pAudioParams->nPortIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
        port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[pAudioParams->nPortIndex];
        memcpy(pAudioParams, &port->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
      } else {
        err = OMX_ErrorBadPortIndex;
      }
      break;
    case OMX_IndexVendorParamDump:
		pDumpParams = (OMX_OTHER_PARAM_DUMPTYPE*)ComponentParameterStructure;
		if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_OTHER_PARAM_DUMPTYPE))) != OMX_ErrorNone) {
		  break;
		}
		 if (pDumpParams->nPortIndex <= omx_dump_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts) {
          port= (omx_dump_component_PortType *)omx_dump_component_Private->ports[pDumpParams->nPortIndex];
          memcpy(pDumpParams, &port->pDumpParam, sizeof(OMX_OTHER_PARAM_DUMPTYPE));
        } else {
          err = OMX_ErrorBadPortIndex;
        }
	break;
    case OMX_IndexParamStandardComponentRole:
      pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
      if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
        break;
      }
      strcpy( (char*) pComponentRole->cRole, DUMP_COMP_ROLE);
      break;
    default:
      err = omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;
}

OMX_ERRORTYPE omx_dump_component_SetState(OMX_COMPONENTTYPE* openmaxStandComp,
	OMX_U32 dest_state)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	/* Check which structure we are being fed and make control its header */
	omx_dump_component_PrivateType* omx_dump_component_Private = openmaxStandComp->pComponentPrivate;
	OMX_STATETYPE pre_state;

	omx_debug(" %p, %ld\n", openmaxStandComp, dest_state);

	if (dest_state == OMX_StateExecuting && (omx_dump_component_Private->state == OMX_StateIdle ||
		omx_dump_component_Private->state == OMX_StatePause)) {
		if (omx_dump_component_Private->dumpfile != NULL) {
			omx_debug("Device not closed while entering StateIdle");
			omx_dump_component_exit(openmaxStandComp);
		}
		omx_debug("dump init\n");
		ret = omx_dump_component_init(openmaxStandComp);
		if (OMX_ErrorNone != ret)
			return ret;
	}

	pre_state = omx_dump_component_Private->state;
	ret = omx_base_component_DoStateSet(openmaxStandComp, dest_state);
	if (dest_state == OMX_StatePause && pre_state == OMX_StateExecuting) {
		omx_debug("dump exit\n");
		omx_dump_component_exit(openmaxStandComp);
	}
	if (dest_state == OMX_StateIdle &&
		(pre_state == OMX_StateExecuting || pre_state == OMX_StatePause)) {
		omx_debug("dump exit\n");
		omx_dump_component_exit(openmaxStandComp);
	}
	return ret;

}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_dump_BufferMgmtFunction (void* param) {

    OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
    omx_dump_component_PrivateType* omx_dump_component_Private = (omx_dump_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_base_PortType *pInPort=(omx_base_PortType *)omx_dump_component_Private->ports[OMX_DUMP_INPUTPORT_INDEX];
    omx_base_PortType *pOutPort=(omx_base_PortType *)omx_dump_component_Private->ports[OMX_DUMP_OUTPUTPORT_INDEX];
    omx_sem_t* pInputSem = pInPort->pBufferSem;
    omx_sem_t* pOutputSem = pOutPort->pBufferSem;
    queue_t* pInputQueue = pInPort->pBufferQueue;
    queue_t* pOutputQueue = pOutPort->pBufferQueue;
    OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
    OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
    OMX_BOOL isInputBufferNeeded=OMX_TRUE,isOutputBufferNeeded=OMX_TRUE;
    int inBufExchanged=0,outBufExchanged=0;

    /* checks if the component is in a state able to receive buffers */
    while(omx_dump_component_Private->state == OMX_StateIdle || omx_dump_component_Private->state == OMX_StateExecuting ||  omx_dump_component_Private->state == OMX_StatePause ||
            omx_dump_component_Private->transientState == OMX_TransStateLoadedToIdle) {

        /*Wait till the ports are being flushed*/
        omx_thread_mutex_lock(&omx_dump_component_Private->flush_mutex);
        while( PORT_IS_BEING_FLUSHED(pInPort) ||
                PORT_IS_BEING_FLUSHED(pOutPort)) {
            omx_thread_mutex_unlock(&omx_dump_component_Private->flush_mutex);

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

            omx_sem_up(omx_dump_component_Private->flush_all_condition);
            omx_sem_down(omx_dump_component_Private->flush_condition);
            omx_thread_mutex_lock(&omx_dump_component_Private->flush_mutex);
        }
        omx_thread_mutex_unlock(&omx_dump_component_Private->flush_mutex);

        /*No buffer to process. So wait here*/
        if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
                (omx_dump_component_Private->state != OMX_StateLoaded && omx_dump_component_Private->state != OMX_StateInvalid)) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_dump_component_Private->bMgmtSem);

        }
        if(omx_dump_component_Private->state == OMX_StateLoaded || omx_dump_component_Private->state == OMX_StateInvalid) {
            omx_debug("In %s Buffer Management Thread is exiting\n",__func__);
            break;
        }
        if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
                (omx_dump_component_Private->state != OMX_StateLoaded && omx_dump_component_Private->state != OMX_StateInvalid) &&
                !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_dump_component_Private->bMgmtSem);

        }
        if(omx_dump_component_Private->state == OMX_StateLoaded || omx_dump_component_Private->state == OMX_StateInvalid) {
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
				   omx_debug("In %s Had NULL output buffer!!\n",__func__);
				   int ret;
				   ret = omx_sem_timed_down(omx_dump_component_Private->bMgmtSem, 1000);
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
                    omx_debug("Had NULL output buffer!! op is=%d,iq=%d\n",pOutputSem->semval,pOutputQueue->nelem);
					omx_sem_down(omx_dump_component_Private->bMgmtSem);
					continue;

                }
            }
        }

        if(isInputBufferNeeded==OMX_FALSE) {
            if(pInputBuffer->hMarkTargetComponent != NULL) {
                if((OMX_COMPONENTTYPE*)pInputBuffer->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
                    /*Clear the mark and generate an event*/
                    (*(omx_dump_component_Private->callbacks->EventHandler))
                    (openmaxStandComp,
                     omx_dump_component_Private->callbackData,
                     OMX_EventMark, /* The command was completed */
                     1, /* The commands was a OMX_CommandStateSet */
                     0, /* The state has been changed in message->messageParam2 */
                     pInputBuffer->pMarkData);
                } else {
                    /*If this is not the target component then pass the mark*/
                    omx_dump_component_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
                    omx_dump_component_Private->pMark.pMarkData            = pInputBuffer->pMarkData;
                }
                pInputBuffer->hMarkTargetComponent = NULL;
            }
        }

        if(isInputBufferNeeded==OMX_FALSE && isOutputBufferNeeded==OMX_FALSE) {

            if(omx_dump_component_Private->pMark.hMarkTargetComponent != NULL) {
                pOutputBuffer->hMarkTargetComponent = omx_dump_component_Private->pMark.hMarkTargetComponent;
                pOutputBuffer->pMarkData            = omx_dump_component_Private->pMark.pMarkData;
                omx_dump_component_Private->pMark.hMarkTargetComponent = NULL;
                omx_dump_component_Private->pMark.pMarkData            = NULL;
            }

            pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
                omx_debug("Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags = pInputBuffer->nFlags;
                pInputBuffer->nFlags = 0;
            }

            if(omx_dump_component_Private->state == OMX_StateExecuting)  {
                if (omx_dump_component_Private->BufferMgmtCallback && pInputBuffer->nFilledLen > 0) {
                    (*(omx_dump_component_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, pOutputBuffer);
                } else {
                    /*It no buffer management call back the explicitly consume input buffer*/
                    pInputBuffer->nFilledLen = 0;
                }
            } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                omx_debug("In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_dump_component_Private->state);
            } else {
                pInputBuffer->nFilledLen = 0;
            }

            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) {
                omx_debug("Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags=pInputBuffer->nFlags;
                pInputBuffer->nFlags=0;
                (*(omx_dump_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_dump_component_Private->callbackData,
                 OMX_EventBufferFlag, /* The command was completed */
                 1, /* The commands was a OMX_CommandStateSet */
                 pOutputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
                 NULL);
                omx_dump_component_Private->bIsEOSReached = OMX_TRUE;
            }
            if(omx_dump_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                /*Waiting at paused state*/
                omx_sem_wait(omx_dump_component_Private->bStateSem);
            }

            /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
            if((pOutputBuffer->nFilledLen != 0) || ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) || (omx_dump_component_Private->bIsEOSReached == OMX_TRUE)) {
                pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
                outBufExchanged--;
                pOutputBuffer=NULL;
                isOutputBufferNeeded=OMX_TRUE;
            }
        }

        if(omx_dump_component_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            /*Waiting at paused state*/
            omx_sem_wait(omx_dump_component_Private->bStateSem);
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

