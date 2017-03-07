/* prefs.c
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
 *
 * REFS
 * http://www.gtkbook.com/tutorial.php?page=keyfile
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "rip_manager.h"
#include "mchar.h"
#include "prefs.h"
#include "debug.h"

/******************************************************************************
 * Private variables
 *****************************************************************************/
#define DEFAULT_USER_AGENT ("streamripper/" SRVERSION)
static GKeyFile *m_key_file = NULL;

enum PrefsVersion {
    PREFS_VERSION_1_63_BETA_2,
    PREFS_VERSION_1_63_BETA_8,
    PREFS_VERSION_1_63_4,
    PREFS_VERSION_1_64_5
};

static const char* prefs_version_strings[] = {
    "",		         /* None = "1.63-beta-2" */
    "1.63-beta-8",       /* Change dropcount from 0 to 1 */
    "1.63.4",            /* Change overwrite from larger to version */
    "1.64.5",            /* Change localost from localhost to 127.0.0.1
			    Change id3 codeset to UTF-16
			    Change metadata and relay codesets from UTF-8 
			    to iso-8859-1
			    Change splitpoint padding from 300 to 0. */
    0
};

/* Preference file versions */
#define PREFS_VERSION_CURRENT "1.64.5"


/******************************************************************************
 * Private function protoypes
 *****************************************************************************/
static enum PrefsVersion string_to_prefs_version (char* str);
static gchar* prefs_get_config_dir (void);
static void prefs_get_global_defaults (GLOBAL_PREFS* global_prefs);

static void prefs_get_stream_defaults (STREAM_PREFS* prefs);
static void prefs_get_stream_prefs_keyfile (STREAM_PREFS* prefs, char* group);
static void prefs_set_stream_prefs_keyfile (STREAM_PREFS* prefs, 
					    STREAM_PREFS* default_prefs, 
					    char* group);

static void prefs_get_wstreamripper_defaults (WSTREAMRIPPER_PREFS* prefs);

static int prefs_get_string (char* dest, gsize dest_size, char* group, char* key);
static int prefs_get_int (int *dest, char *group, char *key);
static int prefs_get_ulong (u_long *dest, char *group, char *key);
static int prefs_get_long (long *dest, char *group, char *key);
static void prefs_set_string (char* group, char* key, char* value);
static void prefs_set_integer (char* group, char* key, gint value);

/******************************************************************************
 * Private Vars
 *****************************************************************************/

/******************************************************************************
 * Public functions
 *****************************************************************************/
/* Return 0 if the user doesn't have a preference file yet, 
   and non-zero if he does.  This is used by the winamp plugin 
   to override the default output directory. */
