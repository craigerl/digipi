#include <string.h>
#include "compression.h"
#include "log.h"

int encstathuf(char *src, int srclen, char *dest, int *destlen)
{
	char *srcptr;
	char *destptr;
	int wrklen;
	int bit16;
	int bit8;
	unsigned short huffcode;
	int hufflen;
	
	if (!src || !dest) {
		do_log(L_INFO, "Huffman encode: src or dest NULL!");
		return(1);
	}
	if (srclen > 255) {
		do_log(L_INFO, "Huffman encode: srclen > 255: %d", srclen);
		return(1);
	}
	srcptr = src;
	destptr = &dest[1];
	*destlen = 0;
	wrklen = 0;
	bit8 = 0;
	*destptr = 0;
	huffcode = huffencodtab[(int)*srcptr].code;
	hufflen = huffencodtab[(int)*srcptr].len;
	bit16 = 0;
	for (;;) {
		if (huffcode & mask16tab[bit16])
			*destptr |= mask8tab[bit8];
		bit8++;
		if (bit8 > 7) {
			destptr++;
			(*destlen)++;
			if ((*destlen) >= srclen) {
				/* coding uneffective, use copy */
				memcpy(&dest[1],src,srclen);
				dest[0] = 255;
				*destlen = srclen + 1;
				return(0);
			}
			bit8 = 0;
			*destptr = 0;
		}
		bit16++;
		if (bit16 == hufflen) {
			srcptr++;
			wrklen++;
			if (wrklen == srclen) break;
			huffcode = huffencodtab[(int)*srcptr].code;
			hufflen = huffencodtab[(int)*srcptr].len;
			bit16 = 0;
		}
	}
	if (bit8 != 0) (*destlen)++;
	(*destlen)++;
	dest[0] = (char)(srclen-1);
	return(0);
}

/* The buffer in src (first byte length-1) is decoded into dest
	 (first byte length-1). If decoding is not successful, non-0 
   is returned
*/


int decstathuf(char *src, char *dest, int srclen, int *destlen)
{
	unsigned char *srcptr;
	unsigned char *destptr;
	int wrklen;
	unsigned char bitmask;
	unsigned short decod;
	unsigned short newnode;

	if (!src || !dest) return(1);

	srcptr = src;
	destptr = dest;
	*destlen = (int)((*srcptr)+1);
	srcptr++;
	if (--srclen == 0) {
		return(1);
	}
	if (*destlen == 0) {
		return(1);
	}

	if (*destlen == 256) {
		/* no compression, only copy */
		memcpy(dest,src+1,srclen);
		*destlen = (unsigned char)(srclen);
		return(0);
	}
	wrklen = 0;
	decod = 0;
	bitmask = 0x80;
	for (;;) {
		while (bitmask > 0) {
			if ((*srcptr) & bitmask) {
				newnode = huffdecodtab[decod].node2;
				if (newnode == 0) {
					*destptr = (char)huffdecodtab[decod].node1;
					destptr++;
					wrklen++;
					if (wrklen >= *destlen) break; /* decoding finished */
					decod = 0;
				}
				else decod = newnode;
			}
			else {
				newnode = huffdecodtab[decod].node1;
				if (huffdecodtab[decod].node2 == 0) {
					*destptr = (char)huffdecodtab[decod].node1;
					destptr++;
					wrklen++;
					if (wrklen >= *destlen) break; /* decoding finished */
					decod = 0;
				}
				else decod = newnode;
			}
			if (decod) bitmask = bitmask >> 1;
		}
		if (wrklen >= *destlen) break;
		bitmask = 0x80;
		srcptr++;
		if (srclen == 0) return(1);
		srclen--;
	}
	return(0);
}
