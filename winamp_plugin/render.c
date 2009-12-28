/* render.c
 * renders the streamripper for winamp screen. skinning and stuff.
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
#include "render.h"
#include "render_2.h"
#include "winamp_exe.h"
#include "plugin_main.h"
#include "debug.h"
#include "debug_box.h"

/* On screen, streamripper is 276 x 150 */
/* Winamp is 275 x 116 */

/*********************************************************************************
 * Private defines
 *********************************************************************************/
#define BIG_IMAGE_WIDTH		400
#define BIG_IMAGE_HEIGHT	150
#define WIDTH(rect)		((rect).right-(rect).left+1)
#define HEIGHT(rect)		((rect).bottom-(rect).top+1)

#define HOT_TIMEOUT		1

#define SR_BLACK		RGB(10, 42, 31)
#define	TIMER_ID		101

#define BTN_MODE_COUNT		0x04
#define BTN_MODE_NORMAL		0x00
#define BTN_MODE_PRESSED	0x01
#define BTN_MODE_HOT		0x02
#define BTN_MODE_GRAYED		0x03


/*********************************************************************************
 * Private structs
 *********************************************************************************/
typedef struct BUTTONst
{
    RECT	dest;
    RECT	rt[BTN_MODE_COUNT];
    short	mode;
    void	(*clicked)();
    time_t	hot_timeout;
    BOOL	enabled;
} BUTTON;

typedef struct DISPLAYDATAst
{
    char	str[MAX_RENDER_LINE];
    RECT	rt;
} DISPLAYDATA;

typedef struct BITMAPDCst
{
    HBITMAP	bm;
    HDC		hdc;
} BITMAPDC;

typedef struct SKINDATAst
{
    BITMAPDC	bmdc;
    COLORREF	textcolor;
    HBRUSH	hbrush;
    int		rgn_npoints;
    POINT*	rgn_points;
    RECT	background_rect;
    int		m_num_buttons;
    BUTTON	m_buttons[MAX_BUTTONS];
} SKINDATA;


/*********************************************************************************
 * Private functions
 *********************************************************************************/
