/* plugin_main.c
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
#include "sr_config.h"
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <windows.h>
#include "resource.h"
#include "callback.h"
#include "debug_box.h"
#include "srtypes.h"
#include "rip_manager.h"
#include "winamp_exe.h"
#include "gen.h"
#include "options.h"
#include "shellapi.h"
#include "wstreamripper.h"
#include "render.h"
#include "render_2.h"
#include "mchar.h"
#include "dock.h"
#include "filelib.h"
#include "commctrl.h"
#include "debug.h"
#include "errors.h"

#define ID_TRAY	1

/* GCS -- why? */
#undef MAX_FILENAME
#define MAX_FILENAME	1024

#define WINDOW_WIDTH		276
#define WINDOW_HEIGHT		150
#define WM_MY_TRAY_NOTIFICATION WM_USER+0

static int  init();
static void quit();
static BOOL CALLBACK WndProc(HWND hwnd,UINT umsg,WPARAM wParam,LPARAM lParam);
static void rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data);
static void populate_history_popup (void);
static void insert_riplist (char* url, int pos);
static void launch_pipe_threads (void);
static void handle_wm_app (HWND hwnd, WPARAM wParam, LPARAM lParam);

/* True globals */
int			g_running_standalone;
STREAM_PREFS		g_rmo;
HWND			g_winamp_hwnd;
WSTREAMRIPPER_PREFS	g_gui_prefs;

static HINSTANCE                m_hinstance;
static RIP_MANAGER_INFO		*m_rmi = 0;
static HWND			m_hwnd;
static BOOL			m_bRipping = FALSE;
static TCHAR 			m_szToopTip[] = "Streamripper For Winamp";
static NOTIFYICONDATA		m_nid;
static char			m_output_dir[MAX_FILENAME];
static char			m_szWindowClass[] = "sripper";
static HMENU			m_hmenu_systray = NULL;
static HMENU			m_hmenu_systray_sub = NULL;
static HMENU			m_hmenu_context = NULL;
static HMENU			m_hmenu_context_sub = NULL;
// a hack to make sure the options dialog is not
// open if the user trys to disable streamripper
static BOOL			m_doing_options_dialog = FALSE;
// Ugh, this is awful.  Cache the stream url when requesting 
// from winamp, so that we can detect when there was a change
static char m_winamp_stream_cache[MAX_URL_LEN] = "0";

static HANDLE m_hpipe_exe_read = NULL;
static HANDLE m_hpipe_exe_write = NULL;

