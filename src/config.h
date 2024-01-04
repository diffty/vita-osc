#ifndef TUI_CONFIG_H
#define TUI_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osc_mapping.h"


typedef struct OscConnection {
    char address[40];
    char port[6];
} OscConnection;

#endif