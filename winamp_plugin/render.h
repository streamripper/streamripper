/* render.h
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
#ifndef __RENDER_H__
#define __RENDER_H__

#define REN_STATUS_BUFFERING	0x01
#define REN_STATUS_RIPPING	0x02
#define REN_STATUS_RECONNECTING	0x03

#define MAX_RENDER_LINE		1024
#define IDR_NUMFIELDS		0x06

#define IDR_STATUS		0x00
#define IDR_FILENAME		0x01
#define IDR_STREAMNAME		0x02
#define IDR_SERVERTYPE		0x03
#define IDR_BITRATE		0x04
#define IDR_METAINTERVAL	0x05

#define MAX_BUTTONS			10

typedef int	HBUTTON;

BOOL render_init (HWND hWnd, LPCTSTR szBmpFile);
BOOL render_change_skin (LPCTSTR szBmpFile);
BOOL render_destroy ();
BOOL render_create_preview (char* skinfile, HDC hdc, long left, long top,
			    long width, long height);
VOID render_set_prog_bar (BOOL on_off);
VOID render_do_mousemove (HWND hWnd, LONG wParam, LONG lParam);
VOID render_do_lbuttonup (HWND hWnd, LONG wParam, LONG lParam);
VOID render_do_lbuttondown (HWND hWnd, LONG wParam, LONG lParam);
VOID render_clear_all_data ();
BOOL render_set_display_data (int idr, char *format, ...);
BOOL render_do_paint (HDC hdc);
void render_start_button_enable ();
void render_start_button_disable ();
void render_stop_button_enable ();
void render_stop_button_disable ();
void render_relay_button_enable ();
void render_relay_button_disable ();

#endif //__RENDER_H__
