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


#ifdef __cplusplus
extern "C" {
#endif


#include "omx_base_component.h"

/**
 * This function releases all the resources allocated by the base constructor if something fails.
 * It checks if any item has been already allocated/configured
 */

void base_constructor_remove_garbage_collected(omx_base_component_PrivateType* omx_base_component_private) {

    omx_base_component_PrivateType* omx_base_component_Private = omx_base_component_private;

    if (omx_base_component_Private->flush_condition) {
        omx_sem_deinit(omx_base_component_Private->flush_condition);
        omx_free(omx_base_component_Private->flush_condition);
        omx_base_component_Private->flush_condition = NULL;
    }
    if (omx_base_component_Private->deinterlace_flush_condition) {
        omx_sem_deinit(omx_base_component_Private->deinterlace_flush_condition);
        omx_free(omx_base_component_Private->deinterlace_flush_condition);
        omx_base_component_Private->deinterlace_flush_condition = NULL;
    }
    if (omx_base_component_Private->flush_all_condition) {
        omx_sem_deinit(omx_base_component_Private->flush_all_condition);
        omx_free(omx_base_component_Private->flush_all_condition);
        omx_base_component_Private->flush_all_condition = NULL;
    }
    if (omx_base_component_Private->name) {
        omx_free(omx_base_component_Private->name);
    }
    if (omx_base_component_Private->bStateSem) {
        omx_sem_deinit(omx_base_component_Private->bStateSem);
        omx_free(omx_base_component_Private->bStateSem);
        omx_base_component_Private->bStateSem = NULL;
    }
    if (omx_base_component_Private->bMgmtSem) {
        omx_sem_deinit(omx_base_component_Private->bMgmtSem);
        omx_free(omx_base_component_Private->bMgmtSem);
        omx_base_component_Private->bMgmtSem = NULL;
    }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
    if (omx_base_component_Private->bDispBufFullSem) {
        omx_sem_deinit(omx_base_component_Private->bDispBufFullSem);
        omx_free(omx_base_component_Private->bDispBufFullSem);
        omx_base_component_Private->bDispBufFullSem = NULL;
    }
#endif
    if (omx_base_component_Private->messageSem) {
        omx_sem_deinit(omx_base_component_Private->messageSem);
        omx_free(omx_base_component_Private->messageSem);
        omx_base_component_Private->messageSem = NULL;
    }
    if (omx_base_component_Private->messageQueue) {
        queue_deinit(omx_base_component_Private->messageQueue);
        omx_free(omx_base_component_Private->messageQueue);
        omx_base_component_Private->messageQueue = NULL;
    }
    if (omx_base_component_Private) {
        omx_free(omx_base_component_Private);
        omx_base_component_Private = NULL;
    }
}
/**
 * @brief The base constructor for the OpenMAX components
 *
 * This function is executed by the component loader.
 * It takes care of constructing the instance of the component.
 * For the base_component component, the following is done:
 *
 * 1) Fills the basic OpenMAX structure. The fields can be overwritten
 *    by derived components.
 * 2) Allocates (if needed) the omx_base_component_PrivateType private structure
 *
 * @param openmaxStandComp the component to be initialized
 * @param cComponentName the OpenMAX string that describes the component
 *
 * @return OMX_ErrorInsufficientResources if a memory allocation fails
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName) {
    omx_base_component_PrivateType* omx_base_component_Private;
    OMX_U32 i;
    int err;
	char buf[32];
    omx_thread_attr_t attr;
    struct omx_sched_param sched;

    omx_debug("In %s for component %p priv %p\n", __func__, openmaxStandComp, openmaxStandComp->pComponentPrivate);

    if (openmaxStandComp->pComponentPrivate) {
        omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    } else {
        omx_base_component_Private = omx_alloc(sizeof(omx_base_component_PrivateType));
        if (!omx_base_component_Private) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private, 0x00, sizeof(omx_base_component_PrivateType));
    }

    if(!omx_base_component_Private->messageQueue) {
        omx_base_component_Private->messageQueue = omx_alloc(sizeof(queue_t));
        if (!omx_base_component_Private->messageQueue) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->messageQueue, 0x00, sizeof(queue_t));

        err = queue_init(omx_base_component_Private->messageQueue);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    if(!omx_base_component_Private->messageSem) {
        omx_base_component_Private->messageSem = omx_alloc(sizeof(omx_sem_t));
        if (!omx_base_component_Private->messageSem) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->messageSem, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->messageSem, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    if(!omx_base_component_Private->bMgmtSem) {
        omx_base_component_Private->bMgmtSem = omx_alloc(sizeof(omx_sem_t));
        if (!omx_base_component_Private->bMgmtSem) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->bMgmtSem, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->bMgmtSem, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
    if(!omx_base_component_Private->bDispBufFullSem) {
        omx_base_component_Private->bDispBufFullSem = omx_alloc(sizeof(omx_sem_t));
        if (!omx_base_component_Private->bDispBufFullSem) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->bDispBufFullSem, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->bDispBufFullSem, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }
#endif
    if(!omx_base_component_Private->bStateSem) {
        omx_base_component_Private->bStateSem = omx_alloc(sizeof(omx_sem_t));
        if (!omx_base_component_Private->bStateSem) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->bStateSem, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->bStateSem, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    openmaxStandComp->nSize = sizeof(OMX_COMPONENTTYPE);
    openmaxStandComp->pApplicationPrivate = NULL;
    //openmaxStandComp->GetComponentVersion = omx_base_component_GetComponentVersion;
    openmaxStandComp->SendCommand = omx_base_component_SendCommand;
    openmaxStandComp->GetParameter = omx_base_component_GetParameter;
    openmaxStandComp->SetParameter = omx_base_component_SetParameter;
    openmaxStandComp->GetConfig = omx_base_component_GetConfig;
    openmaxStandComp->SetConfig = omx_base_component_SetConfig;
    openmaxStandComp->GetState = omx_base_component_GetState;
    openmaxStandComp->SetCallbacks = omx_base_component_SetCallbacks;
    openmaxStandComp->ComponentDeInit = omx_base_component_ComponentDeInit;
    openmaxStandComp->ComponentRoleEnum = omx_base_component_ComponentRoleEnum;
    openmaxStandComp->ComponentTunnelRequest =omx_base_component_ComponentTunnelRequest;

    /*Will make Specific port Allocate buffer call*/
    openmaxStandComp->AllocateBuffer = omx_base_component_AllocateBuffer;
    openmaxStandComp->UseBuffer = omx_base_component_UseBuffer;
    openmaxStandComp->FreeBuffer = omx_base_component_FreeBuffer;
    openmaxStandComp->EmptyThisBuffer = omx_base_component_EmptyThisBuffer;
    openmaxStandComp->FillThisBuffer = omx_base_component_FillThisBuffer;

    openmaxStandComp->nVersion.s.nVersionMajor = SPECVERSIONMAJOR;
    openmaxStandComp->nVersion.s.nVersionMinor = SPECVERSIONMINOR;
    openmaxStandComp->nVersion.s.nRevision = SPECREVISION;
    openmaxStandComp->nVersion.s.nStep = SPECSTEP;

    omx_base_component_Private->name = omx_alloc(OMX_STRINGNAME_SIZE);
    if (!omx_base_component_Private->name) {
        base_constructor_remove_garbage_collected(omx_base_component_Private);
        return OMX_ErrorInsufficientResources;
    }
    memset(omx_base_component_Private->name, 0x00, OMX_STRINGNAME_SIZE);

    strcpy(omx_base_component_Private->name,cComponentName);
    omx_base_component_Private->state = OMX_StateLoaded;
    omx_base_component_Private->transientState = OMX_TransStateMax;
    omx_base_component_Private->callbacks = NULL;
    omx_base_component_Private->callbackData = NULL;
    omx_base_component_Private->nGroupPriority = 100;
    omx_base_component_Private->nGroupID = 0;
    omx_base_component_Private->pMark.hMarkTargetComponent = NULL;
    omx_base_component_Private->pMark.pMarkData            = NULL;
    omx_base_component_Private->openmaxStandComp = openmaxStandComp;
    omx_base_component_Private->DoStateSet = &omx_base_component_DoStateSet;
    omx_base_component_Private->messageHandler = omx_base_component_MessageHandler;
    omx_base_component_Private->destructor = omx_base_component_Destructor;
    omx_base_component_Private->bufferMgmtThreadID = -1;

    omx_thread_mutex_init(&omx_base_component_Private->flush_mutex, NULL);

    if(!omx_base_component_Private->flush_all_condition) {
        omx_base_component_Private->flush_all_condition = omx_alloc(sizeof(omx_sem_t));
        if(!omx_base_component_Private->flush_all_condition) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->flush_all_condition, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->flush_all_condition, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    if(!omx_base_component_Private->flush_condition) {
        omx_base_component_Private->flush_condition = omx_alloc(sizeof(omx_sem_t));
        if(!omx_base_component_Private->flush_condition) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->flush_condition, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->flush_condition, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    if(!omx_base_component_Private->deinterlace_flush_condition) {
        omx_base_component_Private->deinterlace_flush_condition = omx_alloc(sizeof(omx_sem_t));
        if(!omx_base_component_Private->deinterlace_flush_condition) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_component_Private->deinterlace_flush_condition, 0x00, sizeof(omx_sem_t));

        err = omx_sem_init(omx_base_component_Private->deinterlace_flush_condition, 0);
        if (err != 0) {
            base_constructor_remove_garbage_collected(omx_base_component_Private);
            return OMX_ErrorInsufficientResources;
        }
    }

    for(i=0; i<NUM_DOMAINS; i++) {
        memset(&omx_base_component_Private->sPortTypesParam[i], 0, sizeof(OMX_PORT_PARAM_TYPE));
        setHeader(&omx_base_component_Private->sPortTypesParam[i], sizeof(OMX_PORT_PARAM_TYPE));
    }


    omx_thread_attr_init(&attr);

    sched.sched_priority = configAPPLICATION_OMX_HIGH_PRIORITY;
    omx_thread_attr_setschedparam(&attr,&sched);
    omx_thread_attr_setstacksize(&attr, 4096);

    err = omx_thread_create(&omx_base_component_Private->messageHandlerThread, &attr, compMessageHandlerFunction, openmaxStandComp);
    if (err) {
        base_constructor_remove_garbage_collected(omx_base_component_Private);
        return OMX_ErrorInsufficientResources;
    }

	snprintf(buf, sizeof(buf), "%sMsgHF", omx_base_component_Private->name);
    omx_debug("In %s messageHandlerThread name %s\n", __func__, buf);
	omx_thread_setname_np(omx_base_component_Private->messageHandlerThread, buf);

    omx_debug("Out of %s for component %p priv %p\n", __func__, openmaxStandComp, openmaxStandComp->pComponentPrivate);
    return OMX_ErrorNone;
}

/** @brief The base destructor for ST OpenMAX components
 *
 * This function is called by the standard function ComponentDeInit()
 * that is called by the IL core during the execution of the  FreeHandle()
 *
 * @param openmaxStandComp the ST OpenMAX component to be disposed
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    int err = 0;
    omx_debug("In %s for component %p\n", __func__, openmaxStandComp);
    omx_base_component_Private->state = OMX_StateInvalid;
    omx_base_component_Private->callbacks=NULL;

    /*Send Dummy signal to Component Message handler to exit*/
    omx_sem_up(omx_base_component_Private->messageSem);

    omx_thread_join(omx_base_component_Private->messageHandlerThread, (void **)&err);
    if(err!=0) {
        omx_debug("In %s omx_thread_join returned err=%d\n", __func__, err);
    }
    omx_debug("In %s after omx_thread_join\n", __func__);

    /*Deinitialize and free message queue*/
    if(omx_base_component_Private->messageQueue) {
        queue_deinit(omx_base_component_Private->messageQueue);
        omx_free(omx_base_component_Private->messageQueue);
        omx_base_component_Private->messageQueue=NULL;
    }

    /*Deinitialize and free buffer management semaphore*/
    if(omx_base_component_Private->bMgmtSem) {
        omx_sem_deinit(omx_base_component_Private->bMgmtSem);
        omx_free(omx_base_component_Private->bMgmtSem);
        omx_base_component_Private->bMgmtSem=NULL;
    }
#ifdef WAITING_TIME_FOR_NEXT_OUTPUT_BUFFER_AFTER_DISPLAY_BUFFER_FULL
    /*Deinitialize and free buffer display buffer full semaphore*/
    if(omx_base_component_Private->bDispBufFullSem) {
        omx_sem_deinit(omx_base_component_Private->bDispBufFullSem);
        omx_free(omx_base_component_Private->bDispBufFullSem);
        omx_base_component_Private->bDispBufFullSem=NULL;
    }
#endif
    /*Deinitialize and free message semaphore*/
    if(omx_base_component_Private->messageSem) {
        omx_sem_deinit(omx_base_component_Private->messageSem);
        omx_free(omx_base_component_Private->messageSem);
        omx_base_component_Private->messageSem=NULL;
    }

    if(omx_base_component_Private->bStateSem) {
        omx_sem_deinit(omx_base_component_Private->bStateSem);
        omx_free(omx_base_component_Private->bStateSem);
        omx_base_component_Private->bStateSem=NULL;
    }

    if(omx_base_component_Private->name) {
        omx_free(omx_base_component_Private->name);
        omx_base_component_Private->name=NULL;
    }

    omx_thread_mutex_destroy(&omx_base_component_Private->flush_mutex);

    if(omx_base_component_Private->flush_all_condition) {
        omx_sem_deinit(omx_base_component_Private->flush_all_condition);
        omx_free(omx_base_component_Private->flush_all_condition);
        omx_base_component_Private->flush_all_condition=NULL;
    }

    if(omx_base_component_Private->flush_condition) {
        omx_sem_deinit(omx_base_component_Private->flush_condition);
        omx_free(omx_base_component_Private->flush_condition);
        omx_base_component_Private->flush_condition=NULL;
    }

    if(omx_base_component_Private->deinterlace_flush_condition) {
        omx_sem_deinit(omx_base_component_Private->deinterlace_flush_condition);
        omx_free(omx_base_component_Private->deinterlace_flush_condition);
        omx_base_component_Private->deinterlace_flush_condition=NULL;
    }

    omx_debug("Out of %s for component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

/** @brief This standard functionality is called when the component is
 * destroyed in the FreeHandle standard call.
 *
 * In this way the implementation of the FreeHandle is standard,
 * and it does not need a support by a specific component loader.
 * The implementation of the ComponentDeInit contains the
 * implementation specific part of the destroying phase.
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_ComponentDeInit(
    OMX_HANDLETYPE hComponent) {
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_debug("In %s for component %p\n", __func__, openmaxStandComp);

    omx_base_component_Private->destructor(openmaxStandComp);
    if (openmaxStandComp->pComponentPrivate) {
        omx_free(openmaxStandComp->pComponentPrivate);
        openmaxStandComp->pComponentPrivate=NULL;
    }

    omx_debug("Out of %s for component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

/** Changes the state of a component taking proper actions depending on
 * the transition requested. This base function cover only the state
 * changes that do not involve any port
 *
 * @param openmaxStandComp the OpenMAX component which state is to be changed
 * @param destinationState the requested target state
 *
 * @return OMX_ErrorNotImplemented if the state change is not handled in this base class, but needs
 * a specific handling
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_DoStateSet(OMX_COMPONENTTYPE *openmaxStandComp, OMX_U32 destinationState) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_U32 i,j,k;
    OMX_ERRORTYPE err=OMX_ErrorNone;
    OMX_BOOL bExit = OMX_FALSE;
    omx_thread_attr_t attr;
    struct omx_sched_param sched;
	char buf[32];

    omx_debug("In %s for component %p\n", __func__, openmaxStandComp);
    omx_debug("Changing state from %i to %i\n", omx_base_component_Private->state, (int)destinationState);

#if 0
    if (omx_base_component_Private->state == OMX_StateLoaded && destinationState == OMX_StateIdle) {
        err = RM_getResource(openmaxStandComp);
        if (err != OMX_ErrorNone) {
            return OMX_ErrorInsufficientResources;
        }
    }
    if (omx_base_component_Private->state == OMX_StateIdle && destinationState == OMX_StateLoaded) {
        RM_releaseResource(openmaxStandComp);
    }
#endif

    if(destinationState == OMX_StateLoaded) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StateWaitForResources:
            /* return back from wait for resources */
            omx_base_component_Private->state = OMX_StateLoaded;
            break;
        case OMX_StateLoaded:
            err = OMX_ErrorSameState;
            break;
        case OMX_StateIdle:
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {

                    pPort = omx_base_component_Private->ports[i];
                    if (PORT_IS_TUNNELED(pPort) && PORT_IS_BUFFER_SUPPLIER(pPort)) {
                        while(pPort->pBufferQueue->nelem > 0) {
                            omx_debug("In %s Buffer %d remained in the port %d queue of comp%s\n",
                                  __func__,(int)pPort->pBufferQueue->nelem,(int)i,omx_base_component_Private->name);
                            dequeue(pPort->pBufferQueue);
                        }
                        /* Freeing here the buffers allocated for the tunneling:*/
                        err = pPort->Port_FreeTunnelBuffer(pPort,i);
                        if(err!=OMX_ErrorNone) {
                            omx_debug("In %s Freeing Tunnel Buffer Error=%x\n",__func__,err);
                            return err;
                        }
                    } else {
                        omx_debug("In %s nPortIndex=%d pAllocSem Semval=%x\n", __func__,(int)i,(int)pPort->pAllocSem->semval);

                        /*If ports are enabled then wait till all buffers are freed*/
                        if(PORT_IS_ENABLED(pPort)) {
                            omx_sem_down(pPort->pAllocSem);
                        }
                    }
                    pPort->sPortParam.bPopulated = OMX_FALSE;

                    if(pPort->pInternalBufferStorage != NULL) {
                        omx_free(pPort->pInternalBufferStorage);
                        pPort->pInternalBufferStorage=NULL;
                    }

                    if(pPort->bBufferStateAllocated != NULL) {
                        omx_free(pPort->bBufferStateAllocated);
                        pPort->bBufferStateAllocated=NULL;
                    }
                }
            }
            omx_base_component_Private->state = OMX_StateLoaded;

            if(omx_base_component_Private->bufferMgmtThreadID == 0 ) {
                /*Signal Buffer Management thread to exit*/
                omx_sem_up(omx_base_component_Private->bMgmtSem);
                omx_thread_join(omx_base_component_Private->bufferMgmtThread, NULL);
                omx_base_component_Private->bufferMgmtThreadID = -1;
                if(err != 0) {
                    omx_debug("In %s omx_thread_join returned err=%d\n",__func__,err);
                }
            }

            break;
        default:
            omx_debug("In %s: state transition not allowed\n", __func__);
            err = OMX_ErrorIncorrectStateTransition;
            break;
        }
        omx_debug("Out of %s for component %p with err %x\n", __func__, openmaxStandComp, err);
        return err;
    }

    if(destinationState == OMX_StateWaitForResources) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StateLoaded:
            omx_base_component_Private->state = OMX_StateWaitForResources;
            //err = RM_waitForResource(openmaxStandComp);
            break;
        case OMX_StateWaitForResources:
            err = OMX_ErrorSameState;
            break;
        default:
            omx_debug("In %s: state transition not allowed\n", __func__);
            err = OMX_ErrorIncorrectStateTransition;
            break;
        }
        omx_debug("Out of %s for component %p with err %i\n", __func__, openmaxStandComp, err);
        return err;
    }

    if(destinationState == OMX_StateIdle) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StateWaitForResources:
            omx_base_component_Private->state = OMX_StateIdle;
            break;
        case OMX_StateLoaded:
            /* for all ports */

            for(j = 0; j < NUM_DOMAINS; j++)
            {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++)
                {

                    pPort = omx_base_component_Private->ports[i];

                    if (PORT_IS_TUNNELED(pPort) && PORT_IS_BUFFER_SUPPLIER(pPort))
                    {

                        if(PORT_IS_ENABLED(pPort))
                        {
                            /** Allocate here the buffers needed for the tunneling */
                            err= pPort->Port_AllocateTunnelBuffer(pPort, i);
                            if(err!=OMX_ErrorNone)
                            {
                                omx_debug("In %s Allocating Tunnel Buffer Error=%x\n",__func__,err);
                                return err;
                            }
                        }
                    }
                    else
                    {
                        if(PORT_IS_ENABLED(pPort))
                        {
                            omx_debug("In %s: wait for buffers. port enabled %i,  port populated %i\n",
                                  __func__, pPort->sPortParam.bEnabled,pPort->sPortParam.bPopulated);
                            if (pPort->sPortParam.nBufferCountActual > 0)
                            {
                                omx_sem_down(pPort->pAllocSem);
                                omx_thread_mutex_lock(&pPort->exitMutex);
                                if (pPort->bIsDestroying)
                                {
                                    bExit = OMX_TRUE;
                                    omx_thread_mutex_unlock(&pPort->exitMutex);
                                    continue;
                                }
                                omx_thread_mutex_unlock(&pPort->exitMutex);
                            }
                            pPort->sPortParam.bPopulated = OMX_TRUE;
                        }
                        else
                        {
                            omx_debug("In %s: Port %i Disabled So no wait\n",__func__,(int)i);
                        }
                    }
                    omx_debug("Tunnel status : port %d flags  0x%x\n",(int)i, (int)pPort->nTunnelFlags);
                }
            }

            if (bExit) {
                omx_debug("bExit = true!! break!!\n");
                break;
            }
            omx_debug("bExit = false!! continue!!\n");
            omx_base_component_Private->state = OMX_StateIdle;
            /** starting buffer management thread */

			omx_thread_attr_init(&attr);

			sched.sched_priority = configAPPLICATION_OMX_HIGH_PRIORITY;
			omx_thread_attr_setschedparam(&attr,&sched);
			omx_thread_attr_setstacksize(&attr, 1024 * 16);
            omx_base_component_Private->bufferMgmtThreadID = omx_thread_create(&omx_base_component_Private->bufferMgmtThread,
                    &attr,
                    omx_base_component_Private->BufferMgmtFunction,
                    openmaxStandComp);
            if(omx_base_component_Private->bufferMgmtThreadID != 0) {
                omx_debug("Starting buffer management thread failed\n");
                return OMX_ErrorUndefined;
            }
			snprintf(buf, sizeof(buf), "%sBufMF", omx_base_component_Private->name);
			omx_thread_setname_np(omx_base_component_Private->bufferMgmtThread, buf);
            break;
        case OMX_StateIdle:
            err = OMX_ErrorSameState;
            break;
        case OMX_StateExecuting:
            /*Flush Ports*/
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    omx_debug("Flushing Port %i\n",(int)i);
                    pPort = omx_base_component_Private->ports[i];
                    if(PORT_IS_ENABLED(pPort)) {
                        pPort->FlushProcessingBuffers(pPort);
                    }
                }
            }
            omx_base_component_Private->state = OMX_StateIdle;
            break;
        case OMX_StatePause:
            /*Flush Ports*/
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    omx_debug("Flushing Port %i\n",(int)i);
                    pPort = omx_base_component_Private->ports[i];
                    if(PORT_IS_ENABLED(pPort)) {
                        pPort->FlushProcessingBuffers(pPort);
                    }
                }
            }
            omx_base_component_Private->state = OMX_StateIdle;
            /*Signal buffer management thread if waiting at paused state*/
            omx_sem_signal(omx_base_component_Private->bStateSem);
            break;
        default:
            omx_debug("In %s: state transition not allowed\n", __func__);
            err = OMX_ErrorIncorrectStateTransition;
            break;
        }
        omx_debug("Out of %s for component %p with err %i\n", __func__, openmaxStandComp, err);
        return err;
    }

    if(destinationState == OMX_StatePause) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StatePause:
            err = OMX_ErrorSameState;
            break;
        case OMX_StateIdle:
            omx_base_component_Private->bIsEOSReached = OMX_FALSE;
        case OMX_StateExecuting:
            omx_base_component_Private->state = OMX_StatePause;
            break;
        default:
            omx_debug("In %s: state transition not allowed\n", __func__);
            err = OMX_ErrorIncorrectStateTransition;
            break;
        }
        omx_debug("Out of %s for component %p with err %i\n", __func__, openmaxStandComp, err);
        return err;
    }

    if(destinationState == OMX_StateExecuting) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StateIdle:
            omx_base_component_Private->state = OMX_StateExecuting;
            omx_base_component_Private->bIsEOSReached = OMX_FALSE;
            /*Send Tunneled Buffer to the Neighbouring Components*/
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    pPort = omx_base_component_Private->ports[i];
                    if (PORT_IS_TUNNELED(pPort) && PORT_IS_BUFFER_SUPPLIER(pPort) && PORT_IS_ENABLED(pPort)) {
                        for(k=0; k<pPort->nNumTunnelBuffer; k++) {
                            omx_sem_up(pPort->pBufferSem);
                            /*signal buffer management thread availability of buffers*/
                            omx_sem_up(omx_base_component_Private->bMgmtSem);
                        }
                    }
                }
            }
            omx_base_component_Private->transientState = OMX_TransStateMax;
            err = OMX_ErrorNone;
            break;
        case OMX_StatePause:
            omx_base_component_Private->state=OMX_StateExecuting;

            /* Tunneled Supplier Ports were enabled in paused state. So signal buffer managment thread*/
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {

                    pPort=omx_base_component_Private->ports[i];
                    omx_debug("In %s: state transition Paused 2 Executing, nelem=%d,semval=%d,Buf Count Actual=%d\n", __func__,
                          pPort->pBufferQueue->nelem,pPort->pBufferSem->semval,(int)pPort->sPortParam.nBufferCountActual);

                    if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(pPort) &&
                            (pPort->pBufferQueue->nelem == (int)(pPort->pBufferSem->semval + pPort->sPortParam.nBufferCountActual))) {
                        for(k=0; k < pPort->sPortParam.nBufferCountActual; k++) {
                            omx_sem_up(pPort->pBufferSem);
                            omx_sem_up(omx_base_component_Private->bMgmtSem);
                        }
                    }
                }
            }
            /*Signal buffer management thread if waiting at paused state*/
            omx_sem_signal(omx_base_component_Private->bStateSem);
            break;
        case OMX_StateExecuting:
            err = OMX_ErrorSameState;
            break;
        default:
            omx_debug("In %s: state transition not allowed\n", __func__);
            err = OMX_ErrorIncorrectStateTransition;
            break;
        }
        omx_debug("Out of %s for component %p with err %i\n", __func__, openmaxStandComp, err);
        return err;
    }

    if(destinationState == OMX_StateInvalid) {
        switch(omx_base_component_Private->state) {
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        default:
            omx_base_component_Private->state = OMX_StateInvalid;

            if(omx_base_component_Private->bufferMgmtThreadID == 0 ) {
                omx_sem_signal(omx_base_component_Private->bStateSem);
                /*Signal Buffer Management Thread to Exit*/
                omx_sem_up(omx_base_component_Private->bMgmtSem);
                omx_thread_join(omx_base_component_Private->bufferMgmtThread, NULL);
                omx_base_component_Private->bufferMgmtThreadID = -1;
                if(err!=0) {
                    omx_debug("In %s omx_thread_join returned err=%d\n",__func__,err);
                }
            }
            err = OMX_ErrorInvalidState;
            break;
        }
        omx_debug("Out of %s for component %p with err %i\n", __func__, openmaxStandComp, err);
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

/** @brief Checks the header of a structure for consistency
 * with size and spec version
 *
 * @param header Pointer to the structure to be checked
 * @param size Size of the structure. it is in general obtained
 * with a sizeof call applied to the structure
 *
 * @return OMX error code. If the header has failed the check,
 * OMX_ErrorVersionMismatch is returned.
 * If the header fails the size check OMX_ErrorBadParameter is returned
 */
OSCL_EXPORT_REF OMX_ERRORTYPE checkHeader(OMX_PTR header, OMX_U32 size) {
    OMX_VERSIONTYPE* ver;
    if (header == NULL) {
        omx_debug("In %s the header is null\n",__func__);
        return OMX_ErrorBadParameter;
    }
    ver = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if(*((OMX_U32*)header) != size) {
        omx_debug("In %s the header has a wrong size %i should be %i\n",__func__,(int)*((OMX_U32*)header),(int)size);
        return OMX_ErrorBadParameter;
    }
    if(ver->s.nVersionMajor != SPECVERSIONMAJOR ||
            ver->s.nVersionMinor != SPECVERSIONMINOR) {
        omx_debug("The version does not match\n");
        return OMX_ErrorVersionMismatch;
    }
    return OMX_ErrorNone;
}

/** @brief Simply fills the first two fields in any OMX structure
 * with the size and the version
 *
 * @param header pointer to the structure to be filled
 * @param size size of the structure. It can be obtained with
 * a call to sizeof of the structure type
 */
void setHeader(OMX_PTR header, OMX_U32 size) {
    OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    *((OMX_U32*)header) = size;

    ver->s.nVersionMajor = SPECVERSIONMAJOR;
    ver->s.nVersionMinor = SPECVERSIONMINOR;
    ver->s.nRevision = SPECREVISION;
    ver->s.nStep = SPECSTEP;
}

/**
 * This function verify Component State and Structure header
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_ParameterSanityCheck(OMX_HANDLETYPE hComponent,
        OMX_U32 nPortIndex,
        OMX_PTR pStructure,
        size_t size) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
    omx_base_PortType *pPort;
    int nNumPorts;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);
    nNumPorts = omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts;

    if (nPortIndex >= (OMX_U32)nNumPorts) {
        omx_debug("Bad Port index %i when the component has %i ports\n", (int)nPortIndex, (int)nNumPorts);
        return OMX_ErrorBadPortIndex;
    }

    pPort = omx_base_component_Private->ports[nPortIndex];

    /* if (omx_base_component_Private->state != OMX_StateLoaded && omx_base_component_Private->state != OMX_StateWaitForResources) {
       if(PORT_IS_ENABLED(pPort) && !pPort->bIsTransientToEnabled) {
         DEBUG(DEB_LEV_ERR, "In %s Incorrect State=%x lineno=%d\n",__func__,omx_base_component_Private->state,__LINE__);
         return OMX_ErrorIncorrectStateOperation;
       }
     }*/

    err = checkHeader(pStructure, size);
    if (err != OMX_ErrorNone) {
        omx_debug("In %s failing the checkHeader with err %i\n", __func__, (int)err);
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}


/** @brief Enumerates all the roles of the component.
 *
 * This function is intended to be used only by a core. The ST static core
 * in any case does not use this function, because it can not be used before the
 * creation of the component, but uses a static list.
 * It is implemented only for API completion,and it will be not overriden
 * by a derived component
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_ComponentRoleEnum(
    OMX_HANDLETYPE hComponent,
    OMX_U8 *cRole,
    OMX_U32 nIndex) {
    strcat((char*)cRole, "\0");
    return OMX_ErrorNoMore;
}

/** @brief standard OpenMAX function
 *
 * it sets the callback functions given by the IL client.
 * See OMX_Component.h
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_SetCallbacks(
    OMX_HANDLETYPE hComponent,
    OMX_CALLBACKTYPE* pCallbacks,
    OMX_PTR pAppData) {

    OMX_COMPONENTTYPE *omxcomponent = (OMX_COMPONENTTYPE*)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxcomponent->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_U32 i,j;

    omx_debug("In %s for component %p priv %p\n", __func__, hComponent, omx_base_component_Private);
    omx_base_component_Private->callbacks = pCallbacks;
    omx_base_component_Private->callbackData = pAppData;

    /* for all ports */
    for(j = 0; j < NUM_DOMAINS; j++) {

		omx_debug("In %s StartPortNumber %ld port %p\n", __func__, omx_base_component_Private->sPortTypesParam[j].nStartPortNumber,\
			&omx_base_component_Private->sPortTypesParam[j].nPorts);

        for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {

			omx_debug("In %s ports %p\n", __func__, omx_base_component_Private->ports[i]);
            pPort = omx_base_component_Private->ports[i];
            if (pPort->sPortParam.eDir == OMX_DirInput) {
                pPort->BufferProcessedCallback = omx_base_component_Private->callbacks->EmptyBufferDone;
            } else {
                pPort->BufferProcessedCallback = omx_base_component_Private->callbacks->FillBufferDone;
            }
        }
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}

/** @brief Part of the standard OpenMAX function
 *
 * This function return the parameters not related to any port.
 * These parameters are handled in the derived components
 * See OMX_Core.h for standard reference
 *
 * @return OMX_ErrorUnsupportedIndex if the index is not supported or not handled here
 */
OSCL_EXPORT_REF OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_GetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure) {

    OMX_COMPONENTTYPE *omxcomponent = (OMX_COMPONENTTYPE*)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxcomponent->pComponentPrivate;
    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier;
    omx_base_PortType *pPort;
    OMX_PORT_PARAM_TYPE* pPortDomains;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_VENDOR_PROP_TUNNELSETUPTYPE *pPropTunnelSetup;
   // OMX_PARAM_BELLAGIOTHREADS_ID *threadID;

	omx_debug("In %s for component %p\n", __func__, hComponent);
    omx_debug("Getting parameter %i\n", nParamIndex);
    if (ComponentParameterStructure == NULL) {
        return OMX_ErrorBadParameter;
    }
    switch(nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
        pPortDomains = (OMX_PORT_PARAM_TYPE*)ComponentParameterStructure;
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
            break;
        }
        pPortDomains->nPorts = 0;
        pPortDomains->nStartPortNumber = 0;
        break;
    case OMX_IndexParamPortDefinition:
        pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_PORTDEFINITIONTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (pPortDef->nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
            return OMX_ErrorBadPortIndex;
        }

        memcpy(pPortDef, &omx_base_component_Private->ports[pPortDef->nPortIndex]->sPortParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        break;
    case OMX_IndexParamPriorityMgmt:
        pPrioMgmt = (OMX_PRIORITYMGMTTYPE*)ComponentParameterStructure;
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PRIORITYMGMTTYPE))) != OMX_ErrorNone) {
            break;
        }
        pPrioMgmt->nGroupPriority = omx_base_component_Private->nGroupPriority;
        pPrioMgmt->nGroupID = omx_base_component_Private->nGroupID;
        break;
    case OMX_IndexParamCompBufferSupplier:
        pBufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE*)ComponentParameterStructure;
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE))) != OMX_ErrorNone) {
            break;
        }
        if (pBufferSupplier->nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                            omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                            omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                            omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
            return OMX_ErrorBadPortIndex;
        }

        pPort = omx_base_component_Private->ports[pBufferSupplier->nPortIndex];

        if (pPort->sPortParam.eDir == OMX_DirInput) {
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
            } else if (PORT_IS_TUNNELED(pPort)) {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
            } else {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
            }
        } else {
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyOutput;
            } else if (PORT_IS_TUNNELED(pPort)) {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyInput;
            } else {
                pBufferSupplier->eBufferSupplier = OMX_BufferSupplyUnspecified;
            }
        }
        break;
    default:

        if (OMX_IndexVendorCompPropTunnelFlags == (int)nParamIndex) {

            pPropTunnelSetup = (OMX_VENDOR_PROP_TUNNELSETUPTYPE*)ComponentParameterStructure;

            if (pPropTunnelSetup->nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                                 omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                                 omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                                 omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {

                omx_debug("In %s OMX_IndexVendorCompPropTunnelFlags nPortIndex=%d Line=%d \n",
                      __func__,(int)pPropTunnelSetup->nPortIndex,__LINE__);

                return OMX_ErrorBadPortIndex;
            }

            pPort = omx_base_component_Private->ports[pPropTunnelSetup->nPortIndex];

            pPropTunnelSetup->nTunnelSetup.nTunnelFlags  = pPort->nTunnelFlags;
            pPropTunnelSetup->nTunnelSetup.eSupplier     = pPort->eBufferSupplier;
        }
		/*
        else if (OMX_IndexParameterThreadsID == (int)nParamIndex) {

            if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_BELLAGIOTHREADS_ID))) != OMX_ErrorNone) {
                return err;
            }
            threadID = (OMX_PARAM_BELLAGIOTHREADS_ID *)ComponentParameterStructure;
            threadID->nThreadBufferMngtID = omx_base_component_Private->bellagioThreads->nThreadBufferMngtID;
            threadID->nThreadMessageID = omx_base_component_Private->bellagioThreads->nThreadMessageID;
        }*/
        else {
            err = OMX_ErrorUnsupportedIndex;
        }
        break;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return err;
}

