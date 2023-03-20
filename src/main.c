#include <stdio.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>

#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>

#include <psp2/io/fcntl.h>

#include "tinyosc.h"

#define printf psvDebugScreenPrintf

int main(int argc, char *argv[]) {
	// init
	psvDebugScreenInit();

	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	
	// greetings
	psvDebugScreenPrintf("input test\n");
	psvDebugScreenPrintf("press Select+Start+L+R to stop\n");

	// to enable analog sampling
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	SceCtrlData ctrl;

	const char* btn_label[]={"SELECT ","","","START ",
		"UP ","RIGHT ","DOWN ","LEFT ","L ","R ","","",
		"TRIANGLE ","CIRCLE ","CROSS ","SQUARE "};
	do {
		sceCtrlPeekBufferPositive(0, &ctrl, 1);

		psvDebugScreenPrintf("Buttons:%08X == ", ctrl.buttons);

		int i;

		for(i=0; i < sizeof(btn_label)/sizeof(*btn_label); i++){
			psvDebugScreenPrintf("\e[9%im%s",(ctrl.buttons & (1<<i)) ? 7 : 0, btn_label[i]);
		}

		psvDebugScreenPrintf("\e[m Stick:[%3i:%3i][%3i:%3i]\r", ctrl.lx,ctrl.ly, ctrl.rx,ctrl.ry);
	} while(ctrl.buttons != (SCE_CTRL_START | SCE_CTRL_SELECT | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER) );

	sceKernelExitProcess(0);

	return 0;
}
