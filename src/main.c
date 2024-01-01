#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>

#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>

#include <psp2/io/fcntl.h>

#include <psp2/kernel/threadmgr.h>


#include "platform/defines.h"

#include "core/system.h"
#include "core/graphics.h"
#include "graphics/drawing.h"
#include "core/input.h"

#include "utils/time.h"
#include "osc_mapping.h"
#include "tui.h"

#include <time.h>


#define NET_PARAM_MEM_SIZE (1*1024*1024)


int main(int argc, char *argv[]) {
    System mainSys;
    GraphicsSystem gfxSys;
    
    sys_init_system(&mainSys);
    sys_init_console();

    gfx_init_graphics_system(&gfxSys);
    inp_init_input_system(&mainSys.inputSys);

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
    int32_t sfd = sceNetSocket("osc-destination", SCE_NET_AF_INET, SCE_NET_SOCK_DGRAM, 0);
    if (sfd < 0) {
        printf("Error while creating socket\n");
        sceKernelExitProcess(-1);
        return -1;
    }
    printf("Socket created.\n");


    // Preparing IP destination
    SceNetInAddr dst_addr;			/* destination address */
    printf("Converting IP address.\n");
    sceNetInetPton(SCE_NET_AF_INET, "192.168.1.27", (void*) &dst_addr);
    

    // Connecting to UDP server
    SceNetSockaddrIn addrTo; 	/* server address to send data to */
    memset(&addrTo, 0, sizeof(addrTo));
    addrTo.sin_family = SCE_NET_AF_INET;
    //addrTo.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
    addrTo.sin_addr = dst_addr;
    addrTo.sin_port = sceNetHtons(7001);

    // Network info retrieving
    SceNetCtlInfo vitaNetInfo;
    sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &vitaNetInfo);
    printf("IP address %s\n", vitaNetInfo.ip_address);


    // MOVE ALL OF THIS IN A PROPER SEPARATE SOURCE FILE

    // OSC Mappings management
    int nbMappings = 2;

    OscButtonMapping* oscButtonMapping = NULL;
    oscButtonMapping = (OscButtonMapping*) malloc(nbMappings * sizeof(OscButtonMapping));

    init_osc_button_mapping(&oscButtonMapping[0]);
    init_osc_button_mapping(&oscButtonMapping[1]);

    copy_str(&oscButtonMapping[0].address, "/test/ouida");
    copy_str(&oscButtonMapping[1].address, "/test/oki");

    oscButtonMapping[0].button = 12;
    oscButtonMapping[1].button = 14;

    // TUI
    TUITable tuiTable;
    tui_init_table(&tuiTable);

    int i, j;

    for (i = 0; i < nbMappings; i++) {
        TUITableRow* tuiTableRow = (TUITableRow*) malloc(sizeof(TUITableRow));

        tui_init_table_row(tuiTableRow, 4);
        tui_add_row_to_table(&tuiTable, tuiTableRow);

        char rangeStr[20];
        snprintf(rangeStr, 20, "%f->%f", oscButtonMapping[i].range[0], oscButtonMapping[i].range[1]);

        tui_set_table_row_cell(tuiTableRow, 0, oscButtonMapping[i].address);
        tui_set_table_row_cell(tuiTableRow, 1, BUTTON_NAMES[oscButtonMapping[i].button]);
        tui_set_table_row_cell(tuiTableRow, 2, rangeStr);
        tui_set_table_row_cell(tuiTableRow, 3, OSC_MAPPING_STYLE_NAME[oscButtonMapping[i].mappingStyle]);
    }

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

    tui_redraw_table(&tuiTable, windowWidth, windowHeight);


    printf("Test\n");

    double x = 0.;
    double y = 0.;

    // TODO: REPLACE WITH THE ABSTRACTED INPUT SYSTEM AFTER
    // WE'RE DONE REMAKING IT
    SceCtrlData ctrl;
    unsigned int prevButtonState = 0x0;
    
    while (sys_main_loop(&mainSys)) {
        drawbuffer* currDrawBuffer = &gfxSys.framebuffer[gfxSys.currBackbufferIdx];
        
        // Assign console output framebuffer to current drawing buffer
        psvDebugScreenUseFramebuffer(currDrawBuffer->buffer, currDrawBuffer->mutex);

        // Blanking screen
        sceKernelLockMutex(currDrawBuffer->mutex, 1, NULL);
        gfx_fill_with_color(currDrawBuffer, 0x0);
        sceKernelUnlockMutex(currDrawBuffer->mutex, 1);
        
        if (inp_is_joy_btn_pressed(JOY_DOWN)) {
            y = y + 50. * mainSys.deltaTime;
        }
        else if (inp_is_joy_btn_pressed(JOY_UP)) {
            y = y - 50. * mainSys.deltaTime;
        }

        if (inp_is_joy_btn_pressed(JOY_LEFT)) {
            x = x - 50. * mainSys.deltaTime;
        }
        else if (inp_is_joy_btn_pressed(JOY_RIGHT)) {
            x = x + 50. * mainSys.deltaTime;
        }

        sceKernelLockMutex(currDrawBuffer->mutex, 1, NULL);
        drawBox(currDrawBuffer,
                floor(-10.0 + x), floor(50.0 + y), floor(150.0 + x), floor(150.0 + y),
                (color4) { 255, 0, 0, 255 });
        sceKernelUnlockMutex(currDrawBuffer->mutex, 1);


        // TODO: REPLACE WITH THE ABSTRACTED INPUT SYSTEM AFTER
        // WE'RE DONE REMAKING IT
        sceCtrlPeekBufferPositive(0, &ctrl, 1);

        unsigned int buttonsDown = ctrl.buttons & ~prevButtonState;
        unsigned int buttonsUp = ~ctrl.buttons & prevButtonState;

        if (buttonsDown & SCE_CTRL_DOWN) {
            tuiTable.highlightedRowId = (tuiTable.highlightedRowId + 1) % tuiTable.nbRows;
        }
        else if (buttonsDown & SCE_CTRL_UP) {
            tuiTable.highlightedRowId--;
            if (tuiTable.highlightedRowId < 0) {
                tuiTable.highlightedRowId = tuiTable.nbRows-1;
            }
        }

        for (i = 0; i < nbMappings; i++) {
            if (buttonsDown & (1 << oscButtonMapping[i].button)) {
                send_osc_message_from_mapping(sfd, addrTo, &oscButtonMapping[i], 1.0);
            }

            if (buttonsUp & (1 << oscButtonMapping[i].button)) {
                send_osc_message_from_mapping(sfd, addrTo, &oscButtonMapping[i], 0.0);
            }
        }

        tui_redraw_table(&tuiTable, windowWidth, windowHeight);

        prevButtonState = ctrl.buttons;

        // Swapping front/back buffers
        gfx_swap_buffers(&gfxSys);
        
        gfx_wait_for_blank();
    }


    // Quitting
    sceKernelExitProcess(0);

    return 0;
}
