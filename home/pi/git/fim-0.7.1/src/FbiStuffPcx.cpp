/* $LastChangedDate: 2023-03-25 18:31:40 +0100 (Sat, 25 Mar 2023) $ */
/*
 FbiStuffPcx.cpp : Code for reading PCX files.

 (c) 2014-2023 Michele Martone
 The functions pcx_load_image_fp and pcx_load_image_info_fp are (c) 2014-2014 Mohammed Isam.
 Originally based on code (c) 1998-2006 Gerd Knorr <kraxel@bytesex.org>

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

#include "fim.h"

#if FIM_WITH_PCX

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ENDIAN_H
 #include <endian.h>
#endif /* HAVE_ENDIAN_H */

namespace fim
{

extern CommandConsole cc;

typedef int pcx_err_t;
#define PCX_ERR_NO_ERROR	0
#define PCX_ERR_GENERIC		-1

typedef char BOOL;
#ifdef HAVE_STDINT_H
typedef uint8_t UBYTE;		//define UNSIGNED BYTE as 1 byte
typedef uint16_t UWORD;		//define UNSIGNED WORD as 2 bytes
typedef uint32_t UDWORD;	//define UNSIGNED DOUBLE WORD as 4 bytes
#else /* HAVE_STDINT_H */
typedef __u8 UBYTE;		//define UNSIGNED BYTE as 1 byte
typedef __u16 UWORD;		//define UNSIGNED WORD as 2 bytes
typedef __u32 UDWORD;		//define UNSIGNED DOUBLE WORD as 4 bytes
#endif /* HAVE_STDINT_H */

#if BYTE_ORDER == BIG_ENDIAN
UWORD swap_word(UWORD x) {
	return ((x>>8)&0xff) |      // move byte 1 to byte 0
               ((x<<8)&0xff00);     // move byte 0 to byte 1
}
#endif /* BYTE_ORDER == BIG_ENDIAN */

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
} urgb;

typedef struct {
	char red;
	char green;
	char blue;
} colormap_rgb_s;

typedef struct {
  fim_byte_t     manufacturer;
  fim_byte_t     version;
  fim_byte_t     encoding;
  fim_byte_t     bpp;
  UWORD          xstart, ystart, xend, yend;
  UWORD          hres, vres;
  colormap_rgb_s colormap[16];	//amounts to 48 bytes
  fim_byte_t     reserved;
  fim_byte_t     planes;
  UWORD          bpl;
  UWORD          paletteinfo;
  fim_byte_t     filler[58];
}PCX_HEADER_S;
PCX_HEADER_S PCX_HEADER;

bool PALETTE_DEFINED;
bool COLORMAP_DEFINED;

#define PCX_RETURN_ERROR(x)			\
{ 						\
  fprintf(stderr, "%s@%d:",__FILE__,__LINE__); 	\
  fprintf(stderr, x);	 			\
  goto err; 					\
}

/* ---------------------------------------------------------------------- */
/* libpcx routines */
static pcx_err_t pcx_load_image_info_fp(FILE *fp, int * numpagesp, unsigned int *wp, unsigned int *hp)
{
	if(fseek(fp,0,SEEK_SET) != 0)
	  PCX_RETURN_ERROR("Error setting file pointer\n")
	if(fread(&PCX_HEADER, 128, 1, fp) != 1)
	  PCX_RETURN_ERROR("Error reading first 128 bytes of PCX file\n")
	if(fseek(fp,0,SEEK_SET) != 0)
	  PCX_RETURN_ERROR("Error setting file pointer\n")
		
	if(fread(&PCX_HEADER, 128, 1, fp) != 1)
	  PCX_RETURN_ERROR("Error reading file header\n")
		
	if(PCX_HEADER.manufacturer != 10)
	  PCX_RETURN_ERROR("Not a PCX image\n")
	
	if(PCX_HEADER.encoding != 1)
	  PCX_RETURN_ERROR("Invalid encoding\n")
	 
/* PCX is little-endian. If machine is big-endian, swap bytes.. */
#if BYTE_ORDER == BIG_ENDIAN
	  PCX_HEADER.xend = swap_word(PCX_HEADER.xend);
	  PCX_HEADER.xstart = swap_word(PCX_HEADER.xstart);
	  PCX_HEADER.yend = swap_word(PCX_HEADER.yend);
	  PCX_HEADER.ystart = swap_word(PCX_HEADER.ystart);
	  PCX_HEADER.hres = swap_word(PCX_HEADER.hres);
	  PCX_HEADER.vres = swap_word(PCX_HEADER.vres);
	  PCX_HEADER.bpl = swap_word(PCX_HEADER.bpl);
	  PCX_HEADER.paletteinfo = swap_word(PCX_HEADER.paletteinfo);
#endif /* BYTE_ORDER == BIG_ENDIAN */
	//Calculate the width and height
	*wp = (PCX_HEADER.xend-PCX_HEADER.xstart)+1;
	*hp = (PCX_HEADER.yend-PCX_HEADER.ystart)+1;
	*numpagesp = 1;
       	return PCX_ERR_NO_ERROR;
err:
	return PCX_ERR_GENERIC;
}

