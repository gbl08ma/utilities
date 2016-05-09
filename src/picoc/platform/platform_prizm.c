#include "../picoc.h"
#include "../interpreter.h"

void* calloc(int num, int size) {
    void* p = malloc(num*size);
    if(!p) return NULL;
    memset(p, 0, num*size);
    return p;
}

/* deallocate any storage */
void PlatformCleanup()
{
}

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
    // XXX - unimplemented so far
    return NULL;
}

/* get a character of interactive input */
int PlatformGetCharacter()
{
    // XXX - unimplemented so far
    return 0;
}

char* curoutptr = (char*)0xE5200000;
/* write a character to the console */
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    if(curoutptr < (char*)0xE5200FFF) {
        *curoutptr = (char)OutCh;
        curoutptr++;
    }
}

/* mark where to end the program for platforms which require this */
jmp_buf PicocExitBuf;

/* exit the program */
void PlatformExit(int RetVal)
{
    PicocExitValue = RetVal;
    longjmp(PicocExitBuf, 1);
}

/* Utilities - PicoC output glue code */

char* PlatformGetOutputPointer() {
    return (char*)0xE5200000;
}

char* PlatformGetOutputCursor() {
    return curoutptr;
}

void PlatformClearOutput() {
    curoutptr = (char*)0xE5200000;
}