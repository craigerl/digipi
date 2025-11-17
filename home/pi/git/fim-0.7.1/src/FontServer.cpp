/* $LastChangedDate: 2024-03-25 02:17:19 +0100 (Mon, 25 Mar 2024) $ */
/*
 FontServer.cpp : Font Server code from fbi, adapted for fim.

 (c) 2007-2024 Michele Martone
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
 * This file derives from fbi, and deserves a severe reorganization.
 * Really.
 * */

#include <dirent.h>
#include "fim.h"

#define FIM_FONT_DEBUG cc.getIntVariable(FIM_VID_FB_VERBOSITY)
#define FIM_FDS if(FIM_FONT_DEBUG) std::cout
//#define ff_stderr stdout
#define ff_stderr stderr
#define FIM_PSF1_MAGIC0     0x36
#define FIM_PSF1_MAGIC1     0x04
#define FIM_PSF2_MAGIC0     0x72
#define FIM_PSF2_MAGIC1     0xb5
#define FIM_PSF2_MAGIC2     0x4a
#define FIM_PSF2_MAGIC3     0x86
#define FIM_MAX_FONT_HEIGHT 256
#define FIM_SAVE_CONSOLEFONTNAME(CFN) cc.setVariable(FIM_VID_FBFONT,CFN);
namespace fim
{

#if ( FIM_FONT_MAGNIFY_FACTOR <= 0 )
    fim_int fim_fmf_ = FIM_FONT_MAGNIFY_FACTOR_DEFAULT; /* FIXME */
#else /* FIM_FONT_MAGNIFY_FACTOR */
    FIM_CONSTEXPR fim_int fim_fmf_ = FIM_FONT_MAGNIFY_FACTOR;
#endif /* FIM_FONT_MAGNIFY_FACTOR */

