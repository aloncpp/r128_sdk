/**
  src/base/omx_classmagic.h
	  
  This file contains class handling helper macros
  It is left as an exercise to the reader how they do the magic (FIXME)
  
  Usage Rules:
  1) include this file
  2) if your don't inherit, start your class with CLASS(classname)
  3) if you inherit something, start your class with 
  	DERIVEDCLASS(classname, inheritedclassname)
  4) end your class with ENDCLASS(classname)
  5) define your class variables with a #define classname_FIELDS inheritedclassname_FIELDS
  	inside your class and always add a backslash at the end of line (except last)
  6) if you want to use doxygen, use C-style comments inside the #define, and
  	enable macro expansion in doxyconf and predefine DOXYGEN_PREPROCESSING there, etc.
  
  See examples at the end of this file (in #if 0 block)
  
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

#ifndef OMX_CLASSMAGIC_H_
#define OMX_CLASSMAGIC_H_


#define CLASS(a) typedef struct a a; \
 struct a { 
#define DERIVEDCLASS(a, b) typedef struct a a; \
 struct a {
#define ENDCLASS(a) a##_FIELDS };


#if 0 /*EXAMPLES*/
/**
 * Class A is a nice class
 */
CLASS(A)
#define A_FIELDS \
/** @param a very nice parameter */ \
 	int a; \
/** @param ash another very nice parameter */ \
 	int ash;
ENDCLASS(A)
 
/**
 * Class B is a nice derived class
 */
DERIVEDCLASS(B,A)
#define B_FIELDS A_FIELDS \
/** @param b very nice parameter */ \
	int b;
ENDCLASS(B)

/**
 * Class B2 is a nice derived class
 */
DERIVEDCLASS(B2,A)
#define B2_FIELDS A_FIELDS \
/** @param b2 very nice parameter */ \
	int b2;
ENDCLASS(B2)

/**
 * Class C is an even nicer derived class.
 */
DERIVEDCLASS(C,B)
#define C_FIELDS B_FIELDS \
/** @param c very nice parameter */ \
	int c;
ENDCLASS(C)

#endif /* 0 */

#endif
