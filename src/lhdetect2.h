/*
 * lhdetect2.h
 *
 *  Created on: 13.02.2011
 *      Author: mag
 */

#ifndef LHDETECT2_H_
#define LHDETECT2_H_

typedef struct {
	CvPoint* pts;
	int npts;
} LeafBorder;

typedef struct {
	int tp; // - число правильно предсказанных вершин,
	int fp; // - число предсказанных вершин, которые не совпали с зелеными точками.
	int fn; // - число зеленых точек, для которых не были предсказаны вершины.
	double precision;
	double recall;
	double f1_score;
} Precision;

typedef struct {

	double area;
	CvPoint head_pts[1000];
	int head_pts_index[1000];
	int head_pts_count;

	CvPoint foot_pts[1000];
	int foot_pts_index[1000];
	int foot_pts_count;
	double thr_length[1000];

} Contur;

typedef struct {

	Contur* conturs;
	int contur_count;
	IplImage* img;

} DetectedTrh;

void help();

void img_info (IplImage* image);

void mat (IplImage* image, float smooth);

IplImage* rm_noise (IplImage* image, float smooth);

IplImage* statcolor (IplImage* image, IplImage *color_image, float smooth);

LeafBorder draw_border (IplImage* image, IplImage *color_image, int step, int w_size, int thickness);

IplImage* rm_leaf (IplImage *color_image, int min_trichome);

DetectedTrh detect(IplImage *image, LeafBorder *l_b, int angle, int min_trichome, int percent, int complex_thr_len);

Precision cmp(IplImage *ref_image, DetectedTrh *d_t);

double cmp_border(IplImage *image, IplImage *ref_image);

void point_distr(IplImage* image,int step,int w_size, char* orientation);

int isComplex(Contur *c);

int trhCount(Contur *c);

void printPath(int dest,int *prev);


#endif /* LHDETECT2_H_ */
