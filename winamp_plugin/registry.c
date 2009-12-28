/* registry.c
 * look into the registry
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <windows.h>
#include <stdio.h>
#include "srtypes.h"
#include "wa_ipc.h"
#include "ipc_pe.h"
#include "debug.h"
#include "mchar.h"

#define DbgBox(_x_)	MessageBox(NULL, _x_, "Debug", 0)

/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL
get_string_from_registry (char *path, HKEY hkey, LPCTSTR subkey, LPTSTR name)
{
    LONG rc;
    HKEY hkey_result;
    DWORD size = SR_MAX_PATH;
    char strkey[SR_MAX_PATH];
    int i;
    DWORD type;
    int k = REG_SZ;

    debug_printf ("Trying RegOpenKeyEx: 0x%08x %s\n", hkey, subkey);
    rc = RegOpenKeyEx (hkey, subkey, 0, KEY_QUERY_VALUE, &hkey_result);
    if (rc != ERROR_SUCCESS) {
	return FALSE;
    }

    debug_printf ("Trying RegQueryValueEx: %s\n", name);
    rc = RegQueryValueEx (hkey_result, name, NULL, &type, (LPBYTE)strkey, &size);
    if (rc != ERROR_SUCCESS) {
	debug_printf ("Return code = %d\n", rc);
	RegCloseKey (hkey_result);
	return FALSE;
    }

    debug_printf ("RegQueryValueEx succeeded: %d\n", type);
    for (i = 0; strkey[i] && i < SR_MAX_PATH-1; i++) {
	path[i] = toupper(strkey[i]);
    }
    path[i] = 0;

    RegCloseKey (hkey_result);

    return TRUE;
}

BOOL
strip_registry_path (char* path, char* tail)
{
    int i = 0;
    int tail_len = strlen(tail);

    /* Skip the leading quote */
    debug_printf ("Stripping registry path: %s\n", path);
    if (path[0] == '\"') {
	for (i = 1; path[i]; i++) {
	    path[i-1] = path[i];
	}
	path[i-1] = path[i];
        debug_printf ("Stripped quote mark: %s\n", path);
    }

    /* Search for, and strip, the tail */
    i = 0;
    while (path[i]) {
	if (strncmp (&path[i], tail, tail_len) == 0) {
	    path[i] = 0;
	    debug_printf ("Found path: %s (%s)\n", path, tail);
	    return TRUE;
	}
	i++;
    }

    debug_printf ("Did not find path\n");
    return FALSE;
}

/* Return TRUE if found, FALSE if not found */
BOOL
get_int_from_registry (int *val, HKEY hkey, LPCTSTR subkey, LPTSTR name)
{
    LONG rc;
    HKEY hkey_result;
    DWORD type;
    DWORD dword_val;
    DWORD size = sizeof(DWORD);

    debug_printf ("Trying RegOpenKeyEx: 0x%08x %s\n", hkey, subkey);
    rc = RegOpenKeyEx (hkey, subkey, 0, KEY_QUERY_VALUE, &hkey_result);
    if (rc != ERROR_SUCCESS) {
	return FALSE;
    }

    debug_printf ("Trying RegQueryValueEx: %s\n", name);
    rc = RegQueryValueEx (hkey_result, name, NULL, &type, (LPBYTE) &dword_val, &size);
    if (rc != ERROR_SUCCESS) {
	debug_printf ("Return code = %d\n", rc);
	RegCloseKey (hkey_result);
	return FALSE;
    }
    if (type != REG_DWORD) {
	debug_printf ("Not a DWORD = %d\n", type);
	RegCloseKey (hkey_result);
	return FALSE;
    }

    *val = dword_val;
    RegCloseKey (hkey_result);
    return TRUE;
}

/* Return TRUE if found, FALSE if not found */
BOOL
set_int_to_registry (HKEY hkey, LPCTSTR subkey, LPTSTR name, int val)
{
    LONG rc;
    HKEY hkey_result;
    DWORD disposition;
    DWORD dword_val = val;
    DWORD size = sizeof(DWORD);

    debug_printf ("Trying RegOpenKeyEx: 0x%08x %s\n", hkey, subkey);
    rc = RegCreateKeyEx (hkey, subkey, 0, "foobar", REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &hkey_result, &disposition);
    if (rc != ERROR_SUCCESS) {
	debug_printf ("RegCreateKeyEx Return code = %d\n", rc);
	return FALSE;
    }

    debug_printf ("Trying RegSetValueEx: %s\n", name);
    rc = RegSetValueEx (hkey_result, name, 0, REG_DWORD, (CONST BYTE*) &dword_val, size);
    if (rc != ERROR_SUCCESS) {
	debug_printf ("RegSetValueEx Return code = %d\n", rc);
	RegCloseKey (hkey_result);
	return FALSE;
    }

    RegCloseKey (hkey_result);
    return TRUE;
}

