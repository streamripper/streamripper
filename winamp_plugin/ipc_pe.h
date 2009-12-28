#ifndef __IPC_PE_H
#define __IPC_PE_H

/* ipc.pe.h (modified 14/04/2004 by Darren Owen aka DrO)
**
** Adds working examples on how to use some of the api messages
*/

/*
** To use these messages you will need a valid window handle for the playlist window
** and the format to use them is:
**
** SendMessage(playlist_wnd,WM_WA_IPC,IPC_*,(parameter));
**
** Note:
** 	This IS the OPPOSITE way to how the messages to the main winamp window are sent
** 	SendMessage(hwnd_winamp,WM_WA_IPC,(parameter),IPC_*);
*/

/*
** Playlist Window:
**
** To get the playlist window there are two ways depending on the version you're using
**
** HWND playlist_wnd = 0;
** int wa_version = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETVERSION);
**
** if(wa_version >= 0x2900){
**	// use the built in api to get the handle
** 	playlist_wnd = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,IPC_GETWND_PE,IPC_GETWND);
** }
**
** // if it failed then use the old way :o)
** if(!playlist_wnd){
** 	playlist_wnd = FindWindow("Winamp PE",0);
** }
*/

typedef struct {
	char file[MAX_PATH];
	int index;
} fileinfo;

typedef struct {
	HWND callback;
	int index;
} callbackinfo;

typedef struct {
	int fileindex;
	char filetitle[256];
	char filelength[16];
} fileinfo2;



#define IPC_PE_GETCURINDEX        100 // returns current idx
#define IPC_PE_GETINDEXTOTAL      101 // returns number of items in the playlist

// this is meant to work but i can't seem to get the callback message
// though other api's are available to get the info far more easily :o)
#define IPC_PE_GETINDEXINFO       102 // (copydata) lpData is of type callbackinfo, callback is called with copydata/fileinfo structure and msg IPC_PE_GETINDEXINFORESULT
#define IPC_PE_GETINDEXINFORESULT 103 // callback message for IPC_PE_GETINDEXINFO
/*
** COPYDATASTRUCT cds; 
** callbackinfo f; 
** 	cds.dwData = IPC_PE_GETINDEXINFO; 
** 	f.callback = g_hwnd; // this is the HWND of the window which will receive the callback message 
** 	f.index=index; // Index of the entry to get info from 
** 	cds.lpData = (void *)&f; 
** 	cds.cbData = sizeof(callbackinfo); 
** 	SendMessage(playlist_wnd,WM_COPYDATA,0,(LPARAM)&cds); 
*/

#define IPC_PE_DELETEINDEX        104 // lParam = index

#define IPC_PE_SWAPINDEX          105 // (lParam & 0xFFFF0000) >> 16 = from, (lParam & 0xFFFF) = to
/*
** SendMessage(playlist_wnd,WM_WA_IPC,IPC_PE_SWAPINDEX,MAKELPARAM(from,to));
*/

#define IPC_PE_INSERTFILENAME     106 // (copydata) lpData is of type fileinfo
/* COPYDATASTRUCT cds;
** fileinfo f;
**
** 	lstrcpyn(f.file, file,MAX_PATH);	// path to the file
** 	f.index = position;			// insert file position
**
** 	cds.dwData = IPC_PE_INSERTFILENAME;
** 	cds.lpData = (void*)&f;
** 	cds.cbData = sizeof(fileinfo);
** 	SendMessage(playlist_wnd,WM_COPYDATA,0,(LPARAM)&cds);
*/


#define IPC_PE_GETDIRTY           107 // returns 1 if the playlist changed since the last IPC_PE_SETCLEAN
#define IPC_PE_SETCLEAN	          108 // resets the dirty flag until next modification

#define IPC_PE_GETIDXFROMPOINT    109 // pass a point parm, return a playlist index
/*
** POINT pt;
** RECT rc;
**
** 	// Get the current position of the mouse and the current client area of the playlist window
** 	// and then mapping the mouse position to the client area
** 	GetCursorPos(&pt);
** 	// Get the client area of the playlist window and then map the mouse position to it
** 	GetClientRect(playlist_wnd,&rc);
** 	ScreenToClient(playlist_wnd,&pt);
**
** 	// this corrects so the selection works correctly on the selection boundary
** 	// appears to happen on the older 2.x series as well
** 	pt.y -= 2;
**
** 	// corrections for the playlist window area so that work is only done for valid positions
** 	// and nicely enough it works for both classic and modern skin modes
** 	rc.top += 18;
** 	rc.left += 12;
** 	rc.right -= 19;
** 	rc.bottom -= 40;
**
** 	// is the click in 
** 	if(PtInRect(&rc,pt)){
** 	// get the item index at the given point
**	// if this is out of range then it will return 0 (not very helpful really)
** 	int idx = SendMessage(playlist_wnd,WM_WA_IPC,IPC_PE_GETIDXFROMPOINT,(LPARAM)&pt);
**
** 		// makes sure that the item isn't past the last playlist item
** 		if(idx < SendMessage(playlist_wnd,WM_WA_IPC,IPC_PE_GETINDEXTOTAL,0)){
** 			// ... do stuff in here (this example will start playing the selected track)
** 			SendMessage(plugin.hwndParent,WM_WA_IPC,idx,IPC_SETPLAYLISTPOS);
** 			SendMessage(plugin.hwndParent,WM_COMMAND,WINAMP_BUTTON2,0);
** 		}
** 	}
*/

#define IPC_PE_SAVEEND            110 // pass index to save from
#define IPC_PE_RESTOREEND         111 // no parm



// the following messages are in_process ONLY

#define IPC_PE_GETINDEXTITLE      200 // lParam = pointer to fileinfo2 struct
/*
** fileinfo2 file;
** int ret = 0;
**
** 	file.fileindex = position;	// this is zero based!
** 	ret = SendMessage(playlist_wnd,WM_WA_IPC,IPC_PE_GETINDEXTITLE,(LPARAM)&file);
**
** 	// if it returns 0 then track information was received
** 	if(!ret){
** 		// ... do stuff
** 	}
*/

#endif