static BOOL internal_render_do_paint (SKINDATA* skind, HDC outhdc);
static VOID CALLBACK on_timer(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static BOOL TrimTextOut(HDC hdc, int x, int y, int maxwidth, char *str);
static BOOL load_skindata_from_file (const char* skinfile, SKINDATA* skind);
static void skindata_close (SKINDATA* skind);
static void bitmapdc_close (BITMAPDC b);
static BOOL render_set_background (POINT *rgn_points, int num_points);
static VOID render_set_button_enabled (SKINDATA* skind, HBUTTON hbut, BOOL enabled);
static HBUTTON render_add_button (SKINDATA* skind, RECT *normal, RECT *pressed, RECT *hot, RECT *grayed, RECT *dest, void (*clicked)());
static VOID render_set_prog_rects (RECT *imagert, POINT dest, COLORREF line_color);
static BOOL render_set_display_data_pos (int idr, RECT *rt);
static VOID render_set_text_color(POINT pt);
static void render_set_default (SKINDATA* skind);
void push_rgn_point (SKINDATA* skind, LONG x, LONG y);
void clear_rgn_points (SKINDATA* skind);

#define do_refresh(rect)	(InvalidateRect(m_hwnd, rect, FALSE))

/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static BITMAPDC		m_tempdc;
static SKINDATA		m_current_skin;
static HFONT		m_tempfont;
//static int		m_num_buttons;
//static BUTTON		m_buttons[MAX_BUTTONS];
static RECT		m_prog_rect;
static POINT		m_pt_color;
static time_t		m_time_start;
static BOOL		m_prog_on;
static POINT		m_prog_point = {0, 0};
static COLORREF		m_prog_color;
static HWND		m_hwnd;
static DISPLAYDATA	m_ddinfo[IDR_NUMFIELDS];
static HBUTTON		m_startbut;
static HBUTTON		m_stopbut;
static HBUTTON		m_relaybut;


/******************************************************************************
 * Public functions
 *****************************************************************************/
BOOL
render_init (HWND hWnd, LPCTSTR szBmpFile)
{
    debug_printf ("Render_init looking for skin: %s\n", szBmpFile);

    memset (&m_current_skin, 0, sizeof(SKINDATA));
    render_set_default (&m_current_skin);

    /* Look for requested skin */
    if (!load_skindata_from_file (szBmpFile, &m_current_skin)) {
	/* If requested skin not found, look for default skin(s) */
	if (!load_skindata_from_file ("srskin.zip", &m_current_skin)) {
	    if (!load_skindata_from_file ("srskin.bmp", &m_current_skin)) {
		return FALSE;
	    }
	}
    }

    m_tempdc.hdc = NULL;
    m_tempdc.bm = NULL;

    /* Set a timer for mouseovers */
    SetTimer (hWnd, TIMER_ID, 100, (TIMERPROC)on_timer);
    m_hwnd = hWnd;

    render_set_background (m_current_skin.rgn_points, m_current_skin.rgn_npoints);

    return TRUE;
}

BOOL
render_change_skin (LPCTSTR szBmpFile)
{
    skindata_close (&m_current_skin);
    if (!load_skindata_from_file (szBmpFile, &m_current_skin))
	return FALSE;

    render_set_background (m_current_skin.rgn_points, m_current_skin.rgn_npoints);
    return TRUE;
}

BOOL
render_destroy ()
{

    DeleteObject (m_tempfont);
    skindata_close (&m_current_skin);
    bitmapdc_close (m_tempdc);
    return TRUE;
}

BOOL
render_create_preview (char* skinfile, HDC hdc, long left, long top,
		       long width, long height)
{
    BOOL b;
    //long orig_width = WIDTH(m_rect_background);
    //long orig_hight = HEIGHT(m_rect_background);
    long orig_width;
    long orig_hight;
    SKINDATA skind;
    BITMAPDC tempdc;

    memset (&skind, 0, sizeof(SKINDATA));
    render_set_default (&skind);

    if (!load_skindata_from_file(skinfile, &skind))
	return FALSE;

    orig_width = WIDTH (skind.background_rect);
    orig_hight = HEIGHT (skind.background_rect);
    tempdc.hdc = CreateCompatibleDC(skind.bmdc.hdc);
    tempdc.bm = CreateCompatibleBitmap(skind.bmdc.hdc, orig_width, orig_hight);
    SelectObject(tempdc.hdc, tempdc.bm);

    if (!internal_render_do_paint (&skind, tempdc.hdc))
	return FALSE;
    b = StretchBlt(hdc, left + width / 8, top + height / 4, 
		   3 * (width / 4), 5 * (height / 8),
		   tempdc.hdc, 0, 0, orig_width, orig_hight, 
		   SRCCOPY);
    bitmapdc_close (tempdc);
    skindata_close (&skind);
    if (!b)
	return FALSE;
    return TRUE;
}

VOID
render_set_prog_bar (BOOL on)
{
    m_prog_on = on;
    time(&m_time_start);
    do_refresh(NULL);
}

VOID
render_do_mousemove (HWND hWnd, LONG wParam, LONG lParam)
{
    SKINDATA* skind = &m_current_skin;
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    BUTTON *b;
    int i;

    for (i = 0; i < skind->m_num_buttons; i++)
    {
	b = &skind->m_buttons[i];
	if (PtInRect(&b->dest, pt) &&
	    b->enabled)
	{
	    b->mode = BTN_MODE_HOT;
	    time(&b->hot_timeout);
	    b->hot_timeout += HOT_TIMEOUT;
	    do_refresh(&b->dest);
	}
    }
}

VOID
render_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam)
{
    SKINDATA* skind = &m_current_skin;
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    BUTTON *b;
    int i;

    for (i = 0; i < skind->m_num_buttons; i++)
    {
	b = &skind->m_buttons[i];
	if (PtInRect(&b->dest, pt) &&
	    b->enabled)
	{
	    b->mode = BTN_MODE_HOT;
	    b->clicked();
	}
	do_refresh(&b->dest);
    }
}

