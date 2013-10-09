//
//  main.c
//  Homework1(3)
//
//  Created by Ralf Cheung on 9/13/13.
//  Copyright (c) 2013 Ralf Cheung. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned char byte;
#define dimension 256
#define threshold 240



typedef struct node
{
	int pixel;
    int count;
    int x, y;
	struct node *link;
	
}NODE;

typedef struct
{
	int total;
	int max;
    int min;
    int rMax, gMax, bMax;
    int rMin, gMin, bMin;
	NODE *head;
} LIST;


typedef struct{
    int count;
    NODE* link;
}TABLE;


typedef struct{
    int size, min, max;
    
    TABLE* table;
}LISTTABLE;




void fileExport(char* fileName, byte *imageFile, int BytesPerPixel, int height, int width);
byte *readData(char *fileName, int channel);
void removeImpulseNoise(byte *imageData, int channel);
void printMatrix(byte*** matrix);
void bilateralFilter(byte *lena, int channel);
void histogram(char *filename, byte *image, int channel, int width, int height);
void NLMFilter(byte *lena, int channel);
LISTTABLE *createLISTTable(int size);
void adjustContrast (char *fileName, byte *lena, int channel, int width, int height);
TABLE insertNode (TABLE table, NODE* item);
LIST* insertWithPixel (LIST *pList, int pixel, NODE *pPre, int i, int j);




int main(int argc, const char * argv[])
{

    byte *imageData = readData("peppers_mix.raw", 3);
    
    histogram("pepperHistogram.raw", imageData, 3, dimension, dimension);
    
    removeImpulseNoise(imageData, 3);
    byte *lena = readData("lena.raw", 1);
    bilateralFilter(lena, 1);
    NLMFilter(lena, 1);
    return 0;
}

byte* readData(char *fileName, int channel){

    FILE* file  = fopen(fileName, "rb");
    if (!file) {
        printf("can't find file\n");
        exit(100);
    }
    
    byte *image = (byte*)calloc(dimension * dimension * channel, sizeof(byte));
    fread(image, sizeof(unsigned char), dimension * dimension * channel, file);
    fclose(file);
    
    return image;
    
}

void NLMFilter(byte *lena, int channel){
    
    int h = 60;

    
    
    for (int i = 5; i < dimension; i++) {
        for (int j = 5; j < dimension; j++) {
            
            byte * pixel = lena + dimension * j * channel + channel * i;
            double weight = 0;
            double total = 0;
            double totalWeight = 0;
            //assume the neighboorhood has a size of 1x1
            for (int iWalk = i - 5; iWalk < i + 6; iWalk++) {
                for (int jWalk = j - 5; jWalk < j + 6; jWalk++) {
                    byte *sample = lena + dimension * jWalk * channel + channel * iWalk;
                    weight = exp2(- (pow(pixel[0] - sample[0], 2)) / (h * h));
                    totalWeight += weight;
                    total += weight * sample[0];
                }
            }
            total /= totalWeight;
            pixel[0] =  (int)total;
        }
    }
    
    byte *tempImageData = (byte*)malloc((dimension - 5) * (dimension - 5) * channel * sizeof(byte));
    
    for (int i = 0; i < dimension - 5; i++) {
        for (int j = 0; j < dimension - 5; j++) {
            byte *tempData = tempImageData + (dimension - 5) * j * channel + channel * i;
            byte *pixel = lena + (dimension) * (j + 5) * channel + channel * (i + 5);
            tempData[0] = pixel[0];
        }
    }
    
    
    FILE *output = fopen("lenaNLMOutput.raw", "wb");
    fwrite(tempImageData, sizeof(unsigned char), (dimension - 5) * (dimension - 5) * channel, output);
    fclose(output);

    
//    adjustContrast("lenaNLMContrast.raw", tempImageData, channel, dimension - 5, dimension - 5);
    
    

}


LISTTABLE *createLISTTable(int size){
    LISTTABLE *pixelTable = (LISTTABLE*)malloc(sizeof(LISTTABLE));
    pixelTable->size =  size;
    pixelTable->table = (TABLE*)calloc(size, sizeof(TABLE));
    for (int i = 0; i < size; i++) {
        pixelTable->table[i].count = 0;
        pixelTable->table[i].link = NULL;
    }
    pixelTable->min = 255;
    pixelTable->max = 0;
    return pixelTable;
}


TABLE insertNode (TABLE table, NODE* item) {
    
    
    NODE* pCur = table.link;
    if (!pCur) {
        table.link = item;
    }else{
        item->link = table.link;
        table.link = item;
    }
    table.count++;
    return table;
}


void fileExport(char* fileName, byte *imageFile, int BytesPerPixel, int height, int width){
    
    FILE* file = fopen(fileName, "wb");
    fwrite(imageFile, sizeof(unsigned char), height * width * BytesPerPixel, file);
    fclose(file);
    
}