/** @brief Part of the standard OpenMAX function
 *
 * This function sets the parameters not related to any port.
 * These parameters are handled in the derived components
 * See OMX_Core.h for standard reference
 *
 * @return OMX_ErrorUnsupportedIndex if the index is not supported or not handled here
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_SetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure) {

    OMX_PRIORITYMGMTTYPE* pPrioMgmt;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *omxcomponent = (OMX_COMPONENTTYPE*)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxcomponent->pComponentPrivate;
    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplier;
    omx_base_PortType *pPort;

    omx_debug("In %s for component %p\n", __func__, hComponent);
    omx_debug("Setting parameter %x\n", nParamIndex);
    if (ComponentParameterStructure == NULL) {
        omx_debug("In %s parameter provided is null! err = %x\n", __func__, err);
        return OMX_ErrorBadParameter;
    }

    switch(nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
        /* pPortParam  = (OMX_PORT_PARAM_TYPE* ) ComponentParameterStructure;*/
        if (omx_base_component_Private->state != OMX_StateLoaded &&
                omx_base_component_Private->state != OMX_StateWaitForResources) {
            return OMX_ErrorIncorrectStateOperation;
        }
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
            break;
        }
        err = OMX_ErrorUndefined;
        break;
    case OMX_IndexParamPortDefinition:
        pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
        err = omx_base_component_ParameterSanityCheck(hComponent, pPortDef->nPortIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if(err!=OMX_ErrorNone) {
            omx_debug("In %s Parameter Check Error=%x\n",__func__,err);
            break;
        }
        {
            OMX_PARAM_PORTDEFINITIONTYPE *pPortParam;
            OMX_U32 j,old_nBufferCountActual=0;
            pPortParam = &omx_base_component_Private->ports[pPortDef->nPortIndex]->sPortParam;
            if(pPortDef->nBufferCountActual < pPortParam->nBufferCountMin) {
                omx_debug("In %s port[%d]nBufferCountActual of param (%d) is < of nBufferCountMin of port(%d)\n", \
					__func__, (int)pPortDef->nPortIndex,  (int)pPortDef->nBufferCountActual, (int)pPortParam->nBufferCountMin);
                err = OMX_ErrorBadParameter;
                break;
            }
            old_nBufferCountActual         = pPortParam->nBufferCountActual;
            pPortParam->nBufferCountActual = pPortDef->nBufferCountActual;
            pPortParam->nBufferSize 	   = pPortDef->nBufferSize;

            switch(pPortDef->eDomain) {
            case OMX_PortDomainAudio:
                memcpy(&pPortParam->format.audio, &pPortDef->format.audio, sizeof(OMX_AUDIO_PORTDEFINITIONTYPE));
                break;
            case OMX_PortDomainVideo:
                pPortParam->format.video.pNativeRender          = pPortDef->format.video.pNativeRender;
                pPortParam->format.video.nFrameWidth            = pPortDef->format.video.nFrameWidth;
                pPortParam->format.video.nFrameHeight           = pPortDef->format.video.nFrameHeight;
                pPortParam->format.video.nStride                = pPortDef->format.video.nStride;
                pPortParam->format.video.nSliceHeight                = pPortDef->format.video.nSliceHeight;
                pPortParam->format.video.xFramerate             = pPortDef->format.video.xFramerate;
                pPortParam->format.video.bFlagErrorConcealment  = pPortDef->format.video.bFlagErrorConcealment;
                pPortParam->format.video.eCompressionFormat     = pPortDef->format.video.eCompressionFormat;
                pPortParam->format.video.eColorFormat           = pPortDef->format.video.eColorFormat;
                pPortParam->format.video.pNativeWindow          = pPortDef->format.video.pNativeWindow;
                break;
            case OMX_PortDomainImage:
                pPortParam->format.image.nFrameWidth            = pPortDef->format.image.nFrameWidth;
                pPortParam->format.image.nFrameHeight           = pPortDef->format.image.nFrameHeight;
                pPortParam->format.image.nStride                = pPortDef->format.image.nStride;
                pPortParam->format.image.bFlagErrorConcealment  = pPortDef->format.image.bFlagErrorConcealment;
                pPortParam->format.image.eCompressionFormat     = pPortDef->format.image.eCompressionFormat;
                pPortParam->format.image.eColorFormat           = pPortDef->format.image.eColorFormat;
                pPortParam->format.image.pNativeWindow          = pPortDef->format.image.pNativeWindow;
                break;
            case OMX_PortDomainOther:
                memcpy(&pPortParam->format.other, &pPortDef->format.other, sizeof(OMX_OTHER_PORTDEFINITIONTYPE));
                break;
            default:
                omx_debug("In %s wrong port domain. Out of OpenMAX scope\n",__func__);
                err = OMX_ErrorBadParameter;
                break;
            }


            //DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s re-alloc? nBufferCountActual=%d, old_nBufferCountActual=%d, state=0x%x\n", __func__, (int)pPortParam->nBufferCountActual, (int)old_nBufferCountActual, (int)omx_base_component_Private->state );

            /*If component state Idle/Pause/Executing and re-alloc the following private variables */
            if ((omx_base_component_Private->state == OMX_StateIdle ||
                    omx_base_component_Private->state == OMX_StatePause  ||
                    omx_base_component_Private->state == OMX_StateExecuting ||
                    omx_base_component_Private->state == OMX_StateLoaded) &&
                    (pPortParam->nBufferCountActual > old_nBufferCountActual)) {
                // todo check if here it is not better != instead of >
                pPort = omx_base_component_Private->ports[pPortDef->nPortIndex];
                if(pPort->pInternalBufferStorage) {
                    omx_free(pPort->pInternalBufferStorage);
                    pPort->pInternalBufferStorage = NULL;

                    pPort->pInternalBufferStorage = omx_alloc(pPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
                    if (!pPort->pInternalBufferStorage) {
                        omx_debug("Insufficient memory in %s\n", __func__);
                        err = OMX_ErrorInsufficientResources;
                        break;
                    }
                    memset(pPort->pInternalBufferStorage, 0x00, pPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
                    omx_debug("In %s realloc pInternalBufferStorage for omx_alloc nBufferCountActual=%d\n", __func__, (int)pPort->sPortParam.nBufferCountActual);
                }

                if(pPort->bBufferStateAllocated) {
                    omx_free(pPort->bBufferStateAllocated);
                    pPort->bBufferStateAllocated = NULL;
                    pPort->bBufferStateAllocated = omx_alloc(pPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
                    if (!pPort->bBufferStateAllocated) {
                        omx_debug("Insufficient memory in %s\n", __func__);
                        err = OMX_ErrorInsufficientResources;
                        break;
                    }
                    memset(pPort->bBufferStateAllocated, 0x00, pPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
                    for(j=0; j < pPort->sPortParam.nBufferCountActual; j++) {
                        pPort->bBufferStateAllocated[j] = BUFFER_FREE;
                    }
                    omx_debug("In %s realloc bBufferStateAllocated for omx_alloc nBufferCountActual=%d\n", __func__, (int)pPort->sPortParam.nBufferCountActual);
                }
            }
        }
        break;
    case OMX_IndexParamPriorityMgmt:
        if (omx_base_component_Private->state != OMX_StateLoaded &&
                omx_base_component_Private->state != OMX_StateWaitForResources) {
            return OMX_ErrorIncorrectStateOperation;
        }
        pPrioMgmt = (OMX_PRIORITYMGMTTYPE*)ComponentParameterStructure;
        if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PRIORITYMGMTTYPE))) != OMX_ErrorNone) {
            break;
        }
        omx_base_component_Private->nGroupPriority = pPrioMgmt->nGroupPriority;
        omx_base_component_Private->nGroupID = pPrioMgmt->nGroupID;
        break;
    case OMX_IndexParamCompBufferSupplier:
        pBufferSupplier = (OMX_PARAM_BUFFERSUPPLIERTYPE*)ComponentParameterStructure;

        omx_debug("In %s Buf Sup Port index=%d\n", __func__,(int)pBufferSupplier->nPortIndex);

        if(pBufferSupplier == NULL) {
            omx_debug("In %s pBufferSupplier is null!\n",__func__);
            return OMX_ErrorBadParameter;
        }
        if(pBufferSupplier->nPortIndex > (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                          omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                          omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                          omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
            return OMX_ErrorBadPortIndex;
        }
        err = omx_base_component_ParameterSanityCheck(hComponent, pBufferSupplier->nPortIndex, pBufferSupplier, sizeof(OMX_PARAM_BUFFERSUPPLIERTYPE));
        if(err==OMX_ErrorIncorrectStateOperation) {
            if (PORT_IS_ENABLED(omx_base_component_Private->ports[pBufferSupplier->nPortIndex])) {
                omx_debug("In %s Incorrect State=%x\n",__func__,omx_base_component_Private->state);
                return OMX_ErrorIncorrectStateOperation;
            }
        } else if (err != OMX_ErrorNone) {
            break;
        }

        if (pBufferSupplier->eBufferSupplier == OMX_BufferSupplyUnspecified) {
            omx_debug("In %s: port is already buffer supplier unspecified\n", __func__);
            return OMX_ErrorNone;
        }
        if ((PORT_IS_TUNNELED(omx_base_component_Private->ports[pBufferSupplier->nPortIndex])) == 0) {
            return OMX_ErrorNone;
        }

        pPort = omx_base_component_Private->ports[pBufferSupplier->nPortIndex];

        if ((pBufferSupplier->eBufferSupplier == OMX_BufferSupplyInput) &&
                (pPort->sPortParam.eDir == OMX_DirInput)) {
            /** These two cases regard the first stage of client override */
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                err = OMX_ErrorNone;
            }
            pPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
            pBufferSupplier->nPortIndex = pPort->nTunneledPort;
            err = OMX_SetParameter(pPort->hTunneledComponent, OMX_IndexParamCompBufferSupplier, pBufferSupplier);
        } else if ((pBufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) &&
                   (pPort->sPortParam.eDir == OMX_DirInput)) {
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                pPort->nTunnelFlags &= ~TUNNEL_IS_SUPPLIER;
                pBufferSupplier->nPortIndex = pPort->nTunneledPort;
                err = OMX_SetParameter(pPort->hTunneledComponent, OMX_IndexParamCompBufferSupplier, pBufferSupplier);
            }
            err = OMX_ErrorNone;
        } else if ((pBufferSupplier->eBufferSupplier == OMX_BufferSupplyOutput) &&
                   (pPort->sPortParam.eDir == OMX_DirOutput)) {
            /** these two cases regard the second stage of client override */
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                err = OMX_ErrorNone;
            }
            pPort->nTunnelFlags |= TUNNEL_IS_SUPPLIER;
        } else {
            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                pPort->nTunnelFlags &= ~TUNNEL_IS_SUPPLIER;
                err = OMX_ErrorNone;
            }
            err = OMX_ErrorNone;
        }
        omx_debug("In %s port %d Tunnel flag=%x \n", __func__,(int)pBufferSupplier->nPortIndex, (int)pPort->nTunnelFlags);
        break;
    default:
        err = OMX_ErrorUnsupportedIndex;
        break;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return err;
}