int
init ()
{
    WNDCLASS wc;
    char* sr_debug_env;
    int rc;

    sr_debug_env = getenv ("STREAMRIPPER_DEBUG");
    if (sr_debug_env) {
	debug_enable();
	debug_set_filename (sr_debug_env);
    }

    winamp_init (m_hinstance);

    rc = prefs_load ();
    prefs_get_stream_prefs (&g_rmo, "");
    if (rc == 0) {
	options_get_desktop_folder (g_rmo.output_directory);
    }
    prefs_get_wstreamripper_prefs (&g_gui_prefs);
    prefs_save ();

    /* GCS FIX */
    //m_guiOpt.m_enabled = 1;
    //debug_printf ("Checking if enabled.\n");
    //if (!m_guiOpt.m_enabled)
    //	return 0;
    //debug_printf ("Was enabled.\n");

    memset (&wc,0,sizeof(wc));
    wc.lpfnWndProc = WndProc;			// our window procedure
    wc.hInstance = m_hinstance;
    wc.lpszClassName = m_szWindowClass;		// our window class name
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);

    // Load systray popup menu
    m_hmenu_systray = LoadMenu (m_hinstance, MAKEINTRESOURCE(IDR_TASKBAR_POPUP));
    m_hmenu_systray_sub = GetSubMenu (m_hmenu_systray, 0);
    SetMenuDefaultItem (m_hmenu_systray_sub, 0, TRUE);

    if (!RegisterClass(&wc)) {
	MessageBox (NULL,"Error registering window class","blah",MB_OK);
	return 1;
    }

    /* Ref: http://msdn2.microsoft.com/en-us/library/bb776822.aspx */
    if (g_running_standalone) {
	m_hwnd = CreateWindowEx (
			WS_EX_APPWINDOW,
			m_szWindowClass, "Streamripper Plugin", WS_POPUP,
			g_gui_prefs.oldpos_x, g_gui_prefs.oldpos_y, WINDOW_WIDTH, WINDOW_HEIGHT, 
			NULL, NULL, m_hinstance, NULL);
    } else {
	m_hwnd = CreateWindowEx (
			WS_EX_TOOLWINDOW,
			m_szWindowClass, "Streamripper Plugin", WS_POPUP,
			g_gui_prefs.oldpos_x, g_gui_prefs.oldpos_y, WINDOW_WIDTH, WINDOW_HEIGHT, 
			NULL, NULL, m_hinstance, NULL);
    }

    // Create a systray icon
    memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hIcon = LoadImage(m_hinstance, MAKEINTRESOURCE(IDI_SR_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    m_nid.hWnd = m_hwnd;
    strcpy(m_nid.szTip, m_szToopTip);
    m_nid.uCallbackMessage = WM_MY_TRAY_NOTIFICATION;
    m_nid.uFlags =  NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uID = 1;
    Shell_NotifyIcon(NIM_ADD, &m_nid);

    // Load main popup menu 
    m_hmenu_context = LoadMenu (m_hinstance, MAKEINTRESOURCE(IDR_HISTORY_POPUP));
    m_hmenu_context_sub = GetSubMenu (m_hmenu_context, 0);
    SetMenuDefaultItem (m_hmenu_context_sub, 0, TRUE);

    // Populate main popup menu
    populate_history_popup ();

    if (!g_gui_prefs.m_start_minimized)
	dock_show_window(m_hwnd, SW_SHOWNORMAL);
    else
	dock_show_window(m_hwnd, SW_HIDE);
	
    return 0;
}

void
quit()
{
    debug_printf ("Quitting.\n");

    prefs_set_stream_prefs (&g_rmo, "stream defaults");
    prefs_set_wstreamripper_prefs (&g_gui_prefs);
    prefs_save ();

    CloseHandle (m_hpipe_exe_read);
    CloseHandle (m_hpipe_exe_write);

    if (m_bRipping)
	rip_manager_stop (m_rmi);

    rip_manager_cleanup ();

    debug_printf ("Going to render_destroy()...\n");
    render_destroy();
    debug_printf ("Going to DestroyWindow()...\n");
    DestroyWindow(m_hwnd);
    Shell_NotifyIcon(NIM_DELETE, &m_nid);
    DestroyIcon(m_nid.hIcon);
    debug_printf ("Going to UnregisterClass()...\n");
    UnregisterClass(m_szWindowClass, m_hinstance); // unregister window class
    debug_printf ("Finished UnregisterClass()\n");
}

void
start_button_enable()
{
    render_start_button_enable ();
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STARTRIPPING, MF_ENABLED);
}

void
start_button_disable()
{
    render_start_button_disable ();
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STARTRIPPING, MF_DISABLED|MF_GRAYED);
}

void
stop_button_enable()
{
    render_stop_button_enable ();
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STOPRIPPING, MF_ENABLED);
}

void
stop_button_disable()
{
    render_stop_button_disable ();
    EnableMenuItem(m_hmenu_systray_sub, ID_MENU_STOPRIPPING, MF_DISABLED|MF_GRAYED);
}

void
compose_relay_url (char* relay_url, char *host, u_short port, int content_type)
{
    if (content_type == CONTENT_TYPE_OGG) {
	sprintf (relay_url, "http://%s:%d/.ogg", host, port);
    } else if (content_type == CONTENT_TYPE_NSV) {
	sprintf (relay_url, "http://%s:%d/;stream.nsv", host, port);
    } else {
	sprintf (relay_url, "http://%s:%d", host, port);
    }
}

BOOL
url_is_relay (char* url)
{
    char relay_url[SR_MAX_PATH];
#if defined (commentout)
    debug_printf ("Trying to get_content_type %p\n", m_rmi);
    rip_manager_get_content_type (m_rmi);
    debug_printf ("Trying to compose_relay_url\n");
    compose_relay_url (relay_url, g_gui_prefs.localhost, 
			g_rmo.relay_port, 
			rip_manager_get_content_type (m_rmi));
#endif
    compose_relay_url (relay_url, g_gui_prefs.localhost, 
			g_rmo.relay_port, 
			CONTENT_TYPE_MP3);
    debug_printf ("Comparing %s vs rly %s\n", url, relay_url);
    return (!strncmp(relay_url, url, strlen(relay_url)));
}