static pcx_err_t pcx_load_image_fp(FILE *fp, unsigned int page, unsigned char * rgb, int bytes_per_line)
{
	urgb palette[256];
	long pos2;
	int tmp = 0;
	int h;
	long line_bytes;
	int image_w;
	int image_h;
	long bitmap_size;
	int padding;
	long pos, count;
	unsigned char repeat, value;
	const int FLAG_MASK = 192;   //11000000
	const int REVERSE_MASK = 63; //00111111
	int plane, i;
	
	if(fseek(fp,0,SEEK_SET) != 0)
	  PCX_RETURN_ERROR("Error setting file pointer\n")
	if(fread(&PCX_HEADER, 128, 1, fp) != 1)
	  PCX_RETURN_ERROR("Error reading file header\n")
	
	if(PCX_HEADER.manufacturer != 10)
	  PCX_RETURN_ERROR("Not a PCX image\n")
	if(PCX_HEADER.encoding != 1)
	  PCX_RETURN_ERROR("Invalid encoding\n")
	
/* PCX is little-endian. If machine is big-endian, swap bytes.. */
#if BYTE_ORDER == BIG_ENDIAN
	  PCX_HEADER.xend = swap_word(PCX_HEADER.xend);
	  PCX_HEADER.xstart = swap_word(PCX_HEADER.xstart);
	  PCX_HEADER.yend = swap_word(PCX_HEADER.yend);
	  PCX_HEADER.ystart = swap_word(PCX_HEADER.ystart);
	  PCX_HEADER.hres = swap_word(PCX_HEADER.hres);
	  PCX_HEADER.vres = swap_word(PCX_HEADER.vres);
	  PCX_HEADER.bpl = swap_word(PCX_HEADER.bpl);
	  PCX_HEADER.paletteinfo = swap_word(PCX_HEADER.paletteinfo);
#endif /* BYTE_ORDER == BIG_ENDIAN */
	
	PALETTE_DEFINED = 0;
	COLORMAP_DEFINED = 0;
	pos2 = ftell(fp);
	if(fseek(fp, -769, SEEK_END) != 0)
	  PCX_RETURN_ERROR("Error setting file pointer\n")
	
	h = fread(&tmp, 1, 1, fp);
	if(h != 1) PCX_RETURN_ERROR("Error reading from file\n")
	
	if(tmp == 12)
	{
	    //fprintf(stderr, "Reading palette\n");
	    //check the first color is black
	    h = fread(&palette[0].red,   1, 1, fp);
	    if(h != 1) PCX_RETURN_ERROR("Error reading palette information\n")
	    h = fread(&palette[0].green, 1, 1, fp);
	    if(h != 1) PCX_RETURN_ERROR("Error reading palette information\n")
	    h = fread(&palette[0].blue,  1, 1, fp);
	    if(h != 1) PCX_RETURN_ERROR("Error reading palette information\n")
	    if(palette[0].red == 0 && palette[0].blue == 0 && palette[0].green == 0)
	    {
	      for(h = 1; h < 256; h++)
	      {
		if(fread(&palette[h].red,   1, 1, fp) != 1)
		  PCX_RETURN_ERROR("Error reading palette information\n")
		if(fread(&palette[h].green, 1, 1, fp) != 1)
		  PCX_RETURN_ERROR("Error reading palette information\n")
		if(fread(&palette[h].blue,  1, 1, fp) != 1)
		  PCX_RETURN_ERROR("Error reading palette information\n")
	      }
	      PALETTE_DEFINED = 1;
	      //file_size -= 769; //exclude palette data from image data
	    }
	}
	if(fseek(fp, pos2, SEEK_SET) != 0)
	  PCX_RETURN_ERROR("Error setting file pointer\n")
	
	line_bytes = PCX_HEADER.bpl*PCX_HEADER.planes;
	image_w = (PCX_HEADER.xend-PCX_HEADER.xstart)+1;
	image_h = (PCX_HEADER.yend-PCX_HEADER.ystart)+1;
	bitmap_size = image_w*image_h*3;
	padding = (line_bytes*(8/PCX_HEADER.bpp))-(image_w*PCX_HEADER.planes);
	//fprintf(stderr, "padding=%d, size=%d\n", padding, bitmap_size);
	pos = 0; count = 0;
	if(PCX_HEADER.planes != 1)
	  plane = 0;	//0=RED 1=GREEN 2=BLUE 3=ALPHA 4=SINGLE PLANE
	else plane = 4;
	
	while(pos < bitmap_size)
	{
	  h = fread(&value, 1, 1, fp);
	  //fprintf(stderr, "size=%d, pos=%d\n", bitmap_size, pos);
	  if(h != 1) value = 0;
	    //PCX_RETURN_ERROR("Error reading pixel value from file\n")
	  if((value & FLAG_MASK) == FLAG_MASK)	//RLE code
	  {
	    repeat = (value & REVERSE_MASK);
	    h = fread(&value, 1, 1, fp);
	    if(h != 1) PCX_RETURN_ERROR("Error reading pixel value from file\n")
	    for(h = 0; h < repeat; h++)
	    {
	     switch(PCX_HEADER.bpp)
	     {
	     /////////////////////////////////////
	     //8 bits per pixel
	     ////////////////////////////////////
	     case(8):
	      if(plane == 0)      *(rgb+pos  ) = value;	//red
	      else if(plane == 1) *(rgb+pos+1) = value;	//green
	      else if(plane == 2) *(rgb+pos+2) = value;	//blue
	      else if(plane == 3) ;			//alpha-not used
	      else if(plane == 4)
	      {
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos  ) = PCX_HEADER.colormap[value].red;
		  *(rgb+pos+1) = PCX_HEADER.colormap[value].green;
		  *(rgb+pos+2) = PCX_HEADER.colormap[value].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos  ) = palette[value].red;
		  *(rgb+pos+1) = palette[value].green;
		  *(rgb+pos+2) = palette[value].blue;
		}
		else
		{
		  *(rgb+pos  ) = value;
		  *(rgb+pos+1) = value;
		  *(rgb+pos+2) = value;
		}
	      }
	      pos+=3; count++;
	      if(count >= image_w)
	      {
		if(plane < PCX_HEADER.planes-1)
		{
		  plane++; pos -= (image_w*3);
		}
		else if(plane == PCX_HEADER.planes-1)
		{
		  if(pos >= bitmap_size) break;
		  if(padding)
		    h += padding;
		  plane = 0;
		}
		else 
		{
		  //fprintf(stderr, "h=%d, repeat=%d, pos=%d, count=%d\n", h, repeat, pos, count);
		  if(pos >= bitmap_size) break;
		  if(padding)
		    h += padding;
		}
		count = 0;
	      }
	      break;
	     /////////////////////////////////////
	     //4 bits per pixel
	     ////////////////////////////////////
	     case(4):
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos  ) = PCX_HEADER.colormap[value&240].red;
		  *(rgb+pos+1) = PCX_HEADER.colormap[value&240].green;
		  *(rgb+pos+2) = PCX_HEADER.colormap[value&240].blue;
		  *(rgb+pos+3) = PCX_HEADER.colormap[value&15].red;
		  *(rgb+pos+4) = PCX_HEADER.colormap[value&15].green;
		  *(rgb+pos+5) = PCX_HEADER.colormap[value&15].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos  ) = palette[value&240].red;
		  *(rgb+pos+1) = palette[value&240].green;
		  *(rgb+pos+2) = palette[value&240].blue;
		  *(rgb+pos+3) = palette[value&15].red;
		  *(rgb+pos+4) = palette[value&15].green;
		  *(rgb+pos+5) = palette[value&15].blue;
		}
		pos+=6; count+=2;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  if(padding)
		    h += padding;
		  count = 0;
		}
		break;
	     /////////////////////////////////////
	     //2 bits per pixel
	     ////////////////////////////////////
	     case(2):
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos   ) = PCX_HEADER.colormap[value&192].red;
		  *(rgb+pos+1 ) = PCX_HEADER.colormap[value&192].green;
		  *(rgb+pos+2 ) = PCX_HEADER.colormap[value&192].blue;
		  *(rgb+pos+3 ) = PCX_HEADER.colormap[value&48].red;
		  *(rgb+pos+4 ) = PCX_HEADER.colormap[value&48].green;
		  *(rgb+pos+5 ) = PCX_HEADER.colormap[value&48].blue;
		  *(rgb+pos+6 ) = PCX_HEADER.colormap[value&12].red;
		  *(rgb+pos+7 ) = PCX_HEADER.colormap[value&12].green;
		  *(rgb+pos+8 ) = PCX_HEADER.colormap[value&12].blue;
		  *(rgb+pos+9 ) = PCX_HEADER.colormap[value&3].red;
		  *(rgb+pos+10) = PCX_HEADER.colormap[value&3].green;
		  *(rgb+pos+11) = PCX_HEADER.colormap[value&3].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos   ) = palette[value&192].red;
		  *(rgb+pos+1 ) = palette[value&192].green;
		  *(rgb+pos+2 ) = palette[value&192].blue;
		  *(rgb+pos+3 ) = palette[value&48].red;
		  *(rgb+pos+4 ) = palette[value&48].green;
		  *(rgb+pos+5 ) = palette[value&48].blue;
		  *(rgb+pos+6 ) = palette[value&12].red;
		  *(rgb+pos+7 ) = palette[value&12].green;
		  *(rgb+pos+8 ) = palette[value&12].blue;
		  *(rgb+pos+9 ) = palette[value&3].red;
		  *(rgb+pos+10) = palette[value&3].green;
		  *(rgb+pos+11) = palette[value&3].blue;
		}
		pos+=12; count+=4;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  if(padding)
		    h += padding;
		  count = 0;
		}
		break;
	     /////////////////////////////////////
	     //1 bit per pixel
	     ////////////////////////////////////
	     case(1):
		*(rgb+pos   ) = (value & 128) ? 255 : 0;	//pixel 1 RED
		*(rgb+pos+3 ) = (value &  64) ? 255 : 0;	//pixel 2 RED
		*(rgb+pos+6 ) = (value &  32) ? 255 : 0;	//...
		*(rgb+pos+9 ) = (value &  16) ? 255 : 0;
		*(rgb+pos+12) = (value &   8) ? 255 : 0;
		*(rgb+pos+15) = (value &   4) ? 255 : 0;
		*(rgb+pos+18) = (value &   2) ? 255 : 0;
		*(rgb+pos+21) = (value &   1) ? 255 : 0;
		*(rgb+pos+1 ) = *(rgb+pos   );			//pixel 1 GREEN
		*(rgb+pos+4 ) = *(rgb+pos+3 );			//pixel 2 GREEN
		*(rgb+pos+7 ) = *(rgb+pos+6 );			//...
		*(rgb+pos+10) = *(rgb+pos+9 );
		*(rgb+pos+13) = *(rgb+pos+12);
		*(rgb+pos+16) = *(rgb+pos+15);
		*(rgb+pos+19) = *(rgb+pos+18);
		*(rgb+pos+22) = *(rgb+pos+21);
		*(rgb+pos+2 ) = *(rgb+pos   );			//pixel 1 BLUE
		*(rgb+pos+5 ) = *(rgb+pos+3 );			//pixel 2 BLUE
		*(rgb+pos+8 ) = *(rgb+pos+6 );			//...
		*(rgb+pos+11) = *(rgb+pos+9 );
		*(rgb+pos+14) = *(rgb+pos+12);
		*(rgb+pos+17) = *(rgb+pos+15);
		*(rgb+pos+20) = *(rgb+pos+18);
		*(rgb+pos+23) = *(rgb+pos+21);
		pos+=24; count+=8;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  if(padding)
		    h += padding;
		  count = 0;
		}
		break;
	     }
	    }//end for
	  }
	  else
	  {
	     switch(PCX_HEADER.bpp)
	     {
	     /////////////////////////////////////
	     //8 bits per pixel
	     ////////////////////////////////////
	     case(8):
	      if(plane == 0)      *(rgb+pos  ) = value;	//red
	      else if(plane == 1) *(rgb+pos+1) = value;	//green
	      else if(plane == 2) *(rgb+pos+2) = value;	//blue
	      else if(plane == 3) ;			//alpha - not used
	      else if(plane == 4)
	      {
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos  ) = PCX_HEADER.colormap[value].red;
		  *(rgb+pos+1) = PCX_HEADER.colormap[value].green;
		  *(rgb+pos+2) = PCX_HEADER.colormap[value].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos  ) = palette[value].red;
		  *(rgb+pos+1) = palette[value].green;
		  *(rgb+pos+2) = palette[value].blue;
		}
		else
		{
		  *(rgb+pos  ) = value;
		  *(rgb+pos+1) = value;
		  *(rgb+pos+2) = value;
		}
	      }
	      pos+=3; count++;
	      if(count >= image_w)
	      {
		if(plane < PCX_HEADER.planes-1)
		{
		  plane++; pos -= (image_w*3);
		}
		else if(plane == PCX_HEADER.planes-1)
		{
		  if(pos >= bitmap_size) break;
		  if(padding)
		    for(i = 0; i < padding; i++)
		    {
		      if(fread(&value, 1, 1, fp) != 1)
			PCX_RETURN_ERROR("Error reading from file\n")
		    }
		  plane = 0;
		}
		else 
		{
		  //fprintf(stderr, "size=%d, pos=%d, count=%d\n", bitmap_size, pos, count);
		  if(pos >= bitmap_size) break;
		  /*if(padding)
		    for(i = 0; i < padding; i++)
		    {
		      if(fread(&value, 1, 1, fp) != 1)
			PCX_RETURN_ERROR("Error reading from file\n")
		    }*/
		}
		count = 0;
	      }
	      break;
	     /////////////////////////////////////
	     //4 bits per pixel
	     ////////////////////////////////////
	     case(4):
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos  ) = PCX_HEADER.colormap[value&240].red;
		  *(rgb+pos+1) = PCX_HEADER.colormap[value&240].green;
		  *(rgb+pos+2) = PCX_HEADER.colormap[value&240].blue;
		  *(rgb+pos+3) = PCX_HEADER.colormap[value&15].red;
		  *(rgb+pos+4) = PCX_HEADER.colormap[value&15].green;
		  *(rgb+pos+5) = PCX_HEADER.colormap[value&15].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos  ) = palette[value&240].red;
		  *(rgb+pos+1) = palette[value&240].green;
		  *(rgb+pos+2) = palette[value&240].blue;
		  *(rgb+pos+3) = palette[value&15].red;
		  *(rgb+pos+4) = palette[value&15].green;
		  *(rgb+pos+5) = palette[value&15].blue;
		}
		pos+=6; count+=2;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  /*if(padding)
		    for(i = 0; i < padding; i++)
		    {
		      if(fread(&value, 1, 1, fp) != 1)
			PCX_RETURN_ERROR("Error reading from file\n")
		    }*/
		  count = 0;
		}
		break;
	     /////////////////////////////////////
	     //2 bits per pixel
	     ////////////////////////////////////
	     case(2):
		if(COLORMAP_DEFINED)
		{
		  *(rgb+pos   ) = PCX_HEADER.colormap[value&192].red;
		  *(rgb+pos+1 ) = PCX_HEADER.colormap[value&192].green;
		  *(rgb+pos+2 ) = PCX_HEADER.colormap[value&192].blue;
		  *(rgb+pos+3 ) = PCX_HEADER.colormap[value&48].red;
		  *(rgb+pos+4 ) = PCX_HEADER.colormap[value&48].green;
		  *(rgb+pos+5 ) = PCX_HEADER.colormap[value&48].blue;
		  *(rgb+pos+6 ) = PCX_HEADER.colormap[value&12].red;
		  *(rgb+pos+7 ) = PCX_HEADER.colormap[value&12].green;
		  *(rgb+pos+8 ) = PCX_HEADER.colormap[value&12].blue;
		  *(rgb+pos+9 ) = PCX_HEADER.colormap[value&3].red;
		  *(rgb+pos+10) = PCX_HEADER.colormap[value&3].green;
		  *(rgb+pos+11) = PCX_HEADER.colormap[value&3].blue;
		}
		else if(PALETTE_DEFINED)
		{
		  *(rgb+pos   ) = palette[value&192].red;
		  *(rgb+pos+1 ) = palette[value&192].green;
		  *(rgb+pos+2 ) = palette[value&192].blue;
		  *(rgb+pos+3 ) = palette[value&48].red;
		  *(rgb+pos+4 ) = palette[value&48].green;
		  *(rgb+pos+5 ) = palette[value&48].blue;
		  *(rgb+pos+6 ) = palette[value&12].red;
		  *(rgb+pos+7 ) = palette[value&12].green;
		  *(rgb+pos+8 ) = palette[value&12].blue;
		  *(rgb+pos+9 ) = palette[value&3].red;
		  *(rgb+pos+10) = palette[value&3].green;
		  *(rgb+pos+11) = palette[value&3].blue;
		}
		pos+=12; count+=4;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  /*if(padding)
		    for(i = 0; i < padding; i++)
		    {
		      if(fread(&value, 1, 1, fp) != 1)
			PCX_RETURN_ERROR("Error reading from file\n")
		    }*/
		  count = 0;
		}
		break;
	     /////////////////////////////////////
	     //1 bit per pixel
	     ////////////////////////////////////
	     case(1):
		*(rgb+pos   ) = (value & 128) ? 255 : 0;	//pixel 1 RED
		*(rgb+pos+3 ) = (value &  64) ? 255 : 0;	//pixel 2 RED
		*(rgb+pos+6 ) = (value &  32) ? 255 : 0;	//...
		*(rgb+pos+9 ) = (value &  16) ? 255 : 0;
		*(rgb+pos+12) = (value &   8) ? 255 : 0;
		*(rgb+pos+15) = (value &   4) ? 255 : 0;
		*(rgb+pos+18) = (value &   2) ? 255 : 0;
		*(rgb+pos+21) = (value &   1) ? 255 : 0;
		*(rgb+pos+1 ) = *(rgb+pos   );			//pixel 1 GREEN
		*(rgb+pos+4 ) = *(rgb+pos+3 );			//pixel 2 GREEN
		*(rgb+pos+7 ) = *(rgb+pos+6 );			//...
		*(rgb+pos+10) = *(rgb+pos+9 );
		*(rgb+pos+13) = *(rgb+pos+12);
		*(rgb+pos+16) = *(rgb+pos+15);
		*(rgb+pos+19) = *(rgb+pos+18);
		*(rgb+pos+22) = *(rgb+pos+21);
		*(rgb+pos+2 ) = *(rgb+pos   );			//pixel 1 BLUE
		*(rgb+pos+5 ) = *(rgb+pos+3 );			//pixel 2 BLUE
		*(rgb+pos+8 ) = *(rgb+pos+6 );			//...
		*(rgb+pos+11) = *(rgb+pos+9 );
		*(rgb+pos+14) = *(rgb+pos+12);
		*(rgb+pos+17) = *(rgb+pos+15);
		*(rgb+pos+20) = *(rgb+pos+18);
		*(rgb+pos+23) = *(rgb+pos+21);
		pos+=24; count+=8;
		if(count >= image_w)
		{
		  if(pos >= bitmap_size) break;
		  /*if(padding)
		    for(i = 0; i < padding; i++)
		    {
		      if(fread(&value, 1, 1, fp) != 1)
			PCX_RETURN_ERROR("Error reading from file\n")
		    }*/
		  count = 0;
		}
		break;
	     }
	  }
	}//end while

       	return PCX_ERR_NO_ERROR;
