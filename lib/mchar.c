/* mchar.c
 * Codeset and wide character processing
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
 * REFS:
 *   http://mail.nl.linux.org/linux-utf8/2001-04/msg00083.html
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html
 *   http://mail.nl.linux.org/linux-utf8/2001-06/msg00020.html
 *   http://mail.nl.linux.org/linux-utf8/2001-04/msg00254.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include "srconfig.h"
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#if defined HAVE_LOCALE_CHARSET
#include <localcharset.h>
#elif defined HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif
#include <locale.h>
#include <time.h>
#include <errno.h>
#include "debug.h"
#include "srtypes.h"
#include "mchar.h"

#if WIN32
    #define ICONV_WCHAR "UCS-2-INTERNAL"
    #define vsnprintf _vsnprintf
//    #define vswprintf _vsnwprintf
#else
    #define ICONV_WCHAR "WCHAR_T"
    /* This prototype is missing in some systems */
//    int vswprintf (wchar_t * ws, size_t n, const wchar_t * format, va_list arg);
#endif


/*****************************************************************************
 * Public functions
 *****************************************************************************/
char	*left_str(char *str, int len);
char	*subnstr_until(const char *str, char *until, char *newstr, int maxlen);
char	*format_byte_size(char *str, long size);
void	trim(char *str);


/*****************************************************************************
 * These functions are NOT mchar related
 *****************************************************************************/
char*
subnstr_until(const char *str, char *until, char *newstr, int maxlen)
{
    const char *p = str;
    int len = 0;

    for (len = 0; strncmp(p, until, strlen(until)) != 0 && len < maxlen-1; p++)
    {
	newstr[len] = *p;
	len++;
    }
    newstr[len] = '\0';

    return newstr;
}

char *left_str(char *str, int len)
{
    int slen = strlen(str);

    if (slen <= len)
	return str;

    str[len] = '\0';
    return str;
}

char *format_byte_size(char *str, long size)
{
    const long ONE_K = 1024;
    const long ONE_M = ONE_K*ONE_K;

    if (size < ONE_K)
	sprintf(str, "%ldb", size);
    else if (size < ONE_M)
	sprintf(str, "%ldkb", size/ONE_K);
    else 
	sprintf(str, "%.2fM", (float)size/(ONE_M));
	
    return str;
}

void trim(char *str)
{
    char *start = str;
    char *end;
    char *original_end;
    char *test;
    
    /* skip over initial whitespace */
    while (*start && isspace(*start)) {
        ++start;
    }
    
    /* locate end of string */
    end = start;
    while (*end) {
        ++end;
    }
    original_end = end;
    
    /* backtrack over final whitespace */
    while (end > start) {
        test = end-1;
        if (isspace(*test)) {
            end = test;
        } else {
            break;
        }
    }
    
    /* move non-whitespace text if initial whitespace was found above. */
    /* move is unnecessary if resulting string is empty */
    if (start > str && start != end) {
        memmove(str, start, end-start);
    }
    
    /* null-terminate resulting string. */
    /* this is necessary in all cases except when the string was not modified */
    if (start > str || end < original_end) {
        str[end-start] = '\0';
    }
}

/* This is a little different from standard strncpy, because:
   1) behavior is known when dst & src overlap
   2) only copy n-1 characters max
   3) then add the null char
*/
void
sr_strncpy (char* dst, char* src, int n)
{
    int i = 0;
    for (i = 0; i < n-1; i++) {
	if (!(dst[i] = src[i])) {
	    return;
	}
    }
    dst[i] = 0;
}

/*****************************************************************************
 * Lstring functions
 *****************************************************************************/
void
lstring_initialize (Lstring* lstring)
{
    lstring->num_bytes = 0;
    lstring->data = 0;
}


/*****************************************************************************
 * These functions ARE mchar related
 *****************************************************************************/
void
gmem_concat
(
 gchar** base_mem,          /* In/Out: The memory to be concatenated to */
 gsize base_bytes,          /* Input: Size of base memory (in bytes) */
 char* concat_mem,	    /* Input: The memory to concatenate */
 gsize concat_bytes	    /* Input: Size of concat_mem (in bytes) */
 )
{
    *base_mem = g_realloc (*base_mem, base_bytes + concat_bytes);
    memcpy (*base_mem + base_bytes, concat_mem, concat_bytes);
}

