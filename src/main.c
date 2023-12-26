#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>

#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>

#include <psp2/touch.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>

#include <psp2/io/fcntl.h>

#include "debugScreen.h"
#include "../deps/osclib/src/osc_data.h"

#include "tui.h"


#define printf psvDebugScreenPrintf

#define NET_PARAM_MEM_SIZE (1*1024*1024)


int main(int argc, char *argv[]) {
    // Initialize debug screen
    psvDebugScreenInit();

	// Initializing touch screens and analogs
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);

    // Initialize Net
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET); // load NET module
    
    int ret = sceNetShowNetstat();
    SceNetInitParam net_init_param; /* Net init param structure */

    if (ret == SCE_NET_ERROR_ENOTINIT) {
        net_init_param.memory = malloc(NET_PARAM_MEM_SIZE);
        net_init_param.size = NET_PARAM_MEM_SIZE;
        net_init_param.flags = 0;
        memset(net_init_param.memory, 0, NET_PARAM_MEM_SIZE);
        ret = sceNetInit(&net_init_param);
        if (ret < 0) printf("An error occurred while starting network.");
    }

    // Initialize NetCtl (for network info retrieving)
    sceNetCtlInit();

    // Init socket connection
    // open a socket to listen for datagrams (i.e. UDP packets) on port 7001
    int32_t sfd = sceNetSocket("osc-destination", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0); // SCE_NET_IPPROTO_UDP
    if (sfd < 0) {
        printf("Error while creating socket\n");
        sceKernelExitProcess(-1);
        return -1;
    }
    printf("Socket created.\n");

    // Preparing IP destination
    SceNetInAddr dst_addr;			/* destination address */
    printf("Converting IP address.\n");
    //sceNetInetPton(SCE_NET_AF_INET, "192.168.87.198", (void*)&dst_addr);
    
    // Connecting to UDP server
    SceNetSockaddrIn addrTo; 	/* server address to send data to */
    memset(&addrTo, 0, sizeof(addrTo));
    addrTo.sin_family = SCE_NET_AF_INET;
    addrTo.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
    addrTo.sin_port = sceNetHtons(7001);

    SceNetCtlInfo vitaNetInfo;
    sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &vitaNetInfo);

    printf("IP address %s\n", vitaNetInfo.ip_address);


    // Building OSC message
    int arg1 = 6969;
    OscMessage newMsg = make_osc_message("/o/la/fami", "i", &arg1);
    
    char* fullArgs = (char*) malloc(newMsg.size);
    assemble_osc_message_args(&newMsg, fullArgs);
    print_memory_block_hex(fullArgs, newMsg.size);

    int typeTagsStrPadSize = calculate_size_with_padding(newMsg.argsCount + 2);
    char* typeTags = (char*) malloc(typeTagsStrPadSize);
    memset(typeTags, '\0', typeTagsStrPadSize);
    assemble_osc_message_type_tags(&newMsg, typeTags);

    printf("%s\n", typeTags);
    print_memory_block_hex(typeTags, typeTagsStrPadSize);

    printf("%s\n", newMsg.address);


    int msgAddrSize = calculate_size_with_padding(strlen(newMsg.address) + 1);
    
    int wholeMsgSize = msgAddrSize
                       + typeTagsStrPadSize
                       + newMsg.size;

    char* wholeMsg = (char*) malloc(wholeMsgSize);
    memset(wholeMsg, '\0', wholeMsgSize);

    // TODO faire une routine pour assembler des bytestream comme ça
    memcpy(&wholeMsg[0], newMsg.address, msgAddrSize);
    memcpy(&wholeMsg[msgAddrSize], typeTags, typeTagsStrPadSize);
    memcpy(&wholeMsg[msgAddrSize + typeTagsStrPadSize], fullArgs, newMsg.size);
    
    print_memory_block_hex(&wholeMsg, wholeMsgSize);
    
    
    int result = sceNetSendto(sfd,
                              wholeMsg,
                              wholeMsgSize,
                              0,
                              &addrTo,
                              sizeof(addrTo));

    if (result < 0) {
        printf("Error while sending packet to destination. (%d)\n", result);
    }


    // greetings
    psvDebugScreenPrintf("input test\n");
    psvDebugScreenPrintf("press Select+Start+L+R to stop\n");

    // to enable analog sampling
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    
    SceCtrlData ctrl;

    /*const char* btn_label[]={"SELECT ","","","START ",
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
    } while(ctrl.buttons != (SCE_CTRL_START | SCE_CTRL_SELECT | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER) );*/


    // TUI
    int windowWidth = 120;
    int windowHeight = 30;

    printf("\033[2J");

    printf("\e[1;H");
    printf("Address:");

    printf("\e[1;10H");
    char address[16] = "xxx.xxx.xxx.xxx";
    int port = 7001;

    printf("%s", address);

    printf("\e[1;%dH", windowWidth - 12);
    printf("Port:");

    printf("\e[1;%dH", windowWidth - 6);
    printf("%d", port);

    TUITable tuiTable;
    tui_init_table(&tuiTable);

    TUITableRow tuiTableRow1;
    tui_init_table_row(&tuiTableRow1, 4);
    tui_add_row_to_table(&tuiTable, &tuiTableRow1);
    tui_set_table_row_cell(&tuiTableRow1, 0, "Test");
    tui_set_table_row_cell(&tuiTableRow1, 1, "Oui");
    tui_set_table_row_cell(&tuiTableRow1, 2, "Non");
    tui_set_table_row_cell(&tuiTableRow1, 3, "Oki");

    TUITableRow tuiTableRow2;
    tui_init_table_row(&tuiTableRow2, 4);
    tui_add_row_to_table(&tuiTable, &tuiTableRow2);
    tui_set_table_row_cell(&tuiTableRow2, 0, "Lol");
    tui_set_table_row_cell(&tuiTableRow2, 1, "Mdr");
    tui_set_table_row_cell(&tuiTableRow2, 2, "Ptdr");
    tui_set_table_row_cell(&tuiTableRow2, 3, "Expldr");


    tui_redraw_table(&tuiTable, windowWidth, windowHeight);

    int moveKeyIsDown = 0;

    do {
        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        tui_redraw_table(&tuiTable, windowWidth, windowHeight);

        if (ctrl.buttons == 0x0) {
            moveKeyIsDown = 0;
        }
        else if (ctrl.buttons != 0x0 && !moveKeyIsDown) {
            if (ctrl.buttons & SCE_CTRL_DOWN) {
                tuiTable.highlightedRowId = (tuiTable.highlightedRowId + 1) % tuiTable.nbRows;
            }
            else if (ctrl.buttons & SCE_CTRL_UP) {
                tuiTable.highlightedRowId--;
                if (tuiTable.highlightedRowId < 0) {
                    tuiTable.highlightedRowId = tuiTable.nbRows-1;
                }
            }
            moveKeyIsDown = 1;
        }

    } while(ctrl.buttons != (SCE_CTRL_START | SCE_CTRL_SELECT | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER) );


    sceKernelExitProcess(0);

    return 0;
}
