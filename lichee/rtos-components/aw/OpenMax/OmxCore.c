/**
  src/omxcore.c

* OpenMAX Integration Layer Core. This library implements the OpenMAX core
* responsible for environment setup, components tunneling and communication.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <assert.h>
#include <console.h>

#include "OMX_Base.h"

#include <OMX_Core.h>
#include <OMX_ContentPipe.h>

#include "OmxCore.h"



static LIST_HEAD(g_omx_elem_list);
static int initialized;


static void print_omx_test(void)
{
	omx_info("OMX IL test");
	omx_debug("OMX IL test");
	omx_err("OMX IL test");
}

int g_omx_debug_mask = 0;

static int cmd_omx_debug(int argc, char *argv[])
{
	if (argc > 2)
		return -1;
	if (argc == 1) {
		printf("OMX IL debug:%d\n", g_omx_debug_mask);
		return 0;
	}

	g_omx_debug_mask = atoi(argv[1]);
	omx_info("set OMX IL debug:%d\n", g_omx_debug_mask);

	print_omx_test();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_omx_debug, omx_debug, omx debug);


struct omx_load {
	struct omx_load_component *omx_comp;
	void *private_data;
};

/*extern struct omx_load_component g_aenc_component;*/
void add_default_omx_component(void)
{
	struct omx_load_component *a = NULL;
	struct list_head *head = &g_omx_elem_list;

	omx_debug("");
#ifdef CONFIG_COMPONENTS_OMX_AUDIO_RECORD
	OMX_LOAD_COMPONENT_DEFAULT(g_arecord_component);
#endif
#ifdef CONFIG_COMPONENTS_OMX_AUDIO_RENDER
	OMX_LOAD_COMPONENT_DEFAULT(g_arender_component);
#endif
#ifdef CONFIG_COMPONENTS_OMX_AUDIO_ECHO_CANCEL
	OMX_LOAD_COMPONENT_DEFAULT(g_aechocancel_component);
#endif
#ifdef CONFIG_COMPONENTS_OMX_AUDIO_EQUALIZER
	OMX_LOAD_COMPONENT_DEFAULT(g_aequalizer_component);
#endif
#ifdef CONFIG_COMPONENTS_OMX_AUDIO_ASR
	OMX_LOAD_COMPONENT_DEFAULT(g_asr_component);
#endif
#ifdef CONFIG_COMPONENTS_OMX_DUMP
	OMX_LOAD_COMPONENT_DEFAULT(g_dump_component);
#endif

	OMX_MAYBE_UNUSED(a);
	OMX_MAYBE_UNUSED(head);
	return;
}

