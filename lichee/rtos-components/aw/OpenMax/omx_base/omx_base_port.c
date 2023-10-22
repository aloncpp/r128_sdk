/*
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

#include <string.h>
#include <unistd.h>

#include "omx_base_component.h"
#include "omx_base_port.h"

/** The default value for wait alloc buffer timeout. */
#define USE_WAIT_ALLOC_BUFFER_TIMEOUT
#define WAIT_ALLOC_BUFFER_TIMEOUT 1000


/** The default value for the number of needed buffers for each port. */
#define DEFAULT_NUMBER_BUFFERS_PER_PORT 2
/** The default value for the minimum number of needed buffers for each port. */
#define DEFAULT_MIN_NUMBER_BUFFERS_PER_PORT 2
/**
* @brief The base contructor for the generic OpenMAX ST port
*
* This function is executed by the component that uses a port.
* The parameter contains the info about the component.
* It takes care of constructing the instance of the port and
* every object needed by the base port.
*
* @param openmaxStandComp in the component that holds the port
* @param openmaxStandPort the ST port to be initialized
* @param nPortIndex the index of the port
* @param isInput specifies if the port is an input or an output
*
* @return OMX_ErrorInsufficientResources if a memory allocation fails
*/

/** @@ Modified code
* added checking validation of a pointer variable once free.
* added WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL.
* added deinterlace case.
* added USE_WAIT_ALLOC_BUFFER_TIMEOUT.
**/

