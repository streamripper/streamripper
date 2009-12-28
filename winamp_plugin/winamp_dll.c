/* winamp_hook.c
 * handles hooking winamp so that winamp can dock
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
#include <string.h>
#include "registry.h"
#include "wa_ipc.h"
#include "debug.h"
#include "gen.h"

#define SNAP_OFFSET		10
#define WINAMP_CLASSIC_WINS	4
#define WINAMP_MODERN_WINS	8

#define DOCKED_TOP_LL		1	// Top Left Left
#define DOCKED_TOP_LR		2	// Top Left Right
#define DOCKED_TOP_RL		3	// Top Right Left
#define DOCKED_LEFT_TT		4
#define DOCKED_LEFT_TB		5
#define DOCKED_LEFT_BT		6
#define DOCKED_BOTTOM_LL	7
#define DOCKED_BOTTOM_RL	8
#define DOCKED_BOTTOM_LR	9
#define DOCKED_RIGHT_TT		10
#define DOCKED_RIGHT_TB		11
#define DOCKED_RIGHT_BT		12

// My extensions
#define DOCKED_LEFT		100
#define DOCKED_TOP		101
#define DOCKED_RIGHT		102
#define DOCKED_BOTTOM		103

#define RTWIDTH(rect)	((rect).right-(rect).left)
#define RTHEIGHT(rect)	((rect).bottom-(rect).top)

/*****************************************************************************
 * Public functions
 *****************************************************************************/

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static LRESULT CALLBACK	hook_winamp_callback(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
static VOID dock_window();
static void notify_dock ();
static BOOL winamp_get_path (char *path);

int write_pipe (char* msg);
void dll_quit ();
int check_child_process (void);

/*****************************************************************************
 * Private Vars
 *****************************************************************************/
struct WINAMP_WINS
{		
    HWND hwnd;
    BOOL visible;
    WNDPROC orig_proc;
};

static HWND m_hwnd = NULL;
static char m_winamp_path[SR_MAX_PATH] = {'\0'};
static struct WINAMP_WINS m_winamp_classic_wins [WINAMP_CLASSIC_WINS];
static struct WINAMP_WINS m_winamp_modern_wins [WINAMP_MODERN_WINS];
static int m_num_modern_wins = 0;
static int m_skin_is_modern = 0;

extern winampGeneralPurposePlugin g_plugin;


#define DEBUG_BUF_LEN 2048

#if WIN32
    #define vsnprintf _vsnprintf
#endif

/* Debug stuff */

//static int debug_on = 1;
static int debug_on = 0;
static int debug_initialized = 0;
FILE* gcsfp = 0;

void
debug_open (void)
{
    if (!debug_on) return;
    if (!gcsfp) {
	gcsfp = fopen("d:\\sripper_1x\\gcs_dll.txt", "a");
	if (!gcsfp) {
	    debug_on = 0;
	}
    }
}

void
debug_printf (char* fmt, ...)
{
    int was_open = 1;
    va_list argptr;

    if (!debug_on) {
	return;
    }

    va_start (argptr, fmt);
    if (!gcsfp) {
	was_open = 0;
	debug_open();
	if (!gcsfp) return;
    }
    if (!debug_initialized) {
	debug_initialized = 1;
	fprintf (gcsfp, "=========================\n");
	fprintf (gcsfp, "DLL\n");
    }

    vfprintf (gcsfp, fmt, argptr);
    fflush (gcsfp);

    va_end (argptr);
    if (!was_open) {
	debug_close ();
    }
}

void
debug_close (void)
{
    if (!debug_on) return;
    if (gcsfp) {
	fclose(gcsfp);
	gcsfp = 0;
    }
}

void
debug_popup (char* fmt, ...)
{
    va_list argptr;
    char buf[DEBUG_BUF_LEN];

    va_start (argptr, fmt);
    vsnprintf (buf, DEBUG_BUF_LEN, fmt, argptr);

    MessageBox (g_plugin.hwndParent, buf, "SR DLL", MB_OK);
    va_end (argptr);
}

void
winamp_dll_init (void)
{
    if (!winamp_get_path (m_winamp_path)) {
	m_winamp_path[0] = 0;
    }
}

HWND
winamp_get_hwnd (void)
{
    /* This used to be done like this:
        return FindWindow("Winamp v1.x", NULL);
       But I found that it's easier and better(?) to use the 
       input from the plugin interface */
    return g_plugin.hwndParent;
}

BOOL
winamp_get_path (char *path)
{
    BOOL rc;
    char* sr_winamp_home_env;


    sr_winamp_home_env = getenv ("STREAMRIPPER_WINAMP_HOME");
    debug_printf ("STREAMRIPPER_WINAMP_HOME = %s\n", sr_winamp_home_env);
    if (sr_winamp_home_env) {
	strncpy (path, sr_winamp_home_env, SR_MAX_PATH);
	path[SR_MAX_PATH-1] = 0;
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

BOOL CALLBACK
EnumWindowsProc (HWND hwnd, LPARAM lparam)
{
    int i;
    char classname[256];
    char title[256];
    HWND bigowner = (HWND) lparam;
    HWND owner = GetWindow (hwnd, GW_OWNER);
    
    LONG win_id, style;
    HINSTANCE hinstance;
    WNDCLASSEX wcx;

    /* Ignore windows which aren't children of my winamp window */
    if (owner != bigowner) {
	return TRUE;
    }

    GetClassName (hwnd, classname, 256); 
    if (!GetWindowText (hwnd, title, 256)) title[0] = 0;
    win_id = GetWindowLong (hwnd, GWL_ID);
    hinstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE);
    style = GetWindowLong (hwnd, GWL_STYLE);
    GetClassInfoEx (hinstance, classname, &wcx);
    debug_printf ("%s/%s [%d]: %d %d %0x04x %0x04x\n", classname, title, hwnd, win_id, hinstance, style, wcx.style);

    if (strcmp (classname, "BaseWindow_RootWnd") == 0) {
	m_skin_is_modern = 1;
	for (i = 0; i < WINAMP_MODERN_WINS; i++) {
	    if (m_winamp_modern_wins[i].hwnd == hwnd) {
		//debug_printf ("%s (repeat[%d]) = %d\n", classname, i, hwnd);
		return TRUE;
	    }
	}
	if (m_num_modern_wins < WINAMP_MODERN_WINS) {
	    //debug_printf ("%s [%d] = %d\n", classname, m_num_modern_wins, hwnd);
	    m_winamp_modern_wins[m_num_modern_wins++].hwnd = hwnd;
	} else {
	    //debug_printf ("%s [RAN OUT OF SLOTS]\n", classname);
	}
    } else if (strcmp(classname, "Winamp PE") == 0) {
	if (!m_winamp_classic_wins[1].hwnd) {
	    m_winamp_classic_wins[1].hwnd = hwnd;
	    //debug_printf ("%s [1] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[1]) = %d\n", classname, hwnd);
	}
    } else if (strcmp(classname, "Winamp EQ") == 0) {
	if (!m_winamp_classic_wins[2].hwnd) {
	    m_winamp_classic_wins[2].hwnd = hwnd;
	    //debug_printf ("%s [2] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[2]) = %d\n", classname, hwnd);
	}
    } else if (strcmp(classname, "Winamp Video") == 0) {
	if (!m_winamp_classic_wins[3].hwnd) {
	    m_winamp_classic_wins[3].hwnd = hwnd;
	    //debug_printf ("%s [3] = %d\n", classname, hwnd);
	} else {
	    //debug_printf ("%s (repeat[3]) = %d\n", classname, hwnd);
	}
    } else {
	//debug_printf ("%s (other) = %d\n", classname, hwnd);
    }

    /* Always enumerate until the end */
    return TRUE;
}

BOOL
hook_winamp (void)
{
    int i;
    long style = 0;

    debug_printf ("--------- EnumWindows() ---------\n");
    m_skin_is_modern = 0;
    EnumWindows (EnumWindowsProc, (LPARAM)m_winamp_classic_wins[0].hwnd);
    //debug_printf ("-------------------------------\n");

    //debug_printf ("is_modern = %d\n", m_skin_is_modern);
    //debug_printf ("par o par = %d\n", GetParent(m_winamp_classic_wins[0].hwnd));

#if defined (commentout)
    {
    void winamp_test_stuff (void);
    winamp_test_stuff();
    }
#endif

    if (m_skin_is_modern) {
	for (i = 0; i < m_num_modern_wins; i++) {
	    /* Set visible flag and hook if unhooked */
	    style = GetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_STYLE);
	    m_winamp_modern_wins[i].visible = style & WS_VISIBLE;
	    if (!m_winamp_modern_wins[i].orig_proc) {
		m_winamp_modern_wins[i].orig_proc = (WNDPROC) SetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_WNDPROC, (LONG) hook_winamp_callback);
		if (m_winamp_modern_wins[i].orig_proc == NULL) {
		    debug_printf ("Hooking failure?\n");
		    return FALSE;
		}
	    }
	}
    } else {
	/* Verify all windows present */
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
    	    if (m_winamp_classic_wins[i].hwnd == NULL)
		return FALSE;
	}
	/* Set visible flag and hook if unhooked */
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	    style = GetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_STYLE);
	    m_winamp_classic_wins[i].visible = style & WS_VISIBLE;
	    if (!m_winamp_classic_wins[i].orig_proc) {
		m_winamp_classic_wins[i].orig_proc = (WNDPROC) SetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_WNDPROC, (LONG) hook_winamp_callback);
		if (m_winamp_classic_wins[i].orig_proc == NULL) {
    		    debug_printf ("Hooking failure?\n");
		    return FALSE;
		}
	    }
	}
    }

