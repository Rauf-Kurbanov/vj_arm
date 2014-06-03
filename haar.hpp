#ifndef __HAAR_H__
#define __HAAR_H__

#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include <vector>
#include "stdio-wrapper.h"

#define MAXLABELS 50

#ifdef __cplusplus
extern "C" {
#endif

typedef  int sumtype;
typedef int sqsumtype;

typedef struct MyPoint
{
    int x;
    int y;
}
MyPoint;

typedef struct
{
    int width;
    int height;
}
MySize;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
}
MyRect;

typedef struct myCascade
{
// number of stages (22)
    int  n_stages;
    int total_nodes;
    float scale; 
 
    // size of the window used in the training set (20 x 20)
    MySize orig_window_size;
//    MySize real_window_size;

    int inv_window_area;

    MyIntImage sum;
    MyIntImage sqsum;
   
    // pointers to the corner of the actual detection window
    sqsumtype *pq0, *pq1, *pq2, *pq3;
    sumtype *p0, *p1, *p2, *p3;

} myCascade;



/* sets images for haar classifier cascade */
void setImageForCascadeClassifier( myCascade* cascade, MyIntImage* sum, MyIntImage* sqsum);

/* runs the cascade on the specified window */
int runCascadeClassifier( myCascade* cascade, MyPoint pt, int start_stage);

void readTextClassifier();//(myCascade* cascade);
void releaseTextClassifier();


//void groupRectangles(MyRect* _vec, int groupThreshold, float eps);
void groupRectangles(std::vector<MyRect>& _vec, int groupThreshold, float eps);

/* draw white bounding boxes around detected faces */
void drawRectangle(MyImage* image, MyRect r);

//void detectObjects( MyImage* image, MySize minSize, MySize maxSize,
//		myCascade* cascade, MyRect *result,
//		float scale_factor,
//		int min_neighbors);

std::vector<MyRect> detectObjects( MyImage* image, MySize minSize, MySize maxSize,
		myCascade* cascade,
		float scale_factor,
		int min_neighbors);
		
#ifdef __cplusplus
}

#endif

#endif
