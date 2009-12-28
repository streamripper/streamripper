#ifndef __DOCK_H__
#define __DOCK_H__

void dock_init (HWND hwnd);
VOID dock_do_mousemove(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttondown(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_do_lbuttonup(HWND hWnd, LONG wParam, LONG lParam);
VOID dock_show_window(HWND hWnd, int nCmdShow);
void dock_update_winamp_wins (HWND hWnd, char* buf);
void dock_resize (HWND hwnd, char* buf);

#endif //__DOCK_H__