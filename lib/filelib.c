/* filelib.c
 * library routines for file operations
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "compat.h"
#include "filelib.h"
#include "mchar.h"
#include "debug.h"
#include <assert.h>
#include <sys/types.h>
#include "glib.h"
#include "glib/gstdio.h"
#include "uce_dirent.h"

#define TEMP_STR_LEN	(SR_MAX_PATH*2)

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static error_code device_split (gchar* dirname, gchar* device, gchar* path);
static error_code mkdir_if_needed (RIP_MANAGER_INFO* rmi, gchar *str);
static error_code mkdir_recursive (RIP_MANAGER_INFO* rmi, gchar *str, int make_last);
static void close_file (FHANDLE* fp);
static void close_files (RIP_MANAGER_INFO* rmi);
static error_code filelib_write (FHANDLE fp, char *buf, u_long size);
static BOOL file_exists (RIP_MANAGER_INFO* rmi, gchar *filename);
static void 
trim_filename (RIP_MANAGER_INFO* rmi, gchar* out, gchar *filename);
static void
trim_mp3_suffix (RIP_MANAGER_INFO* rmi, gchar *filename);
static error_code filelib_open_for_write (RIP_MANAGER_INFO* rmi, FHANDLE* fp, gchar *filename);
static void
parse_and_subst_dir (RIP_MANAGER_INFO* rmi, 
		     gchar* pattern_head, gchar* pattern_tail, 
		     gchar* opat_path, int is_for_showfile);
static void
parse_and_subst_pat (RIP_MANAGER_INFO* rmi,
		     gchar* newfile,
		     TRACK_INFO* ti,
		     gchar* directory,
		     gchar* pattern,
		     gchar* extension);
static void
set_default_pattern (RIP_MANAGER_INFO* rmi, 
		     BOOL get_separate_dirs, BOOL do_count);
static error_code 
set_output_directory (RIP_MANAGER_INFO* rmi, 
		      gchar* global_output_directory,
		      gchar* global_output_pattern,
		      gchar* output_pattern,
		      gchar* output_directory,
		      gchar* default_pattern,
		      gchar* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
		      );
static error_code sr_getcwd (RIP_MANAGER_INFO* rmi, gchar* dirbuf);
static error_code add_trailing_slash (gchar *str);
static int get_next_sequence_number (RIP_MANAGER_INFO* rmi, gchar* fn_base);
static void fill_date_buf (RIP_MANAGER_INFO* rmi, gchar* datebuf, 
			   int datebuf_len);
static error_code filelib_open_showfiles (RIP_MANAGER_INFO* rmi);
static void move_file (RIP_MANAGER_INFO* rmi, gchar* new_filename, gchar* old_filename);
static gchar* replace_invalid_chars (gchar *str);
static void remove_trailing_periods (gchar *str);
static BOOL new_file_is_better (RIP_MANAGER_INFO* rmi, gchar *oldfile, gchar *newfile);
static void delete_file (RIP_MANAGER_INFO* rmi, gchar* filename);
static void truncate_file (RIP_MANAGER_INFO* rmi, gchar* filename);
static void
filelib_rename_versioned (gchar** new_fn, RIP_MANAGER_INFO* rmi, 
			  gchar* directory, gchar* fnbase, gchar* extension);


/*****************************************************************************
 * Public Functions
 *****************************************************************************/
