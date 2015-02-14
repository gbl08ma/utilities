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
static unsigned insertChar(char*start,char*pos,unsigned ln,char c){
	memmove(pos+1,pos,ln-(pos-start)+1);
	*pos=c;
	return ln+1;
}
static unsigned insertShort(char*start,char*pos,unsigned ln,unsigned short c){
	memmove(pos+2,pos,ln-(pos-start)+1);
	pos[0]=c>>8;
	pos[1]=c&255;
	return ln+2;
}
static unsigned removeShort(char*start,char*pos,unsigned ln){
	unsigned len;
	len=ln-(pos-start);
	memmove(pos,pos+2,len);
	pos[len]=0;
	return ln-2;
}
static unsigned removeChar(char*start,char*pos,unsigned ln){
	unsigned len;
	len=ln-(pos-start)+1;
	memmove(pos,pos+1,len);
	pos[len]=0;
	return ln-1;
}
static char*prevLn(char*sh,char*start){
	//If already on newline character skip it.
	for(unsigned i=0;i<3;++i){
		while(*sh){
			if(*(sh-1)&128)
				sh-=2;//Multi byte characters will never be a newline so skip completly
			if(*sh--=='\n')
				break;
			if(sh<=start)
				return start;
		}
	}
	return sh+2;//Should return one character after newline is plus two as sh is always subtracted by one per loop.
}
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
		int x,y;
		unsigned ln=strlen(sText),large=0,mx=216-40;
		char*pos=sText,*posMax=sText+TEXT_BUFFER_SIZE-1,*posShow=sText,*nextLn=pos,*last=pos;
		for(;;){
			for(unsigned i=192*384/2,*v=(unsigned*)0xA8004800;i--;*v++=0xFFFFFFFF);
			char*sh=posShow;
			if(posShow>pos)
				pos=posShow;
			//Now draw it.
			nextLn=posShow;
			if(large){
				char tmp[6];
				tmp[0]=tmp[1]=' ';
				tmp[3]=tmp[4]=tmp[5]=0;
				x=y=1;
				while((*sh)&&(sh<=posMax)){
					unsigned nl=0;
					if((*sh)&128){
						tmp[2]=*sh++;
						tmp[3]=*sh++;
						if(sh==(pos+2))
							PrintXY(x,y,tmp,1,0);
						else
							PrintXY(x,y,tmp,0,0);
						tmp[3]=0;
					}else{
						if(sh[0]=='\r'){
							if(sh[1]=='\n')
								++sh;
							nl=1;
						}
						if(sh[0]=='\n'||nl){
							nl=1;
							++sh;
							if(sh==(pos+1)){
								tmp[2]=' ';
								PrintXY(x,y,tmp,1,0);
							}
							x=1;
							++y;
							if(nextLn==posShow)
								nextLn=sh;
						}else{
							tmp[2]=*sh++;
							if(sh==(pos+1))
								PrintXY(x,y,tmp,1,0);
							else
								PrintXY(x,y,tmp,0,0);
						}
					}
					if(!nl){
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
			}else{
				char tmp[4];
				__builtin_memset(tmp,0,sizeof(tmp));//Should treat as writting to integer
				x=0;
				y=24;
				while((*sh)&&(sh<=posMax)){
					unsigned nl=0;
					if((*sh)&128){
						tmp[0]=*sh++;
						tmp[1]=*sh++;
						unsigned flags=0x40;
						if(sh==(pos+2))
							flags|=0x4;
						PrintMini(&x,&y,tmp,flags,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
						tmp[1]=0;
					}else{
						if(sh[0]=='\r'){
							if(sh[1]=='\n')
								++sh;
							nl=1;
						}
						if(sh[0]=='\n'||nl){
							nl=1;
							++sh;
							if(sh==(pos+1)){
								tmp[0]=' ';
								PrintMini(&x,&y,tmp,0x44,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
							}
							x=0;
							y+=16;
							if(nextLn==posShow)
								nextLn=sh;
						}else{
							tmp[0]=*sh++;
							unsigned flags=0x40;
							if(sh==(pos+1))
								flags|=0x4;
							PrintMini(&x,&y,tmp,flags,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
						}
					}
					if(!nl){
						if(x>384-14){
							x=0;
							y+=16;
							if(nextLn==posShow)
								nextLn=sh;
						}
						if(y>216-40)
							break;
					}
				}
			}
			last=sh;
			if(!((pos-sText)-ln)){
				if(large)
					PrintXY(x,y,"   ",1,0);
				else
					PrintMini(&x,&y," ",0x44,0xFFFFFFFF,0,0,0,0xFFFF,1,0);
			}
			drawFkeyLabels(0x302, 0, 0, 0x02A1, 0x0307); // CHAR, A<>a
skipRedraw:
			int keyflag=GetSetupSetting((unsigned int)0x14),key;
			GetKey(&key);
			if (GetSetupSetting( (unsigned int)0x14) == 0x01 || GetSetupSetting( (unsigned int)0x14) == 0x04 || GetSetupSetting( (unsigned int)0x14) == 0x84) {
				keyflag = GetSetupSetting( (unsigned int)0x14); //make sure the flag we're using is the updated one.
				//we can't update always because that way alpha-not-lock will cancel when F5 is pressed.
			}
			if(key==KEY_CTRL_EXIT)
				return; // user aborted
			else if(key==KEY_CTRL_F1)
				break;//Save file
			else if(key==KEY_CTRL_F2){
				large=0;
				mx=216-40;
			}else if(key==KEY_CTRL_F3){
				large=1;
				mx=7;
			}else if(key==KEY_CTRL_F4){
				Bkey_ClrAllFlags();
				unsigned short character=CharacterSelectDialog();
				if(character){
					if(pos>last&&(y>=mx))
						posShow=nextLn;
					if(character>=128){
						ln=insertShort(sText,pos,ln,character);
						pos+=2;
					}else
						ln=insertChar(sText,pos++,ln,character);
				}
			}else if(key==KEY_CTRL_F5){
				// switch between lower and upper-case alpha
				switch(keyflag){
					case 0x08:
					case 0x88:
						SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
						goto skipRedraw; //do not process the key, because otherwise we will leave alpha status
					case 0x04:
					case 0x84:
						SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
						goto skipRedraw; //do not process the key, because otherwise we will leave alpha status
				}
			}else if(key==KEY_CTRL_ALPHA){
				goto skipRedraw;
			}else if(key==KEY_CTRL_EXE){
				if(pos>last&&(y>=mx))
					posShow=nextLn;
				ln=insertChar(sText,pos++,ln,'\n');
			}else if(key==KEY_CTRL_DEL){
				if(pos>last&&(y>=mx))
					posShow=nextLn;
				if(ln){
					if(*(pos-2)&128){
						pos-=2;
						ln=removeShort(sText,pos,ln);
					}else
						ln=removeChar(sText,--pos,ln);
				}
			}else if(key==KEY_CTRL_LEFT){
				if(pos>sText){
					if(*(pos-2)&128)
						pos-=2;
					else
						--pos;
					if(pos<posShow){
						//Now find start of line
						posShow=prevLn(posShow,sText);
					}
				}
			}else if(key==KEY_CTRL_RIGHT){
				if(*pos&128)
					pos+=2;
				else if(*pos)
					++pos;
				if(pos>last)
					posShow=nextLn;
			}else if(key==KEY_CTRL_UP){
				pos=posShow=prevLn(posShow,sText);
			}else if(key==KEY_CTRL_DOWN){
				pos=posShow=nextLn;
			}else if((key&32768)&&key){
				if(pos<posMax){
					if(pos>last)
						posShow=nextLn;
					ln=insertShort(sText,pos,ln,key);
					pos+=2;
				}
			}else if((key<=127)&&key){
				if(pos<posMax){
					if(pos>last)
						posShow=nextLn;
					ln=insertChar(sText,pos++,ln,key);
				}
			}
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