VOID
render_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam)
{
    SKINDATA* skind = &m_current_skin;
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    BUTTON *b;
    int i;

    for (i = 0; i < skind->m_num_buttons; i++)
    {
	b = &skind->m_buttons[i];

	if (PtInRect(&b->dest, pt) &&
	    b->enabled)
	{
	    b->mode = BTN_MODE_PRESSED;
	}
	do_refresh(&b->dest);
    }
}

VOID
render_clear_all_data()
{
    int i;
    for(i = 0; i < IDR_NUMFIELDS; i++)
	memset(m_ddinfo[i].str, 0, MAX_RENDER_LINE);
}

BOOL
render_set_display_data(int idr, char *format, ...)
{
    va_list va;

    if (idr < 0 || idr > IDR_NUMFIELDS || !format)
	return FALSE;

    va_start (va, format);
    _vsnprintf (m_ddinfo[idr].str, MAX_RENDER_LINE, format, va);
    va_end(va);
    return TRUE;
}

BOOL
render_do_paint (HDC hdc)
{
    return internal_render_do_paint (&m_current_skin, hdc);
}

void
render_start_button_enable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_startbut, TRUE);
}

void
render_start_button_disable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_startbut, FALSE);
}

void
render_stop_button_enable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_stopbut, TRUE);
}

void
render_stop_button_disable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_stopbut, FALSE);
}

void
render_relay_button_enable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_relaybut, TRUE);
}

void
render_relay_button_disable ()
{
    SKINDATA* skind = &m_current_skin;
    render_set_button_enabled (skind, m_relaybut, FALSE);
}

/******************************************************************************
 * Private functions
 *****************************************************************************/
void
push_rgn_point (SKINDATA* skind, LONG x, LONG y)
{
    skind->rgn_npoints ++;
    skind->rgn_points = (POINT*) realloc (skind->rgn_points, sizeof (POINT) * skind->rgn_npoints);
    skind->rgn_points[skind->rgn_npoints-1].x = x;
    skind->rgn_points[skind->rgn_npoints-1].y = y;
}

void
clear_rgn_points (SKINDATA* skind)
{
    if (skind->rgn_points) {
	free (skind->rgn_points);
	skind->rgn_points = 0;
    }
    skind->rgn_npoints = 0;
}