error_code
filelib_init (RIP_MANAGER_INFO* rmi,
	      BOOL do_individual_tracks,
	      BOOL do_count,
	      int count_start,
	      BOOL keep_incomplete,
	      BOOL do_show_file,
	      int content_type,
	      char* output_directory,  /* Locale encoded - from command line */
	      char* output_pattern,    /* Locale encoded - from command line */
	      char* showfile_pattern,  /* Locale encoded - from command line */
	      int get_separate_dirs,
	      int get_date_stamp,
	      char* icy_name)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    gchar tmp_output_directory[SR_MAX_PATH];
    gchar tmp_output_pattern[SR_MAX_PATH];
    gchar tmp_showfile_pattern[SR_MAX_PATH];

    fli->m_file = INVALID_FHANDLE;
    fli->m_show_file = INVALID_FHANDLE;
    fli->m_cue_file = INVALID_FHANDLE;
    fli->m_count = do_count ? count_start : -1;
    fli->m_keep_incomplete = keep_incomplete;
    memset(&fli->m_output_directory, 0, SR_MAX_PATH);
    fli->m_show_name[0] = 0;
    fli->m_do_show = do_show_file;
    fli->m_do_individual_tracks = do_individual_tracks;
    fli->m_track_no = 1;
    
    debug_printf ("FILELIB_INIT: output_directory=%s\n",
		  output_directory ? output_directory : "");
    debug_printf ("FILELIB_INIT: output_pattern=%s\n",
		  output_pattern ? output_pattern : "");
    debug_printf ("FILELIB_INIT: showfile_pattern=%s\n",
		  showfile_pattern ? showfile_pattern : "");
    
    debug_printf ("converting output_directory\n");
    gstring_from_string (rmi, tmp_output_directory, SR_MAX_PATH, 
			 output_directory, CODESET_LOCALE);
    debug_printf ("converting output_pattern\n");
    gstring_from_string (rmi, tmp_output_pattern, SR_MAX_PATH, 
			 output_pattern, CODESET_LOCALE);
    debug_printf ("converting showfile_pattern\n");
    gstring_from_string (rmi, tmp_showfile_pattern, SR_MAX_PATH, 
			 showfile_pattern, CODESET_LOCALE);
    debug_printf ("converting icy_name\n");
    gstring_from_string (rmi, fli->m_icy_name, SR_MAX_PATH, icy_name, 
			 CODESET_METADATA);
    debug_printf ("Converted output directory: len=%d\n", 
		  mstrlen (tmp_output_directory));
    mstrcpy (fli->m_stripped_icy_name, fli->m_icy_name);
    
    debug_printf ("Replacing invalid chars in stripped_icy_name\n");
    replace_invalid_chars (fli->m_stripped_icy_name);
    debug_printf ("  %s\n", fli->m_stripped_icy_name);

    debug_printf ("Removing trailing periods\n");
    remove_trailing_periods (fli->m_stripped_icy_name);
    debug_printf ("  %s\n", fli->m_stripped_icy_name);

    switch (content_type) {
    case CONTENT_TYPE_MP3:
	fli->m_extension = m_(".mp3");
	break;
    case CONTENT_TYPE_NSV:
    case CONTENT_TYPE_ULTRAVOX:
	fli->m_extension = m_(".nsv");
	break;
    case CONTENT_TYPE_OGG:
	fli->m_extension = m_(".ogg");
	break;
    case CONTENT_TYPE_AAC:
	fli->m_extension = m_(".aac");
	break;
    default:
	fprintf (stderr, "Error (wrong suggested content type: %d)\n", 
		 content_type);
	return SR_ERROR_PROGRAM_ERROR;
    }

    /* Initialize session date */
    fill_date_buf (rmi, fli->m_session_datebuf, DATEBUF_LEN);

    /* Set up the proper pattern if we're using -q and -s flags */
    set_default_pattern (rmi, get_separate_dirs, do_count);

    /* Get the path to the "parent" directory.  This is the directory
       that contains the incomplete dir and the show files.
       It might not contain the complete files if an output_pattern
       was specified. */
    set_output_directory (rmi, 
			  fli->m_output_directory,
			  fli->m_output_pattern,
			  tmp_output_pattern,
			  tmp_output_directory,
			  fli->m_default_pattern,
			  m_("%A - %T"),
			  get_separate_dirs,
			  get_date_stamp,
			  0);
    debug_mprintf (m_("m_output_directory: ") m_S m_("\n"),
		   fli->m_output_directory);
    debug_mprintf (m_("m_output_pattern: ") m_S m_("\n"),
		   fli->m_output_pattern);
    msnprintf (fli->m_incomplete_directory, SR_MAX_PATH, m_S m_S m_C, 
	       fli->m_output_directory, m_("incomplete"), PATH_SLASH);

    /* Recursively make the output directory & incomplete directory */
    if (fli->m_do_individual_tracks) {
	debug_mprintf (m_("Trying to make output_directory: ") m_S m_("\n"), 
		       fli->m_output_directory);
	mkdir_recursive (rmi, fli->m_output_directory, 1);

	/* Next, make the incomplete directory */
	if (fli->m_do_individual_tracks) {
	    debug_mprintf (m_("Trying to make incomplete_directory: ") 
			   m_S m_("\n"), fli->m_incomplete_directory);
	    mkdir_if_needed (rmi, fli->m_incomplete_directory);
	}
    }

    /* Compute the amount of remaining path length for the filenames */
    fli->m_max_filename_length = SR_MAX_PATH - mstrlen(fli->m_incomplete_directory);

    /* Get directory and pattern of showfile */
    if (do_show_file) {
	if (*tmp_showfile_pattern) {
	    trim_mp3_suffix (rmi, tmp_showfile_pattern);
	    if (mstrlen(fli->m_show_name) > SR_MAX_PATH - 5) {
		return SR_ERROR_DIR_PATH_TOO_LONG;
	    }
	}
	set_output_directory (rmi, 
			      fli->m_showfile_directory,
			      fli->m_showfile_pattern,
			      tmp_showfile_pattern,
			      tmp_output_directory,
			      fli->m_default_showfile_pattern,
			      m_(""),
			      get_separate_dirs,
			      get_date_stamp,
			      1);
	mkdir_recursive (rmi, fli->m_showfile_directory, 1);
	filelib_open_showfiles (rmi);
    }

    return SR_SUCCESS;
}

error_code
filelib_start (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    gchar newfile[TEMP_STR_LEN];
    gchar fnbase[TEMP_STR_LEN];
    gchar fnbase1[TEMP_STR_LEN];

    if (!fli->m_do_individual_tracks) return SR_SUCCESS;

    close_file (&fli->m_file);

    /* Compose and trim filename (not including directory) */
    msnprintf (fnbase1, TEMP_STR_LEN, m_S m_(" - ") m_S, 
	       ti->artist, ti->title);
    trim_filename (rmi, fnbase, fnbase1);
    msnprintf (newfile, TEMP_STR_LEN, m_S m_S m_S, 
	       fli->m_incomplete_directory, fnbase, fli->m_extension);
    if (fli->m_keep_incomplete) {
	filelib_rename_versioned (0, rmi, fli->m_incomplete_directory, 
				  fnbase, fli->m_extension);
    }
    mstrcpy (fli->m_incomplete_filename, newfile);
    return filelib_open_for_write (rmi, &fli->m_file, newfile);
}

error_code
filelib_write_cue (RIP_MANAGER_INFO* rmi, TRACK_INFO* ti, int secs)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    int rc;
    char buf1[MAX_TRACK_LEN];
    char buf2[MAX_TRACK_LEN];

    if (!fli->m_do_show) return SR_SUCCESS;
    if (!fli->m_cue_file) return SR_SUCCESS;

    rc = snprintf (buf2, MAX_TRACK_LEN, "  TRACK %02d AUDIO\n", 
		   fli->m_track_no++);
    filelib_write (fli->m_cue_file, buf2, rc);
    string_from_gstring (rmi, buf1, MAX_TRACK_LEN, ti->title, CODESET_ID3);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    TITLE \"%s\"\n", buf1);
    filelib_write (fli->m_cue_file, buf2, rc);
    string_from_gstring (rmi, buf1, MAX_TRACK_LEN, ti->artist, CODESET_ID3);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    PERFORMER \"%s\"\n", buf1);
    filelib_write (fli->m_cue_file, buf2, rc);
    rc = snprintf (buf2, MAX_TRACK_LEN, "    INDEX 01 %02d:%02d:00\n", 
		   secs / 60, secs % 60);
    filelib_write (fli->m_cue_file, buf2, rc);

    return SR_SUCCESS;
}