#if defined (commentout)
    for (i = -1; i <= 10; i++) {
	HWND h = (HWND) SendMessage (m_winamp_classic_wins[0].hwnd,WM_WA_IPC,i,IPC_GETWND);
	int ws = (int) SendMessage (m_winamp_classic_wins[0].hwnd,WM_WA_IPC,i,IPC_IS_WNDSHADE);
	debug_printf ("HWND(%d) = %d (%d)\n", i, h, ws);
    }
#endif

    //debug_printf ("Found all the windows\n");
    return TRUE;
}

void
hook_init (HWND hwnd)
{
    int i;

    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	m_winamp_classic_wins[i].hwnd = NULL;
	m_winamp_classic_wins[i].orig_proc = NULL;
    }
    for (i = 0; i < WINAMP_MODERN_WINS; i++) {
	m_winamp_modern_wins[i].hwnd = NULL;
	m_winamp_modern_wins[i].orig_proc = NULL;
    }
    m_hwnd = hwnd;
    //m_winamp_classic_wins[0].hwnd = GetParent (hwnd);
    m_winamp_classic_wins[0].hwnd = hwnd;

    debug_printf ("classic_win[0] = %d\n", m_winamp_classic_wins[0].hwnd);
    //debug_printf ("myself = %d\n", hwnd);
    //debug_printf ("parent = %d\n", m_winamp_classic_wins[0].hwnd);
    //debug_printf ("par o par = %d\n", GetParent(m_winamp_classic_wins[0].hwnd));

}