OMX_ERRORTYPE base_port_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,omx_base_PortType **openmaxStandPort,OMX_U32 nPortIndex, OMX_BOOL isInput) {

    omx_debug("In %s for component %p, priv %p, nPortIndex = %ld\n", __func__, openmaxStandComp, openmaxStandComp->pComponentPrivate, nPortIndex);

    // create ports, but only if the subclass hasn't done it
    if (!(*openmaxStandPort)) {
        *openmaxStandPort = omx_alloc(sizeof(omx_base_PortType));
        /** @@ Modified code
        * added checking validation of a pointer variable once free
        **/
        if (!(*openmaxStandPort)) {
            omx_debug("Out of %s for component %p for a lack of resources\n", __func__, openmaxStandComp);
            return OMX_ErrorInsufficientResources;
        }
        memset(*openmaxStandPort, 0x00, sizeof(omx_base_PortType));
    }

    if (!(*openmaxStandPort)) {
        omx_debug("Out of %s for component %p for a lack of resources\n", __func__, openmaxStandComp);
        return OMX_ErrorInsufficientResources;
    }

    (*openmaxStandPort)->hTunneledComponent = NULL;
    (*openmaxStandPort)->nTunnelFlags = 0;
    (*openmaxStandPort)->nTunneledPort = 0;
    (*openmaxStandPort)->eBufferSupplier = OMX_BufferSupplyUnspecified;
    (*openmaxStandPort)->nNumTunnelBuffer = 0;

    /** @@ Modified code
    * added checking validation of a pointer variable once free
    **/
    if((*openmaxStandPort)->pAllocSem==NULL) {
        (*openmaxStandPort)->pAllocSem = omx_alloc(sizeof(omx_sem_t));
        /** @@ Modified code
        * added checking validation of a pointer variable once free
        **/
        if((*openmaxStandPort)->pAllocSem==NULL) {
            omx_debug("Insufficient memory in %s\n", __func__);
            return OMX_ErrorInsufficientResources;
        }
        memset((*openmaxStandPort)->pAllocSem, 0x00, sizeof(omx_sem_t));
        omx_sem_init((*openmaxStandPort)->pAllocSem, 0);
    }
    (*openmaxStandPort)->nNumBufferFlushed = 0;
    (*openmaxStandPort)->bIsPortFlushed = OMX_FALSE;
    /** Allocate and initialize buffer queue */
    if(!(*openmaxStandPort)->pBufferQueue) {
        (*openmaxStandPort)->pBufferQueue = omx_alloc(sizeof(queue_t));
        if((*openmaxStandPort)->pBufferQueue == NULL) {
            omx_debug("Insufficient memory in %s\n", __func__);
            return OMX_ErrorInsufficientResources;
        }
        memset((*openmaxStandPort)->pBufferQueue, 0x00, sizeof(queue_t));
        queue_init((*openmaxStandPort)->pBufferQueue);
    }
    /*Allocate and initialise port semaphores*/
    if(!(*openmaxStandPort)->pBufferSem) {
        (*openmaxStandPort)->pBufferSem = omx_alloc(sizeof(omx_sem_t));
        if((*openmaxStandPort)->pBufferSem==NULL) {
            omx_debug("Insufficient memory in %s\n", __func__);
            return OMX_ErrorInsufficientResources;
        }

        memset((*openmaxStandPort)->pBufferSem, 0x00, sizeof(omx_sem_t));

        omx_sem_init((*openmaxStandPort)->pBufferSem, 0);
    }

    (*openmaxStandPort)->nNumAssignedBuffers = 0;
    setHeader(&(*openmaxStandPort)->sPortParam, sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
    (*openmaxStandPort)->sPortParam.nPortIndex = nPortIndex;
    (*openmaxStandPort)->sPortParam.nBufferCountActual = DEFAULT_NUMBER_BUFFERS_PER_PORT;
    (*openmaxStandPort)->sPortParam.nBufferCountMin = DEFAULT_MIN_NUMBER_BUFFERS_PER_PORT;
    (*openmaxStandPort)->sPortParam.bEnabled = OMX_TRUE;
    (*openmaxStandPort)->sPortParam.bPopulated = OMX_FALSE;
    (*openmaxStandPort)->sPortParam.eDir  =  (isInput == OMX_TRUE)?OMX_DirInput:OMX_DirOutput;

    (*openmaxStandPort)->standCompContainer=openmaxStandComp;
    (*openmaxStandPort)->bIsTransientToEnabled=OMX_FALSE;
    (*openmaxStandPort)->bIsTransientToDisabled=OMX_FALSE;
    (*openmaxStandPort)->bIsFullOfBuffers=OMX_FALSE;
    (*openmaxStandPort)->bIsEmptyOfBuffers=OMX_FALSE;
    (*openmaxStandPort)->bBufferStateAllocated = NULL;
    (*openmaxStandPort)->pInternalBufferStorage = NULL;

    (*openmaxStandPort)->PortDestructor = &base_port_Destructor;
    (*openmaxStandPort)->Port_AllocateBuffer = &base_port_AllocateBuffer;
    (*openmaxStandPort)->Port_UseBuffer = &base_port_UseBuffer;
    (*openmaxStandPort)->Port_FreeBuffer = &base_port_FreeBuffer;
    (*openmaxStandPort)->Port_DisablePort = &base_port_DisablePort;
    (*openmaxStandPort)->Port_EnablePort = &base_port_EnablePort;
    (*openmaxStandPort)->Port_SendBufferFunction = &base_port_SendBufferFunction;
    (*openmaxStandPort)->FlushProcessingBuffers = &base_port_FlushProcessingBuffers;
    (*openmaxStandPort)->ReturnBufferFunction = &base_port_ReturnBufferFunction;
    (*openmaxStandPort)->ComponentTunnelRequest = &base_port_ComponentTunnelRequest;
    (*openmaxStandPort)->Port_AllocateTunnelBuffer = &base_port_AllocateTunnelBuffer;
    (*openmaxStandPort)->Port_FreeTunnelBuffer = &base_port_FreeTunnelBuffer;
    (*openmaxStandPort)->bIsDestroying = OMX_FALSE;
    omx_thread_mutex_init(&((*openmaxStandPort)->exitMutex), NULL);


    omx_debug("Out of %s for component %p priv %p\n", __func__, openmaxStandComp, openmaxStandComp->pComponentPrivate);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE base_port_Destructor(omx_base_PortType *openmaxStandPort) {

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    if(openmaxStandPort->pAllocSem) {
        omx_thread_mutex_lock(&openmaxStandPort->exitMutex);
        openmaxStandPort->bIsDestroying = OMX_TRUE;
        omx_thread_mutex_unlock(&openmaxStandPort->exitMutex);
        /** TODO This semaphore, if activated, can cause memory leaks.
        * It can be necessary to avoid to wait forever if the other component in tunnel
        * doesn't perform required actions. This operation can be timed but can cause problems.
        * Currently it is left commented for reference.
        //		omx_sem_up(openmaxStandPort->pAllocSem);
        */
        if (openmaxStandPort->pAllocSem) {
            omx_sem_deinit(openmaxStandPort->pAllocSem);
            omx_free(openmaxStandPort->pAllocSem);
            openmaxStandPort->pAllocSem=NULL;
        }

    }
    /** Allocate and initialize buffer queue */
    if(openmaxStandPort->pBufferQueue) {
        queue_deinit(openmaxStandPort->pBufferQueue);
        omx_free(openmaxStandPort->pBufferQueue);
        openmaxStandPort->pBufferQueue=NULL;
    }
    /*Allocate and initialize port semaphores*/
    if(openmaxStandPort->pBufferSem) {
        omx_sem_deinit(openmaxStandPort->pBufferSem);
        omx_free(openmaxStandPort->pBufferSem);
        openmaxStandPort->pBufferSem=NULL;
    }

    omx_thread_mutex_destroy(&openmaxStandPort->exitMutex);

    omx_free(openmaxStandPort);
    openmaxStandPort = NULL;
    omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}

/** @brief Releases buffers under processing.
* This function must be implemented in the derived classes, for the
* specific processing
*/
OMX_ERRORTYPE base_port_FlushProcessingBuffers(omx_base_PortType *openmaxStandPort) {
    omx_base_component_PrivateType* omx_base_component_Private;
    OMX_BUFFERHEADERTYPE* pBuffer;
    int errQue;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);
    omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;

    if (openmaxStandPort->sPortParam.eDomain!=OMX_PortDomainOther) { /* clock buffers not used in the clients buffer managment function */
        omx_thread_mutex_lock(&omx_base_component_Private->flush_mutex);
        openmaxStandPort->bIsPortFlushed=OMX_TRUE;
        /*Signal the buffer management thread of port flush,if it is waiting for buffers*/
        if (omx_base_component_Private->bMgmtSem->semval == 0) {
            omx_sem_up(omx_base_component_Private->bMgmtSem);
        }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
        /*Signal the buffer management thread of port flush,if it is waiting for display buffer full sem*/
        omx_sem_up_to_one(omx_base_component_Private->bDispBufFullSem);
#endif
        if(omx_base_component_Private->state != OMX_StateExecuting ) {
            /*Waiting at paused state*/
            omx_sem_signal(omx_base_component_Private->bStateSem);
        }
        omx_debug("In %s waiting for flush all condition port index =%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);
        /* Wait until flush is completed */
        omx_thread_mutex_unlock(&omx_base_component_Private->flush_mutex);
        omx_sem_down(omx_base_component_Private->flush_all_condition);
    }
    omx_debug("In %s flushed all the buffers under processing\n", __func__);

    omx_sem_reset(omx_base_component_Private->bMgmtSem);

    /* Flush all the buffers not under processing */
    while (openmaxStandPort->pBufferSem->semval > 0) {
        omx_debug("In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
              __func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
              (int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

        omx_sem_down(openmaxStandPort->pBufferSem);
        pBuffer = dequeue(openmaxStandPort->pBufferQueue);
        if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
            omx_debug("In %s: Comp %s is returning io:%d buffer\n",
                  __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
            if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
                ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            } else {
                ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            }
        } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
            omx_debug("queue queue queue");
            errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
            if (errQue) {
                /* /TODO the queue is full. This can be handled in a fine way with
                * some retrials, or other checking. For the moment this is a critical error
                * and simply causes the failure of this call
                */
                return OMX_ErrorInsufficientResources;
            }
        } else {
            omx_debug("others EmptyBufferDone or FillBufferDone");
			openmaxStandPort->ReturnBufferFunction(openmaxStandPort, pBuffer);
            /** @@ Modified code
            * replaced the using of BufferProcessedCallback function to ReturnBufferFunction function 
            * in base_port_FlushProcessingBuffers to be overrided a drived component.
            **/  
// replaced by C&M
// 			(*(openmaxStandPort->BufferProcessedCallback))(
// 				openmaxStandPort->standCompContainer,
// 				omx_base_component_Private->callbackData,
// 				pBuffer);
        }
    }
    /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
    if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        while(openmaxStandPort->pBufferQueue->nelem != (int)openmaxStandPort->nNumAssignedBuffers) {
            omx_sem_down(openmaxStandPort->pBufferSem);
            omx_debug("In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
        }
        omx_sem_reset(openmaxStandPort->pBufferSem);
    }

    omx_thread_mutex_lock(&omx_base_component_Private->flush_mutex);
    openmaxStandPort->bIsPortFlushed=OMX_FALSE;
    omx_thread_mutex_unlock(&omx_base_component_Private->flush_mutex);

    omx_sem_up(omx_base_component_Private->flush_condition);

    omx_sem_up(omx_base_component_Private->deinterlace_flush_condition);

    omx_debug("%s, flush_condition->semval: %d, flush_condition->deinterlace_flush_condition: %d\n", __func__,
            omx_sem_get_semval(omx_base_component_Private->flush_condition),
            omx_sem_get_semval(omx_base_component_Private->deinterlace_flush_condition));
    omx_debug("Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
          (int)openmaxStandPort->sPortParam.nPortIndex,(int)openmaxStandPort->bIsPortFlushed,omx_base_component_Private->name);

    omx_debug("In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
          (int)openmaxStandPort->nTunnelFlags,
          (int)openmaxStandPort->pBufferQueue->nelem,
          (int)openmaxStandPort->pBufferSem->semval,
          (int)omx_base_component_Private->bMgmtSem->semval,
          omx_base_component_Private->name);

    omx_debug("Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
    return OMX_ErrorNone;
}

/** @brief Disables the port.
*
* This function is called due to a request by the IL client
*
* @param openmaxStandPort the reference to the port
*
*/
OMX_ERRORTYPE base_port_DisablePort(omx_base_PortType *openmaxStandPort) {

    omx_base_component_PrivateType* omx_base_component_Private;
    OMX_ERRORTYPE err=OMX_ErrorNone;

    omx_debug("In %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
    omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;
    if (! PORT_IS_ENABLED(openmaxStandPort)) {
        return OMX_ErrorNone;
    }

    if(omx_base_component_Private->state!=OMX_StateLoaded) {
        if(!PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
            /*Signal Buffer Mgmt Thread if it's holding any buffer*/
            if(omx_base_component_Private->bMgmtSem->semval==0) {
                omx_sem_up(omx_base_component_Private->bMgmtSem);
            }
            /*Wait till all buffers are freed*/
            omx_sem_down(openmaxStandPort->pAllocSem);
            omx_sem_reset(omx_base_component_Private->bMgmtSem);
        } else {
            /*Since port is being disabled then remove buffers from the queue*/
            while(openmaxStandPort->pBufferQueue->nelem > 0) {
                dequeue(openmaxStandPort->pBufferQueue);
            }

            err = openmaxStandPort->Port_FreeTunnelBuffer(openmaxStandPort,openmaxStandPort->sPortParam.nPortIndex);
            if(err!=OMX_ErrorNone) {
                omx_debug("In %s Freeing Tunnel Buffer Error=%x\n",__func__,err);
            }
            omx_debug("In %s Qelem=%d\n", __func__,openmaxStandPort->pBufferQueue->nelem);
        }
    }

    omx_debug("In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
          (int)openmaxStandPort->nTunnelFlags,
          (int)openmaxStandPort->pBufferQueue->nelem,
          (int)openmaxStandPort->pBufferSem->semval,
          (int)omx_base_component_Private->bMgmtSem->semval,
          omx_base_component_Private->name);
    openmaxStandPort->bIsTransientToDisabled = OMX_FALSE;
    openmaxStandPort->sPortParam.bEnabled = OMX_FALSE;
    omx_debug("Out %s Port Index=%d isEnabled=%d\n", __func__,
          (int)openmaxStandPort->sPortParam.nPortIndex,
          (int)openmaxStandPort->sPortParam.bEnabled);
    return err;
}

/** @brief Enables the port.
*
* This function is called due to a request by the IL client
*
* @param openmaxStandPort the reference to the port
*
*/
OMX_ERRORTYPE base_port_EnablePort(omx_base_PortType *openmaxStandPort) {

    omx_base_component_PrivateType* omx_base_component_Private;
    OMX_ERRORTYPE err=OMX_ErrorNone;
    OMX_U32 i;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);
    if (PORT_IS_ENABLED(openmaxStandPort)) {
        omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
        return OMX_ErrorNone;
    }
    omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;

    openmaxStandPort->sPortParam.bEnabled = OMX_TRUE;

    omx_debug("In %s port T flag=%x popu=%d state=%x\n", __func__,
          (int)openmaxStandPort->nTunnelFlags,
          (int)openmaxStandPort->sPortParam.bPopulated,
          (int)omx_base_component_Private->state);


    if (!PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
        /*Wait Till All buffers are allocated if the component state is not Loaded*/
        if (omx_base_component_Private->state!=OMX_StateLoaded && omx_base_component_Private->state!=OMX_StateWaitForResources)  {
#ifndef USE_WAIT_ALLOC_BUFFER_TIMEOUT
            omx_sem_down(openmaxStandPort->pAllocSem);
#else
            if(omx_sem_timed_down(openmaxStandPort->pAllocSem,WAIT_ALLOC_BUFFER_TIMEOUT)!=0){
	            openmaxStandPort->sPortParam.bEnabled = OMX_FALSE;
	            return OMX_ErrorTimeout;
            }
#endif
            openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
        }
    } else { //Port Tunneled and supplier. Then allocate tunnel buffers
        err= openmaxStandPort->Port_AllocateTunnelBuffer(openmaxStandPort, openmaxStandPort->sPortParam.nPortIndex);
        if(err!=OMX_ErrorNone) {
            omx_debug("In %s Allocating Tunnel Buffer Error=%x\n",__func__,err);
            return err;
        }
        openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
        if (omx_base_component_Private->state==OMX_StateExecuting) {
            for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
                omx_sem_up(openmaxStandPort->pBufferSem);
                omx_sem_up(omx_base_component_Private->bMgmtSem);
            }
        }
        omx_debug("In %s Qelem=%d BSem=%d\n", __func__,openmaxStandPort->pBufferQueue->nelem,openmaxStandPort->pBufferSem->semval);
    }

    openmaxStandPort->bIsTransientToEnabled = OMX_FALSE;

    omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}

/** @brief Called by the standard allocate buffer, it implements a base functionality.
*
* This function can be overriden if the allocation of the buffer is not a simply alloc call.
* The parameters are the same as the standard function, except for the handle of the port
* instead of the handler of the component
* When the buffers needed by this port are all assigned or allocated, the variable
* bIsFullOfBuffers becomes equal to OMX_TRUE
*/
OMX_ERRORTYPE base_port_AllocateBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE** pBuffer,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes) {

    unsigned int i;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
        return OMX_ErrorBadPortIndex;
    }
    if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        return OMX_ErrorBadPortIndex;
    }

    if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
        if (!openmaxStandPort->bIsTransientToEnabled) {
            omx_debug("In %s: The port is not allowed to receive buffers\n", __func__);
            return OMX_ErrorIncorrectStateTransition;
        }
    }

    if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize) {
        omx_debug("In %s: Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", __func__, nSizeBytes, openmaxStandPort->sPortParam.nBufferSize);
        return OMX_ErrorIncorrectStateTransition;
    }

    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
        if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
            openmaxStandPort->pInternalBufferStorage[i] = omx_alloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (!openmaxStandPort->pInternalBufferStorage[i]) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            memset(openmaxStandPort->pInternalBufferStorage[i], 0x00, sizeof(OMX_BUFFERHEADERTYPE));

            setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));
            /* allocate the buffer */
            openmaxStandPort->pInternalBufferStorage[i]->pBuffer = omx_alloc(nSizeBytes);
            if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer==NULL) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            memset(openmaxStandPort->pInternalBufferStorage[i]->pBuffer, 0x00, nSizeBytes);

            openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
            openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
            openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
            *pBuffer = openmaxStandPort->pInternalBufferStorage[i];
            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
            openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;
            if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
                openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            } else {
                openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }
            openmaxStandPort->nNumAssignedBuffers++;
            omx_debug("openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
                openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                omx_debug("In %s nPortIndex=%d\n",__func__,(int)nPortIndex);
                omx_sem_up(openmaxStandPort->pAllocSem);
            }
            omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }
    omx_debug("Out of %s for port %p. Error: no available buffers\n",__func__, openmaxStandPort);
    return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard use buffer, it implements a base functionality.
