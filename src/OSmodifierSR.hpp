#ifndef __OSMODIFIERSR_H
#define __OSMODIFIERSR_H
// OS modifier code - stay resident after add-in is closed

void strcpy_rs(char dest[], const char source[]) __attribute__((section(".rsmem.text")));
void OSmodifierTimerHandler() __attribute__((section(".rsmem.text")));

void setOSmodifier();
void setOSmodStatusBar(int color1, int color2);
void getOSmodStatusBar(int* color1, int* color2);

#endif