#ifndef __RIPLIB_H__
#define __RIPLIB_H__

#include "rip_manager.h"
#include "srtypes.h"
#include "prefs.h"
#include "socklib.h"

error_code
ripstream_init (RIP_MANAGER_INFO* rmi);
error_code ripstream_rip (RIP_MANAGER_INFO* rmi);
void ripstream_clear(RIP_MANAGER_INFO* rmi);

#endif //__RIPLIB__
