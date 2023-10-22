/**
  src/omxcore.h

  OpenMAX Integration Layer Core. This library implements the OpenMAX core
  responsible for environment setup, components tunneling and communication.

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


#ifndef __OMXCORE_H__
#define __OMXCORE_H__

#include <OMX_Component.h>
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Index.h>

#include <aw_list.h>


#define OSCL_IMPORT_REF
#define OSCL_EXPORT_REF


/** Defines the major version of the core */
#define SPECVERSIONMAJOR  1
/** Defines the minor version of the core */
#ifdef ANDROID
#define SPECVERSIONMINOR  0 //for stagefright
#else
#define SPECVERSIONMINOR  1 // for gstreamer
#endif
/** Defines the revision of the core */
#define SPECREVISION      0
/** Defines the step version of the core */
#define SPECSTEP          0

/** @brief Component loader entry points
 *
 * The component loader generic structure contains the entry points for
 * each component loader. The list of component loaders is filled using
 * a special function, called AddComponentLoader.
 */
typedef struct omx_component_load
{
  /** @brief The constructor of the component loader
   *
   * The component loader creator is called by the OMX_Init function. It is
   * implemented by the specific component loader. It must allocate any
   * resource needed by the component loader.
   *
   * @param loader A private data structure, if needed by the component loader.
   * This data structure is passed every time a function of this loader is called.
   * @return OMX_ErrorInsufficientResources if the component loader can not be constructed
   */
  OMX_ERRORTYPE (*InitComponentLoader)(struct omx_component_load *loader, OMX_STRING ComponentName);

  /** @brief The destructor of the component loader
   *
   * The component loader destructor is called by the OMX_Deinit function. It is
   * implemented by the specific component loader. It must free every specific
   * resource used by the component loader.
   *
   * @param loader the specific component loader. This parameter is also specific
   * to the component loader, and its structure is defined by each loader.
   * @return OMX_ErrorNone
   */
  OMX_ERRORTYPE (*DeInitComponentLoader)(struct omx_component_load *loader, OMX_STRING ComponentName);

  /** @brief The component constructor of the current component loader
   *
   * This function implements the OMX_GetHandle function for the
   * specific component loader. Its interface is the same as the
   * standard GetHandle function, except that the first parameter
   * that contains the private data of the specific component loader.
   *
   * @param loader Private data of the component loader
   * @param pHandle the openmax handle returned by the function, or NULL
   * in case of failure.
   * @param cComponentName A string that contains the standard
   * component's name
   * @param pAppData private data of the component (if needed)
   * @param pCallBacks IL client callback function pointers passed
   * to the component
   *
   * @return OMX_ErrorNone if the component is correctly constructed and returned
   * @return OMX_ErrorComponentNotFound if the component is not found
   * @return OMX_ErrorInsufficientResources if the component exists but can not be allocated.
   */
  OMX_ERRORTYPE (*CreateComponent)(
    struct omx_component_load *loader,
    OMX_HANDLETYPE* pHandle,
    OMX_STRING cComponentName,
    OMX_PTR pAppData,
    OMX_CALLBACKTYPE* pCallBacks);

  /** @brief The component destructor of the current component loader
   *
   * This function implements the OMX_FreeHandle function for the
   * specific component loader. Its interface is the same as the
   * standard FreeHandle function, except that the first parameter
   * that contains the private data of the specific component loader.
   *
   * @param loader Private data of the component loader
   * @param pHandle the openmax handle returned by the function, or NULL
   * in case of failure.
   * @param cComponentName A string that contains the standard
   * component's name
   * @param pAppData private data of the component (if needed)
   * @param pCallBacks IL client callback function pointers passed
   * to the component
   *
   * @return OMX_ErrorNone if the component is correctly constructed and returned
   * @return OMX_ErrorComponentNotFound if the component is not found
   * @return OMX_ErrorInsufficientResources if the component exists but can not be allocated.
   */
  OMX_ERRORTYPE (*DestroyComponent)(
      struct omx_component_load *loader,
      OMX_HANDLETYPE hComponent);

  /** @brief An enumerator of the components handled by the current component loader.
   *
   * This function implements the OMX_ComponentNameEnum function
   * for the specific component loader
   *
   * @param loader Private data of the component loader
   * @param cComponentName A pointer to a null terminated string
   * with the component name.  The names of the components are
   * strings less than 127 bytes in length plus the trailing null
   * for a maximum size of 128 bytes (OMX_MAX_STRINGNAME_SIZE).
   * @param nNameLength The number of characters in the
   * cComponentName string.  With all component name strings
   * restricted to less than 128 characters (including the trailing null)
   * it is recommended that the caller provide a input string for the
   * cComponentName of 128 characters.
   * @param nIndex A number containing the enumeration index
   * for the component. Multiple calls to OMX_ComponentNameEnum
   * with increasing values of nIndex will enumerate through the
   * component names in the system until OMX_ErrorNoMore is returned.
   * The value of nIndex is 0 to (N-1), where N is the number of
   * valid installed components in the system.
   *
   * @return OMX_ErrorNone If the command successfully executes
   * @return OMX_ErrorNoMore If the value of nIndex exceeds the
   * number of components handled by the component loader minus 1
   */
  OMX_ERRORTYPE (*ComponentNameEnum)(
    struct omx_component_load *loader,
    OMX_STRING cComponentName,
    OMX_U32 nNameLength,
    OMX_U32 nIndex);

  /** @brief This function implements the OMX_GetRolesOfComponent standard function for the current component loader
   *
   * This function will return the number of roles supported by
   * the given component and (if the roles field is non-NULL)
   * the names of those roles. The call will fail if an insufficiently
   * sized array of names is supplied.
   * To ensure the array is sufficiently sized the client should:
   * - first call this function with the roles field NULL to
   *   determine the number of role names
   * - second call this function with the roles field pointing to
   *   an array of names allocated according to the number
   *   returned by the first call.
   *
   * @param loader Private data of the component loader
   * @param compName The name of the component being queried about.
   * @param pNumRoles This parameter is used both as input and output.
   * If roles is NULL, the input is ignored and the output specifies
   * how many roles the component supports. If compNames is not NULL,
   * on input it bounds the size of the input structure and on output,
   * it specifies the number of roles string names listed within
   * the roles parameter.
   * @param roles If NULL this field is ignored. If non-NULL this points
   * to an array of 128-byte strings which accepts a list of the names of
   * all standard components roles implemented on the specified
   * component name. numComps indicates the number of names.
   */
  OMX_ERRORTYPE (*GetRolesOfComponent)(
    struct omx_component_load *loader,
    OMX_STRING compName,
    OMX_U32 *pNumRoles,
    OMX_U8 **roles);

  /** @brief This function implements the OMX_GetComponentsOfRole
   * standard function for the current component loader
   *
   * This function will return the number of components that support
   * the given role and (if the compNames field is non-NULL) the names
   * of those components. The call will fail if an insufficiently
   * sized array of names is supplied. To ensure the array is
   * sufficiently sized the client should:
   * - first call this function with the compNames field NULL to
   *   determine the number of component names
   * - second call this function with the compNames field pointing
   *   to an array of names allocated according to the number
   *   returned by the first call.
   *
   * @param loader Private data of the component loader
   * @param role This is generic standard component name consisting
   * only of component class name and the type within that class
   * (e.g. 'audio_decoder.aac').
   * @param pNumComps This is used both as input and output. If compNames
   * is NULL, the input is ignored and the output specifies how many
   * components support the given role. If compNames is not NULL,
   * on input it bounds the size of the input structure and on output,
   * it specifies the number of components string names listed within
   * the compNames parameter.
   * @param compNames If NULL this field is ignored. If non-NULL this points
   * to an array of 128-byte strings which accepts a list of the names of
   * all physical components that implement the specified standard component
   * name. Each name is NULL terminated. numComps indicates the number of names.
   */
  OMX_ERRORTYPE (*GetComponentsOfRole) (
    struct omx_component_load *loader,
    OMX_STRING role,
    OMX_U32 *pNumComps,
    OMX_U8  **compNames);

  /** @brief The reference to the current component loader private data
   *
   * The current loader specified by this structure is described with this
   * generic pointer that contains the private data of the loader.
   */
  void *loaderPrivate;

} omx_component_load;

struct omx_load_component {
	char name[32];
	struct omx_component_load *ops;
	struct list_head list;
};

#define OMX_LOAD_COMPONENT_DEFAULT(name) \
extern struct omx_load_component name; \
	a = omx_alloc(sizeof(struct omx_load_component));if (!a) fatal("no memory"); \
	memcpy(a, &name, sizeof(struct omx_load_component)); \
	list_add_tail(&a->list, head);

#endif

