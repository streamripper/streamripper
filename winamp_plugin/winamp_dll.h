#ifndef __WINAMP_HOOK_H__
#define __WINAMP_HOOK_H__

#include <windows.h>
#include "srtypes.h"

void winamp_dll_init (void);
void hook_init (HWND hwnd);
BOOL unhook_winamp (void);
BOOL hook_winamp (void);
void winamp_poll (char url[MAX_URL_LEN]);

#endif