void insertList(LISTTABLE *pixelTable, int pixel, int i, int j){
    
    NODE* item = (NODE*)malloc(sizeof(NODE));
    item->pixel = pixel;
    item->x = i;
    item->y = j;
    
    pixelTable->table[pixel] = insertNode(pixelTable->table[pixel], item);
    //    printTable(pixelTable->table[pixel]);
    
    for (int i = 0; i < 256; i++) {
        if (pixelTable->table[i].count > 0) {
            if (pixelTable->table[i].count < pixelTable->min) {
                pixelTable->min = i;
            }
            if (pixelTable->table[i].count > pixelTable->max) {
                pixelTable->max = i;
            }
        }
    }
    
}


void adjustContrast (char *fileName, byte *lena, int channel, int width, int height){
    
    

    LISTTABLE *pixels = NULL;
    pixels = createLISTTable(256);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            byte *pixel = lena + height * j * channel + channel * i;
            insertList(pixels, pixel[0], i, j);
            
        }
    }
    
    
    NODE* pWalk = NULL;
    NODE* pPre = NULL;
    NODE* nodes = NULL;
    
    for (int i = 0; i < 256; i++) {
        if (pixels->table[i].count > 0) {
            nodes = pixels->table[i].link;
            break;
        }
    }
    
    
    pWalk = nodes;
    pPre = pWalk;
    for (int i = nodes->pixel; i < 256; i++) {

        
        if (pixels->table[i].count > 0 && !pWalk) {
            pWalk = pixels->table[i].link;
            if (pPre->link == NULL) {
                pPre->link = pWalk;
            }
        }

        
        while (pWalk) {
            pPre = pWalk;
            pWalk = pWalk->link;
        }
    }
    
    int sigma = 70;
    int mean = 110;
    int distribution[256] = {0};
    
    for (int i = 0; i < 256; i++) {
        distribution[i] = (int) ((dimension - 5) * (dimension - 5) * exp2f(-(pow((i - mean), 2))/ (2 * pow(sigma, 2))) / (sqrt(2 * M_PI) * sigma));
    }
    
    pWalk = nodes;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < distribution[i] && pWalk; j++) {
            
            pWalk->pixel = i;
            pWalk = pWalk->link;
        }
    }

    
    
    
    pWalk = nodes;
    while (pWalk) {
        byte *pixel = lena + height * pWalk->y * channel + channel * pWalk->x;
        pixel[0] = pWalk->pixel;
        pWalk = pWalk->link;
    }
    
    
    fileExport(fileName, lena, channel, height, width);

}


void bilateralFilter(byte *lena, int channel){
    int sigmaC = 2;
    int sigmaS = 50;

    
    double normalizationFactor = 0;
    for (int i = 1; i < dimension; i++) {
        for (int j = 1; j < dimension; j++) {
            normalizationFactor = 0;

            byte matrix[3][3] = {0};
            byte* image = lena + (dimension) * j * channel + channel * i;
            
            double weight = 0;
            for( int iWalk = i - 1, ia = 0; iWalk < i + 2; ia++, iWalk++) {
                for (int jWalk = j - 1, ja = 0 ; jWalk < j + 2; ja++, jWalk++) {
                    byte* pixelsAround = lena + (dimension) * jWalk * channel + channel * iWalk;
                    
                    matrix[ia][ja] = pixelsAround[0];
                        int intensityDistance = pixelsAround[0] - image[0];
                        double intensityCloseness = exp2( -pow(intensityDistance / sigmaS, 2) / 2);
                        double distance = pow(jWalk - j, 2) + pow(iWalk - i, 2);
                        double geometricClosness = exp2( -pow(distance / sigmaC, 2) / 2) ;
                        normalizationFactor += (intensityCloseness * geometricClosness);
                        weight += (pixelsAround[0] * intensityCloseness * geometricClosness);

                    
                }
            }
            
            weight /= normalizationFactor;
            image[0] = weight;
        }
        
    }
    
    FILE *output = fopen("lenaBilateral.raw", "wb");
    fwrite(lena, sizeof(unsigned char), (dimension) * (dimension) * channel, output);
    fclose(output);
    adjustContrast("lenaBilateralContrast.raw", lena, channel, dimension, dimension);
    
    
}

