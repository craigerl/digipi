/* $LastChangedDate: 2017-07-07 23:19:21 +0200 (Fri, 07 Jul 2017) $ */
/*
 fim_plugin.cpp : Fim plugin stuff

 (c) 2011-2017 Michele Martone

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/* FIXME: this code is still at embryonic stage
 * to test the sample opencv 'plugin', shall include -I/usr/include/opencv and link to  -lcv -lcvaux  (OpenCV version 1 API)
 * e.g.:
 * http://opencv.itseez.com/modules/core/doc/old_basic_structures.html?highlight=iplimage#void cvSetData(CvArr* arr, void* data, int step)
 * */

#include "fim_plugin.h"

#define FIM_WANT_OPENCV_EXAMPLE 0

#if  FIM_WANT_OPENCV_EXAMPLE
#include <cv.h>
#include <highgui.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cfloat>
#include <climits>
#include <ctime>
#include <cctype>

#define FIM_INVERT_BYTE(x) x=~x
#define FIM_BLAKEN_BYTE(x) x=0
#define FIM_PROCESS_BYTE(x) FIM_BLAKEN_BYTE(x)

extern fim::CommandConsole fim::cc;

static void fim_opencv_detect_and_draw( IplImagePtr img, struct ida_image *iimg )
{
	/* 
	 * this code is based on the example from the OpenCV wiki at:
	 * http://opencv.willowgarage.com/wiki/FaceDetection
	 * */
    	CvPoint pt1, pt2;
	int i;
	fim_coo_t w=iimg->i.width;
	static CvMemStorage* storage=FIM_NULL;
	static CvHaarClassifierCascade* cascade=FIM_NULL;
	static int haar_file_existent=-1;
	const fim_char_t*const haarfile="haarcascade_frontalface_alt.xml";
	string haarpath; 
	FIM_CONSTEXPR fim_char_t*FIM_HAAR_PATH="FIM_HAAR_PATH";

	if(haar_file_existent==0)
		return;
    	storage=cvCreateMemStorage(0);
    	cvClearMemStorage(storage);
	haarpath=fim::cc.getStringVariable(FIM_HAAR_PATH);
#ifdef HAVE_GETENV
   	if(haarpath==FIM_CNS_EMPTY_STRING)
		haarpath=getenv(FIM_HAAR_PATH);
#endif /* HAVE_GETENV */
	haarpath+=haarfile;

	if(is_file(haarpath))
		haar_file_existent=1;
	else
		haar_file_existent=0;
	/* Usage :
	 export FIM_HAAR_PATH=~/OpenCV-2.3.1/data/haarcascades/  fim ...
	 or
	 fim -C 'FIM_HAAR_PATH="~/OpenCV-2.3.1/data/haarcascades/"' ...
	 */
	if(!cascade )
    		cascade=(CvHaarClassifierCascade*)cvLoad(haarpath.c_str(),FIM_NULL,FIM_NULL,FIM_NULL);
    	if( cascade )
    	{
        	CvSeq*faces=cvHaarDetectObjects(img,cascade,storage,1.1,2,CV_HAAR_DO_CANNY_PRUNING,cvSize(40,40));
		CvRect * fr=FIM_NULL;
		if(faces)
        	for(i=0;i<(faces ? faces->total:0);i++)
            	if(fr=(CvRect*)cvGetSeqElem(faces,i))
        	{
			pt1.x=fr->x;
			pt2.x=fr->x+fr->width;
			pt1.y=fr->y;
			pt2.y=fr->y+fr->height;
			for(fim_coo_t r=pt1.y;r<pt2.y;++r)
			for(fim_coo_t c=pt1.x;c<pt2.x;++c)
			{
				FIM_PROCESS_BYTE(iimg->data[3*(r*w+c)+0]);
				FIM_PROCESS_BYTE(iimg->data[3*(r*w+c)+1]);
				FIM_PROCESS_BYTE(iimg->data[3*(r*w+c)+2]);
			}
		}
		if(faces)
			cvClearSeq(faces);
	       	/* FIXME: The OpenCV documentation says that cvClearSeq() does not deallocate anything;
		 am not fully sure whether this code is completely correct, then.
		 */
		faces=FIM_NULL;
	}
	if(cascade)
		cvReleaseHaarClassifierCascade(&cascade);
	cascade=FIM_NULL;
}

static fim_err_t fim_opencv_plugin_example(struct ida_image *img, const fim_char_t *filename)
{
	fim_coo_t r,c,h=img->i.height,w=img->i.width;
	FIM_CONSTEXPR int b=30;
       	IplImagePtrcvimage=FIM_NULL;
	int depth=IPL_DEPTH_8U;
       	int channels=3;
	CvSize size;

	if(!img)
		goto err;
#if 0
       	cvimage=cvLoadImage(filename,1);
#else
	size.width=img->i.width;
	size.height=img->i.height;
       	cvimage = cvCreateImage(size,depth,channels);
	cvSetData(cvimage,img->data,3*img->i.width);
#endif
	if(cvimage)
		fim_opencv_detect_and_draw(cvimage,img);
	else
		goto err;

	return FIM_ERR_NO_ERROR;
err:
	return FIM_ERR_GENERIC;
}
#endif /* FIM_WANT_OPENCV_EXAMPLE */

#if FIM_WANT_EXPERIMENTAL_PLUGINS
fim_err_t fim_post_read_plugins_exec(struct ida_image *img, const fim_char_t * filename)
{
	/* FIXME: here shall call all registered post-read plugins */
	/* as a first thing, need here a function drawing random boxes */
#if FIM_WANT_OPENCV_EXAMPLE
	return fim_opencv_plugin_example(img,filename);
#else
	return FIM_ERR_NO_ERROR;
#endif /* FIM_WANT_OPENCV_EXAMPLE */
}
#endif /* FIM_WANT_EXPERIMENTAL_PLUGINS */