	FontServer::FontServer( )
	{
	}


#if 1
void FontServer::fb_text_init1(const fim_char_t *font_, struct fs_font **_f, fim_int vl)
{ 
    const fim_char_t*font=(fim_char_t*)font_;
    const fim_char_t *fonts[2] = { font, FIM_NULL };
    if (FIM_NULL == *_f)
    {
    	//FIM_FDS << "before consolefont: " << "(0x"<<((void*)*_f) <<")\n";
	*_f = fs_consolefont(font ? fonts : FIM_NULL, vl);
	if(_f && *_f)
	{ FIM_FDS << "loaded a font\n"; return; } else
	{ if(_f)*_f=NULL;FIM_FDS << "NOT loaded a font\n"; }
    	//FIM_FDS << "after consolefont : " << "(0x"<<((void*)*_f) <<")\n";
    }
#if FIM_WANT_HARDCODED_FONT
    //if (FIM_NULL == *_f)
	// *_f =...;
#endif /* FIM_WANT_HARDCODED_FONT */
#ifdef FIM_USE_X11_FONTS
    if (FIM_NULL == *_f && 0 == fs_connect(FIM_NULL))
	*_f = fs_open(font ? font : x11_font);
    FIM_FDS << "after fs_open     : " << "(0x"<<((void*)*_f) <<")\n";
#endif /* FIM_USE_X11_FONTS */
    if (FIM_NULL == *_f) {
	FIM_FPRINTF(ff_stderr, "font \"%s\" is not available\n",font);
#if FIM_EXPERIMENTAL_FONT_CMD
	if(vl)
#endif /* FIM_EXPERIMENTAL_FONT_CMD */
		exit(1);
    }
    else
    {
#if FIM_EXPERIMENTAL_FONT_CMD
#if FIM_WANT_HARDCODED_FONT
	font=FIM_DEFAULT_HARDCODEDFONT_STRING;
#endif /* FIM_WANT_HARDCODED_FONT */
	if(vl>1)
		FIM_FPRINTF(ff_stderr, "Using font \"%s\"\n",font);
#endif /* FIM_EXPERIMENTAL_FONT_CMD */
    }
}

#if 1
static const fim_char_t *default_font[] = {
    /* why the heck every f*cking distribution picks another
       location for these fonts ??? (GK)
       +1 (MM) */
#ifdef FIM_DEFAULT_CONSOLEFONT
	FIM_DEFAULT_CONSOLEFONT,
#endif /* FIM_DEFAULT_CONSOLEFONT */
    "/usr/share/kbd/consolefonts/cp866-8x16.psf.gz", /* sles11 */
    "/usr/share/consolefonts/Uni3-TerminusBoldVGA14.psf.gz",
    "/usr/lib/kbd/consolefonts/lat9-16.psf.gz",/* added for a Mandriva backport */
    "/usr/share/consolefonts/lat1-16.psf",
    "/usr/share/consolefonts/lat1-16.psf.gz",
    "/usr/share/consolefonts/lat1-16.psfu.gz",
    "/usr/share/kbd/consolefonts/lat1-16.psf",
    "/usr/share/kbd/consolefonts/lat1-16.psf.gz",
    "/usr/share/kbd/consolefonts/lat1-16.psfu.gz",
    "/usr/lib/kbd/consolefonts/lat1-16.psf",
    "/usr/lib/kbd/consolefonts/lat1-16.psf.gz",
    "/usr/lib/kbd/consolefonts/lat1-16.psfu.gz",
    "/lib/kbd/consolefonts/lat1-16.psf",
    "/lib/kbd/consolefonts/lat1-16.psf.gz",
    "/lib/kbd/consolefonts/lat1-16.psfu.gz",
    /* added for Ubuntu 10, but a search mechanism or a fim user variable would be wiser */
    "/lib/kbd/consolefonts/Lat2-VGA14.psf.gz",
    "/lib/kbd/consolefonts/Lat2-VGA16.psf.gz",
    "/lib/kbd/consolefonts/Lat2-VGA8.psf.gz",
    "/lib/kbd/consolefonts/Uni2-VGA16.psf.gz",
    /* end ubuntu add */
    /* begin debian squeeze add */
    "/usr/share/consolefonts/default8x16.psf.gz",
    "/usr/share/consolefonts/default8x9.psf.gz",
    "/usr/share/consolefonts/Lat15-Fixed16.psf.gz",
    "/usr/share/consolefonts/default.psf.gz",
    /* end debian squeeze add */
#if FIM_WANT_HARDCODED_FONT
    FIM_DEFAULT_HARDCODEDFONT_STRING,
#endif /* FIM_WANT_HARDCODED_FONT */
    FIM_NULL
};

fim::string get_default_font_list(void)
{
	// only meant for dump to man
	fim::string dfl;
#ifdef FIM_DEFAULT_CONSOLEFONT
	const fim_char_t ** const filename=default_font + 1;
#else /* FIM_DEFAULT_CONSOLEFONT */
	const fim_char_t ** const filename=default_font;
#endif /* FIM_DEFAULT_CONSOLEFONT */
	for(int i = 0; filename[i] != FIM_NULL; i++)
       	{
		dfl+=filename[i];
		dfl+="\n";
	}
	return dfl;
}
#endif

static int probe_font_file_fd(FILE *fp, const fim_char_t *fontfilename, int vl)
{
    const int m0=fgetc(fp);
    const int m1=fgetc(fp);

    if (m0 == EOF     || m1 == EOF     ) {
	if(vl)
	FIM_FPRINTF(ff_stderr, "problems reading two first bytes from %s.\n",fontfilename);
	goto oops;
    }
    if (m0 == FIM_PSF2_MAGIC0     && m1 == FIM_PSF2_MAGIC1     ) {
	if(vl>1)
	FIM_FPRINTF(ff_stderr, "can't use font %s: first two magic bytes (0x%x 0x%x) conform to PSF version 2, which is unsupported.\n",fontfilename,m0,m1);
	goto oops;
    }
    if (m0 != FIM_PSF1_MAGIC0     || m1 != FIM_PSF1_MAGIC1     ) {
	if(vl>1)
	FIM_FPRINTF(ff_stderr, "can't use font %s: first two magic bytes (0x%x 0x%x) not conforming to PSF version 1\n",fontfilename,m0,m1);
	goto oops;
    }
    return 0;
oops:
    return -1;
}

static int probe_font_file(const fim_char_t *fontfilename)
{
    	FILE *fp=FIM_NULL;

	if ( strlen(fontfilename)>3 && 0 == strcmp(fontfilename+strlen(fontfilename)-3,".gz"))
	{
		#ifdef FIM_USE_ZCAT
		/* FIXME */
		fp = FIM_TIMED_EXECLP(FIM_EPR_ZCAT,fontfilename,FIM_NULL);
		#endif /* FIM_USE_ZCAT */
	}
	else
	{
		fp = fopen(fontfilename, "r");
	}

	if (FIM_NULL == fp)
		return -1;

	const int rc = probe_font_file_fd(fp,fontfilename,1);

	if(fp)
		fclose(fp);
	return rc;
}

void fim_free_fs_font(struct fs_font *f)
{
	if(f)
	{
		if(f->eindex) fim_free(f->eindex);
		if(f->gindex) fim_free(f->gindex);
		if(f->glyphs) fim_free(f->glyphs);
		if(f->extents) fim_free(f->extents);
		fim_free(f);
	}
}

struct fs_font* FontServer::fs_consolefont(const fim_char_t **filename, fim_int vl)
{
    /* this function is too much involved: it shall be split in pieces */
    int  i=0;
    int  fr;
    const fim_char_t *h=FIM_NULL;
    struct fs_font *f_ = FIM_NULL;
    const fim_char_t *fontfilename=FIM_NULL;
    FILE *fp=FIM_NULL;
    fim_char_t fontfilenameb[FIM_PATH_MAX];
    bool robmn=true;/* retry on bad magic numbers */
#if FIM_WANT_HARDCODED_FONT
    size_t dfdi=0;
    unsigned char dfontdata[] =
#include "default_font_byte_array.h"/* FIXME: this is horrible practice */
#endif /* FIM_WANT_HARDCODED_FONT */

#if FIM_WANT_HARDCODED_FONT
    /* shortcut: no access() call required */
    if (filename && *filename && 0 == strcmp(filename[0],FIM_DEFAULT_HARDCODEDFONT_STRING))
	    goto openhardcodedfont;
#endif /* FIM_WANT_HARDCODED_FONT */

