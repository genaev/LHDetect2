/*
 * lhdetect2.c
 *
 *  Created on: 13.02.2011
 *      Author: mag
 */

#include <stdio.h>
#include "cv.h" /* required to use OpenCV */
#include "highgui.h" /* required to use OpenCV's highgui */
#include "tools.h"
#include "params.h"
#include "lhdetect2.h"
#include "MyLibs/AndysOpenCVLib.h"

void help () {
	puts(	"./lhdetect2 --input image_file_name --cmd command <options>\n"
			"Options:\n"
			"\t--input or -i - input image file\n"
			"\t--output or -o - output image file. if output file undefined image is shown on the screen\n"
			"\t--cmp or -r - input reference image\n"
			"\t--cmd or -c - command name\n"
			"\t\tinfo - print some info about input image and exit\n"
			"\t\tmat - convert image in B/W mode, print color of each pixel and exit\n"
			"\t\tstatcolor - paints an image in different colors depending on the intensity of color pixels in the source image\n"
			"\t\trm_noise - clears the image from noise\n"
			"\t\tdraw_border - draw border of leaf and background and exit (border is a red line thickness of two pixels)\n"
			"\t\trm_leaf - using the leaf and backgrount border removes the leaf field, leaving only the trichomes. after that binarization image.\n"
			"\t\tdetect - detect trichomes. print information to stdout.\n"
			"\t\tcmp - detect trichomes and comparison with reference image\n"
			"\t\tcmp_border - detect border of leaf and background and comparison with reference image\n"
			"\t\tpoint_distr - calculate distributions of point on image. use -w -t -e options\n"
			"\t--smooth or -s - smooth image before processing, the parameter determinate σ in Gaussian blur method (from 0 to 3, default 1)\n"
			"\t--window_border_size or -w - window size in determining the leaf and background border (default 60)\n"
			"\t--border_step or -t - step in determining the leaf and background border (default 1)\n"
			"\t--min_trichome or -m - minimal trichome size. not recommended change this parameter! (default 6)\n"
			"\t--angle or -a - maximum angle (in degrees) of curvature of the contour, which will be considered (default 105)\n"
			"\t--percent or -p - minimum percent of background points (in the circle of radius min_trichome/2 around potential point) at which the potential point will be considered as head trichomes (default 30)\n"
			"\t--orientation or -e - horizontal or vertical. this options used for 'point_distr' command only (default horizontal)\n"
			"\t--complex_thr_len or -l - calculate length of trichomes for complex contours (not calculated by default)\n");
	exit(0);
}

void img_info (IplImage* image) {
	printf( "[i] channels:    %d\n"
			"[i] pixel depth: %d bits\n"
			"[i] width:       %d pixels\n"
			"[i] height:      %d pixels\n"
			"[i] image size:  %d bytes\n"
			"[i] width step:  %d bytes\n"
			,image->nChannels, image->depth, image->width, image->height, image->imageSize, image->widthStep);
}

void mat (IplImage* image, float smooth) {
	cvNamedWindow("original",CV_WINDOW_AUTOSIZE);
	if (smooth > 0)
		cvSmooth(image, image, CV_GAUSSIAN, 0, 0, smooth, 0);
	for( int y=0; y<image->height; y++ ) {
			uchar* ptr = ((uchar*)(image->imageData + image->widthStep*y));
			for( int x=0; x<image->width; x++ ) {
				printf( "%d ",  ptr[x] );
			}
			printf("\n");
	}
}

IplImage* rm_noise (IplImage* image, float smooth) {
	IplImage* bw_image = 0;

	if (smooth > 0)
		cvSmooth(image, image, CV_GAUSSIAN, 0, 0, smooth, 0);

	bw_image = cvCloneImage(image);
	assert( bw_image != 0 );

	int step       = image->widthStep/sizeof(uchar);
	uchar* data    = (uchar *)image->imageData;
	for (int y = 0; y<image->height; y++) {
		for (int x = 0; x<image->width; x++) {
			uchar c = data[y*step+x];
			if (c >= class4 && c <= class5) {
				cvSet2D(bw_image,y,x,WHITE);
			}
			else {
				cvSet2D(bw_image,y,x,BLACK);
			}
		}
	}

	cvFloodFill( bw_image, cvPoint(image->width-1,0), GRAY,
             cvScalarAll(0), // минимальная разность
             cvScalarAll(0), // максимальная разность
             NULL,
             4,
             NULL);

	step       = bw_image->widthStep/sizeof(uchar);
	data    = (uchar *)bw_image->imageData;
	for (int y = 0; y<bw_image->height; y++) {
		for (int x = 0; x<bw_image->width; x++) {
			uchar c = data[y*step+x];
			if (c == 255 || c == 0) {
				cvSet2D(image,y,x,WHITE);
			}
		}
	}
	cvReleaseImage(&bw_image);
	return image;
}

IplImage* statcolor (IplImage* image, IplImage* color_image,  float smooth) {
	if (smooth > 0)
		cvSmooth(image, image, CV_GAUSSIAN, 0, 0, smooth, 0);

	int step       = image->widthStep/sizeof(uchar);
	uchar* data    = (uchar *)image->imageData;

	for (int y = 0; y<image->height; y++) {
		for (int x = 0; x<image->width; x++) {

			uchar c = data[y*step+x];
			if      (c >= 0 && c < class1) {
				//class1
				cvSet2D(color_image,y,x,GREEN);
			}
			else if (c >= class1 && c < class2) {
				//class2
				cvSet2D(color_image,y,x,LBLUE);
			}
			else if (c >= class2 && c < class3) {
				//class3
				cvSet2D(color_image,y,x,BLUE);
			}
			else if (c >= class3 && c < class4) {
				//class4
				cvSet2D(color_image,y,x,RED);
			}
			else if (c >= class4 && c <= class5) {
				//class5
				cvSet2D(color_image,y,x,BLACK);
			}

		}
	}
	return color_image;
}