/* Convert a string, replacing unconvertable characters.
   Returns a gchar* string, which must be freed by caller using g_free. */
void
convert_string_with_replacement
(
 gchar** output_string,     /* Output: The converted string */
 gsize* output_bytes,       /* Output: Size of output string (in bytes) */
 char* input_string,	    /* Input: String to convert */
 gsize input_bytes,	    /* Input: Length of input string (in bytes) */
 char* from_codeset,	    /* Input: Codeset of input string */
 char* to_codeset,	    /* Input: Codeset of output string */
 char* repl		    /* Input: Replacement character (zero terminated,
			       in utf-8) */
 )
{
    GIConv giconv;
    gsize cur = 0;              /* Current byte to convert */
    GError *error = 0;
    gsize bytes_to_convert = input_bytes;
    int need_repl = 1;
    gsize br, bw;
    gchar* repl_string;
    gsize repl_bytes;

    *output_string = 0;
    *output_bytes = 0;

    giconv = g_iconv_open (to_codeset, from_codeset);
    if (giconv == (GIConv) -1) {
	/* Not sure why this would happen */
	debug_printf ("g_iconv_open returned zero\n");
	return;
    }

    /* Convert replacement character from UTF-8 to to_codeset */
    repl_string = g_convert (repl, strlen (repl), 
			     to_codeset, "UTF-8", 
			     &br, &bw, &error);
    if (repl_string && br == strlen (repl)) {
	repl_bytes = bw;
    } else {
	repl_bytes = 0;
	if (repl_string) {
	    g_free (repl_string);
	}
    }

    while (cur < input_bytes) {
	gchar* os;
	int drop_byte = 0;    /* Should we drop a byte? */

	br = 0;
	os = g_convert_with_iconv (&input_string[cur], bytes_to_convert, 
				   giconv, &br, &bw, &error);
	debug_printf ("cur=%d, btc=%d, br=%d, bw=%d (b1=0x%02x)\n", 
		      cur, bytes_to_convert, br, bw, input_string[cur]);

	/* If the conversion was unsuccessful, usually it means that 
	   either the input byte doesn't belong to the from_codeset, 
	   or the code point doesn't belong to the to_codeset.
	*/
	if (error) {
	    /* There could be a partial output -- the specs aren't clear 
	       about this.  Better free it just in case. */
	    if (os) {
		g_free (os);
	    }
	    switch (error->code) {
	    case G_CONVERT_ERROR_ILLEGAL_SEQUENCE:
	    case G_CONVERT_ERROR_FAILED:
	    case G_CONVERT_ERROR_PARTIAL_INPUT:
		debug_printf ("g_convert_with_iconv returned error: %d (%s)\n",
			      error->code, error->message);
		break;
	    case G_CONVERT_ERROR_NO_CONVERSION:
	    default:
		/* This shouldn't happen, as GNU inconv guarantees 
		   conversion to/from UTF-8. */
		debug_printf ("g_convert_with_iconv returned error: %d (%s)\n",
			      error->code, error->message);
		g_error_free (error);
		g_free (repl_string);
		return;
	    }
	    /* How many bytes to try next time? */
	    switch (bytes_to_convert) {
	    case 4:
	    case 3:
	    case 2:
		bytes_to_convert --;
		break;
	    case 1:
		/* Crapped out.  Drop current byte, and add "?" to string. */
		drop_byte = 1;
		break;
	    default:
		/* Best guess based on br value returned from iconv */
		if (br < bytes_to_convert && br > 0) {
		    bytes_to_convert = br;
		} else {
		    bytes_to_convert = 4;
		}
	    }
	    g_error_free (error);
	    error = 0;
	} else {
	    if (br == 0) {
		/* glib 2.16.5 (and probably other versions) doesn't properly 
		   return G_CONVERT_ERROR_PARTIAL_INPUT with partially 
		   translated characters.  We'll detect this condition 
		   when there is no error, but br is 0.	*/
		drop_byte = 1;
	    } else {
		/* A successful conversion. */
		debug_printf ("Successful conversion: %d bytes read\n", br);
		gmem_concat (output_string, *output_bytes, os, bw);
		g_free (os);
		cur += br;
		bytes_to_convert = input_bytes - cur;
		*output_bytes += bw;
		need_repl = 1;
	    }
	}

	/* drop_byte will be true if a conversion failure happened.  
	   Drop current byte from input, and append replacement 
	   character into output.  But only append a single replacement 
	   character for each group of dropped bytes. */
	if (drop_byte) {
	    cur ++;
	    bytes_to_convert = input_bytes - cur;
	    if (need_repl) {
		gmem_concat (output_string, *output_bytes, repl, repl_bytes);
		*output_bytes += repl_bytes;
		need_repl = 0;
	    }
	}
    }
    g_free (repl_string);
    g_iconv_close (giconv);
    debug_printf ("convert_string_with_replacement |%s| -> |%s| (%s -> %s)\n",
		  input_string, *output_string, from_codeset, to_codeset);
    return;
}

