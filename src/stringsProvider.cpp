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
#include "graphicsProvider.hpp"

int isMBfirst(char s) { // there is probably a syscall for this, but it's not documented
  return (s == '\xE5' || s == '\xE6' || s == '\xE7' || s == '\x7F' || s == '\xF7' || s == '\xF9');
}
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
Modified by gbl08ma not to skip blanks at the beginning,
and to not break on a token if it is the body of a multi-byte char.
*/

const char *toksplit(const char *src, /* Source of tokens */
const char tokchar, /* token delimiting char */
char *token, /* receiver of parsed token */
int lgh) /* length token can receive */
/* not including final '\0' */
{
  int prevWasMBLead = 0;
  if (src) {
    while (*src && (tokchar != *src || prevWasMBLead)) {
      if (lgh) {
        *token++ = *src;
        --lgh;
      }
      if(isMBfirst(*src)) prevWasMBLead = 1;
      else prevWasMBLead = 0;
      src++;
    }
    if (*src && (tokchar == *src)) src++;
  }
  *token = '\0';
  return src;
} /* toksplit */


int strEndsWith(const char *str, const char *suffix)
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
void* memmem(char* haystack, int hlen, const char* needle, int nlen, int matchCase) {
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

// strncpy_retlen works like strncpy, but returns the length copied to the string.
int strncpy_retlen(char* dest, const char* src, int n) {
  int i;
  for (i = 0; i < n; i++) {
    if(src[i]) dest[i] = src[i];
    else break;
  }
  int l = i;
  for (; i < n; i++) dest[i] = 0;
  return (l > n ? n : l);
}

// strncpy_replace works like strncpy, but replaces all occurences of char a for char b, unless a
// is the second half of a MB char.
char* strncpy_replace(char* dest, const char* src, int n, char a, char b) {
  int i;
  for (i = 0; i < n; i++) {
    if(src[i]) {
      if(src[i] != a || (i > 0 && isMBfirst(src[i-1])))
        dest[i] = src[i];
      else dest[i] = b;
    }
    else break;
  }
  for (; i < n; i++) dest[i] = 0;
  return dest;
}


// convert a normal text string into a multibyte one where letters become their mini variants
// (F5 screen of the OS's character select dialog)
// dest must be at least double the size of orig.
void stringToMini(char* dest, const char* orig) {
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

int base32_decode(const unsigned char *encoded, unsigned char *result, int size) {
  // assumes encoded value is perfect (no whitespace, invalid chars, etc.)
  // further assumes all letters are upper-case
  int bytes_buffer = 0, bits_count = 0, i = 0;
  for(; i < size && *encoded; encoded++) {
    unsigned char cur_bits;
    if(isupper(*encoded)) {
      cur_bits = *encoded - 'A';
    } else if(*encoded > '1' && *encoded < '8') {
      cur_bits = *encoded - '2' + 26;
    } else return -1;
    // add new bits to end of queue:
    bytes_buffer <<= 5;
    bytes_buffer |= cur_bits;
    bits_count += 5;
    if(bits_count >= 8) {
      // we have enough to fill another byte of result
      bits_count -= 8;
      result[i++] = bytes_buffer >> bits_count;
    }
  }
  if (i < size) {
    result[i] = 0;
  }
  return i;
}

int base32_validate(const char* string) {
  // returns 1 if the provided string is valid base32
  // returns 0 if not
  for(; *string; string++) {
    if(!(isupper(*string) || (*string > '1' && *string < '8'))) return 0;
  }
  return 1;
}

void itoa_zeropad(int n, char* buffer, int digits) {
  // pad with zeros:
  char* ptr = buffer;
  int tens = ipow(10,--digits);
  for(; digits; digits--) {
    if(n < tens) {
      *ptr = '0';
      ptr++;
    }
    tens /= 10;
  }
  itoa(n, (unsigned char*)ptr);
}