*
* This function can be overriden if the use buffer implicate more complicated operations.
* The parameters are the same as the standard function, except for the handle of the port
* instead of the handler of the component.
* When the buffers needed by this port are all assigned or allocated, the variable
* bIsFullOfBuffers becomes equal to OMX_TRUE
*/
OMX_ERRORTYPE base_port_UseBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8* pBuffer) {

    unsigned int i;
    OMX_BUFFERHEADERTYPE* returnBufferHeader;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);
    if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
        return OMX_ErrorBadPortIndex;
    }
    if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        return OMX_ErrorBadPortIndex;
    }

    if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
        if (!openmaxStandPort->bIsTransientToEnabled) {
            omx_debug("In %s: The port of Comp %s is not allowed to receive buffers\n", __func__,omx_base_component_Private->name);
            return OMX_ErrorIncorrectStateTransition;
        }
    }

    if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize) {
        omx_debug("In %s: Port %d Given Buffer Size %u is less than Minimum Buffer Size %u\n", __func__, (int)nPortIndex, (int)nSizeBytes, (int)openmaxStandPort->sPortParam.nBufferSize);
        return OMX_ErrorIncorrectStateTransition;
    }

    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
        if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
            openmaxStandPort->pInternalBufferStorage[i] = omx_alloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (!openmaxStandPort->pInternalBufferStorage[i]) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            memset(openmaxStandPort->pInternalBufferStorage[i], 0x00, sizeof(OMX_BUFFERHEADERTYPE));

            openmaxStandPort->bIsEmptyOfBuffers = OMX_FALSE;
            setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));

            openmaxStandPort->pInternalBufferStorage[i]->pBuffer = pBuffer;
            openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
            openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
            openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ASSIGNED;
            openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;

            returnBufferHeader = omx_alloc(sizeof(OMX_BUFFERHEADERTYPE));
            if (!returnBufferHeader) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            memset(returnBufferHeader, 0x00, sizeof(OMX_BUFFERHEADERTYPE));

            setHeader(returnBufferHeader, sizeof(OMX_BUFFERHEADERTYPE));
            returnBufferHeader->pBuffer = pBuffer;
            returnBufferHeader->nAllocLen = nSizeBytes;
            returnBufferHeader->pPlatformPrivate = openmaxStandPort;
            returnBufferHeader->pAppPrivate = pAppPrivate;
            if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
                openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                returnBufferHeader->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            } else {
                openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                returnBufferHeader->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }
            *ppBufferHdr = returnBufferHeader;
            openmaxStandPort->nNumAssignedBuffers++;
            omx_debug("openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
                openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                omx_sem_up(openmaxStandPort->pAllocSem);
            }
            omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }
    omx_debug("In %s Error: no available buffers CompName=%s\n",__func__,omx_base_component_Private->name);
    return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard function.
