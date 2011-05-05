/*
 * tools.c
 *
 *  Created on: 13.02.2011
 *      Author: mag
 */
#include <stdio.h>
#include "cv.h" /* required to use OpenCV */
//#include "highgui.h" /* required to use OpenCV's highgui */
#include "tools.h"

double p_dist(CvPoint *p1, CvPoint *p2) {
	return sqrt ( pow(p2->x-p1->x,2) + pow(p2->y-p1->y,2) ) ;
}

double  min_p_dist(CvPoint *p, CvPoint *pts, int c) {
	double min_dst = p_dist(p,pts);
	for (int i = 1; i < c; i++) {
		double dst = p_dist(p,pts+i);
		if (dst < min_dst)
			min_dst = dst;
	}
	return min_dst;
}

int close_to_border (IplImage* img, CvPoint *p, int dist) {
	int x = p->x;
	int y = p->y;
	for (int new_x = x; new_x<x+dist; new_x++) {
		int b = ((uchar*)(img->imageData + img->widthStep*y))[new_x*3];
		int g = ((uchar*)(img->imageData + img->widthStep*y))[new_x*3+1];
		int r = ((uchar*)(img->imageData + img->widthStep*y))[new_x*3+2];
		if (r==128 && g==128 && b==128)
			return 1;
	}
	return 0;
}

double ang (CvPoint *p, CvPoint *p1, CvPoint *p2) {
	double x1 = p1->x - p->x; double y1 = p1->y - p->y;
	double x2 = p2->x - p->x; double y2 = p2->y - p->y;
	//Calculate vector length
	double l1 =  sqrt( x1*x1 + y1*y1 );
	double l2 =  sqrt( x2*x2 + y2*y2 );

	//Normalize
	x1 = x1/l1; y1 = y1/l1;
	x2 = x2/l2; y2 = y2/l2;

	//Scalar product
	double s = x1*x2 + y1*y2;
	//Angle
	double ang = acos(s);

	//if (ang > PI)
	//	ang = 2*PI - ang;

	return ang;
}

int on_image_border(CvPoint *p, int r, int width, int height) {
	if (p->x+r > width || p->x-r < 0 || p->y+r > height || p->y-r < 0) {
//		printf("%d x %d\n",p->x,p->y);
		return 1;
	}
	return 0;
}

int near_with_black (IplImage *img, CvPoint *p) {
	int x = p->x;
	int y = p->y;
	//cvCircle( img, *p, 5, CV_RGB(255,255,0), CV_FILLED, 8, 0 );
	CvScalar s;


	s=cvGet2D(img,y+1,x-1);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;
	s=cvGet2D(img,y+1,x);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;
	s=cvGet2D(img,y+1,x+1);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;

	s=cvGet2D(img,y-1,x-1);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;
	s=cvGet2D(img,y-1,x);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;
	s=cvGet2D(img,y-1,x+1);
	if (s.val[0]==0 && s.val[0]==0 && s.val[0]==0) return 1;

	return 0;
}

int* recalc_simple_contur(CvSeq* src, CvSeq* dst, CvPoint foot_pts[], int foot_pts_index[], int head_pts_index) {
	CvSeqReader reader;
	CvSeqWriter writer;
	cvStartReadSeq(src, &reader, 0);
	cvStartAppendToSeq(dst, &writer);
	CvPoint *Pt;
	CvPoint MidPt;
	int* index = malloc(2*sizeof(int));
	int i = 0;
	int j = 0;
	int ignore = 0;
	printf("%d %d\n",foot_pts_index[0],foot_pts_index[1]);
	do {
		Pt = (CvPoint*) reader.ptr;

		if (i == foot_pts_index[1]) {
			ignore = 0;
			puts("ingnore off");
		}

		if (ignore == 0) {
			CV_WRITE_SEQ_ELEM( *Pt, writer);
			j++;
			if (i == head_pts_index)
				index[0] = j;
			puts("write");
		}
		else {puts("ingnore");}

		if (i == foot_pts_index[0]) {
			//Find the midpoint, and write it to the sequence (auto-increments)
			MidPt = cvPoint((int) (foot_pts[0].x + foot_pts[1].x) / 2, (int) (foot_pts[0].y + foot_pts[1].y) / 2);
			CV_WRITE_SEQ_ELEM( MidPt, writer);
			j++;
			index[1] = j;
			ignore = 1;
			puts("ingnore on");
		}

		//Push the pointers along the boundaries in opposite directions
		CV_NEXT_SEQ_ELEM(sizeof(CvPoint),reader);
		i++;

	} while (i < src->total);
	cvEndWriteSeq(&writer);
	printf("befor: %d, now:%d \n",src->total, j);
	return index;
}

void dijkstra(int s, int GRAPHSIZE, int **dist, int *d, int *prev) {

	int n = GRAPHSIZE-1;
	int i, k, mini;
	int *visited = NULL;
	int inf = GRAPHSIZE*GRAPHSIZE;
	visited = (int*)malloc( sizeof(int)*GRAPHSIZE );

	for (i = 1; i <= n; ++i) {
		d[i] = inf;
		prev[i] = -1; /* no path has yet been found to i */
		visited[i] = 0; /* the i-th element has not yet been visited */
	}

	d[s] = 0;

	for (k = 1; k <= n; ++k) {
		mini = -1;
		for (i = 1; i <= n; ++i)
			if (!visited[i] && ((mini == -1) || (d[i] < d[mini])))
				mini = i;

		visited[mini] = 1;

		for (i = 1; i <= n; ++i)
			if (dist[mini][i])
				if (d[mini] + dist[mini][i] < d[i]) {
					d[i] = d[mini] + dist[mini][i];
					prev[i] = mini;
				}
	}

/*
	for (i = 0; i < n; i++) {
		d[i] = inf;
		prev[i] = -1;  //no path has yet been found to i
		visited[i] = 0;  //the i-th element has not yet been visited
	}

	d[s] = 0;

	for (k = 0; k < n; k++) {
		mini = -1;
		for (i = 0; i < n; i++)
			if (!visited[i] && ((mini == -1) || (d[i] < d[mini])))
				mini = i;

		visited[mini] = 1;

		for (i = 0; i < n; i++)
			if (dist[mini][i])
				if (d[mini] + dist[mini][i] < d[i]) {
					d[i] = d[mini] + dist[mini][i];
					prev[i] = mini;
				}
	}
*/

}