#if defined (commentout)
void
switch_docking_index (void)
{
    int i;
    HWND hwnd = m_winamp_modern_wins[m_docked_index].hwnd;
    char title_1[256], title_2[256];
    LONG style;
    
    if (!GetWindowText (hwnd, title_1, 256)) return;

    for (i = 0; i < m_num_modern_wins; i++) {
	if (i == m_docked_index) continue;
	hwnd = m_winamp_modern_wins[i].hwnd;
	if (!GetWindowText (hwnd, title_2, 256)) continue;
	if (strcmp (title_1, title_2) == 0) {
	    style = GetWindowLong (hwnd, GWL_STYLE);
	    if (style & WS_VISIBLE) {
		//debug_printf ("Switching %d -> %d\n", m_docked_index, i);
		m_docked_index = i;
		break;
	    }
	}
    }
}
#endif

WNDPROC
find_winamp_callback (HWND hwnd)
{
    int i;
    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	if (m_winamp_classic_wins[i].hwnd == hwnd)
	    return m_winamp_classic_wins[i].orig_proc;
    }
    for (i = 0; i < m_num_modern_wins; i++) {
	if (m_winamp_modern_wins[i].hwnd == hwnd)
	    return m_winamp_modern_wins[i].orig_proc;
    }
    return 0;
}

