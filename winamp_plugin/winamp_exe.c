/* winamp_exe.c
 * gets info from winamp in various hacky ass ways
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

#include "wstreamripper.h"
#include "srtypes.h"
#include "wa_ipc.h"
#include "ipc_pe.h"
#include "winamp_exe.h"
#include "debug.h"
#include "mchar.h"
#include "registry.h"

#define DbgBox(_x_)	MessageBox(NULL, _x_, "Debug", 0)

/*********************************************************************************
 * Public functions
 *********************************************************************************/
BOOL winamp_init();			
BOOL winamp_add_track_to_playlist(char *track);
void winamp_add_rip_to_menu (void);
//void winamp_test_stuff (void);

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static char m_winamps_path[SR_MAX_PATH] = {'\0'};
extern HWND g_winamp_hwnd;

BOOL
winamp_init ()
{
    BOOL rc;
    rc = winamp_get_path (m_winamps_path);
    if (!rc) return rc;
    //winamp_test_stuff ();
    return TRUE;
}

BOOL
winamp_get_path(char *path)
{
    BOOL rc;
    char* sr_winamp_home_env;

    sr_winamp_home_env = getenv ("STREAMRIPPER_WINAMP_HOME");
    debug_printf ("STREAMRIPPER_WINAMP_HOME = %s\n", sr_winamp_home_env);
    if (sr_winamp_home_env) {
	sr_strncpy (path, sr_winamp_home_env, SR_MAX_PATH);
	return TRUE;
    }

    rc = get_string_from_registry (path, HKEY_LOCAL_MACHINE, 
	    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Winamp"),
	    TEXT("UninstallString"));
    if (rc == TRUE) {
	rc = strip_registry_path (path, "UNINSTWA.EXE");
	if (rc == TRUE) return TRUE;
    }

    rc = get_string_from_registry (path, HKEY_CLASSES_ROOT, 
	    "Winamp.File\\shell\\Enqueue\\command", NULL);
    if (rc == TRUE) {
	rc = strip_registry_path (path, "WINAMP.EXE");
	if (rc == TRUE) return TRUE;
    }

    /* The alternative is: */
/* SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETINIDIRECTORY);
    #define IPC_GETINIDIRECTORY 335
    (Requires winamp 5.0)
**
** This returns a pointer to the directory where winamp.ini can be found and is
** useful if you want store config files but you don't want to use winamp.ini.
*/

    return FALSE;
}

#if defined (commentout)
HWND
winamp_get_hwnd (void)
{
    /* This used to be done like this:
        return FindWindow("Winamp v1.x", NULL);
       But I found that it's easier and better(?) to use the 
       input from the plugin interface */
#if defined (commentout)
    return g_plugin.hwndParent;
#endif
    return g_winamp_hwnd;
}

HWND
winamp_get_hwnd_pe (void)
{
    HWND hwnd_winamp;
    HWND hwnd_pe = 0;
    int wa_version;

    hwnd_winamp = winamp_get_hwnd ();
    wa_version = SendMessage (hwnd_winamp,WM_WA_IPC,0,IPC_GETVERSION);

    if (wa_version >= 0x2900) {
    	// use the built in api to get the handle
	debug_printf ("Trying new way to get PE HWND...\n");
    	hwnd_pe = (HWND)SendMessage(hwnd_winamp,WM_WA_IPC,IPC_GETWND_PE,IPC_GETWND);
    }

    // if it failed then use the old way :o)
    if (!hwnd_pe) {
	debug_printf ("Trying new way to get PE HWND...\n");
    	hwnd_pe = FindWindow("Winamp PE",0);
    }
    return hwnd_pe;
}
#endif

BOOL
winamp_add_relay_to_playlist (char *host, u_short port, int content_type)
{
    char relay_url[SR_MAX_PATH];
    char relay_file[SR_MAX_PATH];
    char winamp_path[SR_MAX_PATH];

    sprintf (winamp_path, "%s%s", m_winamps_path, "winamp.exe");
    compose_relay_url (relay_url, host, port, content_type);
    sprintf (relay_file, "/add %s", relay_url);
    ShellExecute (NULL, "open", winamp_path, relay_file, NULL, SW_SHOWNORMAL);

    return TRUE;
}

