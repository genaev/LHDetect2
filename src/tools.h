/*
 * tools.h
 *
 *  Created on: 13.02.2011
 *      Author: mag
 */

#ifndef TOOLS_H_
#define TOOLS_H_

double p_dist(CvPoint *p1, CvPoint *p2);

double  min_p_dist(CvPoint *p, CvPoint *pts, int c);

int close_to_border (IplImage *img, CvPoint *p, int dist);

int near_with_black (IplImage *img, CvPoint *p);

double ang (CvPoint *p, CvPoint *p1, CvPoint *p2);

int on_image_border(CvPoint *p, int r, int width, int height);

int* recalc_simple_contur(CvSeq* src, CvSeq* dst, CvPoint foot_pts[], int foot_pts_index[], int head_pts_index);

void dijkstra(int s, int GRAPHSIZE, int **dist, int *d, int *prev);

#endif /* TOOLS_H_ */