/** @brief The OMX_Init standard function
 *
 * This function calls the init function of each component loader added. 
 *
 * @return OMX_ErrorNone
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_Init() {

	OMX_ERRORTYPE err;

	omx_debug("In %s\n", __func__);

	if(initialized == 0) {
		initialized = 1;

		add_default_omx_component();

		struct omx_load_component *omx_comp = NULL;
		list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
			err = omx_comp->ops->InitComponentLoader(omx_comp->ops, omx_comp->name);
			if (err != OMX_ErrorNone) {
				omx_err("A Component loader constructor fails. Exiting\n");
				return OMX_ErrorInsufficientResources;
			}
		}
	}

    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorNone;
}

/** @brief The OMX_Deinit standard function
 *
 * In this function the Deinit function for each component loader is performed
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_Deinit() {

	OMX_ERRORTYPE err;
	omx_debug("In %s\n", __func__);

	if(initialized == 1) {
		struct omx_load_component *omx_comp = NULL;
		list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
			err = omx_comp->ops->DeInitComponentLoader(omx_comp->ops, omx_comp->name);
			if (err != OMX_ErrorNone) {
				omx_err("A Component unloader fails. Exiting\n");
				return OMX_ErrorInsufficientResources;
			}
		}
	}

	initialized = 0;

	omx_debug("Out of %s\n", __func__);
	return OMX_ErrorNone;
}


/** @brief the OMX_GetHandle standard function
 *
 * This function will scan inside any component loader to search for
 * the requested component. If there are more components with the same name
 * the first component is returned. The existence of multiple components with
 * the same name is not contemplated in OpenMAX specification. The assumption is
 * that this behavior is NOT allowed.
 *
 * @return OMX_ErrorNone if a component has been found
 *         OMX_ErrorComponentNotFound if the requested component has not been found
 *                                    in any loader
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE* pHandle,
        OMX_STRING cComponentName,
        OMX_PTR pAppData,
        OMX_CALLBACKTYPE* pCallBacks) {

    OMX_ERRORTYPE err = OMX_ErrorNone;

    omx_debug("In %s for %s\n", __func__, cComponentName);

	struct omx_load_component *omx_comp = NULL;
	list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
		err = omx_comp->ops->CreateComponent(
					omx_comp->ops,
					pHandle,
					cComponentName,
					pAppData,
					pCallBacks);
		if (err == OMX_ErrorNone) {
			// the component has been found
			return OMX_ErrorNone;
		}
	}
    /*Required to meet conformance test: do not remove*/
    if (err == OMX_ErrorInsufficientResources) {
        return OMX_ErrorInsufficientResources;
    }
    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorComponentNotFound;
}