BOOL
url_is_stream (char* url)
{
    return (strchr(url, ':') != 0);
}

void
set_ripping_url (char* url)
{
    if (url) {
	debug_printf ("IDR_STREAMNAME:Press start to rip %s\n", url);
	render_set_display_data (IDR_STREAMNAME, "Press start to rip %s", url);
	start_button_enable ();
    } else {
	debug_printf ("IDR_STREAMNAME:No stream loaded\n");
	render_set_display_data (IDR_STREAMNAME, "No stream loaded");
	start_button_disable ();
    }
}

void
add_url_from_winamp (char* url)
{
    debug_printf ("AUFW got winamp stream: %s\n", url);
    if (!strcmp (url, m_winamp_stream_cache)) {
	debug_printf ("AUFW return - cached\n");
	return;
    }
    strcpy (m_winamp_stream_cache, url);
    
    if (!url_is_stream(url) || url_is_relay (url)) {
	if (g_rmo.url[0]) {
	    set_ripping_url (g_rmo.url);
	} else {
	    set_ripping_url (0);
	}
    } else {
	strcpy(g_rmo.url, url);
	insert_riplist (url, 0);
	set_ripping_url (url);
    }
}

BOOL CALLBACK
load_url_dialog_proc (HWND hwndDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    char url[MAX_URL_LEN];

    switch (umsg)
    {
    case WM_INITDIALOG:
	/* Set focus to edit control */
	SendMessage (hwndDlg, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hwndDlg, IDC_LOAD_URL_EDIT), TRUE);
	break;
    case WM_COMMAND:
	switch(LOWORD(wParam))
	{
	case IDOK:
	    GetDlgItemText (hwndDlg, IDC_LOAD_URL_EDIT, url, MAX_URL_LEN);
	    insert_riplist (url, 0);
	    strcpy(g_rmo.url, url);
	    set_ripping_url (url);
	    EndDialog(hwndDlg, 0);
	case IDCANCEL:
	    EndDialog(hwndDlg, 0);
	    break;
	}
	break;
    case WM_CLOSE:
	EndDialog(hwndDlg, 0);
	break;
    }

    return 0;
}

void
open_load_url_dialog ()
{
    int rc;
    rc = DialogBox (m_hinstance, 
	       MAKEINTRESOURCE (IDD_LOAD_URL), 
	       m_hwnd,
	       load_url_dialog_proc);
}

void
UpdateNotRippingDisplay (HWND hwnd)
{
#if defined (commentout)
    WINAMP_INFO winfo;

    // debug_printf ("UNRD begin\n");
    if (winamp_get_info (&winfo, m_guiOpt.use_old_playlist_ret)) {
        debug_printf ("UNRD got winamp stream: %s\n", winfo.url);
	if (!strcmp (winfo.url, m_winamp_stream_cache)) {
	    debug_printf ("UNRD return - cached\n");
	    return;
	}
	strcpy (m_winamp_stream_cache, winfo.url);
	
	if (!url_is_stream(winfo.url) || url_is_relay (winfo.url)) {
	    debug_printf ("UNRD not_stream/is_relay: %d\n", g_rmo.url[0]);
	    if (g_rmo.url[0]) {
		set_ripping_url (g_rmo.url);
	    } else {
		set_ripping_url (0);
	    }
	} else {
	    debug_printf ("UNRD setting g_rmo.url: %s\n", g_rmo.url);
	    strcpy(g_rmo.url, winfo.url);
	    insert_riplist (winfo.url, 0);
	    set_ripping_url (winfo.url);
	}
    }
#endif
}

