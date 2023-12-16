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

struct {
    char* fileBuffer;
    int bufferSize;
    vector<vector<unsigned char>> redChannel;
    vector<vector<unsigned char>> greenChannel;
    vector<vector<unsigned char>> blueChannel;
} Photo;

#pragma pack(pop)

int rows;
int cols;
long fileLength; //total length of the bmp file



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
                    Photo.redChannel[i][j] = fileReadBuffer[end - count];
                    break;
                case 1:
                    Photo.greenChannel[i][j] = fileReadBuffer[end - count];
                    // fileReadBuffer[end - count] is the green value
                    break;
                case 2:
                    Photo.blueChannel[i][j] = fileReadBuffer[end - count];
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
                    fileBuffer[bufferSize - count] = Photo.redChannel[i][j];
                    // write red value in fileBuffer[bufferSize - count]
                    break;
                case 1:
                    fileBuffer[bufferSize - count] = Photo.greenChannel[i][j];
                    // write green value in fileBuffer[bufferSize - count]
                    break;
                case 2:
                    fileBuffer[bufferSize - count] = Photo.blueChannel[i][j];
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

int main(int argc, char* argv[]) {
    if (!fillAndAllocate(Photo.fileBuffer, argv[1], rows, cols, Photo.bufferSize)) {
        std::cout << "File read error" << std::endl;
        return 1;
    }
    
    //Store each channel seperately
    Photo.redChannel.resize(rows, vector<unsigned char>(cols));
    Photo.greenChannel.resize(rows, vector<unsigned char>(cols));
    Photo.blueChannel.resize(rows, vector<unsigned char>(cols));
    
    // read input file
    getPixelsFromBMP24(fileLength, rows, cols, Photo.fileBuffer);
    // apply filters
    // write output file
    writeOutBmp24(Photo.fileBuffer, "output.bmp", Photo.bufferSize);

    return 0;
}