static void
render_set_default (SKINDATA* skind)
{
    {
	// points for the shape of the main window
	int i, npoints;
	POINT default_rgn_points[] = {
	    {2,		0},
	    {273+1,	0},
	    {273+1,	1},
	    {274+1,	1},
	    {274+1,	2},
	    {275+1,	2},
	    {275+1,	147+1},
	    {274+1,	147+1},
	    {274+1,	148+1},
	    {273+1,	148+1},
	    {273+1,	149+1},
	    {2,	149+1},
	    {2,	148+1},
	    {1,	148+1},
	    {1,	147+1},
	    {0,	147+1},
	    {0,	2},
	    {1,	2},
	    {1,	1},
	    {2,	1}
	};

	clear_rgn_points (skind);
	npoints = sizeof (default_rgn_points)/sizeof(POINT);
	for (i = 0; i < npoints; i++) {
	    push_rgn_point (skind, default_rgn_points[i].x, default_rgn_points[i].y);
	}
    }

    {
	RECT rt = {0, 0, 276, 150};	// background
        memcpy (&skind->background_rect, &rt, sizeof(RECT));
    }

    // Start button
    {
	RECT rt[] = {
	    {277, 1, 325, 21},		// Noraml
	    {327, 1, 375, 21},		// Pressed
	    {277, 67, 325, 87},		// Over
	    {327, 67, 375, 87},		// Grayed
	    {12, 123, 60, 143}		// Dest
	};
	m_startbut = render_add_button (skind, &rt[0], &rt[1], &rt[2], &rt[3], &rt[4], start_button_pressed);
    }

    // Stop button
    {
	RECT rt[] = {
	    {277, 23, 325, 43},
	    {327, 23, 375, 43},
	    {277, 89, 325, 109},
	    {327, 89, 375, 109},
	    {68, 123, 116, 143} // dest
	};
	m_stopbut = render_add_button (skind, &rt[0], &rt[1], &rt[2], &rt[3], &rt[4], stop_button_pressed);
    }

    // Options button
    {
	RECT rt[] = {
	    {277, 45, 325, 65},
	    {327, 45, 375, 65},
	    {277, 111, 325, 131},
	    {327, 111, 375, 131},
	    {215, 123, 263, 143} // dest
	};
	render_add_button (skind, &rt[0], &rt[1], &rt[2], &rt[3], &rt[4], options_button_pressed);
    }

    // Min  button
    {
	RECT rt[] = {
	    {373, 133, 378, 139},
	    {387, 133, 392, 139},	//pressed
	    {380, 133, 385, 139},	//over
	    {373, 133, 378, 139},	//gray
	    {261, 4, 266, 10}
	};
	render_add_button (skind, &rt[0], &rt[1], &rt[2], &rt[3], &rt[4], close_button_pressed);
    }

    // Relay button
    {
	RECT rt[] = { 
	    {277, 133, 299, 148},	// normal
	    {325, 133, 347, 148},	// pressed
	    {301, 133, 323, 148},
	    {349, 133, 371, 148},   // Grayed
	    {10, 24, 32, 39}
	};
	m_relaybut = render_add_button (skind, &rt[0], &rt[1], &rt[2], &rt[3], &rt[4], relay_pressed);
    }

    // Set the progress bar
    {
	RECT rt = {373, 141, 378, 148};	// progress bar image
	POINT pt = {105, 95};
	render_set_prog_rects (&rt, pt, RGB(100, 111, 132));
    }

    // Set positions for out display text
    {
	int i;
	RECT rt[] = {
	    {42, 95, 88, 95+12},	// Status
	    {42, 49, 264, 49+12},	// Filename
	    {42, 26, 264, 26+12},	// Streamname
	    {42, 66, 264, 66+12},	// Server Type
	    {42, 78, 85, 78+12},	// Bitrate
	    {92, 78, 264, 78+12}	// MetaInt
	};
				
	for(i = 0; i < IDR_NUMFIELDS; i++)
	    render_set_display_data_pos(i, &rt[i]);
    }
    {
	POINT pt = {380, 141};
	render_set_text_color (pt);
    }

    {
	debug_printf ("IDR_STREAMNAME:Loading please wait...\n");
	render_set_display_data(IDR_STREAMNAME, "Loading please wait...");
    }
}

static VOID
render_set_text_color(POINT pt)
{
    m_pt_color = pt;
    m_current_skin.textcolor = GetPixel (m_current_skin.bmdc.hdc, pt.x, pt.y);

    if (m_current_skin.hbrush)
	DeleteObject (m_current_skin.hbrush);
    m_current_skin.textcolor = GetPixel (m_current_skin.bmdc.hdc, pt.x, pt.y);
    m_current_skin.hbrush = CreateSolidBrush (m_current_skin.textcolor);
}

static BOOL
render_set_background (POINT *rgn_points, int num_points)
{
    HRGN rgn = CreatePolygonRgn (rgn_points, num_points, WINDING);

    if (!rgn)
	return FALSE;

    if (!SetWindowRgn (m_hwnd, rgn, TRUE))
	return FALSE;

    return TRUE;
}

static VOID
render_set_prog_rects (RECT *imagert, POINT dest, COLORREF line_color)
{
    memcpy(&m_prog_rect, imagert, sizeof(RECT));
    m_prog_color= line_color;
    m_prog_point = dest;
}