    if (FIM_NULL == filename)
	filename = fim::default_font;
#ifdef FIM_DEFAULT_CONSOLEFONT
    FIM_FDS << "configured with default (prioritary) consolefont: " << FIM_DEFAULT_CONSOLEFONT << "\n";
#endif /* FIM_DEFAULT_CONSOLEFONT */

scanlistforafontfile:
    for(i = 0; filename[i] != FIM_NULL; i++) {
	if (-1 == access(filename[i],R_OK))
	{
#if FIM_WANT_HARDCODED_FONT
    		if (0 == strcmp(filename[i],FIM_DEFAULT_HARDCODEDFONT_STRING))
		{
    			fontfilename = FIM_NULL;
    			FIM_FDS << "switching to hardcoded font, equivalent to: " << FIM_ENV_FBFONT << "=" << FIM_DEFAULT_HARDCODEDFONT_STRING << "\n";
			goto openhardcodedfont;
		}
#endif /* FIM_WANT_HARDCODED_FONT */
	    FIM_FDS << "no access to font file " << filename[i] << "\n";
	    fim_perror(FIM_NULL);
	    continue;
	}
	break;
    }
    fontfilename=filename[i];
    filename+=i;//new

#if FIM_LINUX_CONSOLEFONTS_DIR_SCAN 
    if(FIM_NULL == fontfilename)
    {
    	FIM_FDS << "will scan " << FIM_LINUX_CONSOLEFONTS_DIR " directory for console fonts" << "\n";
	
	fim::string nf = FIM_LINUX_CONSOLEFONTS_DIR;
	DIR *dir=FIM_NULL;
	struct dirent *de=FIM_NULL;

	if( !is_dir( nf.c_str() ))
		goto oops;
	if ( ! ( dir = opendir(nf.c_str() ) ))
		goto oops;

	while( ( de = readdir(dir) ) != FIM_NULL )
#ifndef __MINGW32__
	if( de->d_type == DT_REG ) // not in mingw
#endif
	if( regexp_match(de->d_name,"8x.*\\.psf") )
	{
		nf = FIM_LINUX_CONSOLEFONTS_DIR;
		nf+="/";
		nf+=de->d_name;

		if( 0 == access(nf.c_str(),R_OK) )
    		{
			strncpy(fontfilenameb,nf.c_str(),FIM_PATH_MAX-1);
			fontfilename=fontfilenameb;
			if(probe_font_file(fontfilename)==0)
				break; // found
			else
				fontfilename = FIM_NULL;
		}
	}
	closedir(dir);
    }
#endif /* FIM_LINUX_CONSOLEFONTS_DIR_SCAN */

#if FIM_WANT_HARDCODED_FONT
openhardcodedfont:
    if (FIM_NULL == fontfilename)
    {
	FIM_SAVE_CONSOLEFONTNAME(FIM_DEFAULT_HARDCODEDFONT_STRING)
#if HAVE_FMEMOPEN
    	fp=fmemopen(dfontdata,sizeof(dfontdata),"r");
	if(fp)
#else
	if (1) // we'll just read the bytes and cleanup the spaghetti all over the place one day
#endif
		goto gotafp;
    }
#endif /* FIM_WANT_HARDCODED_FONT */
    if (FIM_NULL == fontfilename) {
	if(vl)
	FIM_FPRINTF(ff_stderr, "can't find console font file\n");
	goto oops;
    }
    FIM_FDS << "probing font file: " << fontfilename << "\n";

