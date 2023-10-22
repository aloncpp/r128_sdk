/**
  src/base/omx_base_audio_port.c

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


#include <string.h>
#include <unistd.h>
#include <OmxCore.h>

#include "omx_base_component.h"
#include "omx_base_audio_port.h"

/**
  * @brief The base constructor for the generic OpenMAX ST Audio port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the component.
  * It takes care of constructing the instance of the port and
  * every object needed by the base port.
  *
  * @param openmaxStandComp pointer to the Handle of the component
  * @param openmaxStandPort the ST port to be initialized
  * @param nPortIndex Index of the port to be constructed
  * @param isInput specifies if the port is an input or an output
  *
  * @return OMX_ErrorInsufficientResources if a memory allocation fails
  */

OMX_ERRORTYPE base_audio_port_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,omx_base_PortType **openmaxStandPort,OMX_U32 nPortIndex, OMX_BOOL isInput) {
    OMX_ERRORTYPE err;
    omx_base_audio_PortType *omx_base_audio_Port;

    omx_debug("In %s of component %p\n", __func__, openmaxStandComp);
    if (!(*openmaxStandPort)) {
        *openmaxStandPort = omx_alloc(sizeof(omx_base_audio_PortType));
        if (!(*openmaxStandPort)) {
            omx_debug("Insufficient memory in %s\n", __func__);
            return OMX_ErrorInsufficientResources;
        }

        memset(*openmaxStandPort, 0x00, sizeof(omx_base_audio_PortType));
    }

    if (!(*openmaxStandPort)) {
        omx_debug("Insufficient memory in %s\n", __func__);
        return OMX_ErrorInsufficientResources;
    }

    err = base_port_Constructor(openmaxStandComp,openmaxStandPort,nPortIndex, isInput);
    if (err != OMX_ErrorNone) {
        omx_debug("In %s base port constructor failed\n", __func__);
        return err;
    }

    omx_base_audio_Port = (omx_base_audio_PortType *)*openmaxStandPort;

    setHeader(&omx_base_audio_Port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    omx_base_audio_Port->sAudioParam.nPortIndex = nPortIndex;
    omx_base_audio_Port->sAudioParam.nIndex = 0;
    omx_base_audio_Port->sAudioParam.eEncoding = OMX_AUDIO_CodingUnused;

    omx_base_audio_Port->sPortParam.eDomain = OMX_PortDomainAudio;
    omx_base_audio_Port->sPortParam.format.audio.cMIMEType = omx_alloc(DEFAULT_MIME_STRING_LENGTH);
    if (!omx_base_audio_Port->sPortParam.format.audio.cMIMEType) {
        omx_debug("Memory allocation failed in %s\n", __func__);
        return OMX_ErrorInsufficientResources;
    }
    strcpy(omx_base_audio_Port->sPortParam.format.audio.cMIMEType, "raw/audio");
    omx_base_audio_Port->sPortParam.format.audio.pNativeRender = 0;
    omx_base_audio_Port->sPortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
    omx_base_audio_Port->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingUnused;

    omx_base_audio_Port->sPortParam.nBufferSize = (isInput == OMX_TRUE)?DEFAULT_IN_BUFFER_SIZE:DEFAULT_OUT_BUFFER_SIZE ;

    omx_base_audio_Port->PortDestructor = &base_audio_port_Destructor;

    omx_debug("Out of %s of component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

/**
  * @brief The base audio port destructor for the generic OpenMAX ST Audio port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  *
  * @param openmaxStandPort the ST port to be destructed
  *
  * @return OMX_ErrorNone
  */

OMX_ERRORTYPE base_audio_port_Destructor(omx_base_PortType *openmaxStandPort) {
    OMX_ERRORTYPE err;
    omx_debug("In %s of port %p\n", __func__, openmaxStandPort);
    if(openmaxStandPort->sPortParam.format.audio.cMIMEType) {
        omx_free(openmaxStandPort->sPortParam.format.audio.cMIMEType);
        openmaxStandPort->sPortParam.format.audio.cMIMEType = NULL;
    }
    err = base_port_Destructor(openmaxStandPort);
    if (err != OMX_ErrorNone) {
        omx_debug("In %s base port destructor failed\n", __func__);
        return err;
    }
    omx_debug("Out of %s of port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}
