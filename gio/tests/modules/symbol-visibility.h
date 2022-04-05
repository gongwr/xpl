#ifndef XPL_TEST_SYMBOL_VISIBILITY
#define XPL_TEST_SYMBOL_VISIBILITY

/* This is the same check that's done in configure to create config.h */
#ifdef _WIN32
#ifdef XPL_STATIC_COMPILATION
#define XPL_TEST_EXPORT_SYMBOL extern
#else
#ifdef _MSC_VER
#define XPL_TEST_EXPORT_SYMBOL __declspec(dllexport) extern
#else
#define XPL_TEST_EXPORT_SYMBOL __attribute__ ((visibility ("default"))) __declspec(dllexport) extern
#endif
#endif
/* Matches GCC and Clang */
#elif defined(__GNUC__) && (__GNUC__ >= 4)
# define XPL_TEST_EXPORT_SYMBOL __attribute__((visibility("default"))) extern
#endif

#endif /* XPL_TEST_SYMBOL_VISIBILITY */
