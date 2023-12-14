#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#pragma pack(pop)

//Declaring a filter for furthur use
int gaussianBlur[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
float gaussianBlurCoef = 0.0625;

int rows;
int cols;
long fileLength; //total length of the bmp file

vector<vector<unsigned char>> redChannel;
vector<vector<unsigned char>> greenChannel;
vector<vector<unsigned char>> blueChannel;

bool fillAndAllocate(char*& buffer, const char* fileName, int& rows, int& cols, int& bufferSize) {
    std::ifstream file(fileName);
    if (!file) {
        std::cout << "File" << fileName << " doesn't exist!" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);
    fileLength = length;
    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return true;
}

void getPixelsFromBMP24(int end, int rows, int cols, char* fileReadBuffer) {
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {
            for (int k = 0; k < 3; k++) {
                switch (k) {
                case 0:
                    // fileReadBuffer[end - count] is the red value
                    redChannel[i][j] = fileReadBuffer[end - count];
                    break;
                case 1:
                    greenChannel[i][j] = fileReadBuffer[end - count];
                    // fileReadBuffer[end - count] is the green value
                    break;
                case 2:
                    blueChannel[i][j] = fileReadBuffer[end - count];
                    // fileReadBuffer[end - count] is the blue value
                    break;
                }
                count++;
                // go to the next position in the buffer
            }
        }
    }
}

void writeOutBmp24(char* fileBuffer, const char* nameOfFileToCreate, int bufferSize) {
    std::ofstream write(nameOfFileToCreate);
    if (!write) {
        std::cout << "Failed to write " << nameOfFileToCreate << std::endl;
        return;
    }

    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {   
            for (int k = 0; k < 3; k++) {
                switch (k) {
                case 0:
                    fileBuffer[bufferSize - count] = redChannel[i][j];
                    // write red value in fileBuffer[bufferSize - count]
                    break;
                case 1:
                    fileBuffer[bufferSize - count] = greenChannel[i][j];
                    // write green value in fileBuffer[bufferSize - count]
                    break;
                case 2:
                    fileBuffer[bufferSize - count] = blueChannel[i][j];
                    // write blue value in fileBuffer[bufferSize - count]
                    break;
                }
                count++;
                // go to the next position in the buffer
            }
        }
    }
    write.write(fileBuffer, bufferSize);
}


void verticalMirror(int rows, int cols){
    
    unsigned char temp;

    for (int i = 0; i < (rows/2) ; i++) {
        for (int j = 0; j < cols; j++) {   
            for(int k = 0; k < 3; k++){
                switch (k) {
                    case 0:
                        temp = redChannel[i][j];
                        redChannel[i][j] = redChannel[rows - i - 1][j];
                        redChannel[rows - i - 1][j] = temp;
                        break;
                    case 1:
                        temp = greenChannel[i][j];
                        greenChannel[i][j] = greenChannel[rows - i - 1][j];
                        greenChannel[rows - i - 1][j] = temp;
                        break;
                    case 2:
                        temp = blueChannel[i][j];
                        blueChannel[i][j] = blueChannel[rows - i - 1][j];
                        blueChannel[rows - i - 1][j] = temp;
                        break;
                }
            }
        }
    }
}

//function written with a 3*3 kernel in mind
void applyKernel(int rows, int cols, int kernel[3][3], float norm){
    
    //make a copy of current channels
    vector<vector<unsigned char>> tempRed = redChannel, tempGreen = greenChannel, tempBlue = blueChannel;

    for(int i = 1; i < rows - 1; i++){
        for(int j = 1; j < cols - 1; j++){
            float sumRed = 0, sumGreen = 0, sumBlue = 0;

            for(int k = -1; k <= 1; k++){
                for(int l = -1; l <= 1; l++){
                    sumRed += tempRed[i + k][j + l] * kernel[k + 1][l + 1] * norm;
                    sumGreen += tempGreen[i + k][j + l] * kernel[k + 1][l + 1] * norm;
                    sumBlue += tempBlue[i + k][j + l] * kernel[k + 1][l + 1] * norm;
                }
            }

            //Check if the output of filter is still in bounderies of our pixels colors
            sumRed = (sumRed < 0) ? 0 : (sumRed > 255) ? 255 : (sumRed);
            sumGreen = (sumGreen < 0) ? 0 : (sumGreen > 255) ? 255 : (sumGreen);
            sumBlue = (sumBlue < 0) ? 0 : (sumBlue > 255) ? 255 : (sumBlue);
            
            redChannel[i][j] = static_cast<unsigned char>(sumRed);
            greenChannel[i][j] = static_cast<unsigned char>(sumGreen);
            blueChannel[i][j] = static_cast<unsigned char>(sumBlue);
        }
    }
}


void purpleHaze(int rows, int cols){
    vector<vector<unsigned char>> tempRed = redChannel, tempGreen = greenChannel, tempBlue = blueChannel;

    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            //This formula is different that what the description said, but it works better!
            blueChannel[i][j] = min(255.0 , (0.5 * tempRed[i][j]) + (0.3 * tempGreen[i][j]) + (0.5 * tempBlue[i][j]));
            redChannel[i][j] = min(255.0, (0.16 * tempRed[i][j]) + (0.5 * tempGreen[i][j]) + (0.16 * tempBlue[i][j]));
            greenChannel[i][j] = min(255.0 ,(0.6 * tempRed[i][j]) + (0.2 * tempGreen[i][j]) + (0.8 * tempBlue[i][j]));
        }
    }
}


void drawDiagonalLines(int rows, int cols){
    for(int i = 0; i < rows; i++){
        for(int j = 0; j < cols; j++){
            if((i+j == rows) || (i+j == rows/2) || (i+j == rows + rows/2)){
                // white color code -> 255
                redChannel[i][j] = 255;
                greenChannel[i][j] = 255;
                blueChannel[i][j] = 255;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    char* fileBuffer;
    int bufferSize;
    if (!fillAndAllocate(fileBuffer, argv[1], rows, cols, bufferSize)) {
        std::cout << "File read error" << std::endl;
        return 1;
    }

    //Store each channel seperately
    redChannel.resize(rows, vector<unsigned char>(cols));
    greenChannel.resize(rows, vector<unsigned char>(cols));
    blueChannel.resize(rows, vector<unsigned char>(cols));

    // read input file
    getPixelsFromBMP24(fileLength, rows, cols, fileBuffer);
    // apply filters
    verticalMirror(rows, cols);
    applyKernel(rows, cols, gaussianBlur, gaussianBlurCoef);
    purpleHaze(rows, cols);
    drawDiagonalLines(rows, cols);
    // write output file
    writeOutBmp24(fileBuffer, "output.bmp", bufferSize);

    return 0;
}