/** @brief base GetConfig function
 *
 * This base function is not implemented. If a derived component
 * needs to support any config, it must implement a derived
 * version of this function and assign it to the correct pointer
 * in the private component descriptor
 */
OSCL_EXPORT_REF OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure) {
    return OMX_ErrorNone;
}

/** @brief base SetConfig function
 *
 * This base function is not implemented. If a derived component
 * needs to support any config, it must implement a derived
 * version of this function and assign it to the correct pointer
 * in the private component descriptor
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure) {
    return OMX_ErrorNone;
}

/** @return the state of the component
 *
 * This function does not need any override by derived components
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_GetState(
    OMX_HANDLETYPE hComponent,
    OMX_STATETYPE* pState) {
    OMX_COMPONENTTYPE *omxcomponent = (OMX_COMPONENTTYPE*)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxcomponent->pComponentPrivate;
    omx_debug("In %s for component %p\n", __func__, hComponent);
    *pState = omx_base_component_Private->state;
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}

/** @brief standard SendCommand function
 *
 * In general this function does not need a overwrite, but
 * a special derived component could do it.
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_SendCommand(
    OMX_HANDLETYPE hComponent,
    OMX_COMMANDTYPE Cmd,
    OMX_U32 nParam,
    OMX_PTR pCmdData) {
    OMX_COMPONENTTYPE* omxComponent = (OMX_COMPONENTTYPE*)hComponent;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
    internalRequestMessageType *message;
    queue_t* messageQueue;
    omx_sem_t* messageSem;
    OMX_U32 i,j,k;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    int errQue;
    omx_debug("In %s for component %p\n", __func__, hComponent);

    messageQueue = omx_base_component_Private->messageQueue;
    messageSem = omx_base_component_Private->messageSem;

    if (omx_base_component_Private->state == OMX_StateInvalid) {
        return OMX_ErrorInvalidState;
    }

    message = omx_alloc(sizeof(internalRequestMessageType));
    if (!message) {
        omx_debug("Insufficient memory in %s\n", __func__);
        return OMX_ErrorInsufficientResources;
    }
    memset(message, 0x00, sizeof(internalRequestMessageType));

    message->messageParam = nParam;
    message->pCmdData=pCmdData;
    /** Fill in the message */
    switch (Cmd) {
    case OMX_CommandStateSet:
        message->messageType = OMX_CommandStateSet;
        if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StateLoaded)) {
            /*Allocate Internal Buffer Storage and Buffer Allocation State flags*/
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {

                    pPort = omx_base_component_Private->ports[i];

                    if(pPort->pInternalBufferStorage == NULL) {
                        pPort->pInternalBufferStorage = omx_alloc(pPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
                        if (!pPort->pInternalBufferStorage) {
                            omx_debug("Insufficient memory in %s\n", __func__);
                            return OMX_ErrorInsufficientResources;
                        }
                        memset(pPort->pInternalBufferStorage, 0x00, pPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
                    }

                    if(pPort->bBufferStateAllocated == NULL) {
                        pPort->bBufferStateAllocated = omx_alloc(pPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
                        if (!pPort->bBufferStateAllocated) {
                            omx_debug("Insufficient memory in %s\n", __func__);
                            return OMX_ErrorInsufficientResources;
                        }
                        memset(pPort->bBufferStateAllocated, 0x00, pPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
                    }

                    for(k=0; k < pPort->sPortParam.nBufferCountActual; k++) {
                        pPort->bBufferStateAllocated[k] = BUFFER_FREE;
                    }
                }
            }

            omx_base_component_Private->transientState = OMX_TransStateLoadedToIdle;
        } else if ((nParam == OMX_StateLoaded) && (omx_base_component_Private->state == OMX_StateIdle)) {
            omx_base_component_Private->transientState = OMX_TransStateIdleToLoaded;
        } else if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StateExecuting)) {
            omx_base_component_Private->transientState = OMX_TransStateExecutingToIdle;
        } else if ((nParam == OMX_StateIdle) && (omx_base_component_Private->state == OMX_StatePause)) {
            omx_base_component_Private->transientState = OMX_TransStatePauseToIdle;
        }
        break;
    case OMX_CommandFlush:
        if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) {
            return OMX_ErrorBadPortIndex;
        }
        message->messageType = OMX_CommandFlush;
        break;
    case OMX_CommandPortDisable:
        if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) {
            return OMX_ErrorBadPortIndex;
        }
        message->messageType = OMX_CommandPortDisable;
        if(message->messageParam == (int)OMX_ALL) {
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    omx_base_component_Private->ports[i]->bIsTransientToDisabled = OMX_TRUE;
                }
            }
        } else {
            omx_base_component_Private->ports[message->messageParam]->bIsTransientToDisabled = OMX_TRUE;
        }
        break;
    case OMX_CommandPortEnable:
        if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) {
            return OMX_ErrorBadPortIndex;
        }
        message->messageType = OMX_CommandPortEnable;
        if(message->messageParam == (int)(int)OMX_ALL) {
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    omx_base_component_Private->ports[i]->bIsTransientToEnabled = OMX_TRUE;
                }
            }
        } else {
            omx_base_component_Private->ports[message->messageParam]->bIsTransientToEnabled = OMX_TRUE;
        }


        break;
    case OMX_CommandMarkBuffer:
        if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) {
            return OMX_ErrorBadPortIndex;
        }
        message->messageType = OMX_CommandMarkBuffer;
        break;
    default:
        err = OMX_ErrorUnsupportedIndex;
        break;
    }

    if (err == OMX_ErrorNone) {
        errQue = queue(messageQueue, message);
        if (errQue) {
            /* /TODO the queue is full. This can be handled in a fine way with
             * some retrials, or other checking. For the moment this is a critical error
             * and simply causes the failure of this call
             */
            return OMX_ErrorInsufficientResources;
        }
        omx_sem_up(messageSem);
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return err;
}

