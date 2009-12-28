#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

void
debug_box (char* fmt, ...)
{
#define SIZ 1024
    char buf[SIZ];
    va_list argptr;
    va_start (argptr, fmt);
    _vsnprintf (buf, SIZ, fmt, argptr);
    buf[SIZ-1] = 0;
    va_end (argptr);
    
    MessageBox (NULL, buf, "Debug Box", MB_ICONEXCLAMATION);
}

