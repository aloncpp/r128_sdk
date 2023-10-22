/**
* src/OmxComponentLoader.c

* specific component loader for local components.
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

#include "OMX_Base.h"
#include "aec_component.h"
#include "aeq_component.h"
#include "asr_component.h"
#include "arecord_component.h"
#include "arender_component.h"
#include "dump_component.h"


static int NumComponents = 0;
static int NumComponentsfordeinit = 0;

OmxLoaderComponentType** g_lateList = NULL;
OMX_ERRORTYPE (*fun_ptr[MAX_COMPONENTS])(OmxLoaderComponentType *OmxComponents) = { 0 };

/*
 * audio encode component
 * ops:		init create
 * */
static struct omx_component_load omx_component_ops = {
	.InitComponentLoader = OmxInitComponentLoader,
	.DeInitComponentLoader = OmxDeInitComponentLoader,
	.CreateComponent = OmxCreateComponent,
	.DestroyComponent = OmxDestroyComponent,
	.ComponentNameEnum = OmxComponentNameEnum,
	.GetRolesOfComponent = OmxGetRolesOfComponent,
	.GetComponentsOfRole = OmxGetComponentsOfRole,
};

struct omx_load_component g_arecord_component = {
	.name = RECORD_COMP_NAME,
	.ops = &omx_component_ops,
};

struct omx_load_component g_arender_component = {
	.name = RENDER_COMP_NAME,
	.ops = &omx_component_ops,
};

struct omx_load_component g_aechocancel_component = {
	.name = ECHO_CANCEL_COMP_NAME,
	.ops = &omx_component_ops,
};

struct omx_load_component g_aequalizer_component = {
	.name = EQUALIZER_COMP_NAME,
	.ops = &omx_component_ops,
};

struct omx_load_component g_asr_component = {
	.name = ASR_COMP_NAME,
	.ops = &omx_component_ops,
};

struct omx_load_component g_dump_component = {
	.name = DUMP_COMP_NAME,
	.ops = &omx_component_ops,
};


/** @brief the loader constructor
 *
 * This function creates the component loader, and creates
 * the list of available components, based on a registry file
 * created by a separate application. It is called omxregister,
 * and must be called before the use of this loader
 */
OMX_ERRORTYPE OmxInitComponentLoader(omx_component_load *loader, OMX_STRING ComponentName) {

	int listindex = -1;
	int i = 0;
	OmxLoaderComponentType** templateList = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	omx_debug("In %s ComponentName %s\n", __func__, ComponentName);

	if (NumComponents == 0) {

#ifdef CONFIG_COMPONENTS_OMX_AUDIO_RECORD
		fun_ptr[++listindex] = omx_audio_record_component_setup;
#endif

#ifdef CONFIG_COMPONENTS_OMX_AUDIO_RENDER
		fun_ptr[++listindex] = omx_audio_render_component_setup;
#endif

#ifdef CONFIG_COMPONENTS_OMX_AUDIO_ECHO_CANCEL
		fun_ptr[++listindex] = omx_audio_echocancel_component_setup;
#endif

#ifdef CONFIG_COMPONENTS_OMX_AUDIO_EQUALIZER
		fun_ptr[++listindex] = omx_audio_equalizer_component_setup;
#endif

#ifdef CONFIG_COMPONENTS_OMX_AUDIO_ASR
		fun_ptr[++listindex] = omx_audio_asr_component_setup;
#endif

#ifdef CONFIG_COMPONENTS_OMX_DUMP
		fun_ptr[++listindex] = omx_dump_component_setup;
#endif

		if (listindex < 0) {
			omx_info("There hasn't be loaded any component \n");
			return OMX_ErrorNone;
		}

		if (listindex > MAX_COMPONENTS) {
			omx_err("The number of components are more than max of %d \n", MAX_COMPONENTS);
			return OMX_ErrorInsufficientResources;
		}

		templateList = (OmxLoaderComponentType**)omx_alloc(sizeof (OmxLoaderComponentType*) * (listindex + 1));
		memset(templateList, 0x00, (listindex + 1) * sizeof(OmxLoaderComponentType*));
		NumComponents = listindex + 1;
		NumComponentsfordeinit = NumComponents;
		for (i = 0; i< NumComponents; i++) {
			templateList[i] = omx_alloc(sizeof(OmxLoaderComponentType));
			memset(templateList[i], 0x00, sizeof(OmxLoaderComponentType));
		}
		g_lateList = templateList;
	}

	for (i = 0;i < NumComponents; ++i) {

		if (!strnlen(g_lateList[i]->name, OMX_STRINGNAME_SIZE)) {
			eError = fun_ptr[i](g_lateList[i]);
			if (eError == OMX_ErrorNone) {
				// the component has been loader
				break;
			}
		} else {
			omx_info("The Component of %s has been loader \n", g_lateList[i]->name);
		}
	}

    if (eError != OMX_ErrorNone) {
        return eError;
    }

	if (g_lateList)
		loader->loaderPrivate = g_lateList;

	omx_debug("Out of %s\n", __func__);
	return eError;
}

