/* $LastChangedDate: 2023-04-09 16:02:31 +0200 (Sun, 09 Apr 2023) $ */
/*
 FbiStuffJpeg.cpp : fbi functions for JPEG files, modified for fim

 (c) 2007-2023 Michele Martone
 (c) 1998-2006 Gerd Knorr <kraxel@bytesex.org>

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
/*
 * This file comes from fbi, and will undergo severe reorganization.
 * */


#include <cstdio>
#include <cstdlib>

#if 0

#ifdef HAVE_STDLIB_H
//#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H_BACKUP HAVE_STDLIB_H
#endif /* HAVE_STDLIB_H */
#endif /* 0 */


#include <cstring>
//#include <stddef.h>
//#include <errno.h>

#include "FbiStuff.h"
#include "FbiStuffLoader.h"
//#include "loader.h"
//#define FIM_WITH_LIBEXIF 1
#ifdef FIM_WITH_LIBEXIF
#include <libexif/exif-data.h>
#define HAVE_NEW_EXIF 1
#endif /* FIM_WITH_LIBEXIF */

//
extern "C"
{
// we require C linkage for these symbols
#include <jpeglib.h>
}
#ifndef HAVE_BOOLEAN
#ifndef TRUE
#define TRUE 1
#endif
#endif /* HAVE_BOOLEAN */
#define FIM_JPEGLIB_TRUE TRUE /* jpeglib 8.0 uses 'boolean' */


/*
	From fbi's misc.h :
*/

#if 0
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (fim_char_t *)__mptr - offsetof(type,member) );})
#define array_size(x) (sizeof(x)/sizeof(x[0]))
#endif

/*								*/

#ifdef USE_X11
 #include <X11/Xlib.h>
 #include <X11/Intrinsic.h>
 #include <Xm/Xm.h>
 #include <Xm/Text.h>
 #include <Xm/SelectioB.h>
 #include "RegEdit.h"
 #include "ida.h"
 #include "viewer.h"