void
UpdateRippingDisplay ()
{
    static int buffering_tick = 0;
    char sStatusStr[50];

    if (m_rmi->status == 0)
	return;

    switch(m_rmi->status)
    {
    case RM_STATUS_BUFFERING:
	buffering_tick++;
	if (buffering_tick == 30)
	    buffering_tick = 0;
	strcpy(sStatusStr,"Buffering... ");
	break;
    case RM_STATUS_RIPPING:
	strcpy(sStatusStr, "Ripping...    ");
	break;
    case RM_STATUS_RECONNECTING:
	strcpy(sStatusStr, "Re-connecting..");
	break;
    default:
	debug_printf("************ what am i doing here?");
    }
    render_set_display_data(IDR_STATUS, "%s", sStatusStr);

    if (!m_rmi->streamname[0]) {
	return;
    }

    debug_printf ("IDR_STREAMNAME:%s\n", m_rmi->streamname);
    render_set_display_data(IDR_STREAMNAME, "%s", m_rmi->streamname);
    render_set_display_data(IDR_BITRATE, "%dkbit", m_rmi->bitrate);
    render_set_display_data(IDR_SERVERTYPE, "%s", m_rmi->server_name);

    if ((m_rmi->meta_interval == -1) && 
	(strstr(m_rmi->server_name, "Nanocaster") != NULL))
    {
	render_set_display_data (IDR_METAINTERVAL, "Live365 Stream");
    } else if (m_rmi->meta_interval) {
	render_set_display_data (IDR_METAINTERVAL, "MetaInt:%d", m_rmi->meta_interval);
    } else {
	render_set_display_data (IDR_METAINTERVAL, "No track data");
    }

#if defined (commentout)
    /* GCS FIX! */
    if (m_rmi->filename[0]) {
	char strsize[50];
	format_byte_size(strsize, m_rmi->filesize);
	render_set_display_data(IDR_FILENAME, "[%s] - %s", strsize, m_rmi->filename);
    } else {
	render_set_display_data(IDR_FILENAME, "Getting track data...");
    }
#endif
    render_set_display_data(IDR_FILENAME, "Sorry.  Track data disabled.");
}

VOID CALLBACK
UpdateDisplay(HWND hwnd, UINT umsg, UINT_PTR idEvent, DWORD dwTime)
{

    if (m_bRipping)
	UpdateRippingDisplay();
    else
	UpdateNotRippingDisplay(hwnd);

    debug_printf ("*** Invalidating rectangle %d\n", m_bRipping);
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void
rip_callback (RIP_MANAGER_INFO* rmi, int message, void *data)
{
    ERROR_INFO *err;
    switch(message)
    {
    case RM_UPDATE:
#if defined (commentout)
	info = (RIP_MANAGER_INFO*)data;
	memcpy (&m_rmiInfo, info, sizeof(RIP_MANAGER_INFO));
#endif
	break;
    case RM_ERROR:
	err = (ERROR_INFO*) data;
	debug_printf("***RipCallback: about to post error dialog: %s\n", err->error_str);
	MessageBox (m_hwnd, err->error_str, "Streamripper", MB_SETFOREGROUND);
	debug_printf("***RipCallback: done posting error dialog\n");
	break;
    case RM_DONE:
	//stop_button_pressed();
	//
	// calling the stop button in here caused all kinds of stupid problems
	// so, fuck it. call the clearing button shit here. 
	//
	debug_printf("***RipCallback: RM_DONE\n");
	render_clear_all_data();
	m_bRipping = FALSE;
	set_ripping_url (m_winamp_stream_cache);
	start_button_enable();
	stop_button_disable();
	render_set_prog_bar(FALSE);

	break;
    case RM_TRACK_DONE:
	if (g_gui_prefs.m_add_finished_tracks_to_playlist)
	    winamp_add_track_to_playlist((char*)data);
	break;
    case RM_NEW_TRACK:
	break;
    case RM_STARTED:
	stop_button_enable();
	break;
#if defined (commentout)
    /* GCS: This no longer applies as of adding the filename patterns */
    case RM_OUTPUT_DIR:
	strcpy(m_output_dir, (char*)data);
#endif
    }
}

void
start_button_pressed (void)
{
    int ret;
    debug_printf ("Start button pressed\n");

    assert(!m_bRipping);
    render_clear_all_data();
    debug_printf ("IDR_STREAMNAME:Connecting...\n");
    render_set_display_data(IDR_STREAMNAME, "Connecting...");
    start_button_disable();

    ret = rip_manager_start (&m_rmi, &g_rmo, rip_callback);
    insert_riplist (g_rmo.url, 0);

    if (ret != SR_SUCCESS) {
	MessageBox (m_hwnd, errors_get_string (ret),
		    "Failed to connect to stream", MB_ICONSTOP);
	start_button_enable();
	return;
    }
    m_bRipping = TRUE;

    render_set_prog_bar (TRUE);
    PostMessage (m_hwnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
}

void
stop_button_pressed (void)
{
    debug_printf ("Stop button pressed\n");

    stop_button_disable();
    assert(m_bRipping);
    render_clear_all_data();

    rip_manager_stop (m_rmi);

    m_bRipping = FALSE;
    start_button_enable();
    stop_button_disable();
    set_ripping_url(g_rmo.url);
    render_set_prog_bar(FALSE);
}

void
options_button_pressed (void)
{
    debug_printf ("Options button pressed\n");

    m_doing_options_dialog = TRUE;
    options_dialog_show (m_hinstance, m_hwnd);

    if (OPT_FLAG_ISSET (g_rmo.flags, OPT_MAKE_RELAY)) {
	render_relay_button_enable();
    } else {
	render_relay_button_disable();
    }
    m_doing_options_dialog = FALSE;
}

void
close_button_pressed (void)
{
    dock_show_window (m_hwnd, SW_HIDE);
    g_gui_prefs.m_start_minimized = TRUE;
}

void
relay_pressed (void)
{
    if (m_rmi) {
	winamp_add_relay_to_playlist (g_gui_prefs.localhost, 
					g_rmo.relay_port, 
					rip_manager_get_content_type (m_rmi));
    }
}

void
debug_riplist (void)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	debug_printf ("riplist%d=%s\n", i, g_gui_prefs.riplist[i]);
    }
}

