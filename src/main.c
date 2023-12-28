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


typedef enum EOscMappingStyle {
    EOscMappingStyle_MOMENTARY,
    EOscMappingStyle_TOGGLE,
} EOscMappingStyle;


typedef struct OscButtonMapping {
    char* address;
    int button;
    EOscMappingStyle mappingStyle;
    float range[2];
} OscButtonMapping;


const char BUTTON_NAMES[16][10] = {
    "SELECT",
    "",
    "",
    "START",
    "UP",
    "RIGHT",
    "DOWN",
    "LEFT",
    "L",
    "R",
    "",
    "",
    "TRIANGLE",
    "CIRCLE",
    "CROSS",
    "SQUARE"
};


const char OSC_MAPPING_STYLE_NAME[2][10] = {
    "MOMENTARY",
    "TOGGLE"
};


void init_osc_button_mapping(OscButtonMapping* btnMapping) {
    btnMapping->address = malloc(1);
    btnMapping->address[0] = '\0';
    btnMapping->button = -1;
    btnMapping->mappingStyle = EOscMappingStyle_MOMENTARY;
    btnMapping->range[0] = 0.0;
    btnMapping->range[1] = 1.0;
}

void free_osc_button_mapping(OscButtonMapping* btnMapping) {
    free(btnMapping->address);
}

void copy_str(char** strDst, const char* strSrc) {
    int strDstSize = strlen(*strDst);
    int strSrcSize = strlen(strSrc);

    if (strSrcSize > strDstSize) {
        resizeArray(strDst, strDstSize + 1, strSrcSize + 1);
    }
    else if (strSrcSize < strDstSize) {
        memset(*strDst, '\0', strDstSize + 1);
    }

    memcpy(*strDst, strSrc, strSrcSize + 1);
}

// Building OSC message
void send_osc_message_from_mapping(int sfd, SceNetSockaddrIn addrTo, OscButtonMapping* oscMapping, float mappingValue) {
    float arg = oscMapping->range[0] + (oscMapping->range[1] - oscMapping->range[0]) * mappingValue;
    OscMessage newMsg = make_osc_message(oscMapping->address, "f", &arg);
    
    char* fullArgs = (char*) malloc(newMsg.size);
    assemble_osc_message_args(&newMsg, fullArgs);

    int typeTagsStrPadSize = calculate_size_with_padding(newMsg.argsCount + 2);
    char* typeTags = (char*) malloc(typeTagsStrPadSize);
    memset(typeTags, '\0', typeTagsStrPadSize);
    assemble_osc_message_type_tags(&newMsg, typeTags);

    int msgAddrSize = calculate_size_with_padding(strlen(newMsg.address) + 1);
    int wholeMsgSize = msgAddrSize + typeTagsStrPadSize + newMsg.size;

    char* wholeMsg = (char*) malloc(wholeMsgSize);
    memset(wholeMsg, '\0', wholeMsgSize);

    // TODO faire une routine pour assembler des bytestream comme ça
    memcpy(&wholeMsg[0], newMsg.address, msgAddrSize);
    memcpy(&wholeMsg[msgAddrSize], typeTags, typeTagsStrPadSize);
    memcpy(&wholeMsg[msgAddrSize + typeTagsStrPadSize], fullArgs, newMsg.size);
    
    int result = sceNetSendto(sfd,
                              wholeMsg,
                              wholeMsgSize,
                              0,
                              &addrTo,
                              sizeof(addrTo));

    if (result < 0) {
        printf("Error while sending packet to destination. (%d)\n", result);
    }

    free(fullArgs);
    free(typeTags);
    free(wholeMsg);

    free_osc_message(&newMsg);
}

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
    sceNetInetPton(SCE_NET_AF_INET, "192.168.1.47", (void*) &dst_addr);
    

    // Connecting to UDP server
    SceNetSockaddrIn addrTo; 	/* server address to send data to */
    memset(&addrTo, 0, sizeof(addrTo));
    addrTo.sin_family = SCE_NET_AF_INET;
    //addrTo.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
    addrTo.sin_addr = dst_addr;
    addrTo.sin_port = sceNetHtons(7001);

    SceNetCtlInfo vitaNetInfo;
    sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &vitaNetInfo);

    printf("IP address %s\n", vitaNetInfo.ip_address);


    // Enable analog sampling
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);


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

    tui_redraw_table(&tuiTable, windowWidth, windowHeight);


    // Input events management
    SceCtrlData ctrl;
    unsigned int prevButtonState = 0x0;

    do {
        sceCtrlPeekBufferPositive(0, &ctrl, 1);

        if (ctrl.buttons != prevButtonState) {
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
        }

        tui_redraw_table(&tuiTable, windowWidth, windowHeight);

        prevButtonState = ctrl.buttons;
    } while(ctrl.buttons != (SCE_CTRL_START | SCE_CTRL_SELECT | SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER) );

    // Quitting
    sceKernelExitProcess(0);

    return 0;
}
