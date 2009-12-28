#ifndef __RENDER_2_H__
#define __RENDER_2_H__

#include "windows.h"

void
render2_load_skin (HBITMAP* hbitmap,     /* output */
		   void** txt,           /* output */
		   int* txt_len,         /* output */
		   const char* skinfile  /* input */
		   );

#endif //__RENDER_2_H__
