#ifndef FILEGUI_H
#define FILEGUI_H

#include <fxcg/misc.h>
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/file.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <vector>
//#include <string>
//#include <algorithm>
//#include "icons.h"

//#define true 1
//#define false 0

extern const color_t folder[];
typedef struct
{
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;
} file_type_t;

typedef struct
{
	char *name;
	file_type_t info;
	char buffer[21];
} FBL_FileItem;

struct FBL_Scroller_Data {
	int start;
	int sel;
	TScrollbar sb;
	char *dlgTitle;
	int ytop, ybottom;
	// data
	int data_length;
};

typedef struct {
	int size;
	int capacity;
	FBL_FileItem** items;
} itemholder;

struct FBL_Filelist_Data {
	struct FBL_Scroller_Data* fblsd;
	int result;
	int menu_id;
	int find_handle;
	char* filter;
	char* currentpath;
	char* title;
	itemholder ih;
};
	
struct FBL_Scroller_Data* FBL_Scroller_cons(int ytop, int ybottom);
void FBL_Scroller_destr(struct FBL_Scroller_Data* fblsd);
void FBL_Scroller_render(struct FBL_Filelist_Data* fblfd);
void FBL_Scroller_key_up(struct FBL_Scroller_Data* fblsd);
void FBL_Scroller_key_down(struct FBL_Scroller_Data* fblsd);
void FBL_Scroller_bounds_check(struct FBL_Scroller_Data* fblsd);

struct FBL_Filelist_Data* FBL_Filelist_cons(char* listpath, char *filter, char* title);
void FBL_Filelist_destr(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_chdir(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_clear(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_bake(struct FBL_Filelist_Data* fblfd, FBL_FileItem *item);
void FBL_Filelist_data_render(struct FBL_Filelist_Data* fblfd, char* buffer,
                              int index, int selected, int pix_x, int pix_y);
void FBL_Filelist_render(struct FBL_Filelist_Data* fblfd);
void FBL_Scroller_key_up(struct FBL_Scroller_Data* fblsd);
void FBL_Scroller_key_down(struct FBL_Scroller_Data* fblsd);
void FBL_Filelist_key_enter(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_key_menu(struct FBL_Filelist_Data* fblfd, int x);
void FBL_Filelist_key_exit(struct FBL_Filelist_Data* fblfd);
int FBL_Filelist_isDone(struct FBL_Filelist_Data* fblfd);
FILE *FBL_Filelist_getFile(struct FBL_Filelist_Data* fblfd, char *mode);
char* FBL_Filelist_getFilename(struct FBL_Filelist_Data* fblfd, char* str, int maxlen);
void FBL_Filelist_go(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_initBackground(struct FBL_Filelist_Data* fblfd);
void FBL_Filelist_setupStatusBar(struct FBL_Filelist_Data* fblfd);

// Missing header items, not provided here.
void GetFKeyPtr( int, void* );
void FKey_Display( int, void* );

#endif //define FILEGUI_H