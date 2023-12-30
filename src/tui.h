#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct TUITableRow {
    int nbColumns;
    char** columnsData;
    int* aCellStyle;
} TUITableRow;


typedef struct TUITable {
    TUITableRow* aRows;
    int nbRows;
    int highlightedRowId;
} TUITable;



void tui_init_table_row(TUITableRow* tuiTableRow, int nbColumns) {
    tuiTableRow->aCellStyle = NULL;
    tuiTableRow->nbColumns = nbColumns;
    tuiTableRow->columnsData = (char**) malloc(1 * nbColumns * sizeof(char*));
    int i;
    for (i = 0; i < nbColumns; i++) {
        tuiTableRow->columnsData[i] = (char*) malloc(1 * sizeof(char));
        tuiTableRow->columnsData[i][0] = '\0';
    }
}

void tui_free_table_row(TUITableRow* tuiTableRow) {
    // TODO: #FreeColumsData
    free(tuiTableRow->aCellStyle);
    free(tuiTableRow->columnsData);
}

void tui_redraw_table_row(TUITableRow* tuiTableRow) {}



void tui_init_table(TUITable* tuiTable) {
    tuiTable->aRows = NULL;
    tuiTable->nbRows = 0;
    tuiTable->highlightedRowId = -1;
}

void tui_free_table(TUITable* tuiTable) {
    int i;
    for (i = 0; i < tuiTable->nbRows; i++) {
        tui_free_table_row(&tuiTable->aRows[i]);
    }
}

void resizeArray(void* array, int oldSize, int newSize) {
    char** originalArray = (char**) array;
    char* newArray = (char*) malloc(newSize);
    
    memset(newArray, '\0', newSize);
    memcpy(newArray, *originalArray, oldSize);
    free(*originalArray);

    *originalArray = newArray;
}

void tui_add_row_to_table(TUITable* tuiTable, TUITableRow* tuiTableRow) {
    if (tuiTable->aRows == NULL) {
        tuiTable->aRows = (TUITableRow*) malloc(sizeof(TUITableRow));
    }
    else {
        resizeArray(&tuiTable->aRows,
                    tuiTable->nbRows * sizeof(TUITableRow),
                    (tuiTable->nbRows+1) * sizeof(TUITableRow));
    }
    memcpy(&tuiTable->aRows[tuiTable->nbRows], tuiTableRow, sizeof(TUITableRow));
    tuiTable->nbRows++;
}

void tui_remove_row_from_table(TUITable* tuiTable, int tableRowId) {}

void tui_redraw_table(TUITable* tuiTable, int windowWidth, int windowHeight) {
    int i;
    int j;

    for (i = 0; i < tuiTable->nbRows; i++) {
        TUITableRow tableRow = tuiTable->aRows[i];
        
        if (tuiTable->highlightedRowId == i) printf("\e[7m");

        for (j = 0; j < tableRow.nbColumns; j++) {
            char* colData = tableRow.columnsData[j];
            
            printf("\e[%d;%dH", 3 + i, windowWidth / (tableRow.nbColumns) * j);
            printf("%s", colData);
        }

        if (tuiTable->highlightedRowId == i) printf("\e[27m");
    }
}

void tui_set_table_row_cell(TUITableRow* tuiTableRow, int colNum, char* content) {
    int contentSize = strlen(content)+1;

    resizeArray(&tuiTableRow->columnsData[colNum],
                strlen(tuiTableRow->columnsData[colNum]+1),
                contentSize);

    memcpy(tuiTableRow->columnsData[colNum], content, contentSize);
}

void tui_highlight_row_table(TUITable* tuiTable, int tableRowId) {}
