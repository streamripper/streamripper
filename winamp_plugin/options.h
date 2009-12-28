#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "rip_manager.h"
#include "wstreamripper.h"

void options_dialog_show (HINSTANCE inst, HWND parent);
void options_get_desktop_folder (char* path);
BOOL options_load (void);
BOOL options_save (void);

#endif //__OPTIONS_H__