LRESULT CALLBACK
hook_winamp_callback (HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    int i, rc;

    /* Test if the child process has exited.  If so, we need to unhook. */
    rc = check_child_process ();
    if (rc == 0) {
	WNDPROC wp = find_winamp_callback (hwnd);
	dll_quit ();
	if (wp) {
	    return CallWindowProc (wp, hwnd, umsg, wparam, lparam);
	} else {
	    return FALSE;
	}
    }

#if defined (commentout)
    if (umsg == WM_USER || umsg == WM_COPYDATA) {
	debug_printf ("callback: %d/0x%04x/0x%04x/0x%08x\n",hwnd,umsg,wparam,lparam);
    }
#endif

    if (umsg == WM_SHOWWINDOW && wparam == FALSE) {
#if defined (commentout)
	/* GCS FIX: DO NOT DELETE THIS CODE.  THIS IS FOR DOCKING TO MODERN SKIN. */
	if (m_docked && m_skin_is_modern) {
	    if (m_winamp_modern_wins[m_docked_index].hwnd == hwnd) {
		switch_docking_index ();
		dock_window();
	    }
	}
#endif
	/* GCS FIX: Here I should notify the exe, and it might choose 
	    to dock to another window */
    }

    if (umsg == WM_MOVE) {
	/* Do nothing */
    }

    if (umsg == WM_WINDOWPOSCHANGED) {
#if defined (commentout)
	if ((m_skin_is_modern && m_winamp_modern_wins[m_docked_index].hwnd == hwnd)
	    || (!m_skin_is_modern && m_winamp_classic_wins[m_docked_index].hwnd == hwnd))
	{
	    UINT wpf = ((WINDOWPOS*)lparam)->flags;
	    debug_printf ("WM_WINDOWPOSCHANGED: %d/0x%04x/0x%04x/0x%04x/0x%04x\n",hwnd,umsg,wparam,lparam,((WINDOWPOS*)lparam)->flags);
	    debug_printf ("%s %s %s %s %s %s %s %s %s %s %s %s %s\n",
		(wpf & SWP_DRAWFRAME)     ? "DRAWF" : "-----",
		(wpf & SWP_FRAMECHANGED)  ? "FRAME" : "-----",
		(wpf & SWP_HIDEWINDOW)    ? "HIDEW" : "-----",
		(wpf & SWP_NOACTIVATE)    ? "NOACT" : "-----",
		(wpf & SWP_NOCOPYBITS)    ? "NOCOP" : "-----",
		(wpf & SWP_NOMOVE)        ? "NOMOV" : "-----",
		(wpf & SWP_NOOWNERZORDER) ? "NOOWN" : "-----",
		(wpf & SWP_NOREDRAW)      ? "NORED" : "-----",
		(wpf & SWP_NOREPOSITION)  ? "NOREP" : "-----",
		(wpf & SWP_NOSENDCHANGING)? "NOSEN" : "-----",
		(wpf & SWP_NOSIZE)        ? "NOSIZ" : "-----",
		(wpf & SWP_NOZORDER)      ? "NOZOR" : "-----",
		(wpf & SWP_SHOWWINDOW)    ? "SHOWW" : "-----");
	}
#endif
	notify_dock();
    }
    if (umsg == WM_SIZE) {
	int rc;
	/* Need to send minimize events */
	if (wparam == SIZE_MINIMIZED) {
	    rc = write_pipe ("Resize 0");
	} else {
	    rc = write_pipe ("Resize 1");
	}
	if (rc != 0) {
	    dll_quit ();
	}
    }

#if defined (commentout)
    debug_printf ("callback: %d/0x%04x/0x%04x/0x%08x\n",hwnd,umsg,wparam,lparam);
#endif
    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	if (m_winamp_classic_wins[i].hwnd == hwnd)
	    return CallWindowProc (m_winamp_classic_wins[i].orig_proc, hwnd, umsg, wparam, lparam); 
    }
    for (i = 0; i < m_num_modern_wins; i++) {
	if (m_winamp_modern_wins[i].hwnd == hwnd)
	    return CallWindowProc (m_winamp_modern_wins[i].orig_proc, hwnd, umsg, wparam, lparam);
    }

#if defined (commentout)
    debug_printf ("hook_callback problem: %d/0x%04x/0x%04x/0x%04x\n",hwnd,umsg,wparam,lparam);
#endif
    return FALSE;
}