*
* It frees the buffer header and in case also the buffer itself, if needed.
* When all the buffers are done, the variable bIsEmptyOfBuffers is set to OMX_TRUE
*/
OMX_ERRORTYPE base_port_FreeBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_U32 nPortIndex,
    OMX_BUFFERHEADERTYPE* pBuffer) {

    unsigned int i;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
        return OMX_ErrorBadPortIndex;
    }
    if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        return OMX_ErrorBadPortIndex;
    }

    if (omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded) {
        if (!openmaxStandPort->bIsTransientToDisabled) {
            omx_debug("In %s: The port is not allowed to free the buffers\n", __func__);
            (*(omx_base_component_Private->callbacks->EventHandler))
            (omxComponent,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
             nPortIndex, /* The state has been changed in message->messageParam2 */
             NULL);
        }
    }

    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
        if (openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED))
        {
            openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;
            if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED) {
                if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer) {
                    omx_debug("In %s freeing %i pBuffer=%p\n",__func__, (int)i, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                    omx_free(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
                }
            } else if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ASSIGNED) {
                if (pBuffer) {
                    omx_free(pBuffer);
                    pBuffer = NULL;
                }
            }
            if(openmaxStandPort->bBufferStateAllocated[i] & HEADER_ALLOCATED) {
                if (openmaxStandPort->pInternalBufferStorage[i]) {
                    omx_free(openmaxStandPort->pInternalBufferStorage[i]);
                    openmaxStandPort->pInternalBufferStorage[i]=NULL;
                }
            }

            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

            openmaxStandPort->nNumAssignedBuffers--;
            omx_debug("openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if (openmaxStandPort->nNumAssignedBuffers == 0) {
                openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
                openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
                omx_sem_up(openmaxStandPort->pAllocSem);
            }
            omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }
    omx_debug("Out of %s for port %p with OMX_ErrorInsufficientResources\n", __func__, openmaxStandPort);
    return OMX_ErrorInsufficientResources;
}