LeafBorder draw_border (IplImage* image, IplImage *color_image, int step, int w_size, int thickness) {

	// формируем массив точек границы фона и растения. растение - лист + трихомы
	int board[image->height];
	//for( int y=0; y<image->height; y++ ) {
	//	uchar* ptr = ((uchar*)(image->imageData + image->widthStep*y));
	//	for( int x=0; x<image->width; x++ ) {
	//		if (ptr[x] >= 0 && ptr[x] < class1) {
	//			board[y] = x;
	//			break;
	//		}
	//	}
	//}
	for( int y=0; y<image->height; y++ ) {
		uchar* ptr = ((uchar*)(image->imageData + image->widthStep*y));
		for(int x=image->width-100; x>=0; x--) {
			if (ptr[x]>=class1) {
				board[y] = x-1;
				break;
			}
		}
	}

	// Вычисляем границу листа и фона. pts[] - массив точек для рисования границы
	int step_count = (image->height-w_size)/step;
	CvPoint pts[step_count+2];
	int n=0;
	for (int i=0; i<image->height-w_size; i+=step) {
		int max_x = 0;
		int y = 0;
		for (int j=i; j<i+w_size; j++) {
			if (board[j]>max_x) {
				max_x = board[j];
				y = j;
			}
		}
		// границу листа и фона сверху
		if (i == 0) {
			pts[0] = cvPoint(board[0],0);
			n++;
		}
		pts[n] = cvPoint(max_x,y);
		n++;
		// границу листа и фона снизу
		if (i+step >= image->height-w_size)
			pts[n] = cvPoint(board[image->height-1],image->height-1);
	}

	// конвертируем изображение в RGB
	cvCvtColor( image, color_image, CV_GRAY2RGB );

	// Рисуем границу листа и фона c помощью линий (старый способ)
	//for (int i=0; i<step_count+1; i++) {
		//printf("i=%d\n",i);
		//CvPoint p1 = pts[i];
		//CvPoint p2 = pts[i+1];
		// Рисуем границу листа и фона
		//cvLine(new_image, p1, p2, red, 2, 0, 0);
	//}
	// Дорисовываем границу листа и фона снизу и сверху
	//cvLine(new_image, cvPoint(board[0],0), pts[0], red, 2, 0, 0);
	//cvLine(new_image, pts[step_count-1], cvPoint(board[image->height-1],image->height-1), red, 2, 0, 0);

	//Рисуем границу листа и фона (новый способ)

	LeafBorder l_b;
	//CvPoint* ptsArr[1] = pts;
	//int ptsN[1] = {step_count+2};
	//l_b.npts = ptsN;
	//l_b.pts = ptsArr;
	l_b.npts = step_count+2;
	l_b.pts = pts;

	cvPolyLine(color_image,&l_b.pts,&l_b.npts,1,0,RED,thickness,8,0);
	// Draw points of local minimum for demonstration aim
	//for (int i=0; i < l_b.npts; i++) {
	//	cvCircle(color_image,pts[i],2,GREEN,1,8,0);
	//}
	return l_b;
}

IplImage* rm_leaf (IplImage *color_image, int min_trichome) {
	cvFloodFill( color_image, cvPoint(color_image->width-1,color_image->height-1), WHITE,
			 cvScalarAll(100), // минимальная разность
			 cvScalarAll(100), // максимальная разность
			 NULL,
			 4,
			 NULL);
	// Удаляем все шумы меньше чем min_trichome
	for (int y=0; y<color_image->height; y++) {
		int red = 0;
		for (int x=color_image->width-1; x>0; x--) {

			int b = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3];
			int g = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3+2];
			if (r==255 && g==0 && b==0) {
			  //printf("red %dx%d\n", x,y );
			  red = 1;
			}
			else if (red == 1) {
				red = 0;
				int no_trichome = 0;
				for (int new_x = x; new_x>x-min_trichome; new_x--) {
					//printf ("new_X = %d\n",new_x);
					b = ((uchar*)(color_image->imageData + color_image->widthStep*y))[new_x*3];
					g = ((uchar*)(color_image->imageData + color_image->widthStep*y))[new_x*3+1];
					r = ((uchar*)(color_image->imageData + color_image->widthStep*y))[new_x*3+2];
					//printf ("(%d %d %d)\n",r,g,b);
					if (r==255 && g==255 && b==255) {
						no_trichome = 1;
					}
				}
				if (no_trichome == 1) {
					for (int new_x = x; new_x>x-min_trichome; new_x--) {
						//printf("delete pixel %dx%d\n", new_x,y );
						cvSet2D(color_image,y,new_x,WHITE);
					}
				}
			}
		}
	}
	// бинаризация изображения
	for (int y=0; y<color_image->height; y++) {
		for (int x=0; x<color_image->width; x++) {
			int b = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3];
			int g = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(color_image->imageData + color_image->widthStep*y))[x*3+2];
			if (r==255 && g==255 && b==255) {
				cvSet2D(color_image,y,x,BLACK);
				continue;
			}
			else if (r==255 && g==0 && b==0) {
				cvSet2D(color_image,y,x,BLACK);
			}
			else if (r > class3) {
				cvSet2D(color_image,y,x,BLACK);
			}
			else {
				cvSet2D(color_image,y,x,WHITE);
			}
		}
	}
	return color_image;
}