BOOL
unhook_winamp ()
{
    int i;

    debug_printf ("Unhooking...\n");
    for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	if (m_winamp_classic_wins[i].orig_proc) {
	    debug_printf ("%d %p\n", m_winamp_classic_wins[i].hwnd, m_winamp_classic_wins[i].orig_proc);
	    SetWindowLong (m_winamp_classic_wins[i].hwnd, GWL_WNDPROC, (LONG) m_winamp_classic_wins[i].orig_proc);
	    m_winamp_classic_wins[i].hwnd = 0;
	    m_winamp_classic_wins[i].orig_proc = 0;
	}
    }

    for (i = 0; i < m_num_modern_wins; i++) {
	if (m_winamp_modern_wins[i].orig_proc) {
	    SetWindowLong (m_winamp_modern_wins[i].hwnd, GWL_WNDPROC, (LONG) m_winamp_modern_wins[i].orig_proc);
	    m_winamp_modern_wins[i].hwnd = 0;
	    m_winamp_modern_wins[i].orig_proc = 0;
	}
    }

//    m_dragging = FALSE;
//    m_docked = FALSE;
//    m_docked_side = 0;
    m_hwnd = NULL;

    return TRUE;
}

static void
notify_dock ()
{
#define DOCK_BUF_SIZE 4096
    int i, rc;
    RECT rtparents[WINAMP_MODERN_WINS];
    char buf[DOCK_BUF_SIZE];
    int bi;

    strcpy (buf, "Dock ");
    bi = strlen ("Dock ");
    buf[bi] = 0;
    if (m_skin_is_modern) {
	for (i = 0; i < m_num_modern_wins; i++) {
	    GetWindowRect (m_winamp_modern_wins[i].hwnd, &rtparents[i]);
	    rc = _snprintf (&buf[bi], DOCK_BUF_SIZE-bi, "%d %d %d %d %d %d\n", 
		m_winamp_modern_wins[i].hwnd,
		m_winamp_modern_wins[i].visible,
		rtparents[i].left,
		rtparents[i].top,
		rtparents[i].right,
		rtparents[i].bottom);
	    if (rc > 0) {
		bi += rc;
	    } else {
		buf[bi] = 0;
	    }
	}
    } else {
	for (i = 0; i < WINAMP_CLASSIC_WINS; i++) {
	    GetWindowRect (m_winamp_classic_wins[i].hwnd, &rtparents[i]);
	    rc = _snprintf (&buf[bi], DOCK_BUF_SIZE-bi, "%d %d %d %d %d %d\n", 
		m_winamp_classic_wins[i].hwnd,
		m_winamp_classic_wins[i].visible,
		rtparents[i].left,
		rtparents[i].top,
		rtparents[i].right,
		rtparents[i].bottom);
	    if (rc > 0) {
		bi += rc;
	    } else {
		buf[bi] = 0;
	    }
	}
    }
    if (bi > 0) {
	rc = write_pipe (buf);
	if (rc != 0) {
	    dll_quit();
	}
    }
}

/* If path found, url is filled in.  If not, url[0] is set to 0 */
void
winamp_poll (char url[MAX_URL_LEN])
{
    HWND hwnd_winamp;
    int useoldway = 0;

    url[0] = 0;
    hwnd_winamp = winamp_get_hwnd ();

    if (useoldway) {
	// Send a message to winamp to save the current playlist
	// to a file, 'n' is the index of the currently selected item
	int n  = SendMessage (hwnd_winamp, WM_USER, (WPARAM)NULL, 120); 
	char m3u_path[SR_MAX_PATH];
	char buf[4096] = {'\0'};
	FILE *fp;

	/* Get winamp path */
	if (!m_winamp_path[0])
	    return;

	sprintf (m3u_path, "%s%s", m_winamp_path, "winamp.m3u");	
	if ((fp = fopen (m3u_path, "r")) == NULL)
	    return;

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
	    strcpy (url, buf);
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

	if (fname) {
	    if (purl = strstr (fname, "http://")) {
		strncpy (url, purl, MAX_URL_LEN);
		return;
	    }
	    if (purl = strstr (fname, "icyx://")) {
		strncpy (url, purl, MAX_URL_LEN);
		return;
	    }
	}
    }
}