/* return -1 if not in riplist */
int
find_url_in_riplist (char* url)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	if (!strcmp(g_gui_prefs.riplist[i],url)) {
	    return i;
	}
    }
    return -1;
}

/* pos is 0 to put at top, or 1 to put next to top */
void
insert_riplist (char* url, int pos)
{
    int i;
    int oldpos;

    debug_printf ("Insert riplist (0)\n");

    /* Don't add if it's the relay stream */
    if (url_is_relay (url)) return;

    debug_printf ("Insert riplist (1): %d %s\n", pos, url);
    debug_riplist ();

    /* oldpos is previous position of this url. Don't shift if 
       it's already at that position or above. */
    oldpos = find_url_in_riplist (url);
    if (oldpos == -1) {
	oldpos = RIPLIST_LEN - 1;
    }
    if (oldpos <= pos) return;

    /* Shift the url to the correct position */
    for (i = oldpos; i > pos; i--) {
	strcpy(g_gui_prefs.riplist[i], g_gui_prefs.riplist[i-1]);
    }
    strcpy(g_gui_prefs.riplist[pos], url);

    debug_printf ("Insert riplist (2): %d %s\n", pos, url);
    debug_riplist ();

    /* Rebuild the history menu */
    populate_history_popup ();
}

void
populate_history_popup (void)
{
    int i;
    for (i = 0; i < RIPLIST_LEN; i++) {
	RemoveMenu (m_hmenu_context_sub, ID_MENU_HISTORY_LIST+i, MF_BYCOMMAND);
    }
    for (i = 0; i < RIPLIST_LEN; i++) {
	if (g_gui_prefs.riplist[i][0]) {
	    AppendMenu (m_hmenu_context_sub, MF_ENABLED | MF_STRING, ID_MENU_HISTORY_LIST+i, g_gui_prefs.riplist[i]);
	}
    }
}

