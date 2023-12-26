#include <stdlib.h>
#include <stdio.h>
#include "../deps/osclib/src/osc_data.h"
#include "tui.h"


int main() {
    struct winsize sz;
    ioctl( 0, TIOCGWINSZ, &sz );

    int windowHeight = sz.ws_row;
    int windowWidth = sz.ws_col;

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

    tui_redraw_table(&tuiTable, windowHeight, windowWidth);
}