/** @brief The OMX_FreeHandle standard function
 *
 * This function executes the BOSA_DestroyComponent of the component loaders
 *
 * @param hComponent the component handle to be freed
 *
 * @return The error of the BOSA_DestroyComponent function or OMX_ErrorNone
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent) {

    OMX_ERRORTYPE err;
    omx_debug("In %s for %p\n", __func__, hComponent);

	struct omx_load_component *omx_comp = NULL;
	list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
		err = omx_comp->ops->DestroyComponent(
					omx_comp->ops,
					hComponent);
		if (err == OMX_ErrorNone) {
			 // the component has been found and destroyed
			return OMX_ErrorNone;
		}
	}
    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorComponentNotFound;
}

/** @brief the OMX_ComponentNameEnum standard function
 *
 * This function build a complete list of names from all the loaders.
 * For each loader the index is from 0 to max, but this function must provide a single
 * list, with a common index. This implementation orders the loaders and the
 * related list of components.
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_ComponentNameEnum(
    OMX_STRING cComponentName,
    OMX_U32 nNameLength,
    OMX_U32 nIndex)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    int index = 0;
    int offset = 0;

    omx_debug("In %s\n", __func__);

	struct omx_load_component *omx_comp = NULL;
	list_for_each_entry(omx_comp, &g_omx_elem_list, list) {

		offset = 0;
		omx_debug("In %s loaders[%s]\n", __func__, omx_comp->name);
		while((err = omx_comp->ops->ComponentNameEnum(
					omx_comp->ops,
					cComponentName,
                     nNameLength,
                     offset)) != OMX_ErrorNoMore)
         
		if (index == (int)nIndex)
		{
			return err;
		}
		offset++;
		index++;

	}

   	omx_debug("Out of %s\n", __func__);
    return OMX_ErrorNoMore;
}

/** @brief the OMX_SetupTunnel standard function
 *
 * The implementation of this function is described in the OpenMAX spec
 *
 * @param hOutput component handler that controls the output port of the tunnel
 * @param nPortOutput index of the output port of the tunnel
 * @param hInput component handler that controls the input port of the tunnel
 * @param nPortInput index of the input port of the tunnel
 *
 * @return OMX_ErrorBadParameter, OMX_ErrorPortsNotCompatible, tunnel rejected by a component
 * or OMX_ErrorNone if the tunnel has been established
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_SetupTunnel(
    OMX_HANDLETYPE hOutput,
    OMX_U32 nPortOutput,
    OMX_HANDLETYPE hInput,
    OMX_U32 nPortInput) {

    OMX_ERRORTYPE err;
    OMX_COMPONENTTYPE* component;
    OMX_TUNNELSETUPTYPE* tunnelSetup;

    omx_debug("In %s the output port is:%p/%i, the input port is %p/%i\n",
          __func__, hOutput, (int)nPortOutput, hInput, (int)nPortInput);

    if (hOutput == NULL && hInput == NULL)
        return OMX_ErrorBadParameter;

    tunnelSetup = omx_alloc(sizeof(OMX_TUNNELSETUPTYPE));
    if (!tunnelSetup) {
        omx_debug("Insufficient memory in %s\n", __func__);
        return OMX_ErrorInsufficientResources;
    }

    component = (OMX_COMPONENTTYPE*)hOutput;
    tunnelSetup->nTunnelFlags = 0;
    tunnelSetup->eSupplier = OMX_BufferSupplyUnspecified;

    if (hOutput) {
        err = (component->ComponentTunnelRequest)(hOutput, nPortOutput, hInput, nPortInput, tunnelSetup);
        if (err != OMX_ErrorNone) {
            omx_debug("Tunneling failed: output port rejects it - err = %x\n", err);
            omx_free(tunnelSetup);
            tunnelSetup = NULL;
            return err;
        }
    }
    omx_debug("First stage of tunneling acheived:\n");
    omx_debug("       - supplier proposed = %i\n", tunnelSetup->eSupplier);
    omx_debug("       - flags             = %i\n", (int)tunnelSetup->nTunnelFlags);

    component = (OMX_COMPONENTTYPE*)hInput;
    if (hInput) {
        err = (component->ComponentTunnelRequest)(hInput, nPortInput, hOutput, nPortOutput, tunnelSetup);
        if (err != OMX_ErrorNone) {
            omx_debug("Tunneling failed: input port rejects it - err = %08x\n", err);
            // the second stage fails. the tunnel on poutput port has to be removed
            component = (OMX_COMPONENTTYPE*)hOutput;
            err = (component->ComponentTunnelRequest)(hOutput, nPortOutput, NULL, 0, tunnelSetup);
            if (err != OMX_ErrorNone) {
                // This error should never happen. It is critical, and not recoverable
                omx_free(tunnelSetup);
                tunnelSetup = NULL;
                omx_debug("Out of %s with OMX_ErrorUndefined\n", __func__);
                return OMX_ErrorUndefined;
            }
            omx_free(tunnelSetup);
            tunnelSetup = NULL;
            omx_debug("Out of %s with OMX_ErrorPortsNotCompatible\n", __func__);
            return OMX_ErrorPortsNotCompatible;
        }
    }
    omx_debug("Second stage of tunneling acheived:\n");
    omx_debug("       - supplier proposed = %i\n", (int)tunnelSetup->eSupplier);
    omx_debug("       - flags             = %i\n", (int)tunnelSetup->nTunnelFlags);
    omx_free(tunnelSetup);
    tunnelSetup = NULL;
    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorNone;
}

/** @brief the OMX_GetRolesOfComponent standard function
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_STRING CompName,
    OMX_U32 *pNumRoles,
    OMX_U8 **roles) {

    OMX_ERRORTYPE err = OMX_ErrorNone;

    omx_debug("In %s\n", __func__);

	struct omx_load_component *omx_comp = NULL;
	list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
		err = omx_comp->ops->GetRolesOfComponent(
					omx_comp->ops,
					CompName,
                  	pNumRoles,
                  	roles);
		if (err == OMX_ErrorNone) {
			return OMX_ErrorNone;
		}
	}
    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorComponentNotFound;
}

/** @brief the OMX_GetComponentsOfRole standard function
 *
 * This function searches in all the component loaders any component
 * supporting the requested role
 *
 * @param role See spec
 * @param pNumComps See spec
 * @param compNames See spec
 *
 */
OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_STRING role,
    OMX_U32 *pNumComps,
    OMX_U8  **compNames) {

    OMX_ERRORTYPE err = OMX_ErrorNone;
    int i;
    int only_number_requested = 0, full_number=0;
    OMX_U32 temp_num_comp = 0;

    OMX_U8 **tempCompNames;

    omx_debug("In %s\n", __func__);

    if (compNames == NULL) {
        only_number_requested = 1;
    } else {
        only_number_requested = 0;
    }

    struct omx_load_component *omx_comp = NULL;
	list_for_each_entry(omx_comp, &g_omx_elem_list, list) {
        temp_num_comp = *pNumComps;
        err = omx_comp->ops->GetComponentsOfRole(
                  omx_comp->ops,
                  role,
                  &temp_num_comp,
                  NULL);
        if (err != OMX_ErrorNone) {
            omx_debug("Out of %s\n", __func__);
            return OMX_ErrorComponentNotFound;
        }
        if (only_number_requested == 0) {
            tempCompNames = omx_alloc(temp_num_comp * sizeof(OMX_STRING));
            if (!tempCompNames) {
                omx_debug("Insufficient memory in %s\n", __func__);
                return OMX_ErrorInsufficientResources;
            }
            for (i=0; i<(int)temp_num_comp; i++) {
                tempCompNames[i] = omx_alloc(OMX_MAX_STRINGNAME_SIZE * sizeof(char));
                if (!tempCompNames[i]) {
                    omx_debug("Insufficient memory in %s\n", __func__);
                    return OMX_ErrorInsufficientResources;
                }
            }
            err = omx_comp->ops->GetComponentsOfRole(
                      omx_comp->ops,
                      role,
                      &temp_num_comp,
                      tempCompNames);
            if (err != OMX_ErrorNone) {
                omx_debug("Out of %s\n", __func__);
                return OMX_ErrorComponentNotFound;
            }

            for (i = 0; i<(int)temp_num_comp; i++) {
                if (full_number + i < (int)(*pNumComps)) {
                    strncpy((char *)compNames[full_number + i], (const char *)tempCompNames[i], 128);
                }
            }
        }
        full_number += temp_num_comp;
    }

    *pNumComps = full_number;
    omx_debug("Out of %s\n", __func__);
    return OMX_ErrorNone;
}


#if 0
OMX_ERRORTYPE BOSA_AddComponentLoader(BOSA_COMPONENTLOADER *pLoader)
{
    BOSA_COMPONENTLOADER **newLoadersList = NULL;
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);

    assert(pLoader);

    bosa_loaders++;
    newLoadersList = realloc(loadersList, bosa_loaders * sizeof(BOSA_COMPONENTLOADER *));

    if (!newLoadersList)
        return OMX_ErrorInsufficientResources;

    loadersList = newLoadersList;

    loadersList[bosa_loaders - 1] = pLoader;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Loader added at index %d\n", bosa_loaders - 1);

    return OMX_ErrorNone;
}

OSCL_EXPORT_REF OMX_ERRORTYPE OMX_GetContentPipe(
    OMX_HANDLETYPE *hPipe,
    OMX_STRING szURI) {

    OMX_ERRORTYPE err = OMX_ErrorContentPipeCreationFailed;
    CPresult res;
    omx_debug("In %s\n", __func__);

    if(strncmp(szURI, "file", 4) == 0) {
        res = file_pipe_Constructor((CP_PIPETYPE*) hPipe, szURI);
        if(res == 0x00000000)
            err = OMX_ErrorNone;
    }

    else if(strncmp(szURI, "inet", 4) == 0) {
        res = inet_pipe_Constructor((CP_PIPETYPE*) hPipe, szURI);
        if(res == 0x00000000)
            err = OMX_ErrorNone;
    }
    omx_debug("Out of %s\n", __func__);
    return err;
}

#endif

