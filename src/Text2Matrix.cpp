#include <Arduino.h>
#include "Text2Matrix.h"

Text2Matrix::Text2Matrix(int rPins[8], int cPins[8], boolean inv) {
    invert = inv;
    
    for (int i = 0; i < 8; i++) {
        rowPins[i] = rPins[i];
        pinMode(rowPins[i], OUTPUT);
        colPins[i] = cPins[i];
        pinMode(colPins[i], OUTPUT);
    }
}


void Text2Matrix::drawRow(int row, byte contents) {
    // Disable all other rows
    for (int j = 0; j < 8; j++) {
        digitalWrite(rowPins[j], invert);
    }
    // Enable desired row
    digitalWrite(rowPins[row], !invert);
    // Display contents of current row
    for (int i = 0; i < 8; i++) {
        /* we need XNOR
        bitRead	invert	result
        1		0		0
        0		0		1
        1		1		1
        0		1		0
        */
        digitalWrite(colPins[i], (bitRead(contents, i) ^ !invert));
    }
}


void Text2Matrix::drawMatrix(byte img[8], int totalMs) {
    const int timePerRow = 2;
    for (int t = 0; t < totalMs; t += (timePerRow * 8)) {
        for (int i = 0; i < 8; i++) {
            drawRow(i, img[i]);
            delay(timePerRow);
        }
    }
}



void Text2Matrix::displayTextInMatrix(String text, float speed) {
    byte** matrix = textToMatrix(text);
    
    displayArrayMoving(matrix, speed);

    // free memory
    int i = 0;
    while (matrix[i] != NULL) {
        free(matrix[i]);
        i++;
    }
    free(matrix);
}

byte** Text2Matrix::textToMatrix(String text) {
    int nBits = text.length() * 6;
    int nBytes = (nBits%8 > 0) ? (nBits/8)+1 : (nBits/8);
    
    // Allocate array of nBytes byte-pointers
    byte **result = (byte**) malloc((nBytes + 1)*sizeof(byte*));
    // for each pointer, create array of 8 bytes and initialise everything to all zeros
    for (int j = 0; j < nBytes; j++) {
        result[j] = (byte*) malloc(8*sizeof(byte));
        for (int b = 0; b < 8; b++) {
            result[j][b] = B00000000;
        }
    }
    free(result[nBytes]);
    result[nBytes] = NULL;

    
    int asciiCode = 0, positionDifference = 0;
    byte selectedBit, byteToAdd;
    for (int i = 0; i < nBits; i++) {
        asciiCode = (int)text.charAt(i/6);

        // original column - column in byte
        positionDifference = (i%6) - (i%8);
        
        if (asciiCode >= 64) {
            // loop through rows and write it
            for (int row = 0; row < 8; row++) {
                // for each row, add 0 or 1 from the char in the next column
                selectedBit = (IMAGES[asciiCode-64][row] & (1 << (7 - i%6)));

                // Now we need to shift selectedBit so it aligns with the rest of the text
                if (positionDifference >= 0) {
                    // original col is greater than final col, shift left
                    byteToAdd = selectedBit << positionDifference;
                } else {
                    byteToAdd = selectedBit >> -positionDifference;
                }

                result[i/8][row] |= byteToAdd;
            }

        } else {
            // Show a whitespace (TODO)
            if (asciiCode == 0) break;
        }
    }
    return result;
}


void Text2Matrix::displayArrayMoving(byte** img, float speed) {
    byte show[8] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };
    // draw it
    int i = 0;
    while (true) {
        if (img[i] == NULL) break;

        for (int k = 0; k < 8; k++) {
            for (int j = 0; j < 8; j++) {
                // shift the current bits 1 position to the left
                show[j] = show[j] << 1;
                show[j] |= bitRead(img[i][j], 8-k-1);
            }
            // draw matrix every time a column is updated
            drawMatrix(show, (int)(100/speed));
        }
        
        i++;
    }

    // white space
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // shift the current bits 1 position to the left
            show[j] = show[j] << 1;
        }
        drawMatrix(show, 50);
    }
}