error_code
filelib_write_track (RIP_MANAGER_INFO* rmi, char *buf, u_long size)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    return filelib_write (fli->m_file, buf, size);
}

error_code
filelib_write_show (RIP_MANAGER_INFO* rmi, char *buf, u_long size)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    error_code rc;
    debug_printf ("Testing to write showfile\n");
    if (!fli->m_do_show) {
	return SR_SUCCESS;
    }
    debug_printf ("Trying to write showfile\n");
    rc = filelib_write (fli->m_show_file, buf, size);
    if (rc != SR_SUCCESS) {
	fli->m_do_show = 0;
    }
    return rc;
}

// Moves the file from incomplete to complete directory
// fullpath is an output parameter
error_code
filelib_end (RIP_MANAGER_INFO* rmi,
	     TRACK_INFO* ti,
	     enum OverwriteOpt overwrite,
	     BOOL truncate_dup,
	     gchar *fullpath)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    BOOL ok_to_write = TRUE;
    gchar new_path[TEMP_STR_LEN];
    gchar* new_fnbase;
    gchar* new_dir;

    if (!fli->m_do_individual_tracks) return SR_SUCCESS;

    close_file (&fli->m_file);

    /* Construct filename for completed file */
    parse_and_subst_pat (rmi, new_path, ti, fli->m_output_directory, 
			 fli->m_output_pattern, fli->m_extension);

    /* Build up the output directory */
    mkdir_recursive (rmi, new_path, 0);

    // If we are over writing existing tracks
    debug_printf ("overwrite flag is %d\n", overwrite);
    switch (overwrite) {
    case OVERWRITE_ALWAYS:
	ok_to_write = TRUE;
	break;
    case OVERWRITE_NEVER:
	if (file_exists (rmi, new_path)) {
	    ok_to_write = FALSE;
	} else {
	    ok_to_write = TRUE;
	}
	break;
    case OVERWRITE_LARGER:
	/* Smart overwriting -- only overwrite if new file is bigger */
	ok_to_write = new_file_is_better (rmi, new_path, fli->m_incomplete_filename);
	break;
    case OVERWRITE_VERSION:
    default:
	new_dir = g_path_get_dirname (new_path);
	new_fnbase = g_path_get_basename (new_path);
	trim_mp3_suffix (rmi, new_fnbase);
	filelib_rename_versioned (0, rmi, new_dir, new_fnbase, 
				  fli->m_extension);
	g_free (new_dir);
	g_free (new_fnbase);
	ok_to_write = TRUE;
    }

    if (ok_to_write) {
	if (file_exists (rmi, new_path)) {
	    delete_file (rmi, new_path);
	}
	move_file (rmi, new_path, fli->m_incomplete_filename);
    } else {
	if (truncate_dup && file_exists (rmi, fli->m_incomplete_filename)) {
	    truncate_file (rmi, fli->m_incomplete_filename);
	}
    }

    if (fullpath) {
	mstrcpy (fullpath, new_path);
    }
    if (fli->m_count != -1)
	fli->m_count++;
    return SR_SUCCESS;
}

void
filelib_shutdown (RIP_MANAGER_INFO* rmi)
{
    close_files (rmi);
}


 /*****************************************************************************
 * Private Functions
 *****************************************************************************/
// For now we're not going to care. If it makes it good. it not, will know 
// When we try to create a file in the path.
static error_code
mkdir_if_needed (RIP_MANAGER_INFO* rmi, gchar *str)
{
    char s[SR_MAX_PATH];
    string_from_gstring (rmi, s, SR_MAX_PATH, str, CODESET_FILESYS);
    debug_printf ("mkdir = %s -> %s\n", str, s);
#if WIN32
    mkdir (s);
#else
    mkdir (s, 0777);
#endif
    return SR_SUCCESS;
}

/* Recursively make directories.  If make_last == 1, then the final 
   substring (after the last '/') is considered a directory rather 
   than a file name */
static error_code
mkdir_recursive (RIP_MANAGER_INFO* rmi, gchar *str, int make_last)
{
    gchar buf[SR_MAX_PATH];
    gchar* p = buf;
    gchar q;

    buf[0] = 0;
    while ((q = *p++ = *str++) != 0) {
	if (ISSLASH(q)) {
	    *p = 0;
	    mkdir_if_needed (rmi, buf);
	}
    }
    if (make_last) {
	mkdir_if_needed (rmi, str);
    }
    return SR_SUCCESS;
}

/* This sets the value for m_default_pattern and m_default_showfile_pattern,
   using the -q & -s flags.  This function cannot overflow 
   these static buffers. */
static void
set_default_pattern (RIP_MANAGER_INFO* rmi, 
		     BOOL get_separate_dirs, BOOL do_count)
{
    FILELIB_INFO* fli = &rmi->filelib_info;

    /* Set up m_default_pattern */
    fli->m_default_pattern[0] = 0;
    if (get_separate_dirs) {
	mstrcpy (fli->m_default_pattern, m_("%S") PATH_SLASH_STR);
    }
    if (do_count) {
	if (fli->m_count < 0) {
	    mstrncat (fli->m_default_pattern, m_("%q_"), SR_MAX_PATH);
	} else {
	    msnprintf (&fli->m_default_pattern[mstrlen(fli->m_default_pattern)], 
		       SR_MAX_PATH - mstrlen(fli->m_default_pattern), 
		       m_("%%%dq_"), fli->m_count);
	}
    }
    mstrncat (fli->m_default_pattern, m_("%A - %T"), SR_MAX_PATH);

    /* Set up m_default_showfile_pattern */
    fli->m_default_showfile_pattern[0] = 0;
    if (get_separate_dirs) {
	mstrcpy (fli->m_default_showfile_pattern, m_("%S") PATH_SLASH_STR);
    }
    mstrncat (fli->m_default_showfile_pattern, m_("sr_program_%d"), SR_MAX_PATH);
}

/* This function sets the value of m_output_directory or 
   m_showfile_directory. */