int
prefs_load (void)
{
    gboolean rc;
    GKeyFileFlags flags;
    GError *error = NULL;
    gchar* prefs_dir;
    gchar* prefs_fn;
    GLOBAL_PREFS global_prefs;

    prefs_dir = prefs_get_config_dir ();
    prefs_fn = g_build_filename (prefs_dir,
				 "streamripper.ini",
				 NULL);
    debug_printf ("prefs_fn [utf8] = %s\n", prefs_fn);

    if (!m_key_file) {
	debug_printf ("Trying g_key_file_new()...\n");
	m_key_file = g_key_file_new ();
	if (!m_key_file) {
	    debug_printf ("Error creating key file with g_key_file_new()\n");
	}
    }
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
    rc = g_key_file_load_from_file (m_key_file, prefs_fn,
				    flags, &error);
    debug_printf ("g_key_file_load_from_file returned %d\n", rc);
    if (error) {
	debug_printf ("Error with g_key_file_load_from_data: %s\n",
		error->message);
	g_error_free (error);
    }

    /* Make sure a full set of global preferences are created in keyfile */
    prefs_get_global_prefs (&global_prefs);
    prefs_set_global_prefs (&global_prefs);

    /* Silently update preferences on version change */
    if (rc) {
	int rc1;
	int padding;
	u_long tmp_u_long;
	char overwrite_str[128];
	char codeset_str[MAX_CODESET_STRING];
	char localhost_str[SR_MAX_PATH];

	debug_printf ("Updating prefs %d\n", 
		      string_to_prefs_version (global_prefs.version));
	switch (string_to_prefs_version (global_prefs.version)) {
	case PREFS_VERSION_1_63_BETA_2:
	    /* Silently update dropcount */
	    rc1 = prefs_get_ulong (&tmp_u_long, "stream defaults", "dropcount");
	    if (!rc1 || tmp_u_long == 0) {
		prefs_set_integer ("stream defaults", "dropcount", 1);
	    }
	    /* Fall through */
	case PREFS_VERSION_1_63_BETA_8:
	    /* Silently update overwrite behavior */
	    rc1 = prefs_get_string (overwrite_str, 128, "stream defaults",
				   "over_write_complete");
	    if (!rc1 || !strcmp (overwrite_str, "larger")) {
		prefs_set_string ("stream defaults", "over_write_complete", 
				  "version");
	    }
	    /* Fall through */
	case PREFS_VERSION_1_63_4:
	    /* Silently update localhost to 127.0.0.1 */
	    rc1 = prefs_get_string (localhost_str, SR_MAX_PATH,
				    "wstreamripper", "localhost");
	    if (!rc1 || !strcmp (overwrite_str, "localhost")) {
		prefs_set_string ("wstreamripper", "localhost", "127.0.0.1");
	    }

	    /* Silently update id3 codeset to UTF-16 for all values*/
	    prefs_set_string ("stream defaults", "codeset_id3", "UTF-16");

	    /* Silently update metadata and relay codeset if UTF-8 */
	    rc1 = prefs_get_string (codeset_str, MAX_CODESET_STRING, 
				    "stream defaults", "codeset_metadata");
	    if (!rc1 || !strcmp (codeset_str, "UTF-8")) {
		prefs_set_string ("stream defaults", "codeset_metadata",
				  "ISO-8859-1");
	    }
	    rc1 = prefs_get_string (codeset_str, MAX_CODESET_STRING, 
				    "stream defaults", "codeset_relay");
	    if (!rc1 || !strcmp (codeset_str, "UTF-8")) {
		prefs_set_string ("stream defaults", "codeset_relay",
				  "ISO-8859-1");
	    }
	    
	    /* Silently update padding */
	    prefs_get_int (&padding, "stream defaults", "xs_padding_1");
	    if (!rc1 || padding == 300) {
		prefs_set_integer ("stream defaults", "xs_padding_1", 0);
	    }
	    prefs_get_int (&padding, "stream defaults", "xs_padding_2");
	    if (!rc1 || padding == 300) {
		prefs_set_integer ("stream defaults", "xs_padding_2", 0);
	    }
	    
	    /* Silently update keep_incomplete */
	    prefs_set_integer ("stream defaults", "keep_incomplete", 1);

	    /* Fall through */
	case PREFS_VERSION_1_64_5:
	default:
	    /* Prefs version is up to date -- do nothing */
	    break;
	}
    }

    /* Update to current version */
    prefs_set_string ("sripper", "version", PREFS_VERSION_CURRENT);

    g_free (prefs_fn);
    g_free (prefs_dir);
    return (int) rc;
}

void
prefs_save (void)
{
    FILE* fp;
    GError *error = NULL;
    gchar* prefs_dir;
    gchar* prefs_fn;
    gchar* keyfile_contents;
    gsize keyfile_contents_len;

    debug_printf ("Saving preferences.\n");
    prefs_dir = prefs_get_config_dir ();
    if (g_mkdir_with_parents (prefs_dir, 0755)) {
	debug_printf ("Couldn't make config dir: %s\n", prefs_dir);
	g_free (prefs_dir);
	return;
    }

    prefs_fn = g_build_filename (prefs_dir,
				 "streamripper.ini",
				 NULL);
    g_free (prefs_dir);
    debug_printf ("Filename: %s\n", prefs_fn);

    /* Insert from prefs into keyfile */
    // prefs_copy_to_keyfile (prefs);
    
    /* Convert entire keyfile to a string */
    keyfile_contents = g_key_file_to_data (m_key_file, 
					   &keyfile_contents_len, 
					   &error);
    if (error) {
	debug_printf ("Error with g_key_file_to_data: %s\n",
		error->message);
	g_error_free (error);
	g_free (prefs_fn);
	return;
    }

    /* Write to file */
    /* RMK: We use glib string encoding for conversion of the filename
	rather than --codeset-filesys here. */
    fp = g_fopen (prefs_fn, "w");
    if (fp) {
	debug_printf ("Writing, len = %d\n", keyfile_contents_len);
	fwrite (keyfile_contents, 1, keyfile_contents_len, fp);
	fclose (fp);
    } else {
	debug_printf ("Error opening prefs file for write\n");
    }
    g_free (keyfile_contents);
    g_free (prefs_fn);
}

