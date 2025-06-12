#pragma once

#ifdef DINT_ALWAYS_INLINE
#    undef DINT_ALWAYS_INLINE
#endif
#define DINT_ALWAYS_INLINE __attribute__((always_inline)) inline

#ifdef DINT_NEVER_INLINE
#    undef DINT_NEVER_INLINE
#endif
#define DINT_NEVER_INLINE __attribute__((noinline))

#ifdef DINT_FLATTEN
#    undef DINT_FLATTEN
#endif
#define DINT_FLATTEN __attribute__((flatten))