DetectedTrh detect(IplImage *image, LeafBorder *l_b, int angle, int min_trichome, int percent, int complex_thr_len) {
	//cvContourArea

	//Диаметр
	const int r = min_trichome;
	const int vsize = 2;

	IplImage* src=cvCreateImage(cvSize(image->width, image->height),IPL_DEPTH_8U,1);
	cvCvtColor( image, src, CV_BGR2GRAY );
	assert( src != 0 );

	IplImage* dst = cvCreateImage( cvGetSize(image), 8, 3 );
	IplImage* tmp = cvCreateImage( cvGetSize(image), 8, 3 );
	cvZero(dst);
	cvZero(tmp);
	assert( dst != 0 );
	assert( tmp != 0 );

	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* first_contour = NULL;

	cvFindContours(
			src,
			storage,
			&first_contour,
			sizeof(CvContour),
			CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_SIMPLE,
			cvPoint(0,0)
	);

	// ищем точки выпуклостей на контурах и проводим линию от этой точки до границы листа и фона

//	cvPolyLine(tmp,&(l_b->pts),&(l_b->npts),1,0,GRAY,2,8,0);
//    for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) {
//
//		cvDrawContours(
//			tmp,
//			c,
//			WHITE,
//			BLACK, // black
//			0, // Try different values of max_level, and see what happens
//			CV_FILLED, // 1 - not filled
//			8, //8
//			cvPoint(0,0)
//		);
//
//		int total = c->total;
//
//		CvPoint* p_old; p_old = (CvPoint*)malloc(1*sizeof(CvPoint)); p_old->x = 0; p_old->y = 0;
//		for( int i=0; i<total; ++i ) {
//
//			int i_m = i - vsize;
//			int i_p = i + vsize;
//			if (i_m<0)
//				i_m = total+(i-vsize);
//			if (i_p>total)
//				i_p = (i+vsize)-total;
//
//			CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, c, i );
//			CvPoint* p1 = CV_GET_SEQ_ELEM( CvPoint, c, i_m );
//			CvPoint* p2 = CV_GET_SEQ_ELEM( CvPoint, c, i_p );
//
//			if (ang(p,p1,p2) <= (angle*PI)/180 && on_image_border(p,r/2,tmp->width,tmp->height) == 0) {
//				if ( p_dist(p,p_old) > r ) {
//					// ограничиваем область
//					cvSetImageROI(tmp, cvRect(p->x-r/2,p->y-r/2,r,r));
//					// рисуем красную окружность толщиной 1 пиксель
//					cvCircle( tmp, cvPoint(r/2,r/2), r/2, RED, 1, 8, 0 );
//					// заливаем углы красным цветом
//
//					cvFloodFill( tmp, cvPoint(0,0), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
//					cvFloodFill( tmp, cvPoint(0,r-1), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
//					cvFloodFill( tmp, cvPoint(r-1,0), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
//					cvFloodFill( tmp, cvPoint(r-1,r-1), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
//
//					// считаем сколько точек белого и черного цвета
//					int w_n = 0;
//					int b_n = 0;
//					int b_p = 0;
//					for (int x = p->x-r/2; x<p->x+r; x++) {
//						for (int y = p->y-r/2; y<p->y+r; y++) {
//							int b = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3];
//							int g = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3+1];
//							int r = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3+2];
//							if (r==0 && g==0 && b==0) {
//								b_n++;
//							}
//							else if (r==255 && g==255 && b==255) {
//								w_n++;
//							}
//						}
//					}
//					if (b_n > 0)
//						b_p = (100*b_n)/(w_n+b_n);
//
//					//cvAddS(dst, cvScalar(200,0,0,0), dst, NULL);
//					cvResetImageROI(tmp);
//					if (b_p < percent) { // <- нашли выпуклую точку
//						int y = p->y;
//						for (int x = p->x; y<tmp->width; x++) {
//							int b = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3];
//							int g = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3+1];
//							int r = ((uchar*)(tmp->imageData + tmp->widthStep*y))[x*3+2];
//							if (r==0 && g==0 && b==0) {
//								break;
//							}
//							if (r==128 && g==128 && b==128) {
//								//cvLine(src, *p, cvPoint(x,y), BLACK, 3, 0, 0);
//								break;
//							}
//						}
//					}
//					p_old = p;
//				}
//
//			}
//		}
//    }

	//
	cvFindContours(
			src,
			storage,
			&first_contour,
			sizeof(CvContour),
			CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_SIMPLE,
			cvPoint(0,0)
	);

	//Рисуем границу листа и фона серым цветом
	cvPolyLine(dst,&(l_b->pts),&(l_b->npts),1,0,GRAY,2,8,0);

    int n = 0;
    Contur* conturs;
    conturs = (Contur*)malloc(1000*sizeof(Contur));

    for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) {

		cvDrawContours(
			dst,
			c,
			WHITE,
			BLACK, // black
			0, // Try different values of max_level, and see what happens
			CV_FILLED, // 1 - not filled
			8, //8
			cvPoint(0,0)
		);

		int total = c->total;

		Contur contur;
		//CvPoint head_pts[1000];
		//CvPoint foot_pts[1000];
		contur.head_pts_count = 0;
		contur.foot_pts_count = 0;

		//for (int vsize = 1; vsize <= 2; vsize++) {
		CvPoint* p_old; p_old = (CvPoint*)malloc(1*sizeof(CvPoint)); p_old->x = 0; p_old->y = 0;
		for( int i=0; i<total; ++i ) {

			int i_m = i - vsize;
			int i_p = i + vsize;
			if (i_m<0)
				i_m = total+(i-vsize);
			if (i_p>total)
				i_p = (i+vsize)-total;

			CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, c, i );
			CvPoint* p1 = CV_GET_SEQ_ELEM( CvPoint, c, i_m );
			CvPoint* p2 = CV_GET_SEQ_ELEM( CvPoint, c, i_p );

			if ((ang(p,p1,p2) <= (angle*PI)/180 && on_image_border(p,r/2,dst->width,dst->height) == 0)
					|| (close_to_border(dst,p,2) == 1)) {
				if (close_to_border(dst,p,r*2) == 1) {


/*
					if (close_to_border(dst,p,2) == 1) {
						if (contur.foot_pts_count > 0) {
							// Check min dist
							if (min_p_dist(p,&contur.foot_pts[0],contur.foot_pts_count) > r+2) {
								cvCircle( dst, *p, 1, BLUE, CV_FILLED, 8, 0 );
								contur.foot_pts[contur.foot_pts_count] = *p;
								contur.foot_pts_index[contur.foot_pts_count] = i;
								contur.foot_pts_count++;
							}
						}
						else {
*/

					if(close_to_border(dst,p,2) == 1 && near_with_black(dst,p) == 1) {
							cvCircle( dst, *p, 1, BLUE, CV_FILLED, 8, 0 );
							contur.foot_pts[contur.foot_pts_count] = *p;
							contur.foot_pts_index[contur.foot_pts_count] = i;
							contur.foot_pts_count++;
					}
/*
						}
					}
*/

				}
				else if ( p_dist(p,p_old) > r ) {
					// ограничиваем область
					cvSetImageROI(dst, cvRect(p->x-r/2,p->y-r/2,r,r));
					// рисуем красную окружность толщиной 1 пиксель
					cvCircle( dst, cvPoint(r/2,r/2), r/2, RED, 1, 8, 0 );
					// заливаем углы красным цветом

					cvFloodFill( dst, cvPoint(0,0), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
					cvFloodFill( dst, cvPoint(0,r-1), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
					cvFloodFill( dst, cvPoint(r-1,0), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
					cvFloodFill( dst, cvPoint(r-1,r-1), RED, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);

					// считаем сколько точек белого и черного цвета
					int w_n = 0;
					int b_n = 0;
					int b_p = 0;
					for (int x = p->x-r/2; x<p->x+r; x++) {
						for (int y = p->y-r/2; y<p->y+r; y++) {
							int b = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3];
							int g = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3+1];
							int r = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3+2];
							if (r==0 && g==0 && b==0) {
								b_n++;
							}
							else if (r==255 && g==255 && b==255) {
								w_n++;
							}
						}
					}
					if (b_n > 0)
						b_p = (100*b_n)/(w_n+b_n);

					//cvAddS(dst, cvScalar(200,0,0,0), dst, NULL);
					cvResetImageROI(dst);
					if (b_p > percent) {
						if (contur.head_pts_count > 0) {
							// Check min dist
							if (min_p_dist(p,&contur.head_pts[0],contur.head_pts_count) > r/2) {
								cvCircle( dst, *p, 2, GREEN, CV_FILLED, 8, 0 );
								contur.head_pts[contur.head_pts_count] = *p;
								contur.head_pts_index[contur.head_pts_count] = i;
								contur.thr_length[contur.head_pts_count] = 0;
								contur.head_pts_count++;
							}
						}
						else {
							cvCircle( dst, *p, 2, GREEN, CV_FILLED, 8, 0 );
							contur.head_pts[contur.head_pts_count] = *p;
							contur.head_pts_index[contur.head_pts_count] = i;
							contur.thr_length[contur.head_pts_count] = 0;
							contur.head_pts_count++;
						}
					}
					p_old = p;
				}
				//else {
					//cvCircle( dst, *p, 1, CV_RGB( 0, 255, 0 ), 1, 8, 0 );
				//}
			}
		}
		//}
		if (contur.foot_pts_count == 2 && contur.head_pts_count == 1) { // <- simple thr


//			cvCircle( dst, cvPoint((int) (contur.foot_pts[0].x + contur.foot_pts[1].x) / 2, (int) (contur.foot_pts[0].y + contur.foot_pts[1].y) / 2), 5, GREEN, 1, 8, 0 );

			CvMemStorage* cstorage = cvCreateMemStorage(0);
			CvSeq* Centerline = cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),cstorage);