#endif /* USE_X11 */
namespace fim
{

#if HAVE_NEW_EXIF
extern CommandConsole cc;
#endif /* HAVE_NEW_EXIF */
   int fim_jerr=0;

/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct jpeg_state {
    FILE * infile;                /* source file */
    
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;            /* Output row buffer */
    int row_stride,linelength;    /* physical row width in output buffer */
    fim_byte_t *image,*ptr;

    /* thumbnail */
    fim_byte_t  *thumbnail;
    unsigned int   tpos, tsize;

    struct ida_image_info *i; /* here for load_free_extras */
};

/* ---------------------------------------------------------------------- */
/* data source manager for thumbnail images                               */

/*static void thumbnail_src_init(struct jpeg_decompress_struct *cinfo)
{
    struct jpeg_state *h  = container_of(cinfo, struct jpeg_state, cinfo);
    cinfo->src->next_input_byte = h->thumbnail;
    cinfo->src->bytes_in_buffer = h->tsize;
}*/

static int thumbnail_src_fill(struct jpeg_decompress_struct *cinfo)
{
    FIM_FBI_PRINTF("jpeg: panic: no more thumbnail input data\n");
    exit(1);
}

static void thumbnail_src_skip(struct jpeg_decompress_struct *cinfo,
			       long num_bytes)
{
    cinfo->src->next_input_byte += num_bytes;
}

static void thumbnail_src_term(struct jpeg_decompress_struct *cinfo)
{
    /* nothing */
}

//!!
//see jpeg_init
   static struct jpeg_source_mgr thumbnail_mgr ;
/*
   static struct jpeg_source_mgr thumbnail_mgr = {
    .init_source         = thumbnail_src_init,
    .fill_input_buffer   = thumbnail_src_fill,
    .skip_input_data     = thumbnail_src_skip,
    .resync_to_restart   = jpeg_resync_to_restart,
    .term_source         = thumbnail_src_term,
};
*/
//!!


static void fim_error_exit (j_common_ptr cinfo)
{
	/*
		FIM : NEW, UNFINISHED (if used now, it would break things)
		read jpeglib's docs to finish this :)
	*/
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
//  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  std::cerr << "jpeglib:"<<"\n";
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
//  longjmp(myerr->setjmp_buffer, 1);
   fim_jerr=1;
}

#ifdef FIM_WITH_LIBEXIF
static void fim_ExifContentForeachEntryFunc (ExifEntry *entry, void * user_data)
{
	fim_char_t buffer[FIM_EXIF_BUFSIZE];
	Namespace *nsp = (Namespace*) ((void**)user_data)[0];
	ExifContent *c =  (ExifContent *) ((void**)user_data)[1];
	const fim_char_t * const value = exif_entry_get_value(entry, buffer, sizeof(buffer));
	ExifIfd ifd = exif_content_get_ifd (c);
	const fim_char_t * const title = exif_tag_get_name_in_ifd (entry->tag,ifd); /* exif_tag_get_name() is deprecated */

	if(nsp)
		/* std::cout << "|" << title << "|\n", */
		nsp->setVariable(string("EXIF_") + string(title),value);
}

static void fim_ExifDataForeachContentFunc (ExifContent *content, void * user_data)
{
	void * user_data2[] = {user_data,content};
	exif_content_foreach_entry (content, fim_ExifContentForeachEntryFunc, user_data2);
}

static void dump_exif(FILE *out, ExifData *ed, Namespace *nsp = FIM_NULL)
{
/* FIXME: temporarily here; shall transfer keys/values to a Namespace object */
#if HAVE_NEW_EXIF
    if(nsp)
    	exif_data_foreach_content (ed, fim_ExifDataForeachContentFunc, nsp);
#endif /* HAVE_NEW_EXIF */

/* #if HAVE_NEW_EXIF */
#if 0
    for (int i = 0; i < EXIF_IFD_COUNT; i++) {
    fim_char_t buffer[FIM_EXIF_BUFSIZE];
    const fim_char_t *value=FIM_NULL;
    ExifEntry  *ee=FIM_NULL;
    /* values of EXIF_TAG_* in libexif/exif-tag.h */
    //for (i = 0; i < 1; i++) { // first only
    std::cout << "EXIF_IFD_COUNT " << i << " " << EXIF_IFD_COUNT << "\n";
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_ORIENTATION)){
		// value can be of the form "X - Y", with X and Y in
		// {top,bottom,left,right}
		// here we handle only
		// on: http://sylvana.net/jpegcrop/exif_orientation.html 
		// we got the following combinations:
		// Value	0th Row	0th Column
		// 1	top	left side
		// 2	top	right side
		// 3	bottom	right side
		// 4	bottom	left side
		// 5	left side	top
		// 6	right side	top
		// 7	right side	bottom
		// 8	left side	bottom
		//
		// neatly depicted in an F letter example:
		//
		//   1        2       3      4         5            6           7          8
		//
		//   888888  888888      88  88      8888888888  88                  88  8888888888
		//   88          88      88  88      88  88      88  88          88  88      88  88
		//   8888      8888    8888  8888    88          8888888888  8888888888          88
		//   88          88      88  88
		//   88          88  888888  888888
		//
		// note that (in this order):
		// 2,3,5,7 want a mirror transformation
		// 4,3 want a flip transformation
		// 7,8 want a cw rotation
		// 5,6 want a ccw rotation
		//
		bool shouldmirror,shouldrotatecw,shouldrotateccw,shouldflip; fim_char_t r,c;const fim_char_t *p;fim_char_t f;
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(!value || ((p=strstr(value," - "))==FIM_NULL))goto uhmpf;
		r=tolower(value[0]); c=tolower(p[3]);
		switch(r)
		{
			case 't':
			switch(c){
				case 'l':f=1; break;
				case 'r':f=2; break;
				default: f=0;
			} break;
			case 'b':
			switch(c){
				case 'r':f=3; break;
				case 'l':f=4; break;
				default: f=0;
			} break;
			case 'l':
			switch(c){
				case 't':f=5; break;
				case 'b':f=8; break;
				default: f=0;
			} break;
			case 'r':
			switch(c){
				case 't':f=6; break;
				case 'b':f=7; break;
				default: f=0;
			} break;
			default: f=0;
		}
		//if(f==0)goto uhmpf;
		shouldmirror=(f==2 || f==4 || f==5 || f==7);
		shouldflip=(f==4 || f==3);
		shouldrotatecw=(f==5 || f==6);
		shouldrotateccw=(f==7 || f==8);
//		std::cout << "EXIF_TAG_ORIENTATION FOUND !\n",
//		std::cout << "VALUE: " <<(int)f << r<< c<<
//		shouldmirror<< shouldrotatecw<< shouldrotateccw<< shouldflip,
//		std::cout << "\n";
		if(shouldrotateccw)cc.setVariable("exif_orientation",1);
		if(shouldrotatecw)cc.setVariable("exif_orientation",3);
		if(shouldmirror)cc.setVariable("exif_mirrored",1);
		if(shouldflip)cc.setVariable("exif_flipped",1);
		// FIXME: should complete this code by setting up some mechanism for recovering this rotation/flip info
uhmpf:
		1;
	}
	/* FIXME: shall rationalize the following: */
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_DATE_TIME)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_DATE_TIME",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_DATE_TIME_ORIGINAL)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_DATE_TIME_ORIGINAL",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_DATE_TIME_DIGITIZED)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_DATE_TIME_DIGITIZED",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_EXPOSURE_TIME)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_EXPOSURE_TIME",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_FNUMBER)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_FNUMBER",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_BATTERY_LEVEL)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_BATTERY_LEVEL",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_APERTURE_VALUE)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_APERTURE_VALUE",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_METERING_MODE)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_METERING_MODE",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_EXPOSURE_PROGRAM)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_EXPOSURE_PROGRAM",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_ISO_SPEED_RATINGS)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_ISO_SPEED_RATINGS",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_MODEL)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_MODEL",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_TIME_ZONE_OFFSET)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_TIME_ZONE_OFFSET",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_IMAGE_UNIQUE_ID)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_IMAGE_UNIQUE_ID",value);
	}
	if( ee=exif_content_get_entry(ed->ifd[i],EXIF_TAG_SHARPNESS)){
		value=exif_entry_get_value(ee, buffer, sizeof(buffer));
		if(nsp) nsp->setVariable("EXIF_TAG_SHARPNESS",value);
	}
	// ... EXIF_TAG_XP_TITLE EXIF_TAG_XP_COMMENT EXIF_TAG_XP_AUTHOR EXIF_TAG_XP_KEYWORDS EXIF_TAG_XP_SUBJECT
    }