void
prefs_get_global_prefs (GLOBAL_PREFS *global_prefs)
{
    prefs_get_global_defaults (global_prefs);

    prefs_get_string (global_prefs->version, MAX_VERSION_LEN, "sripper", 
		      "version");
    prefs_get_string (global_prefs->url, MAX_URL_LEN, "sripper", "url");
    prefs_get_stream_prefs (&global_prefs->stream_prefs, "stream defaults");
}

void
prefs_set_global_prefs (GLOBAL_PREFS *global_prefs)
{
    /* Don't set version.  This is done in prefs_load() so we can 
       patch old versions of prefs. */
    prefs_set_string ("sripper", "version", global_prefs->version);
    prefs_set_string ("sripper", "url", global_prefs->url);
    prefs_set_stream_prefs_keyfile (&global_prefs->stream_prefs, 0, 
				    "stream defaults");
}

void
prefs_get_wstreamripper_prefs (WSTREAMRIPPER_PREFS *prefs)
{
    int i, p;
    char* group = "wstreamripper";

    prefs_get_wstreamripper_defaults (prefs);

    if (!m_key_file) return;

    prefs_get_string (prefs->default_skin, SR_MAX_PATH, group, "default_skin");
    prefs_get_int (&prefs->m_enabled, group, "enabled");
    prefs_get_long (&prefs->oldpos_x, group, "window_x");
    prefs_get_long (&prefs->oldpos_y, group, "window_y");
    prefs_get_string (prefs->localhost, SR_MAX_PATH, group, "localhost");
    prefs_get_int (&prefs->m_add_finished_tracks_to_playlist, group, "add_tracks_to_playlist");
    prefs_get_int (&prefs->m_start_minimized, group, "start_minimized");
    prefs_get_int (&prefs->use_old_playlist_ret, group, "use_old_playlist_ret");

    /* Get history */
    for (i = 0, p = 0; i < RIPLIST_LEN; i++) {
	char profile_name[128];
	sprintf (profile_name, "riplist%d", i);
	prefs_get_string (prefs->riplist[p], SR_MAX_PATH, group, profile_name);
	if (prefs->riplist[p][0]) {
	    p++;
	}
    }
}

void
prefs_set_wstreamripper_prefs (WSTREAMRIPPER_PREFS *prefs)
{
    int i, p;
    char* group = "wstreamripper";

    if (!m_key_file) return;

    prefs_set_string (group, "default_skin", prefs->default_skin);
    prefs_set_integer (group, "enabled", prefs->m_enabled);
    prefs_set_integer (group, "window_x", prefs->oldpos_x);
    prefs_set_integer (group, "window_y", prefs->oldpos_y);
    prefs_set_string (group, "localhost", prefs->localhost);
    prefs_set_integer (group, "add_tracks_to_playlist", prefs->m_add_finished_tracks_to_playlist);
    prefs_set_integer (group, "start_minimized", prefs->m_start_minimized);
    prefs_set_integer (group, "use_old_playlist_ret", prefs->use_old_playlist_ret);

    /* Set history */
    for (i = 0, p = 0; i < RIPLIST_LEN; i++) {
	if (prefs->riplist[i][0]) {
	    char profile_name[128];
	    sprintf (profile_name, "riplist%d", p);
	    prefs_set_string (group, profile_name, prefs->riplist[i]);
	    p++;
	}
    }
}