//			CvSeq* c_new =	cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),cstorage);



//			int *index = recalc_simple_contur(c,c_new,contur.foot_pts,contur.foot_pts_index,contur.head_pts_index[0]);
//			int head_pount_index = index[0];
//			int foot_pount_index = index[1];

			CvSeq* OrigBoundA=cvSeqSlice(c,cvSlice(contur.head_pts_index[0],contur.foot_pts_index[0]),cstorage,1);
			CvSeq* OrigBoundB=cvSeqSlice(c,cvSlice(contur.foot_pts_index[1],contur.head_pts_index[0]),cstorage,1);
			//CvSeq* OrigBoundA=cvSeqSlice(c_new,cvSlice(head_pount_index,foot_pount_index),cstorage,1);
			//CvSeq* OrigBoundB=cvSeqSlice(c_new,cvSlice(foot_pount_index,head_pount_index),cstorage,1);
			cvSeqInvert(OrigBoundB);

			/*** Resample One of the Two Boundaries so that both are the same length ***/

			//Create sequences to store the Normalized Boundaries
			CvSeq* NBoundA=	cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),cstorage);
			CvSeq* NBoundB=cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),cstorage);

			//printf("A=%d B=%d\n",OrigBoundA->total , OrigBoundB->total);
			if (OrigBoundA->total > 1 &&  OrigBoundB->total > 1) {
				//Resample L&R boundary to have the same number of points as min(L,R)
				if (OrigBoundA->total > OrigBoundB->total){
					resampleSeq(OrigBoundA,NBoundA,OrigBoundB->total );
					NBoundB=OrigBoundB;
				}
				else {
					resampleSeq(OrigBoundB,NBoundB,OrigBoundA->total );
					NBoundA=OrigBoundA;
				}
				//Now both NBoundA and NBoundB are the same length.
				cvClearSeq(Centerline);

				CvPoint* SideA;
				CvPoint* SideB;
				CvPoint MidPt;
				CvSeqReader readerA;
				CvSeqReader readerB;
				cvStartReadSeq(NBoundA, &readerA, 0);
				cvStartReadSeq(NBoundB, &readerB, 0);
				int i = 0;
				do {
					SideA = (CvPoint*) readerA.ptr;
					SideB = (CvPoint*) readerB.ptr;
					MidPt = cvPoint((int) (SideA->x + SideB->x) / 2, (int) (SideA->y
							+ SideB->y) / 2);
					cvLine(dst,*SideA,*SideB,GRAY,1,8,0);
					cvCircle(dst,MidPt,1,RED,1,8,0);
					CV_NEXT_SEQ_ELEM(sizeof(CvPoint),readerA);
					CV_NEXT_SEQ_ELEM(sizeof(CvPoint),readerB);
					i++;

				} while (i < NBoundA->total);


				FindCenterline(NBoundA,NBoundB,Centerline);
				CvSeqReader readerC;
				cvStartReadSeq(Centerline, &readerC, 0);
				CvPoint* CenterPoint;
				CvPoint CenterPointOld;
				int j = 0;
				double length = 0;
				do {
					CenterPoint = (CvPoint*) readerC.ptr;
					if (j != 0) {
						length += p_dist(&CenterPointOld,CenterPoint);
						cvLine(dst,CenterPointOld,*CenterPoint,BLUE,1,8,0);
					}
					CenterPointOld = *CenterPoint;
					//Push the pointers along the boundaries in opposite directions
					CV_NEXT_SEQ_ELEM(sizeof(CvPoint),readerC);
					j++;

				} while (j < Centerline->total);
				CvPoint foot_center_pt = cvPoint((int) (contur.foot_pts[0].x + contur.foot_pts[1].x) / 2, (int) (contur.foot_pts[0].y + contur.foot_pts[1].y) / 2);
				length += p_dist(&CenterPointOld,&foot_center_pt);
				contur.thr_length[0] = length;
				cvLine(dst,CenterPointOld,foot_center_pt,BLUE,1,8,0);
			}
			else {
				CvPoint foot_center_pt = cvPoint((int) (contur.foot_pts[0].x + contur.foot_pts[1].x) / 2, (int) (contur.foot_pts[0].y + contur.foot_pts[1].y) / 2);
				CvPoint head_pt = contur.head_pts[0];
				contur.thr_length[0] = p_dist(&foot_center_pt,&head_pt);
				cvLine(dst,head_pt,foot_center_pt,BLUE,1,8,0);
			}
		}
		else if (contur.head_pts_count > 0 && contur.foot_pts_count > 1 && complex_thr_len == 1) {
			for (int h = 0; h < contur.head_pts_count; h++) {
				CvPoint head_pt = contur.head_pts[h];
				double min_len = 10000;
				for (int f1 = 0; f1 < contur.foot_pts_count; f1++) {
					for (int f2 = 0; f2 < contur.foot_pts_count; f2++) {
						if (f1 != f2) {
							CvPoint foot_center_pt = cvPoint((int) (contur.foot_pts[f1].x + contur.foot_pts[f2].x) / 2, (int) (contur.foot_pts[f1].y + contur.foot_pts[f2].y) / 2);
							double len = p_dist(&head_pt,&foot_center_pt);
							//cvLine(dst,head_pt,foot_center_pt,BLUE,1,8,0);
							if (len < min_len) min_len = len;
						}
					}
				}
				contur.thr_length[h] = min_len;
			}
//			CvRect rect = cvBoundingRect(c,0);
///*
//			cvLine(dst,cvPoint(rect.x,rect.y),cvPoint(rect.x+rect.width,rect.y),BLUE,1,8,0);
//			cvLine(dst,cvPoint(rect.x,rect.y),cvPoint(rect.x,rect.y+rect.height),BLUE,1,8,0);
//			cvLine(dst,cvPoint(rect.x+rect.width,rect.y+rect.height),cvPoint(rect.x+rect.width,rect.y),BLUE,1,8,0);
//			cvLine(dst,cvPoint(rect.x+rect.width,rect.y+rect.height),cvPoint(rect.x,rect.y+rect.height),BLUE,1,8,0);
//*/
//			double dist_array[rect.x][rect.y];
//			double max_dist = 0;
//			for (int y=rect.y; y<rect.y+rect.height; y++) {
//				for (int x=rect.x; x<rect.x+rect.width; x++) {
//					double dist = cvPointPolygonTest(c,cvPoint2D32f(x,y),1);
//					dist_array[x-rect.x][y-rect.y] = dist;
//					if (dist>max_dist) max_dist = dist;
//				}
//			}
//
//			// нормирование
//			// scale = (B - A) / (Xmax - Xmin) масштабный множитель
//			// X'i = A + (Xi - Xmin) * scale, для всех i = 1.. n
//			int a = 1;   // диапазон интенсивности цвета
//			int b = 254; // от a до b
//			int GRAPHSIZE = (rect.width * rect.height)+1;
//			CvPoint pts[GRAPHSIZE];
//			int pts_index[rect.width][rect.height];
//			double scale = (b - a) / (max_dist - 0);
//			int i = 1; // Id вершин в графе
//			for (int x=0; x<rect.width; x++) {
//				for (int y=0; y<rect.height; y++) {
//					pts[i] = cvPoint(x,y);
//					pts_index[x][y] = i;
//					double d = dist_array[x][y];
//					if (d >= 0) { // если точка находится в контуре
//						dist_array[x][y] = b-(a+(d-0)*scale);
//						cvSet2D(dst,y+rect.y,x+rect.x,cvScalar((int)dist_array[x][y],(int)dist_array[x][y],(int)dist_array[x][y],0));
//					}
//					else { // если точка находится вне контура или на границе контура
//						dist_array[x][y] = 255;
//						//cvSet2D(dst,y+rect.y,x+rect.x,cvScalar(dist_array[x][y],dist_array[x][y],dist_array[x][y],0));
//					}
//					i++;
//				}
//			}
//
//			int **dist = NULL;
//			dist = (int**)malloc( sizeof(int*)*GRAPHSIZE );
//			for ( int i= 0; i<GRAPHSIZE; i++ )
//				dist[i]= (int*)malloc( sizeof(int)*GRAPHSIZE );
//
//			for (int i = 0; i < GRAPHSIZE; i++)
//				for (int j = 0; j < GRAPHSIZE; j++)
//					dist[i][j] = 0;
//
//			for (int x=0; x<rect.width; x++) {
//				for (int y=0; y<rect.height; y++) {
//					int w = dist_array[x][y];
//					if (w == 255) continue;
///*
//					if (x+1 < rect.width && dist_array[x+1][y] <= w)  dist[pts_index[x][y]][pts_index[x+1][y]] = (int)(dist_array[x+1][y]+w)/2;
//					if (x-1 >= 0 && dist_array[x-1][y] <= w)          dist[pts_index[x][y]][pts_index[x-1][y]] = (int)(dist_array[x-1][y]+w)/2;
//					if (y+1 < rect.height && dist_array[x][y+1] <= w) dist[pts_index[x][y]][pts_index[x][y+1]] = (int)(dist_array[x][y+1]+w)/2;
//					if (y-1 >= 0 && dist_array[x][y-1] <= w)          dist[pts_index[x][y]][pts_index[x][y-1]] = (int)(dist_array[x][y-1]+w)/2;
//					if (x-1 >= 0 && y-1 >=0 && dist_array[x-1][y-1] <= w)                   dist[pts_index[x][y]][pts_index[x-1][y-1]] = (int)(dist_array[x-1][y-1]+w)/2;
//					if (x+1 < rect.width && y+1 < rect.height && dist_array[x+1][y+1] <= w) dist[pts_index[x][y]][pts_index[x+1][y+1]] = (int)(dist_array[x+1][y+1]+w)/2;
//					if (x+1 < rect.width && y-1 >= 0 && dist_array[x+1][y-1] <= w)          dist[pts_index[x][y]][pts_index[x+1][y-1]] = (int)(dist_array[x+1][y-1]+w)/2;
//					if (x-1 >= 0 && y+1 < rect.height && dist_array[x-1][y+1] <= w)         dist[pts_index[x][y]][pts_index[x-1][y+1]] = (int)(dist_array[x-1][y+1]+w)/2;
//*/
//					if (x+1 < rect.width)  dist[pts_index[x][y]][pts_index[x+1][y]] = (int)(dist_array[x+1][y]+w)/2;
//					if (x-1 >= 0)          dist[pts_index[x][y]][pts_index[x-1][y]] = (int)(dist_array[x-1][y]+w)/2;
//					if (y+1 < rect.height) dist[pts_index[x][y]][pts_index[x][y+1]] = (int)(dist_array[x][y+1]+w)/2;
//					if (y-1 >= 0)          dist[pts_index[x][y]][pts_index[x][y-1]] = (int)(dist_array[x][y-1]+w)/2;
//					if (x-1 >= 0 && y-1 >=0)                   dist[pts_index[x][y]][pts_index[x-1][y-1]] = (int)(dist_array[x-1][y-1]+w)/2;
//					if (x+1 < rect.width && y+1 < rect.height) dist[pts_index[x][y]][pts_index[x+1][y+1]] = (int)(dist_array[x+1][y+1]+w)/2;
//					if (x+1 < rect.width && y-1 >= 0)          dist[pts_index[x][y]][pts_index[x+1][y-1]] = (int)(dist_array[x+1][y-1]+w)/2;
//					if (x-1 >= 0 && y+1 < rect.height)         dist[pts_index[x][y]][pts_index[x-1][y+1]] = (int)(dist_array[x-1][y+1]+w)/2;
//				}
//			}
//
///*			for (int i = 0; i < GRAPHSIZE; i++) {
//				for (int j = 0; j < GRAPHSIZE; j++) {
//					fprintf(stderr,"%d\t",dist[i][j]);
//				}
//				fprintf(stderr,"\n");
//			}*/
//
//			int *d = NULL;
//			d = (int*)malloc( sizeof(int)*GRAPHSIZE );
//
//			int *prev = NULL;
//			prev = (int*)malloc( sizeof(int)*GRAPHSIZE );
//
//			//for (int thr_pt = 0; thr_pt < contur.head_pts_count; thr_pt++)
//			//	contur.thr_length[thr_pt] = 10000;
//
//			for (int foot_pt = 0; foot_pt < contur.foot_pts_count; foot_pt++) {
//				if (foot_pt < contur.foot_pts_count) {
//					CvPoint foot_center = cvPoint((int) (contur.foot_pts[foot_pt].x + contur.foot_pts[foot_pt+1].x) / 2, (int) (contur.foot_pts[foot_pt].y + contur.foot_pts[foot_pt+1].y) / 2);
//					//cvCircle( dst, foot_center, 1, RED, 1, 8, 0 );
//					foot_pt++;
//
//					dijkstra(pts_index[foot_center.x-rect.x][foot_center.y-rect.y], GRAPHSIZE, dist, d, prev);
//					for (int thr_pt = 0; thr_pt < contur.head_pts_count; thr_pt++) {
//						double length = 0;
//						int node = pts_index[contur.head_pts[thr_pt].x-rect.x][contur.head_pts[thr_pt].y-rect.y];
//						//if (d[node] == GRAPHSIZE*GRAPHSIZE) exit(0);
//						if (d[node] != GRAPHSIZE*GRAPHSIZE) {
//							int old_node,old_node1;
//							while (1) {
//							//for (int b=0; b<=10000; b++) {
//								old_node1 = old_node;
//								old_node = node;
//								node = prev[node];
//								if (node == old_node1) { // <- у нас образовался ЦИКЛ :(
//									length = 0;
//									break;
//								}
//								if (node == -1) break;
//								CvPoint p1 = cvPoint(pts[old_node].x+rect.x,pts[old_node].y+rect.y);
//								CvPoint p2 = cvPoint(pts[node].x+rect.x,pts[node].y+rect.y);
//								length += p_dist(&p1,&p2);
//								//if (thr_pt == 2) printf("%d x %d len=%f p_dist=%f\n", contur.head_pts[thr_pt].x, contur.head_pts[thr_pt].y , length ,p_dist(&p1,&p2));
//								//printf("inf=%d old=%d %d cur=%d %d\n",GRAPHSIZE*GRAPHSIZE,d[old_node],old_node,d[node],node);
//								cvLine( dst, p1, p2, BLUE, 1, 8, 0 );
//							}
//						}
//						if (length != 0)
//							contur.thr_length[thr_pt] = length;
//						//printf("after len is %f\n",contur.thr_length[thr_pt]);
//					}
//				}
//			}
//
//			//CvPoint foot_center = cvPoint((int) (contur.foot_pts[0].x + contur.foot_pts[contur.foot_pts_count-1].x) / 2, (int) (contur.foot_pts[0].y + contur.foot_pts[contur.foot_pts_count-1].y) / 2);
//
//			//dijkstra(pts_index[foot_center.x-rect.x][foot_center.y-rect.y], GRAPHSIZE, dist, d, prev);
//
///*			for (int i=0; i<contur.head_pts_count; i++) {
//				int node = pts_index[contur.head_pts[i].x-rect.x][contur.head_pts[i].y-rect.y];
//				int old_node;
//				while (1) {
//					old_node = node;
//					node = prev[node];
//					if (node == -1) break;
//					cvLine( dst, cvPoint(pts[old_node].x+rect.x,pts[old_node].y+rect.y), cvPoint(pts[node].x+rect.x,pts[node].y+rect.y), BLUE, 1, 8, 0 );
//					//printf("%d\n",node);
//				}
//			}*/
//
//			// Чистим память
//			for ( int i= 0; i<GRAPHSIZE; i++ ) {
//			    free( dist[i] ); dist[i]= NULL;
//			}
//			free(d); d = NULL;
//			free(prev); prev = NULL;
//
//			//printf("foot index=%d\n",pts_index[foot_center.x-rect.x][foot_center.y-rect.y]);
//			//printf("head index=%d\n",pts_index[contur.head_pts[0].x-rect.x][contur.head_pts[0].y-rect.y]);
//
//			//printPath(pts_index[contur.head_pts[0].x-rect.x][contur.head_pts[0].y-rect.y],prev);
//
///*
//			for (int y=rect.y; y<rect.y+rect.height; y++) {
//				for (int x=rect.x; x<rect.x+rect.width; x++) {
//					int b = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3];
//					int g = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3+1];
//					int r = ((uchar*)(dst->imageData + dst->widthStep*y))[x*3+2];
//					if (r==255 && g==255 && b==255) {
//						double dist = cvPointPolygonTest(c,cvPoint2D32f(x,y),1);
//						//if (dist <= 0) {continue;}
//						int norm_dist = 255-(a+(dist-0)*scale);
//						printf("norm dist=%d\n",norm_dist);
//
//							cvSet2D(dst,y,x,cvScalar(norm_dist,norm_dist,norm_dist,0));
//
//					}
//				}
//			}
//*/
		}
		contur.area = fabs(cvContourArea(c,CV_WHOLE_SEQ));
		conturs[n] = contur;
		n++;
    }
    cvReleaseImage(&src);

    DetectedTrh out;
    out.img = dst;
    out.conturs = conturs;
    out.contur_count = n;

    return out;
}

