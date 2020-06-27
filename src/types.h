#ifndef QAK_TYPES_H
#define QAK_TYPES_H

#include <cstdint>

#ifdef _MSC_VER
#  pragma warning(disable : 4127)      /* disable: C4127: conditional expression is constant */
#  define QAK_FORCE_INLINE __forceinline
#else
#  if defined (__cplusplus) || defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* C99 */
#    ifdef __GNUC__
#      define QAK_FORCE_INLINE inline __attribute__((always_inline))
#    else
#      define QAK_FORCE_INLINE inline
#    endif
#  else
#    define QAK_FORCE_INLINE
#  endif /* __STDC_VERSION__ */
#endif

#endif //QAK_TYPES_H