OMX_ERRORTYPE base_port_AllocateTunnelBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_U32 nPortIndex)
{
    unsigned int i;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    OMX_U8* pBuffer=NULL;
    OMX_ERRORTYPE eError=OMX_ErrorNone,err;
    int errQue;
    OMX_U32 numRetry=0,nBufferSize;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_U32 nLocalBufferCountActual;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
        omx_debug("In %s: Bad Port Index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    if (! PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        omx_debug("In %s: Port is not tunneled Flag=%x\n", __func__, (int)openmaxStandPort->nTunnelFlags);
        return OMX_ErrorBadPortIndex;
    }

    if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
        if (!openmaxStandPort->bIsTransientToEnabled) {
            omx_debug("In %s: The port is not allowed to receive buffers\n", __func__);
            return OMX_ErrorIncorrectStateTransition;
        }
    }
    /*Get nBufferSize of the peer port and allocate which one is bigger*/
    nBufferSize = openmaxStandPort->sPortParam.nBufferSize;
    setHeader(&sPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    sPortDef.nPortIndex = openmaxStandPort->nTunneledPort;
    err = OMX_GetParameter(openmaxStandPort->hTunneledComponent, OMX_IndexParamPortDefinition, &sPortDef);
    if(err == OMX_ErrorNone) {
        nBufferSize = (sPortDef.nBufferSize > openmaxStandPort->sPortParam.nBufferSize) ? sPortDef.nBufferSize: openmaxStandPort->sPortParam.nBufferSize;
    } else {
        return OMX_ErrorPortsNotCompatible;
    }
    /* set the number of buffer needed getting the max nBufferCountActual of the two components
    * On the one with the minor nBufferCountActual a setParam should be called to normalize the value,
    * if possible.
    */
    nLocalBufferCountActual = openmaxStandPort->sPortParam.nBufferCountActual;
    if (nLocalBufferCountActual < sPortDef.nBufferCountActual) {
        nLocalBufferCountActual = sPortDef.nBufferCountActual;
        openmaxStandPort->sPortParam.nBufferCountActual = nLocalBufferCountActual;
    } else if (sPortDef.nBufferCountActual < nLocalBufferCountActual) {
        sPortDef.nBufferCountActual = nLocalBufferCountActual;
        err = OMX_SetParameter(openmaxStandPort->hTunneledComponent, OMX_IndexParamPortDefinition, &sPortDef);
        if(err != OMX_ErrorNone) {
            /* for some reasons undetected during negotiation the tunnel cannot be established.
            */
            return OMX_ErrorPortsNotCompatible;
        }
    }
    if (openmaxStandPort->sPortParam.nBufferCountActual == 0) {
        openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
        openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
        omx_debug("In %s Allocated nothing\n",__func__);
        return OMX_ErrorNone;
    }
    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
        if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
            pBuffer = omx_alloc(nBufferSize);
            if(pBuffer==NULL) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            memset(pBuffer, 0x00, nBufferSize);

            /*Retry more than once, if the tunneled component is not in Loaded->Idle State*/
            while(numRetry <TUNNEL_USE_BUFFER_RETRY) {
                eError = OMX_UseBuffer(openmaxStandPort->hTunneledComponent,&openmaxStandPort->pInternalBufferStorage[i],
                                     openmaxStandPort->nTunneledPort,NULL,nBufferSize,pBuffer);
                if(eError!=OMX_ErrorNone) {
                    omx_debug("Tunneled Component Couldn't Use buffer %i From Comp=%s Retry=%d\n",
                          i,omx_base_component_Private->name,(int)numRetry);

                    if((eError ==  OMX_ErrorIncorrectStateTransition) && numRetry<TUNNEL_USE_BUFFER_RETRY) {
                        omx_debug("Waiting for next try %i \n",(int)numRetry);
                        omx_usleep(TUNNEL_USE_BUFFER_RETRY_USLEEP_TIME);
                        numRetry++;
                        continue;
                    }
                    if (pBuffer) {
                        omx_free(pBuffer);
                        pBuffer = NULL;
                    }
                    return eError;
                }
                else {
                    if(openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
                        openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex  = openmaxStandPort->sPortParam.nPortIndex;
                        openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->nTunneledPort;
                    } else {
                        openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex  = openmaxStandPort->nTunneledPort;
                        openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                    }
                    break;
                }
            }
            if(eError!=OMX_ErrorNone) {
                if (pBuffer) {
                    omx_free(pBuffer);
                    pBuffer = NULL;
                }
                omx_debug("In %s Tunneled Component Couldn't Use Buffer err = %x \n",__func__,(int)eError);
                return eError;
            }
            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
            openmaxStandPort->nNumAssignedBuffers++;
            omx_debug("openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
                openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                omx_debug("In %s nPortIndex=%d\n",__func__, (int)nPortIndex);
            }
            errQue = queue(openmaxStandPort->pBufferQueue, openmaxStandPort->pInternalBufferStorage[i]);
            if (errQue) {
                /* /TODO the queue is full. This can be handled in a fine way with
                * some retrials, or other checking. For the moment this is a critical error
                * and simply causes the failure of this call
                */
                return OMX_ErrorInsufficientResources;
            }
        }
    }
    omx_debug("Out of %s for port %p. Allocated all the buffers\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE base_port_FreeTunnelBuffer(omx_base_PortType *openmaxStandPort,OMX_U32 nPortIndex)
{
    unsigned int i;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    OMX_ERRORTYPE eError=OMX_ErrorNone;
    OMX_U32 numRetry=0;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
        omx_debug("In %s: Bad Port Index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    if (! PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        omx_debug("In %s: Port is not tunneled\n", __func__);
        return OMX_ErrorBadPortIndex;
    }

    if (omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded) {
        if (!openmaxStandPort->bIsTransientToDisabled) {
            omx_debug("In %s: The port is not allowed to free the buffers\n", __func__);
            (*(omx_base_component_Private->callbacks->EventHandler))
            (omxComponent,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
             nPortIndex, /* The state has been changed in message->messageParam2 */
             NULL);
        }
    }

    for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++) {
        if (openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED)) {

            openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;
            if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED) {
                if (openmaxStandPort->pInternalBufferStorage[i]->pBuffer) {
                    omx_free(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer = NULL;
                }
            }
            /*Retry more than once, if the tunneled component is not in Idle->Loaded State*/
            while(numRetry <TUNNEL_USE_BUFFER_RETRY) {
                eError=OMX_FreeBuffer(openmaxStandPort->hTunneledComponent,openmaxStandPort->nTunneledPort,openmaxStandPort->pInternalBufferStorage[i]);
                if(eError!=OMX_ErrorNone) {
                    omx_debug("Tunneled Component Couldn't free buffer %i \n",i);
                    if((eError ==  OMX_ErrorIncorrectStateTransition) && numRetry<TUNNEL_USE_BUFFER_RETRY) {
                        omx_debug("Waiting for next try %i \n",(int)numRetry);
                        omx_usleep(TUNNEL_USE_BUFFER_RETRY_USLEEP_TIME);
                        numRetry++;
                        continue;
                    }
                    return eError;
                } else {
                    break;
                }
            }
            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

            openmaxStandPort->nNumAssignedBuffers--;
            omx_debug("openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if (openmaxStandPort->nNumAssignedBuffers == 0) {
                openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
                openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
                //tsem_up(openmaxStandPort->pAllocSem);
            }
        }
    }
    omx_debug("Out of %s for port %p Qelem=%d BSem=%d\n", __func__, openmaxStandPort,
          openmaxStandPort->pBufferQueue->nelem, openmaxStandPort->pBufferSem->semval);
    return OMX_ErrorNone;
}

