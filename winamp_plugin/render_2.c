/* render_2.c
 * Version 2 of skinning interface.
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
#include <time.h>
#include <stdio.h>
#include "glib.h"
#include "render_2.h"
#include "winamp_exe.h"
#include "plugin_main.h"
#include "debug.h"
#include "zlib.h"
#include "ioapi.h"
#include "unzip.h"
#include "debug_box.h"

static int
is_bmp (char* filename)
{
    /* GCS FIX: convert to lowercase first? */
    return g_str_has_suffix (filename, ".bmp");
}

static int
is_txt (char* filename)
{
    /* GCS FIX: convert to lowercase first? */
    return g_str_has_suffix (filename, ".txt");
}

static void*
read_from_zip (unzFile uf, unz_file_info* file_info)
{
    int err;
    void* buf;

    buf = malloc (file_info->uncompressed_size);
    if (!buf) return 0;

    err = unzOpenCurrentFilePassword (uf, 0);
    if (err != UNZ_OK) {
	return 0;
    }

    err = unzReadCurrentFile (uf, buf, file_info->uncompressed_size);
    if (err != (int) file_info->uncompressed_size) {
	/* Didn't read all bytes? */
	free (buf);
	return 0;
    }

    unzCloseCurrentFile (uf);

    return buf;
}

static void
render2_load (void** bmp,           /* output */
	      void** txt,           /* output */
	      int* txt_len,         /* output */
	      const char* zipfile   /* input */
	      )
{
    unzFile uf=NULL;
    uLong i;
    unz_global_info gi;
    int err;
#ifdef USEWIN32IOAPI
    zlib_filefunc_def ffunc;
#endif

#ifdef USEWIN32IOAPI
    fill_win32_filefunc (&ffunc);
    uf = unzOpen2 (zipfile, &ffunc);
#else
    uf = unzOpen (zipfile);
#endif
    if (uf == NULL) {
	/* File not found */
	return;
    }

    err = unzGetGlobalInfo (uf, &gi);
    if (err != UNZ_OK) {
	/* Error during header read */
	return;
    }
    
    for (i = 0; i < gi.number_entry; i++) {
        char filename_inzip[1024];
        unz_file_info file_info;
        err = unzGetCurrentFileInfo (uf, &file_info, filename_inzip, 
		sizeof(filename_inzip), NULL, 0, NULL, 0);
	debug_printf ("Loading zipped file: %s\n", filename_inzip);
	if (err != UNZ_OK) {
	    /* Error during header read */
	    return;
	}
	if (is_bmp (filename_inzip)) {
	    *bmp = read_from_zip (uf, &file_info);
	}
	if (is_txt (filename_inzip)) {
	    *txt = read_from_zip (uf, &file_info);
	    if (*txt) {
		*txt_len = file_info.uncompressed_size;
	    }
	}
        if ((i+1)<gi.number_entry) {
            err = unzGoToNextFile(uf);
	    if (err != UNZ_OK) {
		/* Error during header read */
		return;
	    }
        }
    }
    unzCloseCurrentFile(uf);
}

void
render2_load_skin (HBITMAP* hbitmap,     /* output */
		   void** txt,           /* output */
		   int* txt_len,         /* output */
		   const char* skinfile  /* input */
		   )
{
    void *bmp = 0;
    BITMAPFILEHEADER *bmfh;
    BITMAPINFO *bmi;
    LPBYTE lpbits;
    HDC hdc;

    /* Initialize outputs */
    *hbitmap = NULL;
    *txt = 0;
    *txt_len = 0;

    /* First, load entire bmp & txt into memory */
    render2_load (&bmp, txt, txt_len, skinfile);
    if (!bmp) {
	/* We only return a txt if the bmp is valid */
	if (*txt) free (*txt);
	*txt = 0;
	*txt_len = 0;
	return;
    }

    /* GCS Fix: check BMP sanity */

    /* Next, convert into bitmap */
    bmfh = (BITMAPFILEHEADER*) bmp;
    bmi = (BITMAPINFO*) ((LPBYTE) bmp + sizeof(BITMAPFILEHEADER));
    lpbits = (LPBYTE) bmp + bmfh->bfOffBits;
    hdc = GetDC (NULL);
    *hbitmap = CreateDIBitmap (hdc, (BITMAPINFOHEADER*) bmi, CBM_INIT, lpbits, bmi, DIB_RGB_COLORS);
    ReleaseDC (NULL, hdc);

    /* Free bmp memory -- caller frees txt memory */
    free (bmp);
}