BOOL CALLBACK
WndProc (HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hBrush = NULL;

    switch (umsg)
    {
    case WM_CREATE:
	if (!render_init (hwnd, g_gui_prefs.default_skin)) {
	    MessageBox(hwnd, "Failed to find the skin bitmap", "Error", 0);
	    break;
	}
	stop_button_disable();
	if (OPT_FLAG_ISSET(g_rmo.flags, OPT_MAKE_RELAY)) {
	    render_relay_button_enable ();
	} else {
	    render_relay_button_disable ();
	}

	SetTimer (hwnd, 1, 500, (TIMERPROC)UpdateDisplay);
	dock_init (hwnd);

	return 0;

    case WM_PAINT:
	{
	    PAINTSTRUCT pt;
	    HDC hdc = BeginPaint(hwnd, &pt);
	    render_do_paint(hdc);
	    EndPaint(hwnd, &pt);
	}
	return 0;
		
    case WM_MOUSEMOVE:
	render_do_mousemove (hwnd, wParam, lParam);
	dock_do_mousemove (hwnd, wParam, lParam);
	break;

    case WM_COMMAND:
	switch(wParam)
	{
	case ID_MENU_STARTRIPPING:
	    start_button_pressed();
	    break;
	case ID_MENU_STOPRIPPING:
	    stop_button_pressed();
	    break;
	case ID_MENU_OPTIONS:
	    options_button_pressed();
	    break;
	case ID_MENU_OPEN:
	    PostMessage(hwnd, WM_MY_TRAY_NOTIFICATION, (WPARAM)NULL, WM_LBUTTONDBLCLK);
	    break;
	case ID_MENU_RESET_URL:
	    strcpy(g_rmo.url, "");
	    set_ripping_url (0);
	    break;
	case ID_MENU_LOAD_URL:
	    debug_printf ("Load URL dialog box\n");
	    open_load_url_dialog ();
	    break;
	case ID_MENU_EXIT:
	    debug_printf ("User requested exit\n");
	    quit ();
	    PostQuitMessage( 0 );
	    break;
	default:
	    if (wParam >= ID_MENU_HISTORY_LIST && wParam < ID_MENU_HISTORY_LIST + RIPLIST_LEN) {
		int i = wParam - ID_MENU_HISTORY_LIST;
		char* url = g_gui_prefs.riplist[i];
		debug_printf ("Setting URL through history list\n");
		strcpy(g_rmo.url, url);
		set_ripping_url (url);
	    }
	    break;
	}
	break;

    case WM_MY_TRAY_NOTIFICATION:
	switch(lParam)
	{
	case WM_LBUTTONDBLCLK:
	    dock_show_window(m_hwnd, SW_NORMAL);
	    SetForegroundWindow(hwnd);
	    g_gui_prefs.m_start_minimized = FALSE;
	    break;

	case WM_RBUTTONDOWN:
	    {
		int item;
		POINT pt;
		GetCursorPos(&pt);
		SetForegroundWindow(hwnd);
		item = TrackPopupMenu(m_hmenu_systray_sub, 
				      0,
				      pt.x,
				      pt.y,
				      (int)NULL,
				      hwnd,
				      NULL);
	    }
	    break;
	}
	break;

    case WM_LBUTTONDOWN:
	dock_do_lbuttondown(hwnd, wParam, lParam);
	render_do_lbuttondown(hwnd, wParam, lParam);
	break;

    case WM_LBUTTONUP:
	dock_do_lbuttonup(hwnd, wParam, lParam);
	render_do_lbuttonup(hwnd, wParam, lParam);
	{
	    BOOL rc;
	    RECT rt;
	    rc = GetWindowRect(hwnd, &rt);
	    if (rc) {
		g_gui_prefs.oldpos_x = rt.left;
		g_gui_prefs.oldpos_y = rt.top;
	    }
	}
	break;
	
    case WM_RBUTTONDOWN:
	{
	    int item;
	    POINT pt;
	    if (!m_bRipping) {
		GetCursorPos (&pt);
		SetForegroundWindow (hwnd);
		item = TrackPopupMenu (m_hmenu_context_sub, 
					0,
					pt.x,
					pt.y,
					(int)NULL,
					hwnd,
					NULL);
	    }
	}
	break;
	
    case WM_APP+0:
	handle_wm_app (hwnd, wParam, lParam);
	break;
    case WM_APP+1:
	/* Exit request from thread */
	quit ();
	PostQuitMessage( 0 );
	break;
    case WM_DESTROY:
	debug_printf ("Got WM_DESTROY\n");
	PostQuitMessage( 0 );
	break;
    case WM_QUIT:
	debug_printf ("Got WM_QUIT\n");
	break;
    }
    return DefWindowProc (hwnd, umsg, wParam, lParam);
}

void
handle_wm_app (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam) {
    case 0:
	/* Window moved */
	dock_update_winamp_wins (hwnd, (char*) lParam);
	break;
    case 1:
	/* New URL */
	add_url_from_winamp ((char*) lParam);
	break;
    case 2:
	/* Window minimized/restored */
	dock_resize (hwnd, (char*) lParam);
	break;
    }
}