/* GCS FIX: This doesn't work in the exe version */
BOOL
winamp_get_info (WINAMP_INFO *info, BOOL useoldway)
{
    //HWND hwnd_winamp;
    info->url[0] = '\0';
    info->is_running = FALSE;

#if defined (commentout)
    hwnd_winamp = winamp_get_hwnd ();

    /* Get winamp path */
    if (!m_winamps_path[0])
	return FALSE;

    if (!hwnd_winamp) {
	info->is_running = FALSE;
	return FALSE;
    } else {
	info->is_running = TRUE;
    }

    if (useoldway) {
	// Send a message to winamp to save the current playlist
	// to a file, 'n' is the index of the currently selected item
	int n  = SendMessage (hwnd_winamp, WM_USER, (WPARAM)NULL, 120); 
	char m3u_path[SR_MAX_PATH];
	char buf[4096] = {'\0'};
	FILE *fp;

	sprintf (m3u_path, "%s%s", m_winamps_path, "winamp.m3u");	
	if ((fp = fopen (m3u_path, "r")) == NULL)
	    return FALSE;

	while(!feof(fp) && n >= 0)
	{
	    fgets(buf, 4096, fp);
	    if (*buf != '#')
		n--;
	}
	fclose(fp);
	buf[strlen(buf)-1] = '\0';

	// Make sure it's a URL
	if (strncmp (buf, "http://", strlen("http://")) == 0)
	    strcpy (info->url, buf);
    } else {
	// Much better way to get the filename
	int get_filename = 211;
	int get_position = 125;
	int data = 0;
	int pos;
	char* fname;
	char *purl;

	pos = (int)SendMessage (hwnd_winamp, WM_USER, data, get_position);
	fname = (char*)SendMessage (hwnd_winamp, WM_USER, pos, get_filename);
	/* GCS: This is wrong. Winamp returns null if list is empty. */
#if defined (commentout)
	if (fname == NULL)
	    return FALSE;
	purl = strstr (fname, "http://");
	if (purl)
	    strncpy (info->url, purl, MAX_URL_LEN);
#endif
	if (fname) {
	    purl = strstr (fname, "http://");
	    if (purl) {
		strncpy (info->url, purl, MAX_URL_LEN);
	    }
	}
    }
#endif
    return TRUE;
}

BOOL
winamp_add_track_to_playlist (char *fullpath)
{
    char add_track[SR_MAX_PATH];
    char winamp_path[SR_MAX_PATH];

    sprintf (winamp_path, "%s%s", m_winamps_path, "winamp.exe");
    sprintf (add_track, "/add \"%s\"", fullpath);
    ShellExecute (NULL, "open", winamp_path, add_track, NULL, SW_SHOWNORMAL);
    return TRUE;
}

#if defined (commentout)
void
winamp_handle_pe_click (void)
{
    POINT pt;
    RECT rc;
    HWND hwnd_pe, hwnd_winamp;

    hwnd_winamp = winamp_get_hwnd ();
    hwnd_pe = winamp_get_hwnd_pe ();

    // Get the current position of the mouse and the current client area of the playlist window
    // and then mapping the mouse position to the client area
    GetCursorPos(&pt);
    // Get the client area of the playlist window and then map the mouse position to it
    GetClientRect(hwnd_pe,&rc);
    ScreenToClient(hwnd_pe,&pt);
    // this corrects so the selection works correctly on the selection boundary
    // appears to happen on the older 2.x series as well
    pt.y -= 2;
    // corrections for the playlist window area so that work is only done for valid positions
    // and nicely enough it works for both classic and modern skin modes
    rc.top += 18;
    rc.left += 12;
    rc.right -= 19;
    rc.bottom -= 40;
    // is the click in 
    if(PtInRect(&rc,pt)){
    // get the item index at the given point
    // if this is out of range then it will return 0 (not very helpful really)
    int idx = SendMessage(hwnd_pe,WM_WA_IPC,IPC_PE_GETIDXFROMPOINT,(LPARAM)&pt);
	// makes sure that the item isn't past the last playlist item
	if(idx < SendMessage(hwnd_pe,WM_WA_IPC,IPC_PE_GETINDEXTOTAL,0)){
		// ... do stuff in here (this example will start playing the selected track)
		SendMessage(hwnd_winamp,WM_WA_IPC,idx,IPC_SETPLAYLISTPOS);
		SendMessage(hwnd_winamp,WM_COMMAND,WINAMP_BUTTON2,0);
	}
    }
}

void
winamp_test_stuff (void)
{
    HWND hwnd_pe = winamp_get_hwnd_pe ();
    debug_printf ("Got PE: %d\n", hwnd_pe);

    /* Try sending a user message. Will modern skin get it? */
    SendMessage(hwnd_pe,WM_WA_IPC,0x888,0x888);
}
#endif
