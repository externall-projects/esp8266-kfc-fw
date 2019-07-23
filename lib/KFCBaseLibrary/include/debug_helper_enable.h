/**
* Author: sascha_lammers@gmx.de
*/

// enable debug output (compile time)
// in order to use the macros, the file should be included after the last #include statement before using the macros, since the included file might have defined/undefined them

#ifdef _debug_print
#undef _debug_print
#undef _debug_println
#undef _debug_printf
#undef _debug_printf_P
#undef _debug_print_result_P
#undef _debug_call_P
#undef _IF_DEBUG
#endif
#define _debug_print(...)                           debug_print(__VA_ARGS__)
#define _debug_println(...)                         debug_println(__VA_ARGS__)
#define _debug_printf(...)                          debug_printf(__VA_ARGS__)
#define _debug_printf_P(...)                        debug_printf_P(__VA_ARGS__)
#define _debug_print_result_P(result, fmt, ...)     debug_print_result_P(result, fmt, __VA_ARGS__)
#define _debug_call_P(function, fmt, ...)           debug_call_P(function, fmt, __VA_ARGS__)
#define _IF_DEBUG(...)                              IF_DEBUG(__VA_ARGS__)