static error_code 
set_output_directory (RIP_MANAGER_INFO* rmi, 
		      gchar* global_output_directory,
		      gchar* global_output_pattern,
		      gchar* output_pattern,
		      gchar* output_directory,
		      gchar* default_pattern,
		      gchar* default_pattern_tail,
		      int get_separate_dirs,
		      int get_date_stamp,
		      int is_for_showfile
		      )
{
    error_code ret;
    gchar opat_device[3];
    gchar odir_device[3];
    gchar cwd_device[3];
    gchar* device;
    gchar opat_path[SR_MAX_PATH];
    gchar odir_path[SR_MAX_PATH];
    gchar cwd_path[SR_MAX_PATH];
    gchar cwd[SR_MAX_PATH];

    gchar pattern_head[SR_MAX_PATH];
    gchar pattern_tail[SR_MAX_PATH];

    /* Initialize strings */
    cwd[0] = 0;
    odir_device[0] = 0;
    opat_device[0] = 0;
    odir_path[0] = 0;
    opat_path[0] = 0;
    ret = sr_getcwd (rmi, cwd);
    if (ret != SR_SUCCESS) return ret;

    if (!output_pattern || !(*output_pattern)) {
	output_pattern = default_pattern;
    }

    /* Get the device. It can be empty. */
    if (output_directory && *output_directory) {
	device_split (output_directory, odir_device, odir_path);
	debug_printf ("devicesplit: %d -> %d %d\n", 
		      mstrlen (output_directory), 
		      mstrlen (odir_device),
		      mstrlen (odir_path));
    }
    device_split (output_pattern, opat_device, opat_path);
    device_split (cwd, cwd_device, cwd_path);
    if (*opat_device) {
	device = opat_device;
    } else if (*odir_device) {
	device = odir_device;
    } else {
	/* No device */
	device = m_("");
    }

    /* Generate the output file pattern. */
    if (IS_ABSOLUTE_PATH(opat_path)) {
	cwd_path[0] = 0;
	odir_path[0] = 0;
	debug_printf ("Got opat_path absolute path\n");
    } else if (IS_ABSOLUTE_PATH(odir_path)) {
	cwd_path[0] = 0;
	debug_printf ("Got odir_path absolute path\n");
    }
    if (*odir_path) {
	ret = add_trailing_slash(odir_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (*cwd_path) {
	ret = add_trailing_slash(cwd_path);
	if (ret != SR_SUCCESS) return ret;
    }
    if (mstrlen(device) + mstrlen(cwd_path) + mstrlen(opat_path) 
	+ mstrlen(odir_path) > SR_MAX_PATH-1) {
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }

    /* Fill in %S and %d patterns */
    msnprintf (pattern_head, SR_MAX_PATH, m_S m_S m_S, device, 
	       cwd_path, odir_path);
    debug_printf ("Composed pattern head (%d) <- (%d,%d,%d)\n",
		  mstrlen(pattern_head), mstrlen(device), 
		  mstrlen(cwd_path), mstrlen(odir_path));
    parse_and_subst_dir (rmi, pattern_head, pattern_tail, opat_path, 
			 is_for_showfile);

    /* In case there is no %A, no %T, etc., use the default pattern */
    if (!*pattern_tail) {
	mstrcpy (pattern_tail, default_pattern_tail);
    }

    /* Set the global variables */
    mstrcpy (global_output_directory, pattern_head);
    add_trailing_slash (global_output_directory);
    mstrcpy (global_output_pattern, pattern_tail);

    return SR_SUCCESS;
}

/* Parse & substitute the output pattern.  What we're trying to
   get is everything up to the pattern specifiers that change 
   from track to track: %A, %T, %a, %D, %q, or %Q. 
   If %S or %d appear before this, substitute in. 
   If it's for the showfile, then we don't advance pattern_head 
   If there is no %A, no %T, etc.
*/
static void
parse_and_subst_dir (RIP_MANAGER_INFO* rmi, 
		     gchar* pattern_head, gchar* pattern_tail, 
		     gchar* opat_path, int is_for_showfile)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    int opi = 0;
    unsigned int phi = 0;
    int ph_base_len;
    int op_tail_idx;

    phi = mstrlen(pattern_head);
    opi = 0;
    ph_base_len = phi;
    op_tail_idx = opi;

    while (phi < SR_MAX_BASE) {
	if (ISSLASH(opat_path[opi])) {
	    pattern_head[phi++] = PATH_SLASH;
	    opi++;
	    ph_base_len = phi;
	    op_tail_idx = opi;
	    continue;
	}
	if (opat_path[opi] == 0) {
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	}
	if (opat_path[opi] != m_('%')) {
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (opat_path[opi+1]) {
	case m_('%'):
	    pattern_head[phi++]=m_('%');
	    opi+=2;
	    continue;
	case m_('S'):
	    /* append stream name */
	    mstrncpy (&pattern_head[phi], fli->m_stripped_icy_name, 
		      SR_MAX_BASE-phi);
	    phi = mstrlen (pattern_head);
	    opi+=2;
	    continue;
	case m_('d'):
	    /* append date info */
	    mstrncpy (&pattern_head[phi], fli->m_session_datebuf, 
		      SR_MAX_BASE-phi);
	    phi = mstrlen (pattern_head);
	    opi+=2;
	    continue;
	case m_('0'): case m_('1'): case m_('2'): case m_('3'): case m_('4'): 
	case m_('5'): case m_('6'): case m_('7'): case m_('8'): case m_('9'): 
	case m_('a'):
	case m_('A'):
	case m_('D'):
	case m_('q'):
	case m_('T'):
	    /* These are track specific patterns */
	    break;
	case 0:
	    /* This means there are no artist/title info in the filename.
	       In this case, we fall back on the default pattern. */
	    pattern_head[phi++] = opat_path[opi++];
	    if (!is_for_showfile) {
		ph_base_len = phi;
		op_tail_idx = opi;
	    }
	    break;
	default:
	    /* This is an illegal pattern, so copy the '%' and continue */
	    pattern_head[phi++] = opat_path[opi++];
	    continue;
	}
	/* If we got to here, it means that we hit something like %A or %T */
	break;
    }
    /* Terminate the pattern_head string */
    pattern_head[ph_base_len] = 0;

    mstrcpy (pattern_tail, &opat_path[op_tail_idx]);
}

static void
fill_date_buf (RIP_MANAGER_INFO* rmi, gchar* datebuf, int datebuf_len)
{
    char tmp[DATEBUF_LEN];
    time_t now = time(NULL);
    strftime (tmp, datebuf_len, "%Y_%m_%d_%H_%M_%S", localtime(&now));
    gstring_from_string (rmi, datebuf, DATEBUF_LEN, tmp, CODESET_FILESYS);
}

static error_code
add_trailing_slash (gchar *str)
{
    int len = mstrlen(str);
    if (len >= SR_MAX_PATH-1)
	return SR_ERROR_DIR_PATH_TOO_LONG;
    if (!ISSLASH(str[mstrlen(str)-1]))
	mstrncat (str,  PATH_SLASH_STR, SR_MAX_PATH);
    return SR_SUCCESS;
}

/* Split off the device */
static error_code 
device_split (gchar* dirname,
	      gchar* device,
	      gchar* path
	      )
{
    int di;

    if (HAS_DEVICE(dirname)) {
	device[0] = dirname[0];
	device[1] = dirname[1];
	device[2] = 0;
	di = 2;
    } else {
	device[0] = 0;
	di = 0;
    }
    mstrcpy (path, &dirname[di]);
    return SR_SUCCESS;
}

static error_code 
sr_getcwd (RIP_MANAGER_INFO* rmi, gchar* dirbuf)
{
    char db[SR_MAX_PATH];
#if defined (WIN32)
    if (!_getcwd (db, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#else
    if (!getcwd (db, SR_MAX_PATH)) {
	debug_printf ("getcwd returned zero?\n");
	return SR_ERROR_DIR_PATH_TOO_LONG;
    }
#endif
    gstring_from_string (rmi, dirbuf, SR_MAX_PATH, db, CODESET_FILESYS);
    return SR_SUCCESS;
}

static void
close_file (FHANDLE* fp)
{
    if (*fp != INVALID_FHANDLE) {
#if defined WIN32
	CloseHandle (*fp);
#else
	close (*fp);
#endif
	*fp = INVALID_FHANDLE;
    }
}

static void
close_files (RIP_MANAGER_INFO* rmi)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    close_file (&fli->m_file);
    close_file (&fli->m_show_file);
    close_file (&fli->m_cue_file);
}

static BOOL
file_exists (RIP_MANAGER_INFO* rmi, gchar *filename)
{
    FHANDLE f;
    char fn[SR_MAX_PATH];
    string_from_gstring (rmi, fn, SR_MAX_PATH, filename, CODESET_FILESYS);
#if defined (WIN32)
    f = CreateFile (fn, GENERIC_READ,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL, NULL);
#else
    f = open (fn, O_RDONLY);
#endif
    if (f == INVALID_FHANDLE) {
	return FALSE;
    }
    close_file (&f);
    return TRUE;
}

/* It's a bit touch and go here. My artist substitution might 
   be into a directory, in which case I don't have enough 
   room for a legit file name */
/* Also, what about versioning of completed filenames? */
/* If (TRACK_INFO* ti) is NULL, that means we're being called for the 
   showfile, and therefore some parts don't apply */
static void
parse_and_subst_pat (RIP_MANAGER_INFO* rmi,
		     gchar* newfile,
		     TRACK_INFO* ti,
		     gchar* directory,
		     gchar* pattern,
		     gchar* extension)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    gchar stripped_artist[SR_MAX_PATH];
    gchar stripped_title[SR_MAX_PATH];
    gchar stripped_album[SR_MAX_PATH];
    gchar temp[DATEBUF_LEN];
    gchar datebuf[DATEBUF_LEN];
    int opi = 0;
    int nfi = 0;
    int done;
    gchar* pat = pattern;

    /* Reserve 5 bytes: 4 for the .mp3 extension, and 1 for null char */
    int MAX_FILEBASELEN = SR_MAX_PATH-5;

    mstrcpy (newfile, directory);
    opi = 0;
    nfi = mstrlen(newfile);
    done = 0;

    /* Strip artist, title, album */
    debug_printf ("parse_and_subst_pat: stripping\n");
    if (ti) {
	mstrncpy (stripped_artist, ti->artist, SR_MAX_PATH);
	mstrncpy (stripped_title, ti->title, SR_MAX_PATH);
	mstrncpy (stripped_album, ti->album, SR_MAX_PATH);
	replace_invalid_chars (stripped_artist);
	replace_invalid_chars (stripped_title);
	replace_invalid_chars (stripped_album);
    }

    debug_printf ("parse_and_subst_pat: substitute pattern\n");
    while (nfi < MAX_FILEBASELEN) {
	if (pat[opi] == 0) {
	    done = 1;
	    break;
	}
	if (pat[opi] != m_('%')) {
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    continue;
	}
	/* If we got here, we have a '%' */
	switch (pat[opi+1]) {
	case m_('%'):
	    newfile[nfi++] = m_('%');
	    newfile[nfi] = 0;
	    opi+=2;
	    continue;
	case m_('S'):
	    /* stream name */
	    /* GCS FIX: Not sure here */
	    mstrncat (newfile, fli->m_stripped_icy_name, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('d'):
	    /* append date info */
	    mstrncat (newfile, fli->m_session_datebuf, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('D'):
	    /* current timestamp */
	    fill_date_buf (rmi, datebuf, DATEBUF_LEN);
	    mstrncat (newfile, datebuf, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('a'):
	    /* album */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_album, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('A'):
	    /* artist */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_artist, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('q'):
	    /* automatic sequence number */
	    msnprintf (temp, DATEBUF_LEN, m_("%04d"), 
		       get_next_sequence_number (rmi, newfile));
	    mstrncat (newfile, temp, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case m_('T'):
	    /* title */
	    if (!ti) goto illegal_pattern;
	    mstrncat (newfile, stripped_title, MAX_FILEBASELEN-nfi);
	    nfi = mstrlen (newfile);
	    opi+=2;
	    continue;
	case 0:
	    /* The pattern ends in '%', but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    done = 1;
	    break;
	case m_('0'): case m_('1'): case m_('2'): case m_('3'): case m_('4'): 
	case m_('5'): case m_('6'): case m_('7'): case m_('8'): case m_('9'): 
	    {
		/* Get integer */
		int ai = 0;
		gchar ascii_buf[7];      /* max 6 chars */
		while (isdigit (pat[opi+1+ai]) && ai < 6) {
		    ascii_buf[ai] = pat[opi+1+ai];
		    ai ++;
		}
		ascii_buf[ai] = 0;
		/* If we got a q, get starting number */
		if (pat[opi+1+ai] == m_('q')) {
		    if (fli->m_count == -1) {
			fli->m_count = mtol(ascii_buf);
		    }
		    msnprintf (temp, DATEBUF_LEN, m_("%04d"), fli->m_count);
		    mstrncat (newfile, temp, MAX_FILEBASELEN-nfi);
		    nfi = mstrlen (newfile);
		    opi+=ai+2;
		    continue;
		}
		/* Otherwise, no 'q', so drop through to default case */
	    }
	default:
	illegal_pattern:
	    /* Illegal pattern, but that's ok. */
	    newfile[nfi++] = pat[opi++];
	    newfile[nfi] = 0;
	    continue;
	}
    }

    /* Pop on the extension */
    /* GCS FIX - is SR_MAX_PATH right here? */
    debug_printf ("parse_and_subst_pat: pop on the extension\n");
    mstrncat (newfile, extension, SR_MAX_PATH);
}

static long
get_file_size (RIP_MANAGER_INFO* rmi, gchar *filename)
{
    FILE* fp;
    long len;
    char fn[SR_MAX_PATH];

    string_from_gstring (rmi, fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    fp = fopen (fn, "r");
    if (!fp) return 0;

    if (fseek (fp, 0, SEEK_END)) {
	fclose(fp);
	return 0;
    }

    len = ftell (fp);
    if (len < 0) {
	fclose(fp);
	return 0;
    }

    fclose (fp);
    return len;
}

/*
 * Added by Daniel Lord 29.06.2005 to only overwrite files with better 
 * captures, modified by GCS to get file size from file system 
 */
static BOOL
new_file_is_better (RIP_MANAGER_INFO* rmi, gchar *oldfile, gchar *newfile)
{
    long oldfilesize=0;
    long newfilesize=0;

    oldfilesize = get_file_size (rmi, oldfile);
    newfilesize = get_file_size (rmi, newfile);
    
    /*
     * simple size check for now. Newfile should have at least 1Meg. Else it's
     * not very usefull most of the time.
     */
    /* GCS: This isn't quite true for low bitrate streams.  */
#if defined (commentout)
    if (newfilesize <= 524288) {
	debug_printf("NFB: newfile smaller as 524288\n");
	return FALSE;
    }
#endif

    if (oldfilesize == -1) {
	/* make sure we get the file in case of errors */
	debug_printf("NFB: could not get old filesize\n");
	return TRUE;
    }

    if (oldfilesize == newfilesize) {
	debug_printf("NFB: Size Match\n");
	return FALSE;
    }

    if (newfilesize < oldfilesize) {
	debug_printf("NFB:newfile bigger as oldfile\n");
	return FALSE;
    }

    debug_printf ("NFB:oldfilesize = %li, newfilesize = %li, "
		  "overwriting file\n", oldfilesize, newfilesize);
    return TRUE;
}

static void
truncate_file (RIP_MANAGER_INFO* rmi, gchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_gstring (rmi, fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    debug_printf ("Trying to truncate file: %s\n", fn);
#if defined WIN32
    CloseHandle (CreateFile(fn, GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
		TRUNCATE_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, NULL));
#else
    close (open (fn, O_RDWR | O_CREAT | O_TRUNC, 
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#endif
}

static void
move_file (RIP_MANAGER_INFO* rmi, gchar* new_filename, gchar* old_filename)
{
    char old_fn[SR_MAX_PATH];
    char new_fn[SR_MAX_PATH];
    string_from_gstring (rmi, old_fn, SR_MAX_PATH, old_filename, CODESET_FILESYS);
    string_from_gstring (rmi, new_fn, SR_MAX_PATH, new_filename, CODESET_FILESYS);
#if defined WIN32
    MoveFile(old_fn, new_fn);
#else
    rename (old_fn, new_fn);
#endif
}

static void
delete_file (RIP_MANAGER_INFO* rmi, gchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_gstring (rmi, fn, SR_MAX_PATH, filename, CODESET_FILESYS);
#if defined WIN32
    DeleteFile (fn);
#else
    unlink (fn);
#endif
}

static error_code
filelib_open_for_write (RIP_MANAGER_INFO* rmi, FHANDLE* fp, gchar* filename)
{
    char fn[SR_MAX_PATH];
    string_from_gstring (rmi, fn, SR_MAX_PATH, filename, CODESET_FILESYS);
    debug_printf ("Trying to create file: %s\n", fn);
#if WIN32
    *fp = CreateFile (fn, GENERIC_WRITE,         // open for reading 
		      FILE_SHARE_READ,           // share for reading 
		      NULL,                      // no security 
		      CREATE_ALWAYS,             // existing file only 
		      FILE_ATTRIBUTE_NORMAL,     // normal file 
		      NULL);                     // no attr. template 
    if (*fp == INVALID_FHANDLE) {
	int r = GetLastError();
	r = strlen(fn);
	printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#else
    /* For unix, we need to convert to char, and just open. 
       http://mail.nl.linux.org/linux-utf8/2001-02/msg00103.html
    */
    *fp = open (fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (*fp == INVALID_FHANDLE) {
	/* GCS FIX -- need error message here! */
	// printf ("ERROR creating file: %s\n",filename);
	return SR_ERROR_CANT_CREATE_FILE;
    }
#endif
    return SR_SUCCESS;
}

static error_code
filelib_write (FHANDLE fp, char *buf, u_long size)
{
    if (!fp) {
	debug_printf("filelib_write: fp = 0\n");
	return SR_ERROR_CANT_WRITE_TO_FILE;
    }
#if WIN32
    {
	BOOL rc;
	DWORD bytes_written = 0;
	rc = WriteFile(fp, buf, size, &bytes_written, NULL);
	if (rc == 0) {
	    debug_print_error();
	    debug_printf("filelib_write: WriteFile rc = 0\n");
	    debug_printf("size = %d, bytes_written = %d\n", size, bytes_written);
	    return SR_ERROR_CANT_WRITE_TO_FILE;
	}
    }
#else
    if (write(fp, buf, size) == -1)
	return SR_ERROR_CANT_WRITE_TO_FILE;
#endif

    return SR_SUCCESS;
}

/* This function takes in a directory, filename base, and extension, 
   and renames any existing file "${directory}/${fnbase}${extensions}"
   to a file of the form "${directory}/${fnbase} (${n}}${extensions}" 

   The new name for the file is returned in new_fn (if new_fn is not 0).
*/
static void
filelib_rename_versioned (gchar** new_fn, RIP_MANAGER_INFO* rmi, 
			  gchar* directory, gchar* fnbase, gchar* extension)
{
    /* Compose and trim filename (not including directory) */
    gint n = 1;
    gchar* tgt_file = g_strdup_printf ("%s%s", fnbase, extension);
    gchar* tgt_path = g_build_filename (directory, tgt_file, NULL);
    g_free (tgt_file);

    if (new_fn) {
	*new_fn = 0;
    }

    if (!file_exists (rmi, tgt_path)) {
	debug_printf ("filelib_rename_versioned: tgt_path doesn't exist (%s)\n",
		      tgt_path);
	g_free (tgt_path);
	return;
    }

    for (n = 1; n <= G_MAXINT; n++) {
	gchar *ren_file, *ren_path;
	ren_file = g_strdup_printf ("%s (%d)%s", fnbase, n, extension);
	ren_path = g_build_filename (directory, ren_file, NULL);
	if (file_exists (rmi, ren_path)) {
	    g_free (ren_file);
	    g_free (ren_path);
	    continue;
	}
	/* GCS FIX: This should check for ENAMETOOLONG and other errors */
	debug_printf ("filelib_rename_versioned: moving file (%s)->(%s)\n",
		      tgt_path, ren_path);
	move_file (rmi, ren_path, tgt_path);
	g_free (ren_file);
	if (new_fn) {
	    *new_fn = ren_path;
	} else {
	    g_free (ren_path);
	}
	break;
    }
    g_free (tgt_path);
}

static void
filelib_adjust_cuefile (RIP_MANAGER_INFO* rmi, gchar* new_show_name, 
			gchar* new_cue_name)
{
    gchar* tmp_fn;
    gint fd;
    FILE *fp_in, *fp_out;
    GError *error = NULL;
    char cue_buf[1024];
    gchar mcue_buf[1024];
    gchar* show_fnbase;
    int rc;
    
    /* Create temporary file */
    fd = g_file_open_tmp ("streamripper-XXXXXX", &tmp_fn, &error);
    if (!fd || error) {
        debug_printf ("Error with g_file_open_tmp\n");
        if (tmp_fn) g_free (tmp_fn);
        return;
    }
    close (fd);
    
    /* Open temporary file as FILE* */
    fp_out = g_fopen (tmp_fn, "w");
    if (!fp_out) {
        debug_printf ("Error with g_fopen (%s)\n", tmp_fn);
        if (tmp_fn) g_free (tmp_fn);
        return;
    }

    /* Open input file as FILE* */
    fp_in = g_fopen (new_cue_name, "r");
    if (!fp_in) {
        debug_printf ("Error with g_fopen (%s)\n", new_cue_name);
        if (tmp_fn) g_free (tmp_fn);
	fclose (fp_out);
        return;
    }
   
    debug_printf ("Adjusting cue file=%s\n", tmp_fn);

    /* Write renamed mp3 showfile to output file */
    show_fnbase = g_path_get_basename (new_show_name);
    rc = msnprintf (mcue_buf, 1024, m_("FILE \"") m_S m_("\" MP3\n"), 
		    show_fnbase);
    g_free (show_fnbase);
    rc = string_from_gstring (rmi, cue_buf, 1024, mcue_buf, CODESET_FILESYS);
    fwrite (cue_buf, 1, rc, fp_out);

    /* Skip line in input file */
    fgets (cue_buf, 1024, fp_in);

    /* Copy remaining bytes from input file to output file */
    while (1) {
	rc = fgetc (fp_in);
	if (rc == EOF) {
	    break;
	}
	fputc (rc, fp_out);
    }

    fclose (fp_in);
    fclose (fp_out);

    /* Delete previous cue file */
    delete_file (rmi, new_cue_name);

    /* Move temp file to new cue file */
    move_file (rmi, new_cue_name, tmp_fn);
    g_free (tmp_fn);
}

static error_code
filelib_open_showfiles (RIP_MANAGER_INFO* rmi)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    int rc;
    gchar mcue_buf[1024];
    char cue_buf[1024];
    gchar* basename;
    gchar *new_dir, *new_fnbase;
    gchar *new_show_name, *new_cue_name;

    parse_and_subst_pat (rmi, fli->m_show_name, 0, fli->m_showfile_directory, 
			 fli->m_showfile_pattern, fli->m_extension);
    parse_and_subst_pat (rmi, fli->m_cue_name, 0, fli->m_showfile_directory, 
			 fli->m_showfile_pattern, 
			 m_(".cue"));

    /* Rename previously ripped files with same name */
    new_dir = g_path_get_dirname (fli->m_show_name);
    new_fnbase = g_path_get_basename (fli->m_show_name);
    trim_mp3_suffix (rmi, new_fnbase);
    filelib_rename_versioned (&new_show_name, rmi, new_dir, 
			      new_fnbase, fli->m_extension);
    filelib_rename_versioned (&new_cue_name, rmi, new_dir, new_fnbase, 
			      m_(".cue"));
    if (new_show_name && new_cue_name) {
	/* Rewrite previous cue file to point to renamed mp3 showfile */
	debug_printf ("Trying to adjust cuefile\n");
	filelib_adjust_cuefile (rmi, new_show_name, new_cue_name);
    }
    if (new_show_name) g_free (new_show_name);
    if (new_cue_name) g_free (new_cue_name);
    g_free (new_dir);
    g_free (new_fnbase);

    /* Open cue file, write header */
    if (rmi->http_info.content_type != CONTENT_TYPE_OGG) {
	rc = filelib_open_for_write (rmi, &fli->m_cue_file, fli->m_cue_name);
	if (rc != SR_SUCCESS) {
	    fli->m_do_show = 0;
	    return rc;
	}

	/* Write cue header here */
	/* GCS Nov 29, 2007 - As suggested on the forum, the cue file
	   should use relative path. */
	basename = mstrrchr (fli->m_show_name, PATH_SLASH);
	if (basename) {
	    basename++;
	} else {
	    basename = fli->m_show_name;
	}
	debug_mprintf (m_("show_name: ") m_S m_(", basename: ") m_S m_("\n"),
		       fli->m_show_name, basename);
	rc = msnprintf (mcue_buf, 1024, m_("FILE \"") m_S m_("\" MP3\n"), 
			basename);
	rc = string_from_gstring (rmi, cue_buf, 1024, mcue_buf, CODESET_FILESYS);
	rc = filelib_write (fli->m_cue_file, cue_buf, rc);
	if (rc != SR_SUCCESS) {
	    fli->m_do_show = 0;
	    return rc;
	}
    }

    /* Open show file */
    rc = filelib_open_for_write (rmi, &fli->m_show_file, fli->m_show_name);
    if (rc != SR_SUCCESS) {
	fli->m_do_show = 0;
	return rc;
    }
    return rc;
}

/* GCS: This should get only the name, not the directory */
static void
trim_filename (RIP_MANAGER_INFO* rmi, gchar* out, gchar *filename)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    long maxlen = fli->m_max_filename_length;
    mstrncpy (out, filename, MAX_TRACK_LEN);
    replace_invalid_chars (out);
    out[maxlen-4] = 0;	// -4 = make room for ".mp3"
}

static void
trim_mp3_suffix (RIP_MANAGER_INFO* rmi, gchar *filename)
{
    FILELIB_INFO* fli = &rmi->filelib_info;
    gchar* suffix_ptr;
    if (mstrlen(filename) <= 4) return;
    suffix_ptr = filename + mstrlen(filename) - 4;  // -4 for ".mp3"
    if (mstrcmp (suffix_ptr, fli->m_extension) == 0) {
	*suffix_ptr = 0;
    }
}

/* GCS FIX: This may not work with filesystem charsets where 0-9 are 
   not ascii compatible? */
static int
get_next_sequence_number (RIP_MANAGER_INFO* rmi, gchar* fn_base)
{
    int rc;
    int di = 0;
    int edi = 0;
    int seq;
    gchar dir_name[SR_MAX_PATH];
    gchar fn_prefix[SR_MAX_PATH];
    char dname[SR_MAX_PATH];
    char fnp[SR_MAX_PATH];
    DIR* dp;
    struct dirent* de;

    /* Get directory from fn_base */
    while (fn_base[di]) {
	if (ISSLASH(fn_base[di])) {
	    edi = di;
	}
	di++;
    }
    mstrncpy (dir_name, fn_base, SR_MAX_PATH);
    dir_name[edi] = 0;

    /* Get fn prefix from fn_base */
    fn_prefix[0] = 0;
    mstrcpy (fn_prefix, &fn_base[edi+1]);

    rc = string_from_gstring (rmi, dname, SR_MAX_PATH, dir_name, CODESET_FILESYS);
    rc = string_from_gstring (rmi, fnp, SR_MAX_PATH, fn_prefix, CODESET_FILESYS);

    /* Look through directory for a filenames that match prefix */
    if ((dp = opendir (dname)) == 0) {
	return 0;
    }
    seq = 0;
    while ((de = readdir (dp)) != 0) {
	if (strncmp(de->d_name, fnp, strlen(fnp)) == 0) {
	    if (isdigit(de->d_name[strlen(fnp)])) {
		int this_seq = atoi(&de->d_name[strlen(fnp)]);
		if (seq <= this_seq) {
		    seq = this_seq + 1;
		}
	    }
	}
    }
    closedir (dp);
    return seq;
}

static gchar* 
replace_invalid_chars (gchar *str)
{
    gchar invalid_chars[] = m_("\\/:*?\"<>|~");
    gchar replacement = m_('-');

    gchar *oldstr = str;
    gchar *newstr = str;

    if (!str) return NULL;

    /* Skip leading "." */
    for (;*oldstr; oldstr++) {
	if (*oldstr != '.') {
	    break;
	}
    }

    for (;*oldstr; oldstr++) {
	if (g_ascii_iscntrl (*oldstr)) {
	    /* Do nothing -- skip control characters without replacement */
	}
	else if (mstrchr(invalid_chars, *oldstr) == NULL) {
	    /* Ordinary case -- copy */
	    *newstr++ = *oldstr;
	}
	else {
	    /* Replace case -- append replacement char */
	    *newstr++ = replacement;
	}
    }
    *newstr = '\0';

    return str;
}

static void
remove_trailing_periods (gchar *str)
{
    gchar* s = str + strlen (str);
    while (--s >= str && *s == '.') {
	*s = 0;
    }
}