/** @brief Component's message handler thread function
 *
 * Handles all messages coming from components and
 * processes them by dispatching them back to the
 * triggering component.
 */
void* compMessageHandlerFunction(void* param) {
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)param;
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
    internalRequestMessageType *message;
/*
    omx_debug("In %s for component %p\n", __func__, openmaxStandComp);
    omx_base_component_Private->bellagioThreads->nThreadMessageID = (long int)syscall(__NR_gettid);
    omx_debug("In %s the thread ID is %i\n", __func__, (int)omx_base_component_Private->bellagioThreads->nThreadMessageID);
*/
    while(1) {
        /* Wait for an incoming message */
        if (omx_base_component_Private == NULL) {
            break;
        }

        omx_debug("omx_base_component_Private->messageSem->num : %d",omx_base_component_Private->messageSem->semval);
        omx_sem_down(omx_base_component_Private->messageSem);
        omx_debug("In %s new message\n", __func__);
        /*Destructor has been called. So exit from the loop*/
        if(omx_base_component_Private->state == OMX_StateInvalid) {
            omx_debug("In %s Destructor has been called. So exit from the loop\n", __func__);
            break;
        }
        /* Dequeue it */
        omx_debug("/* Dequeue it */\n");
        message = dequeue(omx_base_component_Private->messageQueue);
        if(message == NULL) {
            omx_debug("In %s: ouch!! had null message!\n", __func__);
            break;
        }
        /* Process it by calling component's message handler method */
        omx_debug("omx_base_component_Private->messageHandler(openmaxStandComp, message)\n");
        omx_base_component_Private->messageHandler(openmaxStandComp, message);
        /* Message ownership has been transferred to us
        * so we gonna free it when finished.
        */
        omx_debug("free Message\n");
        if (message) {
            omx_free(message);
            message = NULL;
        }
    }
	omx_debug("Out of %s for component %p\n", __func__, openmaxStandComp);
    return NULL;
}