/* Assumes src is valid utf8 */
int
utf8cpy (gchar* dst, gchar* src, int dst_len)
{
    gchar *s = src;
    gchar *d = dst;
    gunichar c;
    gint dlen = 0;
    gint clen;

    while (dst_len > 6) {
	c = g_utf8_get_char(s);
	if (!c) break;
	clen = g_unichar_to_utf8 (c, d);
	d += clen;
	dlen += clen;
	dst_len -= clen;
	s = g_utf8_next_char (s);
    }
    *d = 0;
    return dlen;
}

void
lstring_from_lstring (Lstring* lstring_out,/* Output: Output string */
		      Lstring* lstring_in, /* Input: Input string */
		      char* from_codeset,  /* Input: Codeset of input string */
		      char* to_codeset)	   /* Input: Codeset of output string */
{
    convert_string_with_replacement (&lstring_out->data, 
				     &lstring_out->num_bytes, 
				     lstring_in->data, 
				     lstring_out->num_bytes, 
				     from_codeset,
				     to_codeset,
				     "?");
}

void
lstring_from_gstring (Lstring* lstring_out,/* Output: Output string */
		      gchar* gstring_in,   /* Input: Input string */
		      char* to_codeset)	   /* Input: Codeset of output string */
{
    convert_string_with_replacement (&lstring_out->data, 
				     &lstring_out->num_bytes, 
				     gstring_in, 
				     strlen (gstring_in) + 1, 
				     "UTF-8", 
				     to_codeset,
				     "?");
}

/* Input value mlen is measured in mchar, not bytes.
   Return value is the number of mchar occupied by the converted string, 
   not including the null character. 
   
   For GLIB UTF8, it makes more sense to return the dynamically allocated 
   string, and pass in the codeset string itself rather than 
   rmi & codeset type. 
   
   GLIB UTF8 returns number of bytes.
*/
int
gstring_from_string (RIP_MANAGER_INFO* rmi, mchar* m, int mlen, 
		     char* c, int codeset_type)
{
    CODESET_OPTIONS* mchar_cs = &rmi->mchar_cs;
    if (mlen < 0) return 0;
    *m = 0;
    if (!c) return 0;

    {
	gchar* gstring;
	gsize gstring_len;
	char* src_codeset;
	int rc;
	
	switch (codeset_type) {
	case CODESET_UTF8:
	    src_codeset = "UTF-8";
	    break;
	case CODESET_LOCALE:
	    src_codeset = mchar_cs->codeset_locale;
	    break;
	case CODESET_FILESYS:
	    src_codeset = mchar_cs->codeset_filesys;
	    break;
	case CODESET_ID3:
	    src_codeset = mchar_cs->codeset_id3;
	    break;
	case CODESET_METADATA:
	    src_codeset = mchar_cs->codeset_metadata;
	    break;
	case CODESET_RELAY:
	    src_codeset = mchar_cs->codeset_relay;
	    break;
	default:
	    printf ("Program error.  Bad codeset m->c (%d)\n", codeset_type);
	    exit (-1);
	}
	/* GCS FIX: This is not correct, as strlen(c) won't work 
	   for UTF-16. */
	convert_string_with_replacement (&gstring, &gstring_len,
					 c, strlen(c) + 1, 
					 src_codeset, "UTF-8", 
					 "?");
	if (!gstring) {
	    debug_printf ("Error converting gstring_from_string\n");
	    return 0;
	}
	rc = utf8cpy (m, gstring, mlen);
	g_free (gstring);
	return rc;
    }
}

