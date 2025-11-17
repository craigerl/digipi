/* @(#) $Id: md5_ampr.c,v 1.1.1.1 2002/02/17 19:52:57 dl9sau Exp $
 *
 * the idea for this routine came from linkt / tnt / dpbox
 * modified 2001 by Thomas Osterried <dl9sau>
 */

#include <stdio.h>
#include <string.h>

#include "md5.h"

void calc_md5_pw (const char *MD5prompt, const char *MD5pw, char *MD5result)
{
  MD5_CTX context;
  short i, n, len;
  char buffer[1024];

  strncpy(buffer, MD5prompt, 10);
  buffer[10] = 0;
  strcat(buffer, MD5pw);

  MD5Init(&context);

  len = strlen(buffer);
  for (i= 0; i < len; i += 16) {
    n = (len - i) > 16 ? 16 : (len - i);
    MD5Update(&context, buffer+i, n);
  }

  MD5Final(&context);

  MD5result[0] = '\0';
  for (i = 0; i < 16; i++) {
    MD5result[i] = context.digest[i];
  }
}