void
prefs_get_stream_prefs (STREAM_PREFS* prefs, char* label)
{
    /* Be careful here.  The label might be prefs->url, so we don't 
       want to overwrite it while we are loading. */
    gchar* label_copy = strdup (label);
    gchar* group = 0;

    if (strcmp (label, "stream defaults")) {
	gsize i, length;
	gchar** group_list;

	/* Look through groups for matching URL */
	group_list = g_key_file_get_groups (m_key_file, &length);
	for (i = 0; i < length; i++) {
	    gchar* value;
	    if (!strcmp (label_copy, group_list[i])) {
		group = g_strdup (group_list[i]);
		break;		/* Found matching label */
	    }

	    value = g_key_file_get_string (m_key_file, group_list[i], "url", 0);
	    if (value) {
		if (!strcmp (label_copy, value)) {
		    group = g_strdup (group_list[i]);
		    g_free (value);
		    break;	/* Found matching URL */
		}
		g_free (value);
	    }
	}
	g_strfreev (group_list);
    }

    /* Copy default values */
    prefs_get_stream_defaults (prefs);
    prefs_get_stream_prefs_keyfile (prefs, "stream defaults");

    /* Copy stream-specific values */
    if (group) {
	prefs_get_stream_prefs_keyfile (prefs, group);
	g_free (group);
    }

    /* If no stream-specific values found, set url to input value */
    if (!group && strcmp (label, "stream defaults")) {
	strcpy (prefs->url, label_copy);
    }

    g_free (label_copy);

    //    printf ("Home dir is: %s\n", g_get_home_dir());
    //    printf ("Config dir is: %s\n", g_get_user_config_dir ());
    //    printf ("Data dir is: %s\n", g_get_user_data_dir ());
}

void
prefs_set_stream_prefs (STREAM_PREFS* prefs, char* label)
{
    if (!label) {
	/* GCS FIX: Here I should assign a new (unused) label */
	return;
    }
    prefs_set_stream_prefs_keyfile (prefs, 0, label);
}