#endif /* HAVE_NEW_EXIF */
#if 0
    for (i = 0; i < EXIF_IFD_COUNT; i++) {
	fprintf(out,"   ifd %s\n", exif_ifd_get_name (i));
	for (tag = 0; tag < 0xffff; tag++) {
	    title = exif_tag_get_title(tag);
	    if (!title)
		continue;
	    ee = exif_content_get_entry (ed->ifd[i], tag);
	    if (FIM_NULL == ee)
		continue;
#ifdef HAVE_NEW_EXIF
	    value = exif_entry_get_value(ee, buffer, sizeof(buffer));
#else /* HAVE_NEW_EXIF */
	    value = exif_entry_get_value(ee);
#endif /* HAVE_NEW_EXIF */
	    fprintf(out,"      0x%04x  %-30s %s\n", tag, title, value);
	}
    }
    if (ed->data && ed->size)
	fprintf(out,"   thumbnail\n      %d bytes data\n", ed->size);
#endif
}
#endif /* FIM_WITH_LIBEXIF */


/* ---------------------------------------------------------------------- */
/* jpeg loader                                                            */

static void*
jpeg_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
    struct jpeg_state *h;
    jpeg_saved_marker_ptr mark;
    fim_jerr=0;
#ifdef FIM_WITH_LIBEXIF
    //std::cout << "EXIF is not implemented, really :) \n";
#endif /* FIM_WITH_LIBEXIF */
    
    h = (struct jpeg_state *)fim_calloc(1,sizeof(*h));
    if(!h) goto oops;

    h->infile = fp;
    h->i = i;

    h->jerr.error_exit=FIM_NULL; // ?
    h->cinfo.err = jpeg_std_error(&h->jerr);	/* FIXME : should use an error manager of ours (this one exits the program!) */
    h->jerr.error_exit = fim_error_exit;	/* FIXME : should use an error manager of ours (this one exits the program!) */
    //h->jerr.error_exit = FIM_NULL ;	/* FIXME : should use an error manager of ours (this one exits the program!) */
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
    jpeg_create_decompress(&h->cinfo);
    /*if(h->jerr.msg_code)goto oops;*/
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
    jpeg_save_markers(&h->cinfo, JPEG_COM,    0xffff); /* comment */
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
    jpeg_save_markers(&h->cinfo, JPEG_APP0+1, 0xffff); /* EXIF */
    jpeg_save_markers(&h->cinfo, JPEG_APP0+13, 0xffff); /* ?? */
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
    jpeg_stdio_src(&h->cinfo, h->infile);
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
//    if(jpeg_read_header(&h->cinfo, FIM_JPEGLIB_TRUE)==0)	goto oops;
    jpeg_read_header(&h->cinfo, FIM_JPEGLIB_TRUE);
    if(fim_jerr /*&& h->jerr.msg_code*/)goto oops;
