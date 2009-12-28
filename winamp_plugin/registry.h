#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <windows.h>

BOOL get_string_from_registry (char *path, HKEY hkey, LPCTSTR subkey, LPTSTR name);
BOOL strip_registry_path (char* path, char* tail);
BOOL get_int_from_registry (int *val, HKEY hkey, LPCTSTR subkey, LPTSTR name);
BOOL set_int_to_registry (HKEY hkey, LPCTSTR subkey, LPTSTR name, int val);

#endif