void printPath(int dest, int *prev) {
	if (prev[dest] != -1)
		printPath(prev[dest],prev);
	printf("%d ", dest);
}

Precision cmp(IplImage* ref_image, DetectedTrh *d_t) {

	Precision p;

	p.tp = 0; // - число правильно предсказанных вершин,
	p.fp = 0; // - число предсказанных вершин, которые не совпали с зелеными точками.
	p.fn = 0; // - число зеленых точек, для которых не были предсказаны вершины.

	// меняем цвет красных точек на зелёные
	for (int y=0; y<ref_image->height; y++) {
		for (int x=0; x<ref_image->width; x++) {
			int b = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3];
			int g = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+2];
			if (r==255 && g==0 && b==0) {
				cvSet2D(ref_image,y,x,GREEN);
			}
		}
	}

	// закрашиваем всё кроме зелёных точек чёрным
	for (int y=0; y<ref_image->height; y++) {
		for (int x=0; x<ref_image->width; x++) {
			int b = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3];
			int g = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+2];
			if (r==0 && g==255 && b==0) {
				continue;
			}
			else  {
				cvSet2D(ref_image,y,x,BLACK);
			}
		}
	}

	// бежим циклом по предсказанным выршинам трихом и в случает если трихома предсказана верно тоже закрашиваем её чёрным
	for (int j=0; j<d_t->contur_count; j++) {
    	Contur contur = d_t->conturs[j];
		for (int i = 0; i < contur.head_pts_count; i++) {
			int x = contur.head_pts[i].x;
			int y = contur.head_pts[i].y;
			int b = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3];
			int g = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+2];
			if (r==0 && g==255 && b==0) {
				p.tp++;
				cvFloodFill( ref_image, cvPoint(x,y), BLACK, cvScalarAll(100), cvScalarAll(100), NULL, 4, NULL);
			}
			else {
				p.fp++;
			}
		}
    }

	// конвертируем изображение в чёрнобелое, что бы можно было найти там контуры
	IplImage* ref_image_wb=cvCreateImage(cvSize(ref_image->width, ref_image->height),IPL_DEPTH_8U,1);
	cvCvtColor( ref_image, ref_image_wb, CV_BGR2GRAY );

	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* first_contour = NULL;

	cvFindContours(
			ref_image_wb,
			storage,
			&first_contour,
			sizeof(CvContour),
			CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_SIMPLE,
			cvPoint(0,0)
	);
	// количество найденых контуров будет соответствовать количеству трихом для которых не были предсказаны вершины
	for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) {
		p.fn++;
	}
	//printf("tp=%d fp=%d fn=%d\n", p.tp, p.fp, p.fn);
	if ((p.tp+p.fp) != 0) {
		p.precision = (double)p.tp/((double)p.tp+(double)p.fp);
	}
	else {
		p.precision = 0;
	}
	if ((p.tp+p.fn) != 0) {
		p.recall = (double)p.tp/((double)p.tp+(double)p.fn);    //recall = TP/(TP+FN)
	}
	else {
		p.recall = 0;
	}
	if ((p.precision+p.recall) != 0) {
		p.f1_score = 2*(p.precision*p.recall)/(p.precision+p.recall);
	}
	else {
		p.f1_score = 0;
	}

	return p;
}

