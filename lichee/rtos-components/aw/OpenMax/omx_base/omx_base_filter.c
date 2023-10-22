/**
  src/base/omx_base_filter.c

  OpenMAX Base Filter component. This component does not perform any multimedia
  processing. It derives from base component and contains two ports. It can be used
  as a base class for codec and filter components.

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


#include <unistd.h>
#include <OmxCore.h>

#include "omx_base_filter.h"

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_filter_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName) {
    OMX_ERRORTYPE err;
    omx_base_filter_PrivateType* omx_base_filter_Private;

    omx_debug("In %s of component %p\n", __func__, openmaxStandComp);
    if (openmaxStandComp->pComponentPrivate) {
        omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
    } else {
        omx_base_filter_Private = omx_alloc(sizeof(omx_base_filter_PrivateType));
        if (!omx_base_filter_Private) {
            omx_debug("Insufficient memory in %s\n", __func__);
            return OMX_ErrorInsufficientResources;
        }
        memset(omx_base_filter_Private, 0x00, sizeof(omx_base_filter_PrivateType));

        openmaxStandComp->pComponentPrivate=omx_base_filter_Private;
    }

    /* Call the base class constructor */
    err = omx_base_component_Constructor(openmaxStandComp,cComponentName);
    if (err != OMX_ErrorNone) {
        omx_debug("The base constructor failed in %s\n", __func__);
        return err;
    }
    /* here we can override whatever defaults the base_component constructor set
    * e.g. we can override the function pointers in the private struct */
    omx_base_filter_Private = openmaxStandComp->pComponentPrivate;

    omx_base_filter_Private->BufferMgmtFunction = omx_base_filter_BufferMgmtFunction;

    omx_debug("Out of %s of component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE omx_base_filter_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {
    OMX_ERRORTYPE err;
    omx_debug("In %s of component %p\n", __func__, openmaxStandComp);
    err = omx_base_component_Destructor(openmaxStandComp);
    if (err != OMX_ErrorNone) {
        omx_debug("The base component destructor failed\n");
        return err;
    }
    //omx_debug("Out of %s of component %p\n", __func__, openmaxStandComp);
    return OMX_ErrorNone;
}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_base_filter_BufferMgmtFunction (void* param) {

    OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
    omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
    omx_base_PortType *pInPort=(omx_base_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    omx_base_PortType *pOutPort=(omx_base_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    omx_sem_t* pInputSem = pInPort->pBufferSem;
    omx_sem_t* pOutputSem = pOutPort->pBufferSem;
    queue_t* pInputQueue = pInPort->pBufferQueue;
    queue_t* pOutputQueue = pOutPort->pBufferQueue;
    OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
    OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
    OMX_BOOL isInputBufferNeeded=OMX_TRUE,isOutputBufferNeeded=OMX_TRUE;
    int inBufExchanged=0,outBufExchanged=0;

    /* checks if the component is in a state able to receive buffers */
    while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||  omx_base_filter_Private->state == OMX_StatePause ||
            omx_base_filter_Private->transientState == OMX_TransStateLoadedToIdle) {

        /*Wait till the ports are being flushed*/
        omx_thread_mutex_lock(&omx_base_filter_Private->flush_mutex);
        while( PORT_IS_BEING_FLUSHED(pInPort) ||
                PORT_IS_BEING_FLUSHED(pOutPort)) {
            omx_thread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

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

            omx_sem_up(omx_base_filter_Private->flush_all_condition);
            omx_sem_down(omx_base_filter_Private->flush_condition);
            omx_thread_mutex_lock(&omx_base_filter_Private->flush_mutex);
        }
        omx_thread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

        /*No buffer to process. So wait here*/
        if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
                (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid)) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_base_filter_Private->bMgmtSem);

        }
        if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
            omx_debug("In %s Buffer Management Thread is exiting\n",__func__);
            break;
        }
        if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
                (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid) &&
                !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
            omx_debug("Waiting for next input/output buffer\n");
            omx_sem_down(omx_base_filter_Private->bMgmtSem);

        }
        if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
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
                    omx_debug("Had NULL input buffer!!\n");
                    break;
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
                    break;
                }
            }
        }

        if(isInputBufferNeeded==OMX_FALSE) {
            if(pInputBuffer->hMarkTargetComponent != NULL) {
                if((OMX_COMPONENTTYPE*)pInputBuffer->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
                    /*Clear the mark and generate an event*/
                    (*(omx_base_filter_Private->callbacks->EventHandler))
                    (openmaxStandComp,
                     omx_base_filter_Private->callbackData,
                     OMX_EventMark, /* The command was completed */
                     1, /* The commands was a OMX_CommandStateSet */
                     0, /* The state has been changed in message->messageParam2 */
                     pInputBuffer->pMarkData);
                } else {
                    /*If this is not the target component then pass the mark*/
                    omx_base_filter_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
                    omx_base_filter_Private->pMark.pMarkData            = pInputBuffer->pMarkData;
                }
                pInputBuffer->hMarkTargetComponent = NULL;
            }
        }

        if(isInputBufferNeeded==OMX_FALSE && isOutputBufferNeeded==OMX_FALSE) {

            if(omx_base_filter_Private->pMark.hMarkTargetComponent != NULL) {
                pOutputBuffer->hMarkTargetComponent = omx_base_filter_Private->pMark.hMarkTargetComponent;
                pOutputBuffer->pMarkData            = omx_base_filter_Private->pMark.pMarkData;
                omx_base_filter_Private->pMark.hMarkTargetComponent = NULL;
                omx_base_filter_Private->pMark.pMarkData            = NULL;
            }

            pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
                omx_debug("Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags = pInputBuffer->nFlags;
                pInputBuffer->nFlags = 0;
            }

            if(omx_base_filter_Private->state == OMX_StateExecuting)  {
                if (omx_base_filter_Private->BufferMgmtCallback && pInputBuffer->nFilledLen > 0) {
                    (*(omx_base_filter_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, pOutputBuffer);
                } else {
                    /*It no buffer management call back the explicitly consume input buffer*/
                    pInputBuffer->nFilledLen = 0;
                }
            } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                omx_debug("In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_base_filter_Private->state);
            } else {
                pInputBuffer->nFilledLen = 0;
            }

            if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) {
                omx_debug("Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
                pOutputBuffer->nFlags=pInputBuffer->nFlags;
                pInputBuffer->nFlags=0;
                (*(omx_base_filter_Private->callbacks->EventHandler))
                (openmaxStandComp,
                 omx_base_filter_Private->callbackData,
                 OMX_EventBufferFlag, /* The command was completed */
                 1, /* The commands was a OMX_CommandStateSet */
                 pOutputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
                 NULL);
                omx_base_filter_Private->bIsEOSReached = OMX_TRUE;
            }
            if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
                /*Waiting at paused state*/
                omx_sem_wait(omx_base_filter_Private->bStateSem);
            }

            /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
            if((pOutputBuffer->nFilledLen != 0) || ((pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) || (omx_base_filter_Private->bIsEOSReached == OMX_TRUE)) {
                pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
                outBufExchanged--;
                pOutputBuffer=NULL;
                isOutputBufferNeeded=OMX_TRUE;
            }
        }

        if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
            /*Waiting at paused state*/
            omx_sem_wait(omx_base_filter_Private->bStateSem);
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
