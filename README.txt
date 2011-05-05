LHDetect2 – program for extracting quantitative characteristics of wheat
leaf hairiness using image processing technique.

Requirements
LHDetect2 requires OpenCV2 (version 2.0 or higher) library
http://sourceforge.net/projects/opencvlibrary/

Building and install
LHDetect2 was developed and testing only in Linux (Ubuntu and CenOS) operating system only.
OpenCV2 headers should be installed to /usr/include/opencv directory.
For building release version
$ cd Release
$ make

Usage
Run lhdetect2 without options for getting help.
Typicale usage is
$ ./lhdetect2 --cmd detect -i ../test/1.png -l
