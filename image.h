#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
	int width;
	int height;
	int maxgrey;
	unsigned char* data;
	int flag;
}
MyImage;

typedef struct 
{
	int width;
	int height;
	int* data;
	int flag;
}
MyIntImage;

int readPgm(char *fileName, MyImage* image);
int writePgm(char *fileName, MyImage* image);
int cpyPgm(MyImage *src, MyImage *dst);
void createImage(int width, int height, MyImage *image);
void createSumImage(int width, int height, MyIntImage *image);
int freeImage(MyImage* image);
int freeSumImage(MyIntImage* image);
void setImage(int width, int height, MyImage *image);
void setSumImage(int width, int height, MyIntImage *image);

#ifdef __cplusplus
}
#endif

#endif