/** This is called by the component message entry point.
 * In this base version this function is named compMessageHandlerFunction
 *
 * A request is made by the component when some asynchronous services are needed:
 * 1) A SendCommand() is to be processed
 * 2) An error needs to be notified
 * 3) ...
 *
 * @param openmaxStandComp the component itself
 * @param message the message that has been passed to core
 */
OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_MessageHandler(OMX_COMPONENTTYPE *openmaxStandComp,internalRequestMessageType* message) {
    omx_base_component_PrivateType* omx_base_component_Private=openmaxStandComp->pComponentPrivate;
    OMX_U32                         i,j,k;
    OMX_ERRORTYPE                   err = OMX_ErrorNone;
    omx_base_PortType*              pPort;

    omx_debug("In %s for component %p with message %i\n", __func__, openmaxStandComp, message->messageType);

    /* Dealing with a SendCommand call.
    * -messageType contains the command to execute
    * -messageParam contains the parameter of the command
    *  (destination state in case of a state change command).
    */
    switch(message->messageType) {
    case OMX_CommandStateSet: {
        /* Do the actual state change */
        err = (*(omx_base_component_Private->DoStateSet))(openmaxStandComp, message->messageParam);
        if (err != OMX_ErrorNone) {
            (*(omx_base_component_Private->callbacks->EventHandler))
            (openmaxStandComp,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             err, /* The commands was a OMX_CommandStateSet */
             0, /* The state has been changed in message->messageParam */
             NULL);
        } else {
            /* And run the callback */
            if (omx_base_component_Private->callbacks) {
                omx_debug("running callback in %s\n", __func__);
                (*(omx_base_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_base_component_Private->callbackData,
                 OMX_EventCmdComplete, /* The command was completed */
                 OMX_CommandStateSet, /* The commands was a OMX_CommandStateSet */
                 message->messageParam, /* The state has been changed in message->messageParam */
                 NULL);
            }
        }
    }
    break;
    case OMX_CommandFlush: {
        /*Flush port/s*/
        if(message->messageParam == (int)OMX_ALL) {
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    omx_base_component_Private->ports[i]->bIsPortFlushed = OMX_TRUE;
                }
            }
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    pPort=omx_base_component_Private->ports[i];
                    err = pPort->FlushProcessingBuffers(pPort);
                }
            }
        }
        else {
            pPort=omx_base_component_Private->ports[message->messageParam];
            err = pPort->FlushProcessingBuffers(pPort);
        }
        if (err != OMX_ErrorNone) {
            (*(omx_base_component_Private->callbacks->EventHandler))
            (openmaxStandComp,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             err, /* The commands was a OMX_CommandStateSet */
             0, /* The state has been changed in message->messageParam */
             NULL);
        } else {
            if(message->messageParam == (int)OMX_ALL) { /*Flush all port*/
                /* for all ports */
                for(j = 0; j < NUM_DOMAINS; j++) {
                    for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                            i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                            omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                        (*(omx_base_component_Private->callbacks->EventHandler))
                        (openmaxStandComp,
                         omx_base_component_Private->callbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
                         i, /* The state has been changed in message->messageParam */
                         NULL);

                        pPort=omx_base_component_Private->ports[i];
                        /* Signal the buffer Semaphore and the buffer managment semaphore, to restart the exchange of buffers after flush */
                        if (PORT_IS_TUNNELED(pPort) && PORT_IS_BUFFER_SUPPLIER(pPort)) {
                            for(k=0; k<pPort->nNumTunnelBuffer; k++) {
                                omx_sem_up(pPort->pBufferSem);
                                /*signal buffer management thread availability of buffers*/
                                omx_sem_up(omx_base_component_Private->bMgmtSem);
                            }
                        }
                    }
                }
            } else {/*Flush input/output port*/
                (*(omx_base_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_base_component_Private->callbackData,
                 OMX_EventCmdComplete, /* The command was completed */
                 OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
                 message->messageParam, /* The state has been changed in message->messageParam */
                 NULL);
                /* Signal the buffer Semaphore and the buffer managment semaphore, to restart the exchange of buffers after flush */
                if (PORT_IS_TUNNELED(omx_base_component_Private->ports[message->messageParam])
                        && PORT_IS_BUFFER_SUPPLIER(omx_base_component_Private->ports[message->messageParam])) {
                    for(j=0; j<omx_base_component_Private->ports[message->messageParam]->nNumTunnelBuffer; j++) {
                        omx_sem_up(omx_base_component_Private->ports[message->messageParam]->pBufferSem);
                        /*signal buffer management thread availability of buffers*/
                        omx_sem_up(omx_base_component_Private->bMgmtSem);
                    }
                }
            }
        }
    }
    break;
    case OMX_CommandPortDisable: {
        /*Flush port/s*/
        if(message->messageParam == (int)OMX_ALL) {
            /*If Component is not in loaded state,then First Flush all buffers then disable the port*/
            if(omx_base_component_Private->state!=OMX_StateLoaded) {
                /* for all ports */
                for(j = 0; j < NUM_DOMAINS; j++) {
                    for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                            i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                            omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                        pPort=omx_base_component_Private->ports[i];
                        err = pPort->FlushProcessingBuffers(pPort);
                    }
                }
            }
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    pPort=omx_base_component_Private->ports[i];
                    err = pPort->Port_DisablePort(pPort);
                }
            }
        }
        else {
            pPort=omx_base_component_Private->ports[message->messageParam];
            if(omx_base_component_Private->state!=OMX_StateLoaded) {
                err = pPort->FlushProcessingBuffers(pPort);
                omx_debug("In %s: Port Flush completed for Comp %s\n",__func__,omx_base_component_Private->name);
            }
            err = pPort->Port_DisablePort(pPort);
        }
        /** This condition is added to pass the tests, it is not significant for the environment */
        if (err != OMX_ErrorNone) {
            (*(omx_base_component_Private->callbacks->EventHandler))
            (openmaxStandComp,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             err, /* The commands was a OMX_CommandStateSet */
             0, /* The state has been changed in message->messageParam */
             NULL);
        } else {
            if(message->messageParam == (int)OMX_ALL) { /*Disable all ports*/
                /* for all ports */
                for(j = 0; j < NUM_DOMAINS; j++) {
                    for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                            i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                            omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                        (*(omx_base_component_Private->callbacks->EventHandler))
                        (openmaxStandComp,
                         omx_base_component_Private->callbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandPortDisable, /* The commands was a OMX_CommandStateSet */
                         i, /* The state has been changed in message->messageParam */
                         NULL);
                    }
                }
            } else {
                (*(omx_base_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_base_component_Private->callbackData,
                 OMX_EventCmdComplete, /* The command was completed */
                 OMX_CommandPortDisable, /* The commands was a OMX_CommandStateSet */
                 message->messageParam, /* The state has been changed in message->messageParam */
                 NULL);
            }
        }
    }
    break;
    case OMX_CommandPortEnable: {
        /*Flush port/s*/
        if(message->messageParam == (int)OMX_ALL) {
            /* for all ports */
            for(j = 0; j < NUM_DOMAINS; j++) {
                for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                        i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                        omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                    pPort=omx_base_component_Private->ports[i];
                    err = pPort->Port_EnablePort(pPort);
                }
            }
        } else {
            pPort=omx_base_component_Private->ports[message->messageParam];
            err = pPort->Port_EnablePort(pPort);
        }
        if (err != OMX_ErrorNone) {
            (*(omx_base_component_Private->callbacks->EventHandler))
            (openmaxStandComp,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             err, /* The commands was a OMX_CommandStateSet */
             0, /* The state has been changed in message->messageParam */
             NULL);
        } else {
            if(message->messageParam != (int)OMX_ALL) {
                (*(omx_base_component_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_base_component_Private->callbackData,
                 OMX_EventCmdComplete, /* The command was completed */
                 OMX_CommandPortEnable, /* The commands was a OMX_CommandStateSet */
                 message->messageParam, /* The state has been changed in message->messageParam */
                 NULL);

                if (omx_base_component_Private->state==OMX_StateExecuting) {
                    pPort=omx_base_component_Private->ports[message->messageParam];
                    if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                        for(i=0; i < pPort->sPortParam.nBufferCountActual; i++) {
                            omx_sem_up(pPort->pBufferSem);
                            omx_sem_up(omx_base_component_Private->bMgmtSem);
                        }
                    }
                }

            } else {
                /* for all ports */
                for(j = 0; j < NUM_DOMAINS; j++) {
                    for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                            i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                            omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                        (*(omx_base_component_Private->callbacks->EventHandler))
                        (openmaxStandComp,
                         omx_base_component_Private->callbackData,
                         OMX_EventCmdComplete, /* The command was completed */
                         OMX_CommandPortEnable, /* The commands was a OMX_CommandStateSet */
                         i, /* The state has been changed in message->messageParam */
                         NULL);
                    }
                }

                if (omx_base_component_Private->state==OMX_StateExecuting) {
                    /* for all ports */
                    for(j = 0; j < NUM_DOMAINS; j++) {
                        for(i = omx_base_component_Private->sPortTypesParam[j].nStartPortNumber;
                                i < omx_base_component_Private->sPortTypesParam[j].nStartPortNumber +
                                omx_base_component_Private->sPortTypesParam[j].nPorts; i++) {
                            pPort=omx_base_component_Private->ports[i];
                            if (PORT_IS_BUFFER_SUPPLIER(pPort)) {
                                for(k=0; k < pPort->sPortParam.nBufferCountActual; k++) {
                                    omx_sem_up(pPort->pBufferSem);
                                    omx_sem_up(omx_base_component_Private->bMgmtSem);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    break;
    case OMX_CommandMarkBuffer: {
        omx_base_component_Private->pMark.hMarkTargetComponent = ((OMX_MARKTYPE *)message->pCmdData)->hMarkTargetComponent;
        omx_base_component_Private->pMark.pMarkData            = ((OMX_MARKTYPE *)message->pCmdData)->pMarkData;
    }
    break;
    default:
        omx_debug("In %s: Unrecognized command %i\n", __func__, message->messageType);
        break;
    }
    omx_debug("Out of %s for component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_AllocateBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE** ppBuffer,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);

    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        omx_debug("In %s: wrong port index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    pPort = omx_base_component_Private->ports[nPortIndex];
    err = pPort->Port_AllocateBuffer(pPort, ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
        return err;
    }
    omx_debug("Out of %s for component %p buffer %p\n", __func__, hComponent, ppBuffer);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_UseBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8* pBuffer) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);
    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        omx_debug("In %s: wrong port index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    pPort = omx_base_component_Private->ports[nPortIndex];
    err = pPort->Port_UseBuffer(pPort, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}


OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_FreeBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_U32 nPortIndex,
    OMX_BUFFERHEADERTYPE* pBuffer) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);
    if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                       omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        omx_debug("In %s: wrong port index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }

    pPort = omx_base_component_Private->ports[nPortIndex];
    err = pPort->Port_FreeBuffer(pPort, nPortIndex, pBuffer);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_EmptyThisBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE* pBuffer) {
    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);

    if (pBuffer->nInputPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        omx_debug("In %s: wrong port index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    pPort = omx_base_component_Private->ports[pBuffer->nInputPortIndex];
    if (pPort->sPortParam.eDir != OMX_DirInput) {
        omx_debug("In %s: wrong port direction in Component %s\n", __func__,omx_base_component_Private->name);
        return OMX_ErrorBadPortIndex;
    }
    err = pPort->Port_SendBufferFunction(pPort, pBuffer);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %s\n", __func__, hComponent, OmxerrorName(err));
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_FillThisBuffer(
    OMX_HANDLETYPE hComponent,
    OMX_BUFFERHEADERTYPE* pBuffer) {

    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p pBuffer = %p\n", __func__, hComponent, pBuffer);
    if (pBuffer->nOutputPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                      omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                      omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                      omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        omx_debug("In %s: wrong port index\n", __func__);
        return OMX_ErrorBadPortIndex;
    }
    pPort = omx_base_component_Private->ports[pBuffer->nOutputPortIndex];
    if (pPort->sPortParam.eDir != OMX_DirOutput) {
        omx_debug("In %s: wrong port(%d) direction(%x) pBuffer=%p in Component %s\n", __func__,
              (int)pBuffer->nOutputPortIndex, (int)pPort->sPortParam.eDir, pBuffer, omx_base_component_Private->name);
        return OMX_ErrorBadPortIndex;
    }
    err = pPort->Port_SendBufferFunction(pPort,  pBuffer);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %s\n", __func__, hComponent, OmxerrorName(err));
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_component_ComponentTunnelRequest(
    OMX_HANDLETYPE hComponent,
    OMX_U32 nPort,
    OMX_HANDLETYPE hTunneledComp,
    OMX_U32 nTunneledPort,
    OMX_TUNNELSETUPTYPE* pTunnelSetup) {

    omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err;

    omx_debug("In %s for component %p\n", __func__, hComponent);
    if (nPort >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                  omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                  omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                  omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
        return OMX_ErrorBadPortIndex;
    }

    pPort = omx_base_component_Private->ports[nPort];
    err = pPort->ComponentTunnelRequest(pPort, hTunneledComp, nTunneledPort, pTunnelSetup);
    if (err != OMX_ErrorNone) {
        omx_debug("Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
        return err;
    }
    omx_debug("Out of %s for component %p\n", __func__, hComponent);
    return OMX_ErrorNone;
}


#ifdef __cplusplus
}
#endif