static VOID
render_set_button_enabled (SKINDATA* skind, HBUTTON hbut, BOOL enabled)
{
    skind->m_buttons[hbut].enabled = enabled;
    if (enabled) {
	if (skind->m_buttons[hbut].mode != BTN_MODE_HOT)
	    skind->m_buttons[hbut].mode = BTN_MODE_NORMAL;
    } else {
	skind->m_buttons[hbut].mode = BTN_MODE_GRAYED;
    }
    do_refresh(NULL);
}

static HBUTTON
render_add_button (SKINDATA* skind, RECT *normal, RECT *pressed, RECT *hot, RECT *grayed, RECT *dest, void (*clicked)())
{
    BUTTON *b;
    if (skind->m_num_buttons > MAX_BUTTONS || !clicked)
	return FALSE;

    b = &skind->m_buttons[skind->m_num_buttons];
    memcpy (&b->rt[BTN_MODE_NORMAL], normal, sizeof(RECT));
    memcpy (&b->rt[BTN_MODE_PRESSED], pressed, sizeof(RECT));
    memcpy (&b->rt[BTN_MODE_HOT], hot, sizeof(RECT));
    memcpy (&b->rt[BTN_MODE_GRAYED], grayed, sizeof(RECT));
    memcpy (&b->dest, dest, sizeof(RECT));
    b->mode = BTN_MODE_NORMAL;
    b->clicked = clicked;
    b->enabled = TRUE;
    skind->m_num_buttons++;
    return (HBUTTON) skind->m_num_buttons-1;
}

static VOID CALLBACK
on_timer (HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    SKINDATA* skind = &m_current_skin;
    POINT pt;
    RECT winrt;
    BUTTON *b = &skind->m_buttons[0];
    time_t now;
    int i;

    GetCursorPos(&pt);
    GetWindowRect(hWnd, &winrt);

    pt.x -= winrt.left;
    pt.y -= winrt.top;

    time(&now);

    // Turn off a hot button if it hasn't been mouse over'd for a while 
    for (i = 0; i < skind->m_num_buttons; i++) {
	b = &skind->m_buttons[i];
	if (!PtInRect(&b->dest, pt) && 
	    b->hot_timeout >= now &&
	    b->mode == BTN_MODE_HOT)
	{
	    b->mode = BTN_MODE_NORMAL;
	    do_refresh(&b->dest);
	}
    }

    // refresh display every second.
    {
	static time_t last_diff = 0;

	if ((now-m_time_start) != last_diff && m_prog_on) {
	    do_refresh(NULL);
	}
	last_diff = now-m_time_start;
    }
}

static BOOL
render_set_display_data_pos(int idr, RECT *rt)
{
    if (idr < 0 || idr > IDR_NUMFIELDS || !rt)
	return FALSE;

    memcpy (&m_ddinfo[idr].rt, rt, sizeof(RECT));

    return TRUE;
}

static BOOL
TrimTextOut(HDC hdc, int x, int y, int maxwidth, char *str)
{
    char buf[MAX_RENDER_LINE];
    SIZE size;

    strcpy(buf, str);
    GetTextExtentPoint32(hdc, buf, strlen(buf), &size); 
    if (size.cx > maxwidth) {
	while(size.cx > maxwidth && strlen(buf)) {
	    GetTextExtentPoint32(hdc, buf, strlen(buf), &size); 
	    buf[strlen(buf)-1] = '\0';
	} 
	strcat(buf, "...");
    }

    return TextOut(hdc, x, y, buf, strlen(buf));
}

/* Parse strings like "x1 x2 x2" or "x1, y1; x2, y2; ...; xn, yn" */
static void
parse_skin_txt_int_list (int** int_list, int* int_list_len, char* value)
{
    gchar* v = value;

    *int_list = 0;
    *int_list_len = 0;

    g_strcanon (value, "-0123456789", ' ');
    while (*v) {
	int n, x, rc;

	rc = sscanf (v, " %d%n", &x, &n);
	if (rc != 1) {
	    break;
	}
	v += n;
	(*int_list_len) ++;
	*int_list = (int*) realloc (*int_list, sizeof(int) * (*int_list_len));
	(*int_list)[*int_list_len-1] = x;
    }
}