    h = fontfilename+strlen(fontfilename)-3;
    if ( h>fontfilename && 0 == strcmp(h,".gz")) {
	#ifdef FIM_USE_ZCAT
    	FIM_FDS << "uncompressing by piping " << fontfilename << " through " << FIM_EPR_ZCAT << "\n";
	/* FIXME */
	fp = FIM_TIMED_EXECLP(FIM_EPR_ZCAT,fontfilename,FIM_NULL);
	#else /* FIM_USE_ZCAT */
	if(vl)
	FIM_FPRINTF(ff_stderr, "built with no gzip decoder!\n");
	#endif /* FIM_USE_ZCAT */
    } else {
	fp = fopen(fontfilename, "r");
    }
    if (FIM_NULL == fp) {
	if(vl)
	FIM_FPRINTF(ff_stderr, "can't open %s: %s\n",fontfilename,strerror(errno));
	goto oops;
    }
    FIM_SAVE_CONSOLEFONTNAME(fontfilename)
#if FIM_WANT_HARDCODED_FONT
gotafp:
#endif /* FIM_WANT_HARDCODED_FONT */
//    FIM_FPRINTF(ff_stderr, "using linux console font \"%s\"\n",filename[i]);
    if ( fp && 0 != probe_font_file_fd(fp,fontfilename,vl) )
    	goto oops;
    dfdi+=2;
    f_ =(struct fs_font*) fim_calloc(1,sizeof(*f_));
    if(!f_)goto aoops;
	
    if(fp)
	    fgetc(fp);
    else
	    dfdi++; // no use for dfontdata[dfdi]
    f_->maxenc = 256;
    f_->width  = 8;	/* FIXME */
    if (fp)
    	f_->height = fgetc(fp);
    else
    	f_->height = dfontdata[dfdi++];// lol
    if(f_->height<0 || f_->height>FIM_MAX_FONT_HEIGHT) goto oops;
    f_->fontHeader.min_bounds.left    = 0;
    f_->fontHeader.max_bounds.right   = f_->width;
    f_->fontHeader.max_bounds.descent = 0;
    f_->fontHeader.max_bounds.ascent  = f_->height;

    f_->glyphs  =(fim_byte_t*) fim_malloc(f_->height * 256);
    if(!f_->glyphs) goto aoops;
    f_->extents = (FSXCharInfo*)fim_malloc(sizeof(FSXCharInfo)*256);
    if(!f_->extents) goto aoops;
    if(fp)
    {
    	fr=fread(f_->glyphs, 256, f_->height, fp);
    	if(!fr)goto aoops;/* new */
    }
    else
    	memcpy(f_->glyphs, dfontdata+dfdi, 256*f_->height);
    if (fp)
    	fclose(fp);
    fp=FIM_NULL;

    f_->eindex  =(FSXCharInfo**) fim_malloc(sizeof(FSXCharInfo*)   * 256);
    if(!f_->eindex) goto aoops;
    f_->gindex  = (fim_byte_t**)fim_malloc(sizeof(fim_byte_t*) * 256);
    if(!f_->gindex) goto aoops;
    for (i = 0; i < 256; i++) {
	f_->eindex[i] = f_->extents +i;
	f_->gindex[i] = f_->glyphs  +i * f_->height;
	f_->eindex[i]->left    = 0;
	f_->eindex[i]->right   = 7;
	f_->eindex[i]->width   = 8;/* FIXME */
	f_->eindex[i]->descent = 0;
	f_->eindex[i]->ascent  = f_->height;
    }
    return f_;
aoops:
    robmn=false;/* no retry: this is a allocation-related oops */
    if(f_)
	fim_free_fs_font(f_);
oops:
    if(fp){fclose(fp);fp=FIM_NULL;}
    if(robmn && filename[0] && filename[1]){++filename;goto scanlistforafontfile;}else robmn=false;
    return FIM_NULL;
}
#endif




}