double cmp_border(IplImage *image, IplImage *ref_image) {
	double err = 0;
	for (int y=0; y<ref_image->height; y++) {
		for (int x=0; x<ref_image->width; x++) {
			int b = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3];
			int g = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+1];
			int r = ((uchar*)(ref_image->imageData + ref_image->widthStep*y))[x*3+2];
			if (r==255 && g==0 && b==0)
				cvSet2D(image,y,x,GREEN);
		}
	}
	for (int y=0; y<image->height; y++) {
		int f1 = 0;
		int f2 = 0;
		for (int x=0; x<image->width; x++) {
			int b = ((uchar*)(image->imageData + image->widthStep*y))[x*3];
			int g = ((uchar*)(image->imageData + image->widthStep*y))[x*3+1];
			int r = ((uchar*)(image->imageData + image->widthStep*y))[x*3+2];
			if ( (r==255 && g==0 && b==0) || (r==0 && g==255 && b==0) ) {
				if (f1 == 0) {
					f1 = x;
				}
				else {
					f2 = x;
					break;
				}
			}
		}
		if (f2 != 0)
			err += abs(f1 - f2);
	}
	return err/image->height;
}

void point_distr(IplImage* image,int step,int w_size, char* orientation) {
	//закрашиваем всё белое черным
	int img_step       = image->widthStep/sizeof(uchar);
	uchar* img_data    = (uchar *)image->imageData;
	for (int y = 0; y<image->height; y++) {
		for (int x = 0; x<image->width; x++) {
			uchar c = img_data[y*img_step+x];
			if (c == 255) {
				cvSet2D(image,y,x,BLACK);
			}
		}
	}

	if (strcmp(orientation, "horizontal") == 0) {
		for (int i=0; i<=image->height-w_size; i+=step) {
			cvSetImageROI(image, cvRect(i,0,w_size,image->height));
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* first_contour = NULL;
			cvFindContours(
					image,
					storage,
					&first_contour,
					sizeof(CvContour),
					CV_RETR_EXTERNAL,
					CV_CHAIN_APPROX_SIMPLE,
					cvPoint(0,0)
			);
			// количество найденых контуров будет соответствовать количеству точек в ROI
			int n = 0;
			for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) {
				n++;
			}
			cvResetImageROI(image);
			printf("%d\n",n);
		}

	}
	else if (strcmp(orientation, "vertical") == 0) {
		for (int i=0; i<=image->width-w_size; i+=step) {
			cvSetImageROI(image, cvRect(0,i,image->width,w_size));
			CvMemStorage* storage = cvCreateMemStorage(0);
			CvSeq* first_contour = NULL;
			cvFindContours(
					image,
					storage,
					&first_contour,
					sizeof(CvContour),
					CV_RETR_EXTERNAL,
					CV_CHAIN_APPROX_SIMPLE,
					cvPoint(0,0)
			);
			// количество найденых контуров будет соответствовать количеству точек в ROI
			int n = 0;
			for( CvSeq* c=first_contour; c!=NULL; c=c->h_next ) {
				n++;
			}
			cvResetImageROI(image);
			printf("%d\n",n);
		}
	}
}

int isComplex(Contur *c) {
	if (c->foot_pts_count > 2 || c->head_pts_count>1) {
		return 1;
	}
	return 0;
}

int trhCount(Contur *c) {
	int f = c->foot_pts_count;
	int h = c->head_pts_count;
	if (f%2 != 0)
		f++;
	f/=2;
	if (h==0 && f==1) return 0;
	if (f>h) return f;
	return h;
}