/* Return value is the number of char occupied by the converted string, 
   not including the null character. */
int
string_from_gstring (RIP_MANAGER_INFO* rmi, char* c, int clen, mchar* m, int codeset_type)
{
    CODESET_OPTIONS* mchar_cs = &rmi->mchar_cs;
    if (clen <= 0) return 0;
    *c = 0;
    if (!m) return 0;
    {
	gchar* cstring;
	gsize cstring_len;
	char* tgt_codeset;
	
	switch (codeset_type) {
	case CODESET_UTF8:
	    tgt_codeset = "UTF-8";
	    break;
	case CODESET_LOCALE:
	    tgt_codeset = mchar_cs->codeset_locale;
	    break;
	case CODESET_FILESYS:
	    tgt_codeset = mchar_cs->codeset_filesys;
	    break;
	case CODESET_ID3:
	    tgt_codeset = mchar_cs->codeset_id3;
	    break;
	case CODESET_METADATA:
	    tgt_codeset = mchar_cs->codeset_metadata;
	    break;
	case CODESET_RELAY:
	    tgt_codeset = mchar_cs->codeset_relay;
	    break;
	default:
	    printf ("Program error.  Bad codeset m->c (%d)\n", codeset_type);
	    exit (-1);
	}
	/* This is the new method */
	convert_string_with_replacement (&cstring, &cstring_len,
					 m, strlen(m) + 1, 
					 "UTF-8", 
					 tgt_codeset, 
					 "?");
	if (!cstring) {
	    debug_printf ("Error converting string_from_gstring\n");
	    return 0;
	}
	/* GCS FIX: truncation can chop multibyte string */
	/* This will be fixed by using dynamic memory here... */
	if (cstring_len >= clen) {
	    cstring_len = clen - 1;
	}
	memcpy (c, cstring, cstring_len);
	/* GCS FIX: If converting to UTF-16, need to add 00 to end */
	c[cstring_len] = 0;
	g_free (cstring);
	return cstring_len;
    }
}

const char*
default_codeset (void)
{
    const char* fromcode = 0;

#if defined HAVE_LOCALE_CHARSET
    debug_printf ("Using locale_charset() to get system codeset.\n");
    fromcode = locale_charset ();
#elif defined HAVE_LANGINFO_CODESET
    debug_printf ("Using nl_langinfo() to get system codeset.\n");
    fromcode = nl_langinfo (CODESET);
#else
    debug_printf ("No way to get system codeset.\n");
#endif
    if (!fromcode || !fromcode[0]) {
	debug_printf ("No default codeset, using ISO-8859-1.\n");
	fromcode = "ISO-8859-1";
    } else {
	debug_printf ("Found default codeset %s\n", fromcode);
    }

#if defined (WIN32)
    {
	/* This is just for debugging */
	LCID lcid;
	lcid = GetSystemDefaultLCID ();
	debug_printf ("SystemDefaultLCID: %04x\n", lcid);
	lcid = GetUserDefaultLCID ();
	debug_printf ("UserDefaultLCID: %04x\n", lcid);
    }
#endif

    // #if defined HAVE_ICONV
    //     debug_printf ("Have iconv.\n");
    // #else
    //     debug_printf ("No iconv.\n");
    // #endif

    return fromcode;
}

void
sr_set_locale (void)
{
    setlocale (LC_ALL, "");
    setlocale (LC_CTYPE, "");
    debug_printf ("LOCALE is %s\n",setlocale(LC_ALL,NULL));
}