void
parse_skin_txt (SKINDATA* skind, void* txt, int txt_len)
{
    GKeyFile *gkf = g_key_file_new ();
    GError *error = 0;
    gchar *value;
    int* int_list = 0;
    int int_list_len = 0;

    g_key_file_load_from_data (gkf, txt, txt_len, G_KEY_FILE_NONE, &error);
    if (error) {
	g_error_free (error);
	g_key_file_free (gkf);
	return;
    } 

    /* Get region polygon */
    value = g_key_file_get_string (gkf, "Background", "region_poly", NULL);
    if (value) {
	int i;
        clear_rgn_points (skind);
	parse_skin_txt_int_list (&int_list, &int_list_len, value);
	for (i = 0; i < (int_list_len / 2); i++) {
	    push_rgn_point(skind, int_list[2*i], int_list[2*i+1]);
	}
	free (int_list);
	g_free (value);
    }

    /* Get background location within image */
    value = g_key_file_get_string (gkf, "Background", "img_loc", NULL);
    if (value) {
	parse_skin_txt_int_list (&int_list, &int_list_len, value);
	if (int_list_len >= 4) {
	    skind->background_rect.left = int_list[0];
	    skind->background_rect.top = int_list[1];
	    skind->background_rect.right = int_list[2];
	    skind->background_rect.bottom = int_list[3];
	}
	free (int_list);
	g_free (value);
    }

    g_key_file_free (gkf);
}

static BOOL
load_skindata_from_file (const char* skinfile, SKINDATA* skind)
{
    gchar *cwd;
    gchar *skinpath;
    void *txt = 0;
    int txt_len;

    if (!skind) {
	return FALSE;
    }

    /* This is only for debugging */
    cwd = g_get_current_dir ();
    debug_printf ("CWD = %s\n", cwd);
    g_free (cwd);

    skinpath = g_build_filename (SKIN_PATH, skinfile, 0);
    debug_printf ("loading skindata from file: %s\n", skinpath);

    skind->bmdc.bm = (HBITMAP) LoadImage(0, skinpath, IMAGE_BITMAP, 
				   BIG_IMAGE_WIDTH, BIG_IMAGE_HEIGHT, 
				   LR_CREATEDIBSECTION | LR_LOADFROMFILE);

    /* LoadImage succeeded.  Try to load the associated txt file. */
    if (skind->bmdc.bm) {
	if (g_str_has_suffix (skinpath, ".bmp")) {
	    gchar* contents = 0;
	    gsize length;
	    gboolean rc;

	    strcpy (&skinpath[strlen(skinpath)-4], ".txt");
	    //debug_box ("Trying to load txt %s", skinpath);
	    rc = g_file_get_contents (skinpath, &contents, &length, NULL);
	    if (rc) {
		parse_skin_txt (skind, contents, length);
		g_free (contents);
	    }
	}
	g_free (skinpath);
    }
    
    /* LoadImage failed.  Try to load zipfile. */
    else {
	debug_printf ("Error.  LoadImage returned NULL\n");
	render2_load_skin (&skind->bmdc.bm, &txt, &txt_len, skinpath);
	g_free (skinpath);
	if (skind->bmdc.bm == NULL) {
	    return FALSE;
	}
	if (txt) {
	    parse_skin_txt (skind, txt, txt_len);
	    free (txt);
	}
    }

    skind->bmdc.hdc = CreateCompatibleDC(NULL);
    SelectObject(skind->bmdc.hdc, skind->bmdc.bm);

    skind->textcolor = GetPixel (skind->bmdc.hdc, m_pt_color.x, m_pt_color.y);
    skind->hbrush = CreateSolidBrush (skind->textcolor);
    return skind->hbrush ? TRUE : FALSE;
}

static void
skindata_close (SKINDATA* skind)
{
    if (skind->rgn_points) {
	free (skind->rgn_points);
	skind->rgn_points = 0;
    }
    bitmapdc_close (skind->bmdc);
    DeleteObject (skind->hbrush);
    skind->hbrush = NULL;
}

