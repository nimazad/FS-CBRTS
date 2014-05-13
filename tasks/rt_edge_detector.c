/*
 * ColorSlider.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: Jonas Larsson
 */

 //MAKE SURE YOU USE THE SAME V4L2 SETTINGS FOR THINGS SUCH AS AUTO EXPOSURE AS YOU WILL IN YOUR OTHER PROGRAM, 
 //OR THE RANGES WILL VARY!

#include<stdio.h>

#include<opencv/highgui.h>
//#include<cv.h>
#include<opencv2/imgproc/imgproc.hpp>
int h = 480;
int w = 640;

//Camera capture and image variables
CvCapture* capture;
IplImage* img;
IplImage* gray;
IplImage * edgeImage; 

int movie_id;
struct timeval tv_start, tv_end;
void initialize()
{
	IplImage* img=cvCreateImage(cvSize(w,h),8,3);

	//capture=cvCreateCameraCapture(-2);
	capture=cvCaptureFromFile("Fast.mov");
	//Initial capture settings
	img = cvQueryFrame(capture);
	gray = cvCreateImage(cvGetSize(img),8,1);
	edgeImage = cvCreateImage(cvGetSize(img),8,1);
	
	w = img->width;
	h = img->height;
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, w);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, h);

	//Windows
	//cvNamedWindow("Original Image",CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("Thresholded Image",CV_WINDOW_AUTOSIZE);
}

int main(int argc, char* argv[])
{
	int task_id = atoi(argv[1]);
	int no_jobs = atoi(argv[2]);
	movie_id = atoi(argv[3]);
	
	double exec_history[no_jobs];
	initialize();
	int i = 0;
	double exec_time;
	img = cvQueryFrame(capture);
	gray = cvCreateImage(cvGetSize(img),8,1);
	edgeImage = cvCreateImage(cvGetSize(img),8,1);
	
	int tmp = rt_attach_task_to_mod(task_id);
	printf("res: %d task %d attached to the module!\n", tmp, task_id);

	while(i<no_jobs)
	{
		gettimeofday(&tv_start, (struct timezone*)0);
		//get image from camera feed
		img = cvQueryFrame(capture);
		gray = cvCreateImage(cvGetSize(img),8,1);
		edgeImage = cvCreateImage(cvGetSize(img),8,1);
		cvCvtColor(img, gray, CV_BGR2GRAY);
		cvCanny(gray, edgeImage, 0.5, 0.5, 3);
		//Showing the images
		//cvShowImage("Original Image",img);
		//cvShowImage("Thresholded Image",thresh);
		//cvShowImage("Thresholded Image",edgeImage);
		gettimeofday(&tv_end, (struct timezone*)0);
		exec_time = ((tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec))/1000.0;
		exec_history[i] = exec_time;
		printf("task <%d>job %d Elapsed time: %f \n", task_id, i, exec_time);
		i++;
		rt_task_finish_job(task_id);
		//Escape Sequence
		//char c=cvWaitKey(33);
		//if(c==27)
		//	break;
	}
	//Write to file
	
	
	rt_detach_task(task_id);
	printf("task %d finished!\n", task_id);
	
	FILE *f = fopen("execs.txt", "wb");
	for (i = 0; i < no_jobs; i++) 
	{
	  fprintf(f, "%f\n", exec_history[i]);
	}
	fclose(f);
	
	printf("task %d cleaning!\n", task_id);
	//Cleanup
	cvReleaseImage(&img);
	cvReleaseImage(&gray);
	cvReleaseImage(&edgeImage);
	cvDestroyAllWindows();
}
