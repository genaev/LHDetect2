/*
 * params.h
 *
 *  Created on: 13.02.2011
 *      Author: mag
 */

#ifndef PARAMS_H_
#define PARAMS_H_

#define PI 2*acos(0.0)

//#define MAX(a, b) ((a > b) ? (a) : (b))

// some colors
#define WHITE CV_RGB( 255, 255, 255 )
#define BLACK CV_RGB( 0, 0, 0 )
#define GRAY  CV_RGB( 128, 128, 128 )
#define RED   CV_RGB( 255, 0, 0 )
#define GREEN CV_RGB( 0, 255, 0 )
#define BLUE  CV_RGB( 0, 0, 255 )
#define LBLUE CV_RGB( 0, 255, 255 ) // LIGHT BLUE

const static int class1 = 50;  // from 0 to 50    RGB(0,255,0) green
const static int class2 = 205; // from 50 to 205  RGB(0,255,255) light blue
const static int class3 = 228; // from 205 to 228 RGB(0,0,255) blue
const static int class4 = 240; // from 228 to 240 RGB(255,0,0) red
const static int class5 = 255; // from 240 to 255 RGB(0,0,0) black

#endif /* PARAMS_H_ */