/** @brief the entry point for sending buffers to the port
*
* This function can be called by the EmptyThisBuffer or FillThisBuffer. It depends on
* the nature of the port, that can be an input or output port.
*/
OMX_ERRORTYPE base_port_SendBufferFunction(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE* pBuffer) {

    OMX_ERRORTYPE err;
    int errQue;
    OMX_U32 portIndex;
    OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);

    portIndex = (openmaxStandPort->sPortParam.eDir == OMX_DirInput)?pBuffer->nInputPortIndex:pBuffer->nOutputPortIndex;
    omx_debug("In %s portIndex %lu\n", __func__, portIndex);

    if (portIndex != openmaxStandPort->sPortParam.nPortIndex) {
        omx_debug("In %s: wrong port for this operation portIndex=%d port->portIndex=%d\n", __func__, (int)portIndex, (int)openmaxStandPort->sPortParam.nPortIndex);
        return OMX_ErrorBadPortIndex;
    }

    if(omx_base_component_Private->state == OMX_StateInvalid) {
        omx_debug("In %s: we are in OMX_StateInvalid\n", __func__);
        return OMX_ErrorInvalidState;
    }

    if(omx_base_component_Private->state != OMX_StateExecuting &&
            omx_base_component_Private->state != OMX_StatePause &&
            omx_base_component_Private->state != OMX_StateIdle) {
        omx_debug("In %s: we are not in executing/paused/idle state, but in %d\n", __func__, omx_base_component_Private->state);
        return OMX_ErrorIncorrectStateOperation;
    }
    if (!PORT_IS_ENABLED(openmaxStandPort) || (PORT_IS_BEING_DISABLED(openmaxStandPort) && !PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) ||
            ((omx_base_component_Private->transientState == OMX_TransStateExecutingToIdle ||
              omx_base_component_Private->transientState == OMX_TransStatePauseToIdle) &&
             (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)))) {
        omx_debug("In %s: Port %d is disabled comp = %s \n", __func__, (int)portIndex,omx_base_component_Private->name);
        return OMX_ErrorIncorrectStateOperation;
    }


    if ((err = checkHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE))) != OMX_ErrorNone) {
        omx_debug("In %s: received wrong buffer header on input port\n", __func__);
        return err;
    }

    /* And notify the buffer management thread we have a fresh new buffer to manage */
    if(!PORT_IS_BEING_FLUSHED(openmaxStandPort) && !(PORT_IS_BEING_DISABLED(openmaxStandPort) && PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)))
    {
        errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }
        omx_sem_up(openmaxStandPort->pBufferSem);
        omx_debug("In %s Signalling bMgmtSem Port Index=%d\n",__func__, (int)portIndex);
        omx_sem_up(omx_base_component_Private->bMgmtSem);
    } else if(PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
        omx_debug("In %s: Comp %s received io:%d buffer\n",
              __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
        errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }
        omx_sem_up(openmaxStandPort->pBufferSem);
    }
    else { // If port being flushed and not tunneled then return error
        omx_debug("In %s \n", __func__);
        return OMX_ErrorIncorrectStateOperation;
    }
    omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}

