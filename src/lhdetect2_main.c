/*
 ============================================================================
 Name        : lhdetect2_main.c
 Author      : Genaev M.A.
 Version     : 0.5
 Description :
 ============================================================================
 */

#include "cv.h" /* required to use OpenCV */
#include "highgui.h" /* required to use OpenCV's highgui */
#include <getopt.h> /* required to use process options */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lhdetect2.h"
#include "tools.h"
#include "params.h"

int main (int argc, char** argv) {

	int c;
	char* job = "";
	char* filename = "";
	char* cmpfilename = "";
	char* outputfilename = "";
	char* orientation = "horizontal"; // or vertical
	float smooth = 1;//0.75;

	// шаг и размер окна при определении границы листа и фона
	int step = 1;
	int w_size = 60;//90;

	int min_trichome = 6;
	int angle = 105;
	int percent = 30;//35;

	int complex_thr_len = 0;

	if (argc == 1)
		help();

    while (1) {
		static struct option long_options[] =
		  {
			{"help",                no_argument,       0, 'h'},
			{"cmd",                 required_argument, 0, 'c'},
			{"input",               required_argument, 0, 'i'},
			{"output",              required_argument, 0, 'o'},
			{"cmp",                 required_argument, 0, 'r'},
			{"smooth",              required_argument, 0, 's'},
			{"window_border_size",  required_argument, 0, 'w'},
			{"border_step",         required_argument, 0, 't'},
			{"min_trichome",        required_argument, 0, 'm'},
			{"angle",               required_argument, 0, 'a'},
			{"percent",             required_argument, 0, 'p'},
			{"orientation",         required_argument, 0, 'e'},
			{"complex_trh_len",     no_argument,       0, 'l'},
			{0, 0, 0, 0}
		  };
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hlc:i:s:o:w:t:m:a:p:e:",
						 long_options, &option_index);


		/* Detect the end of the options. */
		if (c == -1)
		  break;

		switch (c)
		  {
		  case 'h':
			help();
			break;

		  case 'l':
			complex_thr_len = 1;
			break;

		  case 'i':
			filename = optarg;
			//printf("FILE = %s\n",filename);
			break;

		  case 'o':
			outputfilename = optarg;
			//printf("FILE = %s\n",filename);
			break;

		  case 'r':
			cmpfilename = optarg;
			break;

		  case 'c':
			job = optarg;
			//printf("CMD = %s\n",job);
			break;

		  case 'e':
			  orientation = optarg;
			  break;

		  case 's':
			smooth = atof(optarg);
			break;

		  case 'w':
			w_size = atoi(optarg);
			break;

		  case 't':
			step = atoi(optarg);
			break;

		  case 'm':
			min_trichome = atoi(optarg);
			break;

		  case 'a':
			angle = atoi(optarg);
			break;

		  case 'p':
			percent = atoi(optarg);
			break;

		  default:
			help ();
		  }
    }

    if (strcmp(job, "info") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	img_info(image);
		cvReleaseImage(&image);
    }

    else if (strcmp(job, "mat") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	mat (image, smooth);
		cvReleaseImage(&image);
    }

    else if (strcmp(job, "rm_noise") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	image = rm_noise(image,smooth);
		if (strcmp(outputfilename, "") == 0 ) {
			cvNamedWindow("B/W image", CV_WINDOW_AUTOSIZE);
			cvShowImage("B/W image", image);
			cvWaitKey(0);
			exit(0);
		}
		else {
			cvSaveImage(outputfilename,image,0);
		}
    	cvReleaseImage(&image);

    }

    else if (strcmp(job, "draw_border") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	image = rm_noise(image,smooth);
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	draw_border(image,color_image,step,w_size, 2);
    	assert( color_image != 0 );

    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
    }
    else if (strcmp(job, "rm_leaf") == 0) {

    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	image = rm_noise(image,smooth);
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	draw_border(image, color_image, step, w_size, 2);
    	assert( color_image != 0 );
    	color_image = rm_leaf(color_image, min_trichome);
    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
	}
    else if (strcmp(job, "detect") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	//img_info(image); //exit(0);
    	assert( image != 0 );
    	image = rm_noise(image,smooth);
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	LeafBorder l_b = draw_border(image,color_image, step, w_size, 2);
    	assert( color_image != 0 );
    	color_image = rm_leaf(color_image, min_trichome);
    	DetectedTrh d_t = detect(color_image, &l_b, angle, min_trichome, percent, complex_thr_len);
    	color_image = d_t.img;

    	for (int j=0; j<d_t.contur_count; j++) {
        	Contur contur = d_t.conturs[j];
    		printf ("Contur:%d area:%f complex:%d trichome count:%d\n",j+1, contur.area, isComplex(&contur), trhCount(&contur) );
    		printf ("\tHead pixel %d\n",contur.head_pts_count);
    		for (int i = 0; i < contur.head_pts_count; i++) {
    			printf("\t\tLength: %f\n",contur.thr_length[i]);
    			printf ("\t\t%d %d\n",contur.head_pts[i].x,contur.head_pts[i].y);
    		}
    		printf ("\tFoot pixel %d\n",contur.foot_pts_count);
    		for (int i = 0; i < contur.foot_pts_count; i++)
    			printf ("\t\t%d %d\n",contur.foot_pts[i].x,contur.foot_pts[i].y);
    		printf ("\n");
        }

    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
	}
    else if (strcmp(job, "cmp") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );

    	IplImage* ref_image = 0;
    	ref_image = cvLoadImage(cmpfilename,1);
    	assert( image != 0 );

    	image = rm_noise(image,smooth);
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	LeafBorder l_b = draw_border(image,color_image, step, w_size, 2);
    	assert( color_image != 0 );
    	color_image = rm_leaf(color_image, min_trichome);
    	DetectedTrh d_t = detect(color_image, &l_b, angle, min_trichome, percent, complex_thr_len);
    	Precision p = cmp(ref_image,&d_t);
    	color_image = d_t.img;

    	printf ("precision\t%f\n"
    			"recall\t%f\n"
    			"true positive\t%d\n"
    			"false positive\t%d\n"
    			"false negative\t%d\n"
    			"f1 score\t%f\n"
    			,p.precision,p.recall,p.tp,p.fp,p.fn,p.f1_score
    	);

    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
    	cvReleaseImage(&ref_image);
    }
    else if (strcmp(job, "cmp_border") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );

    	IplImage* ref_image = 0;
    	ref_image = cvLoadImage(cmpfilename,1);
    	assert( image != 0 );

    	image = rm_noise(image,smooth);
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	draw_border(image, color_image, step, w_size, 1);
    	double err = cmp_border(color_image,ref_image);
    	printf("%f\n",err);
    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
    	cvReleaseImage(&ref_image);

    }
    else if (strcmp(job, "statcolor") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	IplImage* color_image=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,3);
    	assert( color_image != 0 );
    	color_image = statcolor(image,color_image,smooth);
    	if (strcmp(outputfilename, "") == 0 ) {
    		cvNamedWindow("Color image", CV_WINDOW_AUTOSIZE);
    		cvShowImage("Color image", color_image);
    		cvWaitKey(0);
    		exit(0);
    	}
    	else {
    		cvSaveImage(outputfilename,color_image,0);
    	}
    	cvReleaseImage(&image);
    	cvReleaseImage(&color_image);
    }
    else if (strcmp(job, "point_distr") == 0) {
    	IplImage* image = 0;
    	image = cvLoadImage(filename,0);
    	assert( image != 0 );
    	point_distr(image,step,w_size,orientation);
    	cvReleaseImage(&image);
    }
    else {
    	printf("Unknown command \"%s\"\n",job);
    }
    return 0;
}