static void
display_last_error (void)
{
    char buf[1023];
    LPVOID lpMsgBuf;
    FormatMessage( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
    );
    _snprintf (buf, 1023, "Error: %s\n", lpMsgBuf);
    buf[1022] = 0;
    MessageBox (NULL, buf, "SR EXE Error", MB_OK);
    LocalFree( lpMsgBuf );
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPSTR lpCmdLine, int nCmdShow)
{
    int rc;
    MSG msg;
    int arg1, arg2;
    int exit_code;

    debug_box ("Hello world");

    m_hinstance = hInstance;

    sr_set_locale ();

    /* Gotta split lpCmdLine manually for win98 compatibility */
    rc = sscanf (lpCmdLine, "%d %d", &arg1, &arg2);
    if (rc == 2) {
	char buf[1024];
	sprintf (buf, "%d %d", arg1, arg2);
        //MessageBox (NULL, buf, "SR EXE", MB_OK);
	g_running_standalone = 0;
	g_winamp_hwnd = (HWND) NULL;
	m_hpipe_exe_read = (HANDLE) arg1;
	m_hpipe_exe_write = (HANDLE) arg2;
    } else {
	char buf[1024];
	sprintf (buf, "NUM ARG = %d", rc);
        //MessageBox (NULL, buf, "SR EXE", MB_OK);
	g_running_standalone = 1;
	g_winamp_hwnd = (HWND) NULL;
	m_hpipe_exe_read = (HANDLE) NULL;
	m_hpipe_exe_write = (HANDLE) NULL;
    }

    rip_manager_init ();

    init ();

    debug_printf ("command line args: %d %d %d\n",
	    g_running_standalone, m_hpipe_exe_read,
	    m_hpipe_exe_write);

    if (m_hpipe_exe_read) {
	launch_pipe_threads ();
    }

    exit_code = 0;
    while(1) {
	rc = GetMessage (&msg, NULL, 0, 0);
	if (rc == -1) {
	    // handle the error and possibly exit
	    break;
	} else {
	    if (msg.message == WM_QUIT) {
		debug_printf ("Got a WM_QUIT in message loop\n");
		exit_code = msg.wParam;
		break;
	    }
	    if (msg.message == WM_DESTROY) {
		debug_printf ("Got a WM_DESTROY in message loop\n");
	    }
	    TranslateMessage (&msg);
	    DispatchMessage (&msg);
	}
    }

    debug_printf ("Fell through WinMain()\n");

    return exit_code;
}

static void
pipe_reader (void* arg)
{
    static char msgbuf[1024];
    static int idx = 0;
    BOOL rc;

    int num_read;
    while (1) {
	rc = ReadFile (m_hpipe_exe_read, &msgbuf[idx], 1, &num_read, 0);
	if (rc == 0) {
	    /* GCS FIX: How to distinguish error condition from winamp exit? */
	    //display_last_error ();
	    debug_printf ("Pipe read failed\n");
	    SendMessage (m_hwnd, WM_APP+1, 0, 0);
	    //quit ();
	    //exit (1);
	}
	if (num_read == 1) {
	    if (msgbuf[idx] == '\x03') {
		/* Got a message */
		msgbuf[idx] = 0;
		debug_printf ("Got message from pipe: %s\n", msgbuf);
		idx = 0;
		if (!strcmp (msgbuf, "Hello World")) {
		    /* do nothing */
		} else if (!strncmp (msgbuf, "url ", 4)) {
		    /* URL from winamp */
		    SendMessage (m_hwnd, WM_APP+0, 1, (LPARAM) &msgbuf[4]);
		} else if (!strncmp (msgbuf, "Dock ", 5)) {
		    /* Window moved */
		    SendMessage (m_hwnd, WM_APP+0, 0, (LPARAM) &msgbuf[5]);
		} else if (!strncmp (msgbuf, "Resize ", 7)) {
		    /* Window moved */
		    SendMessage (m_hwnd, WM_APP+0, 2, (LPARAM) &msgbuf[7]);
		} else {
		    /* Unknown message */
		    /* do nothing */
		}
	    } else {
		idx ++;
	    }
	}
	if (idx == 1024) {
	    debug_printf ("Pipe overflow\n");
	    exit (1);
	}
    }
}

static void
launch_pipe_threads (void)
{
    _beginthread ((void*) pipe_reader, 0, 0);
}
