#ifndef __DSP_SRIPPER_H__
#define __DSP_SRIPPER_H__

#include "srtypes.h"
#include "rip_manager.h"
#include "prefs.h"
#include "gen.h"


/* COMMAND ID's 40010 through 40019 are reserved for the history list
   Make sure these are not used by other menu items or commands!  */
#define ID_MENU_HISTORY_LIST	    40010

typedef struct GUI_OPTIONSst
{
    BOOL	m_add_finshed_tracks_to_playlist;
    BOOL	m_start_minimized;
    POINT	oldpos;
    BOOL	m_enabled;
    char	localhost[MAX_HOST_LEN];	// hostname of 'localhost' 
    char	default_skin[MAX_PATH];
    BOOL	use_old_playlist_ret;
    char	riplist[RIPLIST_LEN][MAX_URL_LEN];
} GUI_OPTIONS;

/* Global variable */
extern STREAM_PREFS g_rmo;
extern HWND g_winamp_hwnd;
extern WSTREAMRIPPER_PREFS g_gui_prefs;

/* Public functions */
void compose_relay_url (char* relay_url, char *host, u_short port, int content_type);

#endif //__DSP_SRIPPER_H__
