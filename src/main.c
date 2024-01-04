#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <psp2/sysmodule.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/net/http.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/types.h>
#include <psp2/libime.h>

#include "platform/defines.h"
#include "core/system.h"
#include "core/graphics.h"
#include "graphics/drawing.h"
#include "core/input.h"
#include "utils/time.h"
#include "utils/maths.h"
#include "input/ime.h"
#include "tui/lineedit.h"
#include "tui/label.h"
#include "tui/table.h"
#include "tui/tablerow.h"
#include "tui/msgline.h"

#include "osc_mapping.h"
#include "config.h"


#define NET_PARAM_MEM_SIZE (1*1024*1024)


int main(int argc, char *argv[]) {
    System mainSys;
    GraphicsSystem gfxSys;
    ImeSystem imeSys;
    OscConnection oscConnection;

    memset(oscConnection.address, '\0', 40);
    memset(oscConnection.port, '\0', 6);

    sys_init_system(&mainSys);
    sys_init_console();

    gfx_init_graphics_system(&gfxSys);
    inp_init_input_system(&mainSys.inputSys);
    ime_init_ime_system(&imeSys, 256);

    // Initialize modules
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    
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
    OscButtonMapping* oscButtonMappings = NULL;

    oscButtonMappings = (OscButtonMapping*) malloc(nbMappings * sizeof(OscButtonMapping));

    init_osc_button_mapping(&oscButtonMappings[0]);
    init_osc_button_mapping(&oscButtonMappings[1]);

    copy_str(&oscButtonMappings[0].address, "/test/ouida");
    copy_str(&oscButtonMappings[1].address, "/test/oki");

    oscButtonMappings[0].button = 12;
    oscButtonMappings[1].button = 14;
    

    // TUI
    TUITable tuiTable;
    tui_init_table(&tuiTable);

    TUITableRow* tuiTableRow = (TUITableRow*) malloc(sizeof(TUITableRow));

    tui_init_table_row(tuiTableRow, 4);
    tui_add_row_to_table(&tuiTable, tuiTableRow);

    tui_set_table_row_cell(tuiTableRow, 0, "+ New");
    tui_set_table_row_cell(tuiTableRow, 1, "");
    tui_set_table_row_cell(tuiTableRow, 2, "");
    tui_set_table_row_cell(tuiTableRow, 3, "");
    
    int i, j;

    for (i = 0; i < nbMappings; i++) {
        tuiTableRow = (TUITableRow*) malloc(sizeof(TUITableRow));

        tui_init_table_row(tuiTableRow, 4);
        tui_add_row_to_table(&tuiTable, tuiTableRow);

        char rangeStr[20];
        snprintf(rangeStr, 20, "%.3f->%.3f", oscButtonMappings[i].range[0], oscButtonMappings[i].range[1]);

        tui_set_table_row_cell(tuiTableRow, 0, oscButtonMappings[i].address);
        tui_set_table_row_cell(tuiTableRow, 1, BUTTON_NAMES[oscButtonMappings[i].button]);
        tui_set_table_row_cell(tuiTableRow, 2, rangeStr);
        tui_set_table_row_cell(tuiTableRow, 3, OSC_MAPPING_STYLE_NAME[oscButtonMappings[i].mappingStyle]);
    }

    int windowWidth = 60;
    int windowHeight = 30;

    TUILabel addressLabel;
    tui_init_label(&addressLabel, oscConnection.address, "Address");
    addressLabel.rect.x = 0;
    addressLabel.rect.y = 0;

    TUILabel portLabel;
    tui_init_label(&portLabel, oscConnection.port, "Port");
    portLabel.rect.x = windowWidth - 12;
    portLabel.rect.y = 0;

    TUILineEdit tuiLineEdit;
    tui_init_line_edit(&tuiLineEdit);

    TUIMsgLine tuiMsgLine;
    tui_init_msg_line(&tuiMsgLine);

    double x = 0.;
    double y = 0.;

    // TODO: REPLACE WITH THE ABSTRACTED INPUT SYSTEM AFTER
    // WE'RE DONE REMAKING IT
    SceCtrlData ctrl;
    unsigned int prevButtonState = 0x0;
    
    bool bEditingMode = false;

    bool bEditingDestination = false;
    bool bEditingOscAddress = false;
    bool bEditingButtonAssignation = false;
    bool bEditingOscRange = false;

    while (sys_main_loop(&mainSys)) {
        drawbuffer* currDrawBuffer = &gfxSys.framebuffer[gfxSys.currBackbufferIdx];
        
        // Assign console output framebuffer to current drawing buffer
        psvDebugScreenUseFramebuffer(currDrawBuffer->buffer, currDrawBuffer->mutex);

        // Blanking screen
        sceKernelLockMutex(currDrawBuffer->mutex, 1, NULL);
        gfx_fill_with_color(currDrawBuffer, 0x0);
        sceKernelUnlockMutex(currDrawBuffer->mutex, 1);
        
        // TODO: REPLACE WITH THE ABSTRACTED INPUT SYSTEM AFTER
        // WE'RE DONE REMAKING IT
        sceCtrlPeekBufferPositive(0, &ctrl, 1);

        unsigned int buttonsDown = ctrl.buttons & ~prevButtonState;
        unsigned int buttonsUp = ~ctrl.buttons & prevButtonState;

        prevButtonState = ctrl.buttons;

        if (!bEditingMode) {
            // Show mode.
            // Inputs mapped are firing their matching OSC messages 
            for (i = 0; i < nbMappings; i++) {
                if (buttonsDown & (1 << oscButtonMappings[i].button)) {
                    send_osc_message_from_mapping(sfd, addrTo, &oscButtonMappings[i], 1.0);
                }

                if (buttonsUp & (1 << oscButtonMappings[i].button)) {
                    send_osc_message_from_mapping(sfd, addrTo, &oscButtonMappings[i], 0.0);
                }
            }

            if (buttonsDown & SCE_CTRL_START) {
                bEditingMode = true;
                tuiTable.highlightedRowId = -1;
            }
        }
        else {
            // Edit mode.
            // No OSC messages fired, the user can operate the menu and change
            // mapping values
            if (!bEditingButtonAssignation) {
                if (buttonsDown & SCE_CTRL_DOWN) {
                    tuiTable.highlightedRowId = (tuiTable.highlightedRowId + 1) % tuiTable.nbRows;
                }
                else if (buttonsDown & SCE_CTRL_UP) {
                    tuiTable.highlightedRowId--;
                    if (tuiTable.highlightedRowId < 0) {
                        tuiTable.highlightedRowId = tuiTable.nbRows-1;
                    }
                }
                else if (buttonsDown & SCE_CTRL_SELECT) {
                    char addressAndPort[50];
                    
                    bEditingDestination = true;

                    if (oscConnection.port[0] == '\0') {
                        snprintf(addressAndPort, 50, "%s", oscConnection.address);
                    }
                    else {
                        snprintf(addressAndPort, 50, "%s:%s", oscConnection.address, oscConnection.port);
                    }

                    SceWChar16* addressAndPortW = (SceWChar16*) malloc(50 * sizeof(SceWChar16));

                    int i = 0;
                    char c;
                    while ((c = addressAndPort[i]) != '\0') {
                        addressAndPortW[i++] = (SceWChar16) c;
                    }
                    addressAndPortW[i] = u'\0'; 

                    ime_toggle_ime_system(&imeSys, addressAndPortW);

                    tui_show_line_edit(&tuiLineEdit, "Address/Port", addressAndPort);

                    free(addressAndPortW);
                }
                else if (buttonsDown & SCE_CTRL_CROSS) {
                    bEditingOscAddress = true;

                    if (tuiTable.highlightedRowId >= 0) {
                        SceWChar16* selectedOscAddressW = (SceWChar16*) malloc(256 * sizeof(SceWChar16));
                        selectedOscAddressW[0] = '\0';
                        
                        char* selectedOscAddress = NULL;

                        if (tuiTable.highlightedRowId > 0) {
                            TUITableRow* selectedTableRow = &tuiTable.aRows[tuiTable.highlightedRowId];
                            selectedOscAddress = selectedTableRow->columnsData[0];

                            int i = 0;
                            char c;
                            while ((c = selectedTableRow->columnsData[0][i]) != '\0') {
                                selectedOscAddressW[i++] = (SceWChar16) c;
                            }
                            selectedOscAddressW[i] = u'\0'; 
                        }

                        ime_toggle_ime_system(&imeSys, selectedOscAddressW);

                        if (selectedOscAddress == NULL)
                            tui_show_line_edit(&tuiLineEdit, "New OSC message address", "");
                        else
                            tui_show_line_edit(&tuiLineEdit, "Edit OSC message address", selectedOscAddress);

                        free(selectedOscAddressW);
                    }
                }
                else if (buttonsDown & SCE_CTRL_SQUARE && tuiTable.highlightedRowId > 0) {
                    tui_show_msg_line(&tuiMsgLine, "Press a button to assign to selected OSC message\n(press START to cancel or SELECT to erase)");
                    bEditingButtonAssignation = true;
                }
                else if (buttonsDown & SCE_CTRL_START) {
                    bEditingMode = false;
                }
            }
            else {
                if (buttonsDown > 0) {
                    TUITableRow* selectedTableRow = &tuiTable.aRows[tuiTable.highlightedRowId];
                    int mappingId = tuiTable.highlightedRowId-1;

                    if (buttonsDown & SCE_CTRL_SELECT) {
                        oscButtonMappings[mappingId].button = -1;
                        tui_set_table_row_cell(selectedTableRow, 1, "");
                    }
                    else if (!(buttonsDown & SCE_CTRL_START)) {
                        int btnId = 0;
                        int nbButtons = 16;

                        while ((buttonsDown & (1 << btnId)) == 0) { btnId++; }

                        if (btnId < nbButtons) {
                            tui_set_table_row_cell(selectedTableRow, 1, BUTTON_NAMES[btnId]);
                            oscButtonMappings[mappingId].button = btnId;
                        }
                    }

                    tui_hide_msg_line(&tuiMsgLine);
                    bEditingButtonAssignation = false;
                }
            }

            // Line edit widget TUI widget is active and visible. We're editing something
            if (tuiLineEdit.bActive) {
                int i;
                int imeMaxLength = minInt(imeSys.maxLength, IME_LINE_EDIT_MAX_LENGTH);

                for (i = 0; i < imeMaxLength; i++) {
                    tuiLineEdit.text[i] = (char) imeSys.inputBuffer[i];
                    if (tuiLineEdit.text[i] == '\0') {
                        break;
                    }
                }

                tuiLineEdit.text[i] = '\0';
                
                // Virtual keyboard is hidden. This means we finished writing in the
                // line edit prompt
                if (!imeSys.bVisible) {
                    if (bEditingDestination) {
                        // Validate new address and port edition
                        char c;
                        int j = 0;
                        i = 0;

                        if (tuiLineEdit.text[i] != ':') {
                            while ((c = tuiLineEdit.text[i]) != ':' && c != '\0') {
                                oscConnection.address[i] = c;
                                i++;
                            }
                        }
                        oscConnection.address[i] = '\0';

                        if (tuiLineEdit.text[i] == ':') {
                            i++;

                            while ((c = tuiLineEdit.text[i]) != '\0') {
                                oscConnection.port[j] = c;
                                i++;
                                j++;
                            }
                        }
                        oscConnection.port[j] = '\0';

                        bEditingDestination = false;
                    }
                    else if (bEditingOscAddress) {
                        // Validate new OSC mapping creation
                        if (tuiTable.highlightedRowId == 0) {
                            TUITableRow* newTableRow = (TUITableRow*) malloc(sizeof(TUITableRow));
                            tui_init_table_row(newTableRow, 4);
                            tui_add_row_to_table(&tuiTable, newTableRow);

                            char rangeStr[20];
                            snprintf(rangeStr, 20, "%.3f->%.3f", 0.0, 1.0);

                            tui_set_table_row_cell(newTableRow, 0, tuiLineEdit.text);
                            tui_set_table_row_cell(newTableRow, 2, rangeStr);
                            tui_set_table_row_cell(newTableRow, 3, OSC_MAPPING_STYLE_NAME[0]);

                            resizeArray(&oscButtonMappings,
                                        nbMappings * sizeof(OscButtonMapping),
                                        (nbMappings+1) * sizeof(OscButtonMapping));
                            
                            nbMappings++;

                            init_osc_button_mapping(&oscButtonMappings[nbMappings-1]);
                            copy_str(&oscButtonMappings[nbMappings-1].address, tuiLineEdit.text);
                        }
                        // Validate existing OSC mapping edition
                        else {
                            TUITableRow* selectedTableRow = &tuiTable.aRows[tuiTable.highlightedRowId];
                            tui_set_table_row_cell(selectedTableRow, 0, tuiLineEdit.text);
                        }

                        bEditingOscAddress = false;
                    }

                    tuiLineEdit.bActive = false;
                    //refresh_osc_mapping_view_table_values(&tuiTable, &oscButtonMappings, nbMappings);
                }

                strcpy(addressLabel.text, oscConnection.address);
                strcpy(portLabel.text, oscConnection.port);
            }
        }

        tui_redraw_table(&tuiTable, windowWidth, windowHeight);
        tui_draw_label(&addressLabel);
        tui_draw_label(&portLabel);
        if (tuiLineEdit.bActive) tui_redraw_line_edit(&tuiLineEdit);
        tui_redraw_msg_line(&tuiMsgLine);

        sceImeUpdate();

        gfx_swap_buffers(&gfxSys);
        gfx_wait_for_blank();
    }


    // Quitting
    sceKernelExitProcess(0);

    return 0;
}
