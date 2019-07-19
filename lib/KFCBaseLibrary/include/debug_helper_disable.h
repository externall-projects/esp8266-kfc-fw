/**
* Author: sascha_lammers@gmx.de
*/

// disable debug output (compile time)
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
#define _debug_print(...) ;
#define _debug_println(...) ;
#define _debug_printf(...) ;
#define _debug_printf_P(...) ;
#define _debug_print_result_P(result, fmt, ...)         result
#define _debug_call_P(function, fmt, ...)               function(__VA_ARGS__)
#define _IF_DEBUG(...)