err:
	return PCX_ERR_GENERIC;
}
/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct pcx_state {
	FILE *fp;
	int w; /* image width: 1... */
	int h; /* image height: 1... */
	int np; /* number of pages: 1... */
    	size_t flen; /* file length */
	fim_byte_t*rgb; /* pixels, from upper left to lower right, line by line */
	int bytes_per_line; /* rgb has bytes_per_line bytes per line */
};

static void*
pcx_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct pcx_state *h = FIM_NULL;
    	pcx_err_t errval = PCX_ERR_GENERIC;

	h = (struct pcx_state *)fim_calloc(1,sizeof(*h));

	if(!h)
		goto oops;

    	h->fp = fp;

	if(fseek(h->fp,0,SEEK_END)!=0)
		goto oops;

    	if((h->flen=ftell(h->fp))==(unsigned)-1)
		goto oops;

	errval = pcx_load_image_info_fp(h->fp, &h->np, &i->width, &i->height);
	if(errval != PCX_ERR_NO_ERROR)
	{
		std::cerr << "Failed pcx_load_image_info_fp !\n";
 	   	goto oops;
	}

       	i->npages = 1;
	h->w = i->width;
       	h->h = i->height;
	h->bytes_per_line = i->width * 3;
	h->rgb = (fim_byte_t*)fim_malloc(i->height * h->bytes_per_line );
	if(!h->rgb)
	{
		std::cerr << "Failed fim_malloc!\n";
    		goto oops;
	}

	errval = pcx_load_image_fp(h->fp, page, h->rgb, h->bytes_per_line);
	if(errval != PCX_ERR_NO_ERROR)
	{
		std::cerr << "Failed pcx_load_image_fp!\n";
		goto oops;
	}

	return h;
oops:
	if(h)
	{
		if(h->rgb)
			fim_free(h->rgb);
		fim_free(h);
	}
	return FIM_NULL;
}

static void
pcx_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct pcx_state *h = (struct pcx_state *) data;
	int c;

	for (c=0;c<h->w;++c)
	{
		dst[c*3+0] = h->rgb[h->bytes_per_line*line+c*3+0];
		dst[c*3+1] = h->rgb[h->bytes_per_line*line+c*3+1];
		dst[c*3+2] = h->rgb[h->bytes_per_line*line+c*3+2];
	}
}

static void
pcx_done(void *data)
{
	struct pcx_state *h = (struct pcx_state *) data;

	fclose(h->fp);
	fim_free(h->rgb);
	fim_free(h);
}

struct ida_loader pcx_loader = {
/*
 * 0000000: 0a05 0108 0000 0000 ff03 5702 2c01 2c01  ..........W.,.,.
 */
    /*magic:*/ "\012",
    /*moff:*/  0,
    /*mlen:*/  1,
    /*name:*/  "pcx",
    /*init:*/  pcx_init,
    /*read:*/  pcx_read,
    /*done:*/  pcx_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&pcx_loader);
}

}
#endif /* FIM_WITH_PCX */
