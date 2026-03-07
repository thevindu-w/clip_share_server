#ifndef UTILS_UNISTR_WRAP_H_
#define UTILS_UNISTR_WRAP_H_

#ifndef __GLIBC__
#define __GLIBC__ 0
#endif
#ifndef __NO_INLINE__
#define __NO_INLINE__
#endif
#ifndef _LIBUNISTRING_NO_CONST_GENERICS
#define _LIBUNISTRING_NO_CONST_GENERICS
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wundef"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wundef"
#endif

#include <unistr.h>

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

#endif  // UTILS_UNISTR_WRAP_H_
