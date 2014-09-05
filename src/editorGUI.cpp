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

#include "editorGUI.hpp"
#include "menuGUI.hpp"
#include "textGUI.hpp"
#include "inputGUI.hpp"
#include "settingsProvider.hpp"
#include "keyboardProvider.hpp"
#include "hardwareProvider.hpp"
#include "graphicsProvider.hpp"
#include "selectorGUI.hpp" 
#include "fileProvider.hpp"

void fileTextEditor(char* filename, char* basefolder) {
	int newfile = (filename == NULL);
	char sText[TEXT_BUFFER_SIZE];
	sText[0]=0;
	if(!newfile) {
		newfile = 0;
		int openerror = 0;
		unsigned short pFile[MAX_FILENAME_SIZE];
		Bfile_StrToName_ncpy(pFile, filename, MAX_FILENAME_SIZE); 
		int hFile = Bfile_OpenFile_OS(pFile, READWRITE, 0); // Get handle
		if(hFile >= 0) // Check if it opened
		{ //opened
			unsigned int filesize = Bfile_GetFileSize_OS(hFile);
			if(!filesize || filesize > TEXT_BUFFER_SIZE) {
				openerror = 1;
			} else {
				Bfile_ReadFile_OS(hFile, sText, filesize, 0);
				sText[filesize]=0;//Ensure NULL termination.
			}
			Bfile_CloseFile_OS(hFile);
		} else {
			openerror = 1;
		}
		if(openerror) {
			//Error opening file, abort
			mMsgBoxPush(4);
			mPrintXY(3, 2, (char*)"Error opening", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
			mPrintXY(3, 3, (char*)"file to edit.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
			closeMsgBox();
			return;
		}
	}
	while(1){
		unsigned ln=strlen(sText),large=0;
		char*pos=sText,*posMax=sText+TEXT_BUFFER_SIZE-1,*posShow=sText,*nextLn=pos,*last=pos,*endc=pos+ln;
		Bdisp_AllClr_VRAM();
		for(;;){
			drawFkeyLabels(0x302, 0, 0,  0x02A1, 0x0307); // CHAR, A<>a
			int key,x,y;
			GetKey(&key);
			Bdisp_AllClr_VRAM();
			char*sh=posShow;
			if(key==KEY_CTRL_EXIT)
				return; // user aborted
			else if(key==KEY_CTRL_F1)
				break;//Save file
			else if(key==KEY_CTRL_F2)
				large=0;
			else if(key==KEY_CTRL_F3)
				large=1;
			else if(key==KEY_CTRL_EXE){
				if(pos>last)
					posShow=nextLn;
				*pos++='\n';
				if(pos>endc){
					*pos=0;
					endc=pos;
				}
			}else if(key==KEY_CTRL_LEFT){
				if(pos>sText){
					--pos;
					if(pos<posShow){
						posShow=pos;
						//Now find start of line
						sh=posShow;
						while((*sh)&&(sh>=sText)){
							if(*sh=='\r'){
								++sh;
								if(*sh=='\n'){
									++sh;
									break;
								}
								break;
							}
							if(*sh=='\n'){
								++sh;
								break;
							}
							--sh;
						}
						++sh;
						posShow=sh;

					}
				}
			}else if(key==KEY_CTRL_RIGHT){
				if(pos[1])
					++pos;
				if(pos>last)
					posShow=nextLn;
			}else if(key<=255){
				if(pos<posMax){
					if(pos>last)
						posShow=nextLn;
					*pos++=key;
					if(pos>endc){
						*pos=0;
						endc=pos;
					}
				}
			}
			//Now draw it.
			nextLn=posShow;
			if(large){//18x24
				char tmp[4];
				tmp[0]=tmp[1]=' ';
				tmp[3]=0;
				x=y=1;
				while((*sh)&&(sh<=posMax)){
					unsigned nl=0;
					if(sh[0]=='\r'){
						if(sh[1]=='\n')
							++sh;
						nl=1;
					}
					if(sh[0]=='\n'||nl){
						x=1;
						++y;
						++sh;
						if(nextLn==posShow)
							nextLn=sh;
						if(sh==(pos+1)){
							tmp[2]=' ';
							PrintXY(x,y,tmp,1,0);
						}
					}else{
						tmp[2]=*sh++;
						if(sh==(pos+1))
							PrintXY(x,y,tmp,1,0);
						else
							PrintXY(x,y,tmp,0,0);
						if(y>=7){
							break;
						}
						if(x>=21){
							x=0;
							++y;
							if(nextLn==posShow)
								nextLn=sh;
						}
						++x;
					}
				}
			}else{//14x16
				char tmp[2];
				tmp[1]=0;
				x=y=0;
				while((*sh)&&(sh<=posMax)){
					unsigned nl=0;
					if(sh[0]=='\r'){
						if(sh[1]=='\n')
							++sh;
						nl=1;
					}
					if(sh[0]=='\n'||nl){
						x=0;
						y+=16;
						++sh;
						if(nextLn==posShow)
							nextLn=sh;
						if(sh==(pos+1)){
							tmp[0]=' ';
							PrintMini(&x,&y,tmp,0x44,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
						}
					}else{
						tmp[0]=*sh++;
						unsigned flags=0x40;
						if(sh==(pos+1))
							flags|=0x4;
						PrintMini(&x,&y,tmp,flags,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
						if(x>384-14){
							x=0;
							y+=16;
							if(nextLn==posShow)
								nextLn=sh;
						}
						if(y>216-40){
							break;
						}
					}
				}
			}
			last=sh;
		}
		int backToEditor = 0;
		unsigned short newfilenameshort[0x10A];
		if(newfile) {
			SetBackGround(13);
			drawScreenTitle((char*)"Text Editor", (char*)"Save file as:");
			drawFkeyLabels(0x036F); // <
			textInput ninput;
			ninput.forcetext=1;
			ninput.charlimit=MAX_NAME_SIZE;
			char nfilename[MAX_NAME_SIZE] = "";
			ninput.buffer = (char*)nfilename;
			while(1) {
				ninput.key = 0;
				int nres = doTextInput(&ninput);
				if (nres==INPUT_RETURN_EXIT || (nres==INPUT_RETURN_KEYCODE && ninput.key==KEY_CTRL_F1)) {
					// user aborted
					backToEditor = 1;
					break;
				} else if (nres==INPUT_RETURN_CONFIRM) {
					if(stringEndsInG3A(nfilename)) {
						mMsgBoxPush(4);
						mPrintXY(3, 2, (char*)"g3a files can't", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
						mPrintXY(3, 3, (char*)"be created by", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
						mPrintXY(3, 4, (char*)"an add-in.", TEXT_MODE_TRANSPARENT_BACKGROUND, TEXT_COLOR_BLACK);
						closeMsgBox();
					} else {
						// create and save file
						char newfilename[MAX_FILENAME_SIZE];
						strcpy(newfilename, basefolder);
						strcat(newfilename, nfilename);
						Bfile_StrToName_ncpy(newfilenameshort, newfilename, 0x10A);
						break;
					}
				}
			}
			if(backToEditor) continue;
		} else {
			// delete, then create and save file
			Bfile_StrToName_ncpy(newfilenameshort, filename, 0x10A);
			Bfile_DeleteEntry(newfilenameshort);
		}
		size_t size = strlen(sText);
		Bfile_CreateEntry_OS(newfilenameshort, CREATEMODE_FILE, &size); //create the file

		int h = Bfile_OpenFile_OS(newfilenameshort, READWRITE, 0);
		if(h >= 0) { // Still failing?
			//Write file contents
			Bfile_WriteFile_OS(h, sText, size);
			Bfile_CloseFile_OS(h);
		}
		return;
	}
}