/**
* Returns Input/Output Buffer to the IL client or Tunneled Component
*/
OMX_ERRORTYPE base_port_ReturnBufferFunction(omx_base_PortType* openmaxStandPort,OMX_BUFFERHEADERTYPE* pBuffer) {
    omx_base_component_PrivateType* omx_base_component_Private=openmaxStandPort->standCompContainer->pComponentPrivate;
    queue_t* pQueue = openmaxStandPort->pBufferQueue;
    omx_sem_t* pSem = openmaxStandPort->pBufferSem;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int errQue;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);
    if (PORT_IS_TUNNELED(openmaxStandPort) &&
            ! PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
            pBuffer->nOutputPortIndex = openmaxStandPort->nTunneledPort;
            pBuffer->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                omx_debug("In %s eError %08x in FillThis Buffer from Component %s Non-Supplier\n",
                      __func__, eError,omx_base_component_Private->name);
            }
        } else {
            pBuffer->nInputPortIndex = openmaxStandPort->nTunneledPort;
            pBuffer->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                omx_debug("In %s eError %08x in EmptyThis Buffer from Component %s Non-Supplier\n",
                      __func__, eError,omx_base_component_Private->name);
            }
        }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort) &&
               !PORT_IS_BEING_FLUSHED(openmaxStandPort)) {
        if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                omx_debug("In %s eError %08x in FillThis Buffer from Component %s Supplier\n",
                      __func__, eError,omx_base_component_Private->name);
                /*If Error Occured then queue the buffer*/
                errQue = queue(pQueue, pBuffer);
                if (errQue) {
                    /* /TODO the queue is full. This can be handled in a fine way with
                    * some retrials, or other checking. For the moment this is a critical error
                    * and simply causes the failure of this call
                    */
                    return OMX_ErrorInsufficientResources;
                }
                omx_sem_up(pSem);
            }
        } else {
            eError = ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            if(eError != OMX_ErrorNone) {
                omx_debug("In %s eError %08x in EmptyThis Buffer from Component %s Supplier\n",
                      __func__, eError,omx_base_component_Private->name);
                /*If Error Occured then queue the buffer*/
                errQue = queue(pQueue, pBuffer);
                if (errQue) {
                    /* /TODO the queue is full. This can be handled in a fine way with
                    * some retrials, or other checking. For the moment this is a critical error
                    * and simply causes the failure of this call
                    */
                    return OMX_ErrorInsufficientResources;
                }
                omx_sem_up(pSem);
            }
        }
    } else if (!PORT_IS_TUNNELED(openmaxStandPort)) {
        (*(openmaxStandPort->BufferProcessedCallback))(
            openmaxStandPort->standCompContainer,
            omx_base_component_Private->callbackData,
            pBuffer);
    } else {
        errQue = queue(pQueue, pBuffer);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
            * some retrials, or other checking. For the moment this is a critical error
            * and simply causes the failure of this call
            */
            return OMX_ErrorInsufficientResources;
        }
        openmaxStandPort->nNumBufferFlushed++;
    }

    omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}