//    if(h->jerr.msg_code)goto oops;	// this triggers with apparently good file

    for (mark = h->cinfo.marker_list; FIM_NULL != mark; mark = mark->next) {
	switch (mark->marker) {
	case JPEG_COM:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("jpeg: comment found (COM marker) [%.*s]\n",
			(int)mark->data_length, mark->data);
	    load_add_extra(i,EXTRA_COMMENT,mark->data,mark->data_length);
	    break;
	case JPEG_APP0 +1:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("jpeg: exif data found (APP1 marker)\n");
	    /* load_add_extra(i,EXTRA_COMMENT,mark->data,mark->data_length); */

#if HAVE_NEW_EXIF
#ifdef FIM_WITH_LIBEXIF
	{
		ExifData *ed=exif_data_new_from_data(mark->data,mark->data_length);
    		if(ed)
#if FIM_EXPERIMENTAL_IMG_NMSPC
			dump_exif(stdout,ed,i->nsp);
#else /* FIM_EXPERIMENTAL_IMG_NMSPC */
			dump_exif(stdout,ed,FIM_NULL);
#endif /* FIM_EXPERIMENTAL_IMG_NMSPC */
		exif_data_unref(ed);
	}
#endif /* FIM_WITH_LIBEXIF */
#endif /* HAVE_NEW_EXIF */
#ifdef FIM_WITH_LIBEXIF_nonono
	    if (thumbnail) {
		ExifData *ed;
		
		ed = exif_data_new_from_data(mark->data,mark->data_length);
		if (ed->data &&
		    ed->data[0] == 0xff &&
		    ed->data[1] == 0xd8) {
		    if (FbiStuff::fim_filereading_debug())
			FIM_FBI_PRINTF("jpeg: exif thumbnail found\n");

		    /* save away thumbnail data */
		    h->thumbnail = fim_malloc(ed->size);
    		    if(!h->thumbnail) goto oops;
		    h->tsize = ed->size;
		    memcpy(h->thumbnail,ed->data,ed->size);
		}
		exif_data_unref(ed);
	    }
#endif /* FIM_WITH_LIBEXIF_nonono */
	    break;
	case JPEG_APP0 +13:
	    if (FbiStuff::fim_filereading_debug())
		printf("jpeg: exif data found (APP13 marker)\n");
	    /* load_add_extra(i,EXTRA_COMMENT,mark->data,mark->data_length); */

	    break;
	default:
		printf("jpeg: unknown marker\n");
	}
    }

    // !! 
    thumbnail_mgr.init_source         = /*thumbnail_src_init*/ FIM_NULL /* it is not useful, and breaks c++98 standard C++ compilation */;
    thumbnail_mgr.fill_input_buffer   = thumbnail_src_fill;
    thumbnail_mgr.skip_input_data     = thumbnail_src_skip;
    thumbnail_mgr.resync_to_restart   = jpeg_resync_to_restart;
    thumbnail_mgr.term_source         = thumbnail_src_term;
    // !! 

    if (h->thumbnail) {
	/* save image size */
	i->thumbnail   = 1;
	i->real_width  = h->cinfo.image_width;
	i->real_height = h->cinfo.image_height;

	/* re-setup jpeg */
	jpeg_destroy_decompress(&h->cinfo);
        if(fim_jerr)goto oops;
    //    if(h->jerr.msg_code)goto oops; // this triggers with apparently good files 
	fim_fclose(h->infile);
	h->infile = FIM_NULL;
	jpeg_create_decompress(&h->cinfo);
        if(fim_jerr)goto oops;
  //      if(h->jerr.msg_code)goto oops;
	h->cinfo.src = &thumbnail_mgr;
	jpeg_read_header(&h->cinfo, FIM_JPEGLIB_TRUE);
        if(fim_jerr)goto oops;
//        if(h->jerr.msg_code)goto oops;
    }

    h->cinfo.out_color_space = JCS_RGB;
    jpeg_start_decompress(&h->cinfo);
    if(fim_jerr)goto oops;
//    if(h->jerr.msg_code)goto oops;
    i->width  = h->cinfo.image_width;
    i->height = h->cinfo.image_height;
    i->npages = 1;
    switch (h->cinfo.density_unit) {
    case 0: /* unknown */
	break;
    case 1: /* dot per inch */
	i->dpi = h->cinfo.X_density;
	break;
    case 2: /* dot per cm */
	i->dpi = res_cm_to_inch(h->cinfo.X_density);
	break;
    }

    return h;