void removeImpulseNoise(byte *imageData, int channel){
    
    byte *tempImageData = (byte*)malloc((dimension + 1) * (dimension + 1) * channel * sizeof(byte));
    int replaced = 0;
    
    for (int i = 0; i < dimension; i++) {
        byte *tempPixel = tempImageData + (dimension) * 0 * channel + 3 * i;
        byte * pixel = imageData + (dimension) * 0 * channel + 3 * i;
        tempPixel[0] = pixel[0];
        tempPixel[1] = pixel[1];
        tempPixel[2] = pixel[2];
    }

    for (int j = 0; j < dimension; j++) {
        byte *tempPixel = tempImageData + (dimension) * j * channel + channel * j;
        byte *pixel = imageData + (dimension) * j * channel + channel * 0;
        tempPixel[0] = pixel[0];
        tempPixel[1] = pixel[1];
        tempPixel[2] = pixel[2];
    }
    
    
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            byte *tempPixel = tempImageData + (dimension + 1) * (j + 1) * channel + channel * (i + 1);
            byte *pixel = imageData + dimension * (j) * channel + channel * (i);
            tempPixel[0] = pixel[0];
            tempPixel[1] = pixel[1];
            tempPixel[2] = pixel[2];
        }
    }
    
    byte *tempPixel = tempImageData + (dimension) * 0 * channel + 3 * 0;;
    byte *pixel = imageData + (dimension) * 0 * channel + 3 * 0;
    tempPixel[0] = pixel[0];
    tempPixel[1] = pixel[1];
    tempPixel[2] = pixel[2];
    
    
    
    
    for (int i = 1; i < dimension + 1; i++) {
        for (int j = 1; j < dimension + 1; j++) {
            byte matrix[3][3][3] = {0};
            
            
            int red = 0, green = 0, blue = 0;

            for (int iWalk = i - 1, ia = 0; iWalk < i + 2; ia++, iWalk++) {
                for (int jWalk = j - 1, ja = 0 ; jWalk < j + 2; ja++, jWalk++) {
                    byte* pixelsAround = tempImageData + (dimension + 1) * jWalk * channel + channel * iWalk;

                    matrix[ia][ja][0] = pixelsAround[0];
                    matrix[ia][ja][1] = pixelsAround[1];
                    matrix[ia][ja][2] = pixelsAround[2];
                    
                    red += matrix[ia][ja][0];
                    green += matrix[ia][jWalk-j][1];
                    blue += matrix[ia][jWalk-j][2];

                }
            }
            
            //subtract the target pixel from the total weight
            red -= matrix[1][1][0];
            green -= matrix[1][1][1];
            blue -= matrix[1][1][2];
            
            //calculate the intensity of each channel
            red /= 8;
            green /= 8;
            blue /= 8;
            

//            
//            if ((matrix[1][1][0] <= red + threshold) && (matrix[1][1][0] >= red - threshold) && (matrix[1][1][1] <= green + threshold) && (matrix[1][1][2] >= green - threshold) && (matrix[1][1][2] <= blue + threshold) && (matrix[1][1][2] >= blue - threshold)) {
//                byte *pixel = tempImageData + (dimension + 1) * j * channel + channel * i;
//                pixel[0] = red;
//                pixel[1] = green;
//                pixel[2] = blue;
//                replaced++;
//
//            }
            
            if((matrix[1][1][0] <= red + threshold) && (matrix[1][1][0] >= red - threshold)){
                byte *pixel = tempImageData + (dimension + 1) * j * channel + channel * i;
                pixel[0] = red;

            }
            if ((matrix[1][1][1] <= green + threshold) && (matrix[1][1][2] >= green - threshold)) {
                byte *pixel = tempImageData + (dimension + 1) * j * channel + channel * i;
                pixel[1] = green;

            }
            if ((matrix[1][1][2] <= blue + threshold) && (matrix[1][1][2] >= blue - threshold)) {
                byte *pixel = tempImageData + (dimension + 1) * j * channel + channel * i;
                pixel[2] = blue;

            }
 
        }
    }
    byte *finalImage = (byte*)malloc((dimension) * (dimension) * channel * sizeof(byte));
    for (int i = 0; i < dimension; i++) {
        for (int j = 0; j < dimension; j++) {
            byte *tempData = tempImageData + (dimension + 1) * (j + 1) * channel + channel * (i + 1);
            byte *finalData = finalImage + (dimension) * j * channel + channel * i;
            finalData[0] = tempData[0];
            finalData[1] = tempData[1];
            finalData[2] = tempData[2];
            
        }
    }
    
    FILE *output = fopen("output.raw", "wb");
    fwrite(finalImage, sizeof(unsigned char), (dimension) * (dimension) * channel, output);
    fclose(output);
    histogram("pepperHistogram.raw", finalImage, 3, dimension, dimension);

    return;
}


void histogram(char *filename, byte *image, int channel, int width, int height){
    int red[256] = {0};
    int green[256] = {0};
    int blue[256] = {0};
    int intensity[256] = {0};
    
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            byte *pixel = image + height * j * channel + channel * i;
            if (channel == 1) {
                intensity[pixel[0]]++;
            }
            else{
                red[pixel[0]]++;
                green[pixel[1]]++;
                blue[pixel[2]]++;
            }
        }
    }
    
    FILE* file = fopen(filename, "w");
    if (channel == 1) {
        fprintf(file, "Intensity\n");
        for (int i = 0; i < 256; i++)
            fprintf(file, "%d\n", intensity[i]);
        
    }else{
        fprintf(file, "Red,Green,Blue\n");
        for (int i = 0; i < 256; i++) {
            fprintf(file, "%d,%d,%d\n", red[i], blue[i], green[i]);
        }
    }
    
}