void
set_codesets_default (CODESET_OPTIONS* cs_opt)
{
    const char* fromcode = 0;

    /* Set default codesets */
    fromcode = default_codeset ();
    if (fromcode) {
	strncpy (cs_opt->codeset_locale, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_filesys, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_metadata, fromcode, MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_relay, fromcode, MAX_CODESET_STRING);
    }

    /* Always use UTF-16 for id3 */
    strncpy (cs_opt->codeset_id3, "UTF-16", MAX_CODESET_STRING);

    /* Don't use UTF-8 for metadata */
    if (!strcmp (fromcode, "UTF-8")) {
	strncpy (cs_opt->codeset_metadata, "ISO-8859-1", MAX_CODESET_STRING);
	strncpy (cs_opt->codeset_relay, "ISO-8859-1", MAX_CODESET_STRING);
    }
}

void
register_codesets (RIP_MANAGER_INFO* rmi, CODESET_OPTIONS* cs_opt)
{
    CODESET_OPTIONS* mchar_cs = &rmi->mchar_cs;

    /* For ID3, force UCS-2, UCS-2LE, UCS-2BE, UTF-16LE, and UTF-16BE 
       to be UTF-16.  This way, we get the BOM like we need.
       This might change if we upgrade to id3v2.4, which allows 
       UTF-8 and UTF-16 without BOM. */
    if (!strncmp (cs_opt->codeset_id3, "UCS-2", strlen("UCS-2")) ||
	!strncmp (cs_opt->codeset_id3, "UTF-16", strlen("UTF-16"))) {
	strcpy (cs_opt->codeset_id3, "UTF-16");
    }

    strcpy (mchar_cs->codeset_locale, cs_opt->codeset_locale);
    strcpy (mchar_cs->codeset_filesys, cs_opt->codeset_filesys);
    strcpy (mchar_cs->codeset_id3, cs_opt->codeset_id3);
    strcpy (mchar_cs->codeset_metadata, cs_opt->codeset_metadata);
    strcpy (mchar_cs->codeset_relay, cs_opt->codeset_relay);
    debug_printf ("Locale codeset: %s\n", mchar_cs->codeset_locale);
    debug_printf ("Filesys codeset: %s\n", mchar_cs->codeset_filesys);
    debug_printf ("ID3 codeset: %s\n", mchar_cs->codeset_id3);
    debug_printf ("Metadata codeset: %s\n", mchar_cs->codeset_metadata);
    debug_printf ("Relay codeset: %s\n", mchar_cs->codeset_relay);
}

/* This is used to set the codeset byte for id3v2 frames */
int
is_id3_unicode (RIP_MANAGER_INFO* rmi)
{
    CODESET_OPTIONS* mchar_cs = &rmi->mchar_cs;
    if (!strcmp ("UTF-16", mchar_cs->codeset_id3)) {
	return 1;
    }
    return 0;
}

void
mstrncpy (mchar* dst, mchar* src, int n)
{
    int i = 0;
    for (i = 0; i < n-1; i++) {
	if (!(dst[i] = src[i])) {
	    return;
	}
    }
    dst[i] = 0;
}

mchar* 
mstrdup (mchar* src)
{
    return strdup (src);
}

mchar* 
mstrcpy (mchar* dest, const mchar* src)
{
    return strcpy (dest, src);
}

size_t
mstrlen (mchar* s)
{
    return strlen (s);
}

/* GCS FIX: gcc can give a warning about vswprintf.  This may require 
   setting gcc -std=c99, or gcc -lang-c99 */
int
msnprintf (mchar* dest, size_t n, const mchar* fmt, ...)
{
    int rc;
    va_list ap;
    va_start (ap, fmt);
    rc = vsnprintf (dest, n, fmt, ap);
    va_end (ap);
    return rc;
}

mchar*
mstrchr (const mchar* ws, mchar wc)
{
    return g_utf8_strchr (ws, -1, g_utf8_get_char(&wc));
}

mchar*
mstrrchr (const mchar* ws, mchar wc)
{
    return g_utf8_strrchr (ws, -1, g_utf8_get_char(&wc));
}

mchar*
mstrncat (mchar* ws1, const mchar* ws2, size_t n)
{
    return strncat (ws1, ws2, n);
}

int
mstrcmp (const mchar* ws1, const mchar* ws2)
{
    return strcmp (ws1, ws2);
}

long int
mtol (const mchar* string)
{
    return strtol (string, 0, 0);
}
