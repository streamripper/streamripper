#ifndef _WA_GEN_H_
#define _WA_GEN_H_

typedef struct {
	int version;
	char *description;
	int (*init)();
	void (*config)();
	void (*quit)();
	HWND hwndParent;
	HINSTANCE hDllInstance;
} winampGeneralPurposePlugin;

#define GPPHDR_VER 0x10

extern winampGeneralPurposePlugin *gen_plugins[256];
typedef winampGeneralPurposePlugin * (*winampGeneralPurposePluginGetter)();

#endif