oops:
std::cerr << "OOPS: problems decoding "<< filename <<"...\n";
    if( h ) jpeg_destroy_decompress (&h->cinfo);
    if( h && h->thumbnail) fim_free(h->thumbnail);
    if( h ) fim_free(h);
    fim_jerr=0;/* ready for the next */
    return FIM_NULL;
}

static void
jpeg_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct jpeg_state *h = (struct jpeg_state*)data;
    JSAMPROW row = dst;
//    if(h->jerr.msg_code)goto oops;
    jpeg_read_scanlines(&h->cinfo, &row, 1);
//    oops:
    return;
}

static void
jpeg_done(void *data)
{
    struct jpeg_state *h = (struct jpeg_state*)data;
    jpeg_destroy_decompress(&h->cinfo);
    if (h->infile)
	fim_fclose(h->infile);
    if (h->thumbnail)
	fim_free(h->thumbnail);
    load_free_extras(h->i);
    fim_free(h);
}

static struct ida_loader jpeg_loader = {
    /*magic:*/ "\xff\xd8",
    /*moff:*/  0,
    /*mlen:*/  2,
    /*name:*/  "libjpeg",
    /*init:*/  jpeg_init,
    /*read:*/  jpeg_read,
    /*done:*/  jpeg_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&jpeg_loader);
}

#ifdef USE_X11

/* ---------------------------------------------------------------------- */
/* jpeg writer                                                            */

static Widget jpeg_shell;
static Widget jpeg_text;
static int jpeg_quality = 75;

static void
jpeg_button_cb(Widget widget, XtPointer clientdata, XtPointer call_data)
{
    XmSelectionBoxCallbackStruct *cb = call_data;

    if (XmCR_OK == cb->reason) {
	jpeg_quality = atoi(XmTextGetString(jpeg_text));
	do_save_print();
    }
    XtUnmanageChild(jpeg_shell);
}

static int
jpeg_conf(Widget parent, struct ida_image *img)
{
    fim_char_t tmp[32];
    
    if (!jpeg_shell) {
	/* build dialog */
	jpeg_shell = XmCreatePromptDialog(parent,"jpeg",FIM_NULL,0);
	XmdRegisterEditres(XtParent(jpeg_shell));
	XtUnmanageChild(XmSelectionBoxGetChild(jpeg_shell,XmDIALOG_HELP_BUTTON));
        jpeg_text = XmSelectionBoxGetChild(jpeg_shell,XmDIALOG_TEXT);
	XtAddCallback(jpeg_shell,XmNokCallback,jpeg_button_cb,FIM_NULL);
	XtAddCallback(jpeg_shell,XmNcancelCallback,jpeg_button_cb,FIM_NULL);
    }
    sprintf(tmp,"%d",jpeg_quality);
    XmTextSetString(jpeg_text,tmp);
    XtManageChild(jpeg_shell);
    return 0;
}

static int
jpeg_write(FILE *fp, struct ida_image *img)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    fim_byte_t *line;
    unsigned int i;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width  = img->i.width;
    cinfo.image_height = img->i.height;
    if (img->i.dpi) {
	cinfo.density_unit = 1;
	cinfo.X_density = img->i.dpi;
	cinfo.Y_density = img->i.dpi;
    }
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, jpeg_quality, FIM_JPEGLIB_TRUE);
    jpeg_start_compress(&cinfo, FIM_JPEGLIB_TRUE);

    for (i = 0, line = img->data; i < img->i.height; i++, line += img->i.width*3)
        jpeg_write_scanlines(&cinfo, &line, 1);
    
    jpeg_finish_compress(&(cinfo));
    jpeg_destroy_compress(&(cinfo));
    return 0;
}

struct ida_writer jpeg_writer = {
    /*label:*/  "JPEG",
    /* ext: */    { "jpg", "jpeg", FIM_NULL},
    /*write:*/  jpeg_write,
    /*conf: */   jpeg_conf,
};

static void fim__init init_wr(void)
{
    fim_write_register(&jpeg_writer);
}


#endif /* USE_X11 */
#if 0
#ifdef HAVE_STDLIB_H_BACKUP 
#define HAVE_STDLIB_H HAVE_STDLIB_H_BACKUP 
#undef HAVE_STDLIB_H_BACKUP 
#endif /* HAVE_STDLIB_H_BACKUP */
#endif
}
