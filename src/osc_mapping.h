#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/osclib/src/osc_data.h"


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