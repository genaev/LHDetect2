LHDetect2 – program for extracting quantitative characteristics of wheat
leaf hairiness using image processing technique.

Requirements
LHDetect2 requires OpenCV2 (version 2.0 or higher) library
http://sourceforge.net/projects/opencvlibrary/

Building and install
LHDetect2 was developed and testing in Linux (Ubuntu and CenOS) operating system only.
OpenCV2 headers should be installed to /usr/include/opencv directory.
For building release version
$ cd Release
$ make

Usage
Run lhdetect2 without options for getting help.
Typical usage is
$ ./lhdetect2 --cmd detect -i ../test/1.png -l

Web-Service
You can use the web service based on the LHDetect2 program
http://wheatdb.org/lhdetect2

Contact Information
email mag@bionet.nsc.ru
web site http://wheatdb.org/

COPYING
LHDetect2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LHDetect2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
