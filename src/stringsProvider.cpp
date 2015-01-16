#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "stringsProvider.hpp"


/* copy over the next token from an input string, WITHOUT
skipping leading blanks. The token is terminated by the
first appearance of tokchar, or by the end of the source
string.

The caller must supply sufficient space in token to
receive any token, Otherwise tokens will be truncated.

Returns: a pointer past the terminating tokchar.

This will happily return an infinity of empty tokens if
called with src pointing to the end of a string. Tokens
will never include a copy of tokchar.

A better name would be "strtkn", except that is reserved
for the system namespace. Change to that at your risk.

released to Public Domain, by C.B. Falconer.
Published 2006-02-20. Attribution appreciated.
Modified by gbl08ma not to skip blanks at the beginning.
*/

unsigned char *toksplit(unsigned char *src, /* Source of tokens */
char tokchar, /* token delimiting char */
unsigned char *token, /* receiver of parsed token */
int lgh) /* length token can receive */
/* not including final '\0' */
{
  if (src) {
    while (*src && (tokchar != *src)) {
      if (lgh) {
        *token++ = *src;
        --lgh;
      }
      src++;
    }
    if (*src && (tokchar == *src)) src++;
  }
  *token = '\0';
  return src;
} /* toksplit */


int EndsIWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    //return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
    return strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// not really for strings, but anyway:
// based on http://dsss.be/w/c:memmem
// added case-insensitive functionality
void* memmem(char* haystack, int hlen, char* needle, int nlen, int matchCase) {
  if (nlen > hlen) return 0;
  int i,j=0;
  switch(nlen) { // we have a few specialized compares for certain needle sizes
  case 0: // no needle? just give the haystack
    return haystack;
  case 1: // just use memchr for 1-byte needle
    if(matchCase) return memchr(haystack, needle[0], hlen);
    else {
      void* lc = memchr(haystack, tolower(needle[0]), hlen);
      if(lc!=NULL) return lc;
      else return memchr(haystack, toupper(needle[0]), hlen);
    }
  default: // generic compare for any other needle size
    // walk i through the haystack, matching j as long as needle[j] matches haystack[i]
    for (i=0; i<hlen-nlen+1; i++) {
      if (matchCase ? haystack[i]==needle[j] : tolower(haystack[i])==tolower(needle[j])) {
        if (j==nlen-1) { // end of needle and it all matched?  win.
          return haystack+i-j;
        } else { // keep advancing j (and i, implicitly)
          j++;
        }
      } else { // no match, rewind i the length of the failed match (j), and reset j
        i-=j;
        j=0;
      }
    }
  }
  return NULL;
}

// convert a normal text string into a multibyte one where letters become their mini variants (F5 screen of the OS's character select dialog)
// dest must be at least double the size of orig.
void stringToMini(char* dest, char* orig) {
  int len = strlen(orig);
  int dlen = 0;
  for (int i = 0; i < len; i++) {
    if((orig[i] >= 65 && orig[i] <= 90) || (orig[i] >= 97 && orig[i] <= 122)) { // A-Z a-z
      dest[dlen++] = '\xe7';
      dest[dlen++] = orig[i];
    } else if((orig[i] >= 48 && orig[i] <= 57)) { // 0-9
      dest[dlen++] = '\xe5';
      dest[dlen++] = orig[i]-48+208;
    } else if(orig[i] == '+') {
      dest[dlen++] = '\xe5';
      dest[dlen++] = '\xdb';
    } else dest[dlen++] = orig[i];
  }
  dest[dlen] = '\0';
}