/*
 * ColorSlider.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: Jonas Larsson
 */

 //MAKE SURE YOU USE THE SAME V4L2 SETTINGS FOR THINGS SUCH AS AUTO EXPOSURE AS YOU WILL IN YOUR OTHER PROGRAM, 
 //OR THE RANGES WILL VARY!

#include<opencv/highgui.h>
//#include<cv.h>
#include<opencv2/imgproc/imgproc.hpp>

#include<stdio.h>
#include <sys/time.h>
#include <time.h>
 

#define h 480
#define w 640

CvCapture* capture;
IplImage* img;
IplImage* thresh;

//Variables for sliders
int b1=0;int g1=0;int r1=0;
int b2=255;int g2=120;int r2=0;
int movie_id;
void initialize()
{
//Camera capture and image variables
img=cvCreateImage(cvSize(w,h),8,3);
thresh=cvCreateImage(cvSize(w,h),8,1);


//	capture=cvCreateCameraCapture(1);
	if (movie_id == 1)
	{
		capture=cvCaptureFromFile("GATSBY-1024.mov");
		printf("loading movie_id %d\n", movie_id);
	}
	
// printf("test\n");	
	//Initial capture settings
	
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, w);
 	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, h);
// 	
	//Windows
 	cvNamedWindow("Original",CV_WINDOW_AUTOSIZE);
 	cvNamedWindow("Thresholded",CV_WINDOW_AUTOSIZE);
 	cvNamedWindow("cnt",CV_WINDOW_AUTOSIZE);
	//Creating the sliders
	cvCreateTrackbar("B1","cnt",&b1,255,0);
	cvCreateTrackbar("B2","cnt",&b2,255,0);
	cvCreateTrackbar("G1","cnt",&g1,255,0);
	cvCreateTrackbar("G2","cnt",&g2,255,0);
	cvCreateTrackbar("R1","cnt",&r1,255,0);
	cvCreateTrackbar("R2","cnt",&r2,255,0);
}

struct timeval tv_start, tv_end;

int main(int argc, char* argv[])
{
	
	int task_id = atoi(argv[1]);
	int no_jobs = atoi(argv[2]);
	movie_id = atoi(argv[3]);
	
	initialize();
	if(capture == NULL)
	{
	  printf("capture is null movie_id %d\n", movie_id);
	  return -1;
	}
	img = cvQueryFrame(capture);
	thresh = cvCreateImage(cvGetSize(img),8,1);
	int tmp = 0;//rt_attach_task_to_mod(task_id);
	printf("res: %d task %d attached to the module!\n", tmp, task_id);

	int i = 0;
	while(i<no_jobs)
	{
		i++;
		gettimeofday(&tv_start, (struct timezone*)0);
		//get image from camera feed
		img = cvQueryFrame(capture);

		//thresh = cvCreateImage(cvGetSize(img),8,1);
		//Thresholding the image
		cvInRangeS(img,cvScalar(b1,g1,r1,0),cvScalar(b2,g2,r2,0),thresh);
		//Showing the images
 		cvShowImage("Original",img);
 		cvShowImage("Thresholded",thresh);
		
		gettimeofday(&tv_end, (struct timezone*)0);
		printf("task <%d>job %d Elapsed time: %f imageID %d\n", task_id, i, ((tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec))/1000000.0, img->ID);
// 		
		//rt_task_finish_job(task_id);
		//Escape Sequence
// 		char c=cvWaitKey(33);
// 		if(c==27)
// 			break;
	}
	//Cleanup
 	cvReleaseImage(&img);
 	cvReleaseImage(&thresh);
	cvDestroyAllWindows();
	rt_detach_task(task_id);
	printf("task %d finished!\n", task_id);
}

