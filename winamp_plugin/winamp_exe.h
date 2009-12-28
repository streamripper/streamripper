#ifndef __WINAMP_H__
#define __WINAMP_H__

#include "srtypes.h"

//#define SKIN_PATH "Skins\\SrSkins\\"
#define SKIN_PATH "Skins\\"

typedef struct WINAMP_INFOst
{
	char url[MAX_URL_LEN];
	BOOL is_running;
} WINAMP_INFO;

BOOL winamp_init();
BOOL winamp_add_track_to_playlist(char *track);
BOOL winamp_get_info(WINAMP_INFO *info, BOOL useoldway);
BOOL winamp_get_path(char *path);
BOOL winamp_add_relay_to_playlist(char *host, u_short port, int m_content_type);


#endif //__WINAMP_H__