/* GCS: This is not quite complete, missing splitting & codesets */
void
debug_stream_prefs (STREAM_PREFS* prefs)
{
    debug_printf ("label = %s\n", prefs->label);
    debug_printf ("url = %s\n", prefs->url);
    debug_printf ("proxyurl = %s\n", prefs->proxyurl);
    debug_printf ("output_directory = %s\n", prefs->output_directory);
    debug_printf ("output_pattern = %s\n", prefs->output_pattern);
    debug_printf ("showfile_pattern = %s\n", prefs->showfile_pattern);
    debug_printf ("if_name = %s\n", prefs->if_name);
    debug_printf ("rules_file = %s\n", prefs->rules_file);
    debug_printf ("pls_file = %s\n", prefs->pls_file);
    debug_printf ("relay_ip = %s\n", prefs->relay_ip);
    debug_printf ("ext_cmd = %s\n", prefs->ext_cmd);
    debug_printf ("useragent = %s\n", prefs->useragent);
    debug_printf ("relay_port = %d\n", prefs->relay_port);
    debug_printf ("max_port = %d\n", prefs->max_port);
    debug_printf ("max_connections = %d\n", prefs->max_connections);
    debug_printf ("maxMB_rip_size = %d\n", prefs->maxMB_rip_size);
    debug_printf ("auto_reconnect = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_AUTO_RECONNECT));
    debug_printf ("make_relay = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_MAKE_RELAY));
    debug_printf ("add_id3v1 = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_ADD_ID3V1));
    debug_printf ("add_id3v2 = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_ADD_ID3V2));
    debug_printf ("check_max_bytes = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_CHECK_MAX_BYTES));
    debug_printf ("keep_incomplete = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_KEEP_INCOMPLETE));
    debug_printf ("rip_individual_tracks = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_INDIVIDUAL_TRACKS));
    debug_printf ("rip_single_file = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_SINGLE_FILE_OUTPUT));
    debug_printf ("use_ext_cmd = %d\n",
		  OPT_FLAG_ISSET (prefs->flags, OPT_EXTERNAL_CMD));
    debug_printf ("timeout = %d\n", prefs->timeout);
    debug_printf ("dropcount = %d\n", prefs->dropcount);
    debug_printf ("count_start = %d\n", prefs->count_start);
    debug_printf ("overwrite = %s\n", 
		  overwrite_opt_to_string(prefs->overwrite));
};


/******************************************************************************
 * Private functions
 *****************************************************************************/
static enum PrefsVersion
string_to_prefs_version (char* str)
{
    int i = 0;

    /* If there is no version string, then the version is "1.63-beta-2". */
    if (!str) return 0;

    while (prefs_version_strings[i]) {
	if (strcmp(str, prefs_version_strings[i]) == 0) {
	    return i;
	}
	i++;
    }
    return 0;
}

/* Calling routine must free returned string */
static gchar* 
prefs_get_config_dir (void)
{
    return g_build_filename (g_get_user_config_dir(), 
			     "streamripper", 
			     NULL);
}

static void
prefs_get_global_defaults (GLOBAL_PREFS* global_prefs)
{
    memset (global_prefs, 0, sizeof(GLOBAL_PREFS));
}

static void
prefs_get_stream_defaults (STREAM_PREFS* prefs)
{
    debug_printf ("- set_rip_manager_options_defaults -\n");
    //    prefs->url[0] = 0;
    prefs->proxyurl[0] = 0;

    strcpy(prefs->output_directory, "./");
    prefs->output_pattern[0] = 0;
    prefs->showfile_pattern[0] = 0;
    prefs->if_name[0] = 0;
    prefs->rules_file[0] = 0;
    prefs->pls_file[0] = 0;
    prefs->relay_ip[0] = 0;
    prefs->relay_port = 8000;
    prefs->max_port = 18000;
    prefs->max_connections = 1;
    prefs->maxMB_rip_size = 0;
    prefs->flags = OPT_AUTO_RECONNECT | 
	    OPT_SEPARATE_DIRS | 
	    OPT_SEARCH_PORTS |
	    /* OPT_ADD_ID3V1 | -- removed starting 1.62-beta-2 */
	    OPT_ADD_ID3V2 |
	    OPT_INDIVIDUAL_TRACKS;
    strcpy(prefs->useragent, DEFAULT_USER_AGENT);

    // Defaults for splitpoint - times are in ms
    prefs->sp_opt.xs = 1;
    prefs->sp_opt.xs_min_volume = 1;
    prefs->sp_opt.xs_silence_length = 1000;
    prefs->sp_opt.xs_search_window_1 = 6000;
    prefs->sp_opt.xs_search_window_2 = 6000;
    prefs->sp_opt.xs_offset = 0;
    prefs->sp_opt.xs_padding_1 = 0;   /* Changed from 300 to 0 vers. 1.64.5 */
    prefs->sp_opt.xs_padding_2 = 0;   /* Changed from 300 to 0 vers. 1.64.5 */

    prefs->timeout = 15;
    prefs->dropcount = 1;  /* Changed from 0 to 1 in version 1.63-beta-8 */

    // Defaults for codeset
    memset (&prefs->cs_opt, 0, sizeof(CODESET_OPTIONS));
    set_codesets_default (&prefs->cs_opt);

    prefs->count_start = 0;
    prefs->overwrite = OVERWRITE_VERSION;
    prefs->ext_cmd[0] = 0;
}

static void
prefs_get_wstreamripper_defaults (WSTREAMRIPPER_PREFS* prefs)
{
    int i;

    strcpy(prefs->default_skin, DEFAULT_SKINFILE);
    prefs->m_enabled = 1;
    prefs->oldpos_x = 0;
    prefs->oldpos_y = 0;
    /* Changed from "localhost" to 127.0.0.1 in version 1.64.5 */
    strcpy (prefs->localhost, "127.0.0.1");
    prefs->m_add_finished_tracks_to_playlist = 0;
    prefs->m_start_minimized = 0;
    prefs->use_old_playlist_ret = 0;
    for (i = 0; i < RIPLIST_LEN; i++) {
	prefs->riplist[i][0] = 0;
    }
}

/* Return 0 if value not found, 1 if value found */
static int
prefs_get_string (char* dest, gsize dest_size, char* group, char* key)
{
    GError *error = NULL;
    gchar *value;
    
    value = g_key_file_get_string (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	g_error_free (error);
	return 0;
    }
    if (g_strlcpy (dest, value, dest_size) >= dest_size) {
	/* Value too long, silently truncate */
    }
    g_free (value);
    return 1;
}

static void
prefs_get_ushort (u_short *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	g_error_free (error);
	return;
    }
    if (value < 0 || value > G_MAXUSHORT) {
	/* Bad value, silently ignore (?) */
	return;
    }
    *dest = (u_short) value;
}

/* Return 0 if value not found, 1 if value found */
static int
prefs_get_ulong (u_long *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	g_error_free (error);
	return 0;
    }
    if (value < 0) {
	/* Bad value, silently ignore (?) */
	return 0;
    }
    *dest = (u_long) value;
    return 1;
}

/* Return 0 if value not found, 1 if value found */
static int
prefs_get_long (long *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	g_error_free (error);
	return 0;
    }
    *dest = (long) value;
    return 1;
}

/* Return 0 if value not found, 1 if value found */
static int
prefs_get_int (int *dest, char *group, char *key)
{
    GError *error = NULL;
    gint value;
    
    value = g_key_file_get_integer (m_key_file, group, key, &error);
    if (error) {
	/* Key doesn't exist, do nothing */
	g_error_free (error);
	return 0;
    }
    *dest = (u_long) value;
    return 1;
}

static void
prefs_set_string (char* group, char* key, char* value)
{
    g_key_file_set_string (m_key_file, group, key, value);
}

static void
prefs_set_integer (char* group, char* key, gint value)
{
    g_key_file_set_integer (m_key_file, group, key, value);
}

static void
prefs_get_stream_prefs_keyfile (STREAM_PREFS* prefs, char* group)
{
    u_long temp;
    char overwrite_str[128];

    if (!m_key_file) return;

    prefs_get_string (prefs->url, MAX_URL_LEN, group, "url");
    prefs_get_string (prefs->proxyurl, MAX_URL_LEN, group, "proxy");
    prefs_get_string (prefs->output_directory, SR_MAX_PATH, group, "output_dir");
    prefs_get_string (prefs->output_pattern, SR_MAX_PATH, group, "output_pattern");
    prefs_get_string (prefs->showfile_pattern, SR_MAX_PATH, group, "showfile_pattern");
    prefs_get_string (prefs->if_name, SR_MAX_PATH, group, "if_name");
    prefs_get_string (prefs->rules_file, SR_MAX_PATH, group, "rules_file");
    prefs_get_string (prefs->pls_file, SR_MAX_PATH, group, "pls_file");
    prefs_get_string (prefs->relay_ip, SR_MAX_PATH, group, "relay_ip");
    prefs_get_string (prefs->useragent, MAX_USERAGENT_STR, group, "useragent");
    prefs_get_string (prefs->ext_cmd, SR_MAX_PATH, group, "ext_cmd");
    prefs_get_ushort (&prefs->relay_port, group, "relay_port");
    prefs_get_ushort (&prefs->max_port, group, "max_port");
    prefs_get_ulong (&prefs->max_connections, group, "max_connections");
    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_bytes");
    prefs_get_ulong (&prefs->maxMB_rip_size, group, "maxMB_bytes");
    prefs_get_ulong (&prefs->dropcount, group, "dropcount");

    /* Overwrite */
    if (prefs_get_string (overwrite_str, 128, group, "over_write_complete")) {
	prefs->overwrite = string_to_overwrite_opt (overwrite_str);
    }

    /* Flags */
    if (prefs_get_ulong (&temp, group, "auto_reconnect")) {
	OPT_FLAG_SET (prefs->flags, OPT_AUTO_RECONNECT, temp);
    }
    if (prefs_get_ulong (&temp, group, "separate_dirs")) {
	OPT_FLAG_SET (prefs->flags, OPT_SEPARATE_DIRS, temp);
    }
    if (prefs_get_ulong (&temp, group, "make_relay")) {
	OPT_FLAG_SET (prefs->flags, OPT_MAKE_RELAY, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_numeric_prefix")) {
	OPT_FLAG_SET (prefs->flags, OPT_COUNT_FILES, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3")) {
	/* Obsolete */
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V1, temp);
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V2, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_date_stamp")) {
	OPT_FLAG_SET (prefs->flags, OPT_DATE_STAMP, temp);
    }
    if (prefs_get_ulong (&temp, group, "check_max_bytes")) {
	OPT_FLAG_SET (prefs->flags, OPT_CHECK_MAX_BYTES, temp);
    }
    if (prefs_get_ulong (&temp, group, "keep_incomplete")) {
	OPT_FLAG_SET (prefs->flags, OPT_KEEP_INCOMPLETE, temp);
    }
    if (prefs_get_ulong (&temp, group, "rip_single_file")) {
	OPT_FLAG_SET (prefs->flags, OPT_SINGLE_FILE_OUTPUT, temp);
    }
    if (prefs_get_ulong (&temp, group, "truncate_incomplete_when_in_complete")) {
	OPT_FLAG_SET (prefs->flags, OPT_TRUNCATE_DUPS, temp);
    }
    if (prefs_get_ulong (&temp, group, "rip_individual_tracks")) {
	OPT_FLAG_SET (prefs->flags, OPT_INDIVIDUAL_TRACKS, temp);
    }
    if (prefs_get_ulong (&temp, group, "use_ext_cmd")) {
	OPT_FLAG_SET (prefs->flags, OPT_EXTERNAL_CMD, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3v1")) {
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V1, temp);
    }
    if (prefs_get_ulong (&temp, group, "add_id3v2")) {
	OPT_FLAG_SET (prefs->flags, OPT_ADD_ID3V2, temp);
    }

    /* Splitpoint options */
    prefs_get_int (&prefs->sp_opt.xs, group, "xs");
    prefs_get_int (&prefs->sp_opt.xs_offset, group, "xs_offset");
    prefs_get_int (&prefs->sp_opt.xs_silence_length, group, "xs_silence_length");
    prefs_get_int (&prefs->sp_opt.xs_search_window_1, group, "xs_search_window_1");
    prefs_get_int (&prefs->sp_opt.xs_search_window_2, group, "xs_search_window_2");
    prefs_get_int (&prefs->sp_opt.xs_padding_1, group, "xs_padding_1");
    prefs_get_int (&prefs->sp_opt.xs_padding_2, group, "xs_padding_2");

    /* Codesets */
    prefs_get_string (prefs->cs_opt.codeset_metadata, MAX_CODESET_STRING, group, "codeset_metadata");
    prefs_get_string (prefs->cs_opt.codeset_relay, MAX_CODESET_STRING, group, "codeset_relay");
    prefs_get_string (prefs->cs_opt.codeset_id3, MAX_CODESET_STRING, group, "codeset_id3");
    prefs_get_string (prefs->cs_opt.codeset_filesys, MAX_CODESET_STRING, group, "codeset_filesys");
}

/* gp = global preferences */
static void
prefs_set_stream_prefs_keyfile (STREAM_PREFS* prefs, STREAM_PREFS* gp, 
				char* group)
{
    if (!m_key_file) return;

    if (!gp || !strcmp(prefs->url, gp->url)) {
	prefs_set_string (group, "url", prefs->url);
    }
    if (!gp || !strcmp(prefs->proxyurl, gp->proxyurl)) {
	prefs_set_string (group, "proxy", prefs->proxyurl);
    }
    if (!gp || !strcmp(prefs->output_directory, gp->output_directory)) {
	prefs_set_string (group, "output_dir", prefs->output_directory);
    }
    if (!gp || !strcmp(prefs->output_pattern, gp->output_pattern)) {
	prefs_set_string (group, "output_pattern", prefs->output_pattern);
    }
    if (!gp || !strcmp(prefs->showfile_pattern, gp->showfile_pattern)) {
	prefs_set_string (group, "showfile_pattern", prefs->showfile_pattern);
    }
    if (!gp || !strcmp(prefs->if_name, gp->if_name)) {
	prefs_set_string (group, "if_name", prefs->if_name);
    }
    if (!gp || !strcmp(prefs->rules_file, gp->rules_file)) {
	prefs_set_string (group, "rules_file", prefs->rules_file);
    }
    if (!gp || !strcmp(prefs->pls_file, gp->pls_file)) {
	prefs_set_string (group, "pls_file", prefs->pls_file);
    }
    if (!gp || !strcmp(prefs->relay_ip, gp->relay_ip)) {
	prefs_set_string (group, "relay_ip", prefs->relay_ip);
    }
    /* User agent is treated differently. */
    if (!gp || !strcmp(prefs->useragent, gp->useragent)) {
	if (strcmp(prefs->useragent, DEFAULT_USER_AGENT)) {
	    prefs_set_string (group, "useragent", prefs->useragent);
	} else {
	    prefs_set_string (group, "useragent", "");
	}
    }
    if (!gp || !strcmp(prefs->ext_cmd, gp->ext_cmd)) {
	prefs_set_string (group, "ext_cmd", prefs->ext_cmd);
    }

    prefs_set_integer (group, "relay_port", prefs->relay_port);
    prefs_set_integer (group, "max_port", prefs->max_port);
    prefs_set_integer (group, "max_connections", prefs->max_connections);
    prefs_set_integer (group, "maxMB_bytes", prefs->maxMB_rip_size);
    prefs_set_integer (group, "maxMB_bytes", prefs->maxMB_rip_size);
    prefs_set_integer (group, "dropcount", prefs->dropcount);

    /* Overwrite */
    g_key_file_set_string (m_key_file, group, "over_write_complete", 
			   overwrite_opt_to_string(prefs->overwrite));

    /* Flags */
    prefs_set_integer (group, "auto_reconnect",
		       OPT_FLAG_ISSET (prefs->flags, OPT_AUTO_RECONNECT));
    prefs_set_integer (group, "separate_dirs",
		       OPT_FLAG_ISSET (prefs->flags, OPT_SEPARATE_DIRS));
    prefs_set_integer (group, "make_relay",
		       OPT_FLAG_ISSET (prefs->flags, OPT_MAKE_RELAY));
    prefs_set_integer (group, "add_numeric_prefix",
		       OPT_FLAG_ISSET (prefs->flags, OPT_COUNT_FILES));
    prefs_set_integer (group, "add_date_stamp",
		       OPT_FLAG_ISSET (prefs->flags, OPT_DATE_STAMP));
    prefs_set_integer (group, "check_max_bytes",
		       OPT_FLAG_ISSET (prefs->flags, OPT_CHECK_MAX_BYTES));
    prefs_set_integer (group, "keep_incomplete",
		       OPT_FLAG_ISSET (prefs->flags, OPT_KEEP_INCOMPLETE));
    prefs_set_integer (group, "rip_single_file",
		       OPT_FLAG_ISSET (prefs->flags, OPT_SINGLE_FILE_OUTPUT));
    prefs_set_integer (group, "truncate_incomplete_when_in_complete",
		       OPT_FLAG_ISSET (prefs->flags, OPT_TRUNCATE_DUPS));
    prefs_set_integer (group, "rip_individual_tracks",
		       OPT_FLAG_ISSET (prefs->flags, OPT_INDIVIDUAL_TRACKS));
    prefs_set_integer (group, "use_ext_cmd",
		       OPT_FLAG_ISSET (prefs->flags, OPT_EXTERNAL_CMD));
    prefs_set_integer (group, "add_id3v1",
		       OPT_FLAG_ISSET (prefs->flags, OPT_ADD_ID3V1));
    prefs_set_integer (group, "add_id3v2",
		       OPT_FLAG_ISSET (prefs->flags, OPT_ADD_ID3V2));

    /* Splitpoint options */
    prefs_set_integer (group, "xs", prefs->sp_opt.xs);
    prefs_set_integer (group, "xs_offset", prefs->sp_opt.xs_offset);
    prefs_set_integer (group, "xs_silence_length", 
		       prefs->sp_opt.xs_silence_length);
    prefs_set_integer (group, "xs_search_window_1", 
		       prefs->sp_opt.xs_search_window_1);
    prefs_set_integer (group, "xs_search_window_2", 
		       prefs->sp_opt.xs_search_window_2);
    prefs_set_integer (group, "xs_padding_1", prefs->sp_opt.xs_padding_1);
    prefs_set_integer (group, "xs_padding_2", prefs->sp_opt.xs_padding_2);

    /* Codesets */
    if (!gp || !strcmp(prefs->cs_opt.codeset_metadata, gp->cs_opt.codeset_metadata)) {
	prefs_set_string (group, "codeset_metadata", prefs->cs_opt.codeset_metadata);
    }
    if (!gp || !strcmp(prefs->cs_opt.codeset_relay, gp->cs_opt.codeset_relay)) {
	prefs_set_string (group, "codeset_relay", prefs->cs_opt.codeset_relay);
    }
    if (!gp || !strcmp(prefs->cs_opt.codeset_id3, gp->cs_opt.codeset_id3)) {
	prefs_set_string (group, "codeset_id3", prefs->cs_opt.codeset_id3);
    }
    if (!gp || !strcmp(prefs->cs_opt.codeset_filesys, gp->cs_opt.codeset_filesys)) {
	prefs_set_string (group, "codeset_filesys", prefs->cs_opt.codeset_filesys);
    }
}