/** @brief The destructor of the specific component loader.
 *
 * This function deallocates the list of available components.
 */
OMX_ERRORTYPE OmxDeInitComponentLoader(omx_component_load *loader, OMX_STRING ComponentName) {

	unsigned int i, j;
	OmxLoaderComponentType** templateList;

	omx_debug("In %s\n", __func__);
	templateList = (OmxLoaderComponentType**)loader->loaderPrivate;

	for (i = 0;i < NumComponents; ++i) {
		if (templateList[i] && !strcmp(templateList[i]->name, ComponentName)) {
			omx_info("The Component of %s will been unloader \n", ComponentName);

			if(templateList[i]->name_requested){
				memset(templateList[i]->name_requested, 0x00, OMX_STRINGNAME_SIZE);
			}

			for(j = 0 ; j < templateList[i]->name_specific_length; j++){
				if(templateList[i]->name_specific[j]) {
					memset(templateList[i]->name_specific[j], 0x00, OMX_STRINGNAME_SIZE);
				}
				if(templateList[i]->role_specific[j]){
					memset(templateList[i]->role_specific[j], 0x00, OMX_STRINGNAME_SIZE);
				}
			}

			if(templateList[i]->name){
				memset(templateList[i]->name, 0x00, OMX_STRINGNAME_SIZE);
			}
			omx_free(templateList[i]);
			templateList[i] = NULL;
			NumComponentsfordeinit--;
			break;
		}
	}

	if(templateList && NumComponentsfordeinit == 0) {
		omx_free(templateList);
		templateList = NULL;
		NumComponents = 0;
	}

	omx_debug("Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/** @brief creator of the requested OpenMAX component
 *
 * This function searches for the requested component in the internal list.
 * If the component is found, its constructor is called,
 * and the standard callbacks are assigned.
 * A pointer to a standard OpenMAX component is returned.
 */
OMX_ERRORTYPE OmxCreateComponent(
  omx_component_load *loader,
  OMX_HANDLETYPE* pHandle,
  OMX_STRING cComponentName,
  OMX_PTR pAppData,
  OMX_CALLBACKTYPE* pCallBacks) {

  int i;
  unsigned int j;
  int componentPosition = -1;
  OMX_ERRORTYPE eError = OMX_ErrorNone;
  OmxLoaderComponentType** templateList;
  OMX_COMPONENTTYPE *openmaxStandComp;
  omx_base_component_PrivateType * priv;

  omx_debug("In %s\n", __func__);
  templateList = (OmxLoaderComponentType**)loader->loaderPrivate;

  for (i = 0;i < NumComponents; ++i)  {
    if(!strcmp(templateList[i]->name, cComponentName)) {
      //given component name matches with the general component names
      componentPosition = i;
      break;
    } else {
      for(j = 0;j < templateList[i]->name_specific_length; j++) {
        if(!strcmp(templateList[i]->name_specific[j], cComponentName)) {
          //given component name matches with specific component names
          componentPosition = i;
          break;
        }
      }
      if(componentPosition != -1) {
        break;
      }
    }
  }
  if (componentPosition == -1) {
    omx_debug("Component not found with current component loader.\n");
    return OMX_ErrorComponentNotFound;
  }

  //component name matches with general component name field
  omx_debug("Found base requested template %s\n", cComponentName);
  /* Build component from template and fill fields */
  if (templateList[componentPosition]->name_requested != NULL)
  {    /* This check is to prevent memory leak in case two instances of the same component are loaded */
	  strcpy(templateList[componentPosition]->name_requested, cComponentName);
  }

  openmaxStandComp = omx_alloc(sizeof(OMX_COMPONENTTYPE));
  memset(openmaxStandComp, 0x00, sizeof(OMX_COMPONENTTYPE));
  if (!openmaxStandComp) {
    return OMX_ErrorInsufficientResources;
  }

  eError = templateList[componentPosition]->constructor(openmaxStandComp,cComponentName);
  if (eError != OMX_ErrorNone) {
    if (eError == OMX_ErrorInsufficientResources) {
      *pHandle = openmaxStandComp;
      priv = (omx_base_component_PrivateType *) openmaxStandComp->pComponentPrivate;
      priv->loader = loader;
      return OMX_ErrorInsufficientResources;
    }
    omx_debug("Error during component construction\n");
    openmaxStandComp->ComponentDeInit(openmaxStandComp);
    omx_free(openmaxStandComp);
    openmaxStandComp = NULL;
    return OMX_ErrorComponentNotFound;
  }

  priv = (omx_base_component_PrivateType *) openmaxStandComp->pComponentPrivate;
  priv->loader = loader;

  *pHandle = openmaxStandComp;
  ((OMX_COMPONENTTYPE*)*pHandle)->SetCallbacks(*pHandle, pCallBacks, pAppData);

  omx_debug("Template %s found returning from OMX_GetHandle\n", cComponentName);
  omx_debug("Out of %s\n", __func__);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE OmxDestroyComponent(
	omx_component_load *loader,
	OMX_HANDLETYPE hComponent) {
 
	OMX_ERRORTYPE err = OMX_ErrorNone;
	omx_base_component_PrivateType * priv = (omx_base_component_PrivateType *) ((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;

	/* check if this component was actually loaded from this loader */
	if (priv->loader != loader) {
		return OMX_ErrorComponentNotFound;
	}

	err = ((OMX_COMPONENTTYPE*)hComponent)->ComponentDeInit(hComponent);

	omx_free((OMX_COMPONENTTYPE*)hComponent);
	hComponent = NULL;

	return err;
}

/** @brief This function search for the index from 0 to end of the list
 *
 * This function searches in the list of ST static components and enumerates
 * both the class names and the role specific components.
 */
OMX_ERRORTYPE OmxComponentNameEnum(
	omx_component_load *loader,
	OMX_STRING cComponentName,
	OMX_U32 nNameLength,
	OMX_U32 nIndex) {

	OmxLoaderComponentType** templateList;
	int i;
	unsigned int j, index = 0;
	int found = 0;

	omx_debug("In %s\n", __func__);

	templateList = (OmxLoaderComponentType**)loader->loaderPrivate;
	i = 0;

	for (i = 0;i < NumComponents; ++i)  {
		if (index == nIndex) {
			strncpy(cComponentName, templateList[i]->name, nNameLength);
			found = 1;
			break;
		}
		index++;
		if (templateList[i]->name_specific_length > 0) {
			for (j = 0; j<templateList[i]->name_specific_length; j++) {
				if (index == nIndex) {
					strncpy(cComponentName,templateList[i]->name_specific[j], nNameLength);
					found = 1;
					break;
				}
				index++;
			}
		}
		if (found) {
			break;
		}
	}

	if (!found) {
		omx_debug("Out of %s with OMX_ErrorNoMore\n", __func__);
		return OMX_ErrorNoMore;
	}
	omx_debug("Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/** @brief The specific version of OMX_GetRolesOfComponent
 *
 * This function replicates exactly the behavior of the
 * standard OMX_GetRolesOfComponent function for the ST static
 * component loader
 */
OMX_ERRORTYPE OmxGetRolesOfComponent(
	omx_component_load *loader,
	OMX_STRING compName,
	OMX_U32 *pNumRoles,
	OMX_U8 **roles) {

	OmxLoaderComponentType** templateList;
	int i;
	unsigned int j, index;
	unsigned int max_roles = *pNumRoles;
	int found = 0;

	omx_debug("In %s\n", __func__);
	templateList = (OmxLoaderComponentType**)loader->loaderPrivate;
	*pNumRoles = 0;
	i = 0;

	for (i = 0;i < NumComponents; ++i)  {
		if(!strcmp(templateList[i]->name, compName)) {
			omx_debug("Found requested template %s IN GENERAL COMPONENT\n", compName);
			// set the no of roles field
			*pNumRoles = templateList[i]->name_specific_length;
			if(roles == NULL) {
				return OMX_ErrorNone;
			}
			//append the roles
			for (index = 0; index < templateList[i]->name_specific_length; index++) {
				if (index < max_roles) {
					strcpy ((char*)*(roles+index), templateList[i]->role_specific[index]);
					}
			}
			found = 1;
		} else {
			for(j=0;j<templateList[i]->name_specific_length;j++) {
				if(!strcmp(templateList[i]->name_specific[j], compName)) {
					omx_debug("Found requested component %s IN SPECIFIC COMPONENT \n", compName);
					*pNumRoles = 1;
					found = 1;
					if(roles == NULL) {
						return OMX_ErrorNone;
					}
					if (max_roles > 0) {
						strcpy((char*)*roles , templateList[i]->role_specific[j]);
					}
				}
			}
		}
		if(found) {
			break;
		}
	}

	if(!found) {
		omx_debug("no component match in whole template list has been found\n");
		*pNumRoles = 0;
		return OMX_ErrorComponentNotFound;
	}
	omx_debug("Out of %s\n", __func__);
	return OMX_ErrorNone;
}

/** @brief The specific version of OMX_GetComponentsOfRole
 *
 * This function replicates exactly the behavior of the
 * standard OMX_GetComponentsOfRole function for the ST static
 * component loader
 */
OMX_ERRORTYPE OmxGetComponentsOfRole (
	omx_component_load *loader,
	OMX_STRING role,
	OMX_U32 *pNumComps,
	OMX_U8  **compNames) {

	OmxLoaderComponentType** templateList;
	int i = 0;
	unsigned int j = 0;
	int num_comp = 0;
	int max_entries = *pNumComps;

	omx_debug("In %s\n", __func__);

	templateList = (OmxLoaderComponentType**)loader->loaderPrivate;
	i = 0;
	for (i = 0;i < NumComponents; ++i)  {
		for (j = 0; j<templateList[i]->name_specific_length; j++) {
			if (!strcmp(templateList[i]->role_specific[j], role)) {
				if (compNames != NULL) {
					if (num_comp < max_entries) {
						strcpy((char*)(compNames[num_comp]), templateList[i]->name);
					}
				}
				num_comp++;
			}
		}
		i++;
	}

	*pNumComps = num_comp;
	omx_debug("Out of %s\n", __func__);
	return OMX_ErrorNone;
}