static void
bitmapdc_close (BITMAPDC b)
{
    DeleteDC (b.hdc);
    DeleteObject (b.bm);
    b.hdc = NULL;
    b.bm = NULL;
}

static BOOL
internal_render_do_paint (SKINDATA* skind, HDC outhdc)
{
    BUTTON *b;
    RECT *prt;
    int i;
    HDC thdc = skind->bmdc.hdc;

    // Create out temp dc if we haven't made it yet
    if (m_tempdc.hdc == NULL) {
	LOGFONT ft;

	m_tempdc.hdc = CreateCompatibleDC (thdc);
	m_tempdc.bm = CreateCompatibleBitmap (thdc, WIDTH (skind->background_rect), 
					      HEIGHT (skind->background_rect));
	SelectObject (m_tempdc.hdc, m_tempdc.bm);

	// And the font
	memset (&ft, 0, sizeof(LOGFONT));
	//strcpy(ft.lfFaceName, "Microsoft Sans pSerif");
	strcpy (ft.lfFaceName, "Verdana");
	ft.lfHeight = 12;
	m_tempfont = CreateFontIndirect(&ft);
	SelectObject (m_tempdc.hdc, m_tempfont);
    }

    // Do the background
    BitBlt (m_tempdc.hdc, 
	    0, 
	    0, 
	    WIDTH (skind->background_rect),
	    HEIGHT (skind->background_rect),
	    thdc,
	    skind->background_rect.left,
	    skind->background_rect.top,
	    SRCCOPY);

    // Draw buttons
    for (i = 0; i < skind->m_num_buttons; i++) {
	b = &skind->m_buttons[i];
	prt = &b->rt[b->mode];
	BitBlt (m_tempdc.hdc,
	        b->dest.left,
	        b->dest.top,
	        WIDTH(b->dest),
	        HEIGHT(b->dest),
	        thdc,
	        prt->left,
	        prt->top,
	        SRCCOPY);
    }
	
    // Draw progress bar
    if (m_prog_on) {
	RECT rt = {m_prog_point.x, 
		   m_prog_point.y, 
		   m_prog_point.x + (WIDTH(m_prog_rect)*15) + 11,   // yummy magic numbers
		   m_prog_point.y + HEIGHT(m_prog_rect)+4};
	time_t now;
	int num_bars;
	int i;

	time(&now);
	num_bars = (now-m_time_start) % 15;	// number of bars to draw
	FrameRect (m_tempdc.hdc, &rt, skind->hbrush);
		
	for(i = 0; i < num_bars; i++) {
	    BitBlt (m_tempdc.hdc,
		    rt.left + (i * (WIDTH(m_prog_rect)+1)+2),
		    rt.top+2,
		    WIDTH(m_prog_rect),
		    HEIGHT(m_prog_rect),
		    thdc,
		    m_prog_rect.left,
		    m_prog_rect.top,
		    SRCCOPY);
	}
    }

    // Draw text data on the screen
    SetBkMode (m_tempdc.hdc, TRANSPARENT); 
	    
    // Draw text
    SetTextColor (m_tempdc.hdc, skind->textcolor);

    for (i = 0; i < IDR_NUMFIELDS; i++) {		
	TrimTextOut (m_tempdc.hdc, m_ddinfo[i].rt.left, 
		     m_ddinfo[i].rt.top, 
		     WIDTH(m_ddinfo[i].rt), 
		     m_ddinfo[i].str); 
    }

    debug_printf ("bltting: (%d %d)\n", 
		    WIDTH (skind->background_rect),
		    HEIGHT (skind->background_rect));

    // Onto the actual screen 
    BitBlt (outhdc,
	    0,
	    0,
	    WIDTH (skind->background_rect),
	    HEIGHT (skind->background_rect),
	    m_tempdc.hdc,
	    0,
	    0,
	    SRCCOPY);

    debug_printf ("bltting complete\n");
    return TRUE;
}
