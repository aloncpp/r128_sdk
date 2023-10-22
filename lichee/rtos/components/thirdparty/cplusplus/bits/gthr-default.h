/// Copyright 2018-2023 Piotr Grygorczuk <grygorek@gmail.com>
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all
/// copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.

#ifndef _GTHR_FREERTOS_X__H_
#define _GTHR_FREERTOS_X__H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <sys/time.h>

namespace free_rtos_std
{
  struct Once
  {
    bool v = false;
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    ~Once() { vSemaphoreDelete(m); }
  };
}

extern "C"
{

#define __GTHREAD_COND_INIT_FUNCTION
#define __GTHREADS 1

// returns: 1 - thread system is active; 0 - thread system is not active
static int __gthread_active_p() { return 0; }

typedef free_rtos_std::Once __gthread_once_t;
typedef SemaphoreHandle_t __gthread_mutex_t;
typedef SemaphoreHandle_t __gthread_recursive_mutex_t;
typedef int __gthread_key_t;
typedef int __gthread_cond_t;

#define _GLIBCXX_UNUSED __attribute__((__unused__))

#define __GTHREAD_ONCE_INIT free_rtos_std::Once()

  static inline void __GTHREAD_RECURSIVE_MUTEX_INIT_FUNCTION(
      __gthread_recursive_mutex_t *mutex)
  {
    *mutex = xSemaphoreCreateRecursiveMutex();
  }
  static inline void __GTHREAD_MUTEX_INIT_FUNCTION(__gthread_mutex_t *mutex)
  {
    *mutex = xSemaphoreCreateMutex();
  }

  static int __gthread_once(__gthread_once_t *once, void (*func)(void))
  {
    if (!once->m)
      return 12; // POSIX error: ENOMEM

    bool flag{true};
    xSemaphoreTake(once->m, portMAX_DELAY);
    std::swap(once->v, flag);
    if (flag == false)
      func();
    xSemaphoreGive(once->m);

    return 0;
  }

  static int __gthread_key_create(__gthread_key_t *keyp _GLIBCXX_UNUSED, void (*dtor)(void *) _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static int __gthread_key_delete(__gthread_key_t key _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static void *__gthread_getspecific(__gthread_key_t key _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static int __gthread_setspecific(__gthread_key_t key _GLIBCXX_UNUSED, const void *ptr _GLIBCXX_UNUSED)
  {
    return 0;
  }

  static inline int __gthread_mutex_destroy(__gthread_mutex_t *mutex)
  {
    vSemaphoreDelete(*mutex);
    return 0;
  }
  static inline int __gthread_recursive_mutex_destroy(
      __gthread_recursive_mutex_t *mutex)
  {
    vSemaphoreDelete(*mutex);
    return 0;
  }

  static inline int __gthread_mutex_lock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreTake(*mutex, portMAX_DELAY) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_mutex_trylock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreTake(*mutex, 0) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_mutex_unlock(__gthread_mutex_t *mutex)
  {
    return (xSemaphoreGive(*mutex) == pdTRUE) ? 0 : 1;
  }

  static inline int __gthread_recursive_mutex_lock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreTakeRecursive(*mutex, portMAX_DELAY) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_recursive_mutex_trylock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreTakeRecursive(*mutex, 0) == pdTRUE) ? 0 : 1;
  }
  static inline int __gthread_recursive_mutex_unlock(
      __gthread_recursive_mutex_t *mutex)
  {
    return (xSemaphoreGiveRecursive(*mutex) == pdTRUE) ? 0 : 1;
  }

} // extern "C"

#endif // _GTHR_FREERTOS_X__H_
