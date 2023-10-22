/**
* src/OmxComponentLoader.h

* specific component loader for local components.

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

#ifndef __OMX_COMPONENT_LOADER_H__
#define __OMX_COMPONENT_LOADER_H__

#define MAX_COMPONENTS 8
#define OMX_STRINGNAME_SIZE 32
#define NAME_SPECIFIC_NUM 1

#include "OmxCore.h"

/** @brief the private data structure handled by the loader that described
 * an OpenMAX component
 *
 * This structure contains all the fields that the loader must use to support
 * the loading  unloading functions of the component, that are not described by the
 * standard.
 */
typedef struct OmxLoaderComponentType {
  OMX_VERSIONTYPE componentVersion; /**< the version of the component in the OpenMAX standard format */
  char name[OMX_STRINGNAME_SIZE]; /**< String that represents the name of the component, ruled by the standard */
  unsigned int name_specific_length;/**< this field contains the number of roles of the component */
  char name_specific[NAME_SPECIFIC_NUM][OMX_STRINGNAME_SIZE]; /**< Strings those represent the names of the specific format components */
  char role_specific[NAME_SPECIFIC_NUM][OMX_STRINGNAME_SIZE]; /**< Strings those represent the names of the specific format components */
  char name_requested[OMX_STRINGNAME_SIZE]; /**< This parameter is used to send to the component the string requested by the IL Client */
  OMX_ERRORTYPE (*constructor)(OMX_COMPONENTTYPE*,OMX_STRING cComponentName); /**< constructor function pointer for each Linux ST OpenMAX component */
} OmxLoaderComponentType;

/** @brief The constructor of the ST specific component loader.
 *
 * It is the component loader developed.
 * It is based on a registry file, like in the case of GStreamer. It reads the
 * registry file, and allows the components to register themself to the
 * main list templateList.
 */
OMX_ERRORTYPE OmxInitComponentLoader(omx_component_load *loader, OMX_STRING ComponentName);

/** @brief The destructor of the specific component loader.
 *
 * This function deallocates the list of available components.
 */
OMX_ERRORTYPE OmxDeInitComponentLoader(omx_component_load *loader, OMX_STRING ComponentName);

/** @brief creator of the requested openmax component
 *
 * This function searches for the requested component in the internal list.
 * If the component is found, its constructor is called,
 * and the standard callback are assigned.
 * A pointer to a standard openmax component is returned.
 */
OMX_ERRORTYPE OmxCreateComponent(
  omx_component_load *loader,
  OMX_HANDLETYPE* pHandle,
  OMX_STRING cComponentName,
  OMX_PTR pAppData,
  OMX_CALLBACKTYPE* pCallBacks);

/** @brief destructor of the requested OpenMAX component
 *
 */
OMX_ERRORTYPE OmxDestroyComponent(
  omx_component_load *loader,
  OMX_HANDLETYPE hComponent) ;

/** @brief This function search for the index from 0 to end of the list
 *
 * This function searches in the list of ST static components and enumerates
 * both the class names and the role specific components.
 */
OMX_ERRORTYPE OmxComponentNameEnum(
  omx_component_load *loader,
  OMX_STRING cComponentName,
  OMX_U32 nNameLength,
  OMX_U32 nIndex) ;

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
	OMX_U8 **roles);

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
	OMX_U8  **compNames);

#endif