OMX_ERRORTYPE base_port_ComponentTunnelRequest(omx_base_PortType* openmaxStandPort, OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup) {
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE param;
    OMX_PARAM_BUFFERSUPPLIERTYPE pSupplier;

    omx_debug("In %s for port %p\n", __func__, openmaxStandPort);
    if (pTunnelSetup == NULL || hTunneledComp == 0) {
        /* cancel previous tunnel */
        openmaxStandPort->hTunneledComponent = 0;
        openmaxStandPort->nTunneledPort = 0;
        openmaxStandPort->nTunnelFlags = 0;
        openmaxStandPort->eBufferSupplier=OMX_BufferSupplyUnspecified;
        return OMX_ErrorNone;
    }

    if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        /* Get Port Definition of the Tunnelled Component*/
        param.nPortIndex=nTunneledPort;
        setHeader(&param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        err = OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &param);
        /// \todo insert here a detailed comparison with the OMX_AUDIO_PORTDEFINITIONTYPE
        if (err != OMX_ErrorNone) {
            omx_debug("In %s Tunneled Port Definition error=0x%08x Line=%d\n",__func__,err,__LINE__);
            // compatibility not reached
            return OMX_ErrorPortsNotCompatible;
        }
        openmaxStandPort->nNumTunnelBuffer = param.nBufferCountActual;
        if(param.eDomain!=openmaxStandPort->sPortParam.eDomain) {
            return OMX_ErrorPortsNotCompatible;
        }
        if(param.eDomain==OMX_PortDomainAudio) {
            if(param.format.audio.eEncoding == OMX_AUDIO_CodingMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        } else if(param.eDomain==OMX_PortDomainVideo) {
            if(param.format.video.eCompressionFormat == OMX_VIDEO_CodingMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        } else if(param.eDomain==OMX_PortDomainOther) {
            if(param.format.other.eFormat == OMX_OTHER_FormatMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        }

        /* Get Buffer Supplier type of the Tunneled Component*/
        pSupplier.nPortIndex=nTunneledPort;
        setHeader(&pSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        err = OMX_GetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &pSupplier);
        if (err != OMX_ErrorNone) {
            // compatibility not reached
            omx_debug("In %s Tunneled Buffer Supplier error=0x%08x Line=%d\n",__func__,err,__LINE__);
            return OMX_ErrorPortsNotCompatible;
        } else {
            omx_debug("Tunneled Port eBufferSupplier=%x\n", pSupplier.eBufferSupplier);
        }

        // store the current callbacks, if defined
        openmaxStandPort->hTunneledComponent = hTunneledComp;
        openmaxStandPort->nTunneledPort = nTunneledPort;

        /*Check for and set proprietary communication flag.
        In case a component support Deep Tunneling should set it's tunnel flag to PROPRIETARY_COMMUNICATION_ESTABLISHED */
        if(PORT_IS_DEEP_TUNNELED(openmaxStandPort)) {
            OMX_VENDOR_PROP_TUNNELSETUPTYPE pPropTunnelSetup;
            pPropTunnelSetup.nPortIndex = nTunneledPort;

            err = OMX_GetParameter(hTunneledComp, OMX_IndexVendorCompPropTunnelFlags, &pPropTunnelSetup);
            if (err != OMX_ErrorNone) {
                // compatibility not reached
                omx_debug("In %s Proprietary Tunneled Buffer Supplier nTunneledPort=%d error=0x%08x Line=%d \n",
                      __func__,(int)pPropTunnelSetup.nPortIndex,err,__LINE__);
                openmaxStandPort->nTunnelFlags = 0;
            } else {
                openmaxStandPort->nTunnelFlags = PROPRIETARY_COMMUNICATION_ESTABLISHED;
            }
        } else {
            openmaxStandPort->nTunnelFlags = 0;
        }

        // Negotiation
        if (pTunnelSetup->nTunnelFlags & OMX_PORTTUNNELFLAG_READONLY) {
            // the buffer provider MUST be the output port provider
            pTunnelSetup->eSupplier = OMX_BufferSupplyInput;
            openmaxStandPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
            openmaxStandPort->eBufferSupplier=OMX_BufferSupplyInput;
        } else {
            if (pTunnelSetup->eSupplier == OMX_BufferSupplyInput) {
                openmaxStandPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
                openmaxStandPort->eBufferSupplier=OMX_BufferSupplyInput;
            } else if (pTunnelSetup->eSupplier == OMX_BufferSupplyUnspecified) {
                pTunnelSetup->eSupplier = OMX_BufferSupplyInput;
                openmaxStandPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
                openmaxStandPort->eBufferSupplier=OMX_BufferSupplyInput;
            }
        }
        openmaxStandPort->nTunnelFlags |= TUNNEL_ESTABLISHED;

        /* Set Buffer Supplier type of the Tunnelled Component after final negotiation*/
        pSupplier.nPortIndex=nTunneledPort;
        pSupplier.eBufferSupplier=openmaxStandPort->eBufferSupplier;
        err = OMX_SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &pSupplier);
        if (err != OMX_ErrorNone) {
            // compatibility not reached
            omx_debug("In %s Tunneled Buffer Supplier error=0x%08x Line=%d\n",__func__,err,__LINE__);
            openmaxStandPort->nTunnelFlags=0;
            return OMX_ErrorPortsNotCompatible;
        }
    } else {
        // output port
        // all the consistency checks are under other component responsibility

        /* Get Port Definition of the Tunnelled Component*/
        param.nPortIndex=nTunneledPort;
        setHeader(&param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        err = OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &param);
        if (err != OMX_ErrorNone) {
            omx_debug("In %s Tunneled Port Definition error=0x%08x Line=%d\n",__func__,err,__LINE__);
            // compatibility not reached
            return OMX_ErrorPortsNotCompatible;
        }
        if(param.eDomain!=openmaxStandPort->sPortParam.eDomain) {
            return OMX_ErrorPortsNotCompatible;
        }

        if(param.eDomain==OMX_PortDomainAudio) {
            if(param.format.audio.eEncoding == OMX_AUDIO_CodingMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        } else if(param.eDomain==OMX_PortDomainVideo) {
            if(param.format.video.eCompressionFormat == OMX_VIDEO_CodingMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        } else if(param.eDomain==OMX_PortDomainOther) {
            if(param.format.other.eFormat == OMX_OTHER_FormatMax) {
                return OMX_ErrorPortsNotCompatible;
            }
        }

        /*Check for and set proprietary communication flag*/
        if(PORT_IS_DEEP_TUNNELED(openmaxStandPort)) {
            OMX_VENDOR_PROP_TUNNELSETUPTYPE pPropTunnelSetup;
            pPropTunnelSetup.nPortIndex = nTunneledPort;

            err = OMX_GetParameter(hTunneledComp, OMX_IndexVendorCompPropTunnelFlags, &pPropTunnelSetup);
            if (err != OMX_ErrorNone) {
                // compatibility not reached
                omx_debug("In %s Proprietary Tunneled Buffer Supplier nTunneledPort=%d error=0x%08x Line=%d \n",
                      __func__,(int)pPropTunnelSetup.nPortIndex,err,__LINE__);
                openmaxStandPort->nTunnelFlags = 0;
            } else {
                openmaxStandPort->nTunnelFlags = PROPRIETARY_COMMUNICATION_ESTABLISHED;
            }
        } else {
            openmaxStandPort->nTunnelFlags = 0;
        }

        openmaxStandPort->nNumTunnelBuffer=param.nBufferCountActual;

        openmaxStandPort->hTunneledComponent = hTunneledComp;
        openmaxStandPort->nTunneledPort = nTunneledPort;
        pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
        openmaxStandPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
        openmaxStandPort->nTunnelFlags |= TUNNEL_ESTABLISHED;

        openmaxStandPort->eBufferSupplier=OMX_BufferSupplyOutput;
    }

    omx_debug("Out of %s for port %p\n", __func__, openmaxStandPort);
    return OMX_ErrorNone;
}
