# Utilities add-in for the Casio Prizm (fx-CG 10 and 20)

The Utilities add-in enhances your Casio Prizm calculator by providing functionality not originally found in the OS, or by providing enhanced alternatives to existing functionality.

## Features

  - **Clock functionality** - see and set the current time and date. Multiple time and date formats supported, and you can choose to show an analog clock.

  - **Chronometer functionality** - 20 timers that can go upwards or downwards, and be paused (accuracy: +/- 7 ms). A downwards timer can be set by specifying its duration or specifying a date and time at which it should end. 
  
  - **Calendar** - besides seeing any calendar month, you can add events on specific days (useful for storing exam dates and assignment due dates). You can choose to use the week view, which has the advantage of showing more event details. There is also functionality for counting days (subtracting dates).

    - Calendar events can be edited, deleted and copied or moved to other days. There are eight color-coded categories for the events. You can easily set a stopwatch to remind you about a certain event. Searching for events is supported and you can import events in iCalendar format with the help of an external tool. There are also tools for mass-deleting past events (to free up storage space) and to fix inconsistencies in the calendar events database, introduced by the use of old versions of Utilities.
  
  - **Task list** - like calendar events, but with no set date. You can mark tasks as complete and they have color categories, too.
  
  - **Power management** - fine adjustment of the power off and backlight timeouts, as well as brightness level.
  
  - **File manager** - more advanced than the one provided by Casio in the Memory menu, with Utilities you can move and copy files and folders, rename files and folders, calculate the SHA-256 and SHA-1 sums of files and view and edit text files. Long file names supported. You can also compress and decompress files to and from a Utilities-specific format. There's, too, a file search function that can look inside files.
  
  - **JPEG image viewer** - view images in JPEG format by opening them right from the file manager.

  - **Text editor** - very simple and does not allow for multi-line editing (you must insert line breaks manually with Shift+EXE).
  
  - **Balance manager** - manage your finances. Multiple wallets supported. You can enter a description for every transation, along with a date and time.
  
  - **Password generator** - generate passwords with between 6 and 30 characters in length. Optionally, passwords can contain numbers and symbols. You can also ask for passwords that attempt to be pronounceable. Large amounts of passwords can be generated to text files.
  
  - **TOTP authenticator** - turn your calculator into a authentication second factor, like Google Authenticator.
  
  - **Lantern** - turn your Prizm into a expensive lantern! Modes supported: steady light, flashing light, morse light (screen lightens up while you press a key) and color light.
  
  - **Memory usage viewer** - graphic display of the memory usage (main and storage) of the calculator.
  
  - **Function key color adjuster** - change the color of the function key labels throughout the OS.
  
  - **System information viewer** - view the OS and bootloader version and timestamps, calculator model information, Renesas CPU version registers and the unique device ID. You can also view the current user information (as set on the System menu and displayed during power-off) and its history, including "passwords".
  
  - **Calculator lock** - "lock" the calculator with a password to avoid having curious people messing around with your calculator. Not suitable as a high-security lock (for now, it's easy to bypass). You can set Utilities to turn off the calculator after locking. And after unlocking, you can set it to open Run-Mat or ask you if you want to. To return from Run-Mat back to Utilities, press Shift and Exit at the same time.
  
    - If you fear that the lock function may cause problems at some point (e.g. locking yourself out during an exam), contact gbl08ma for instructions on how to disable it.
  
  - **Add-in manager** - allows for hiding add-ins from the Main Menu without uninstalling them, when they are not necessary. Currently it is an unstable feature due to OS limitations, sometimes causing system errors. This functionality is only available when the advanced tools option is enabled.
  
  - **PicoC script execution** - only available in select editions, allows for executing scripts, written in an interpreted variant of C, that can use most of the functions of the OS and of Utilities.
  
  - Save the current time to the alpha/Basic variable T. This is useful for doing calculations around a certain time. Press the XoT key on the home screen to call the functionality.
  
  - Many settings to customize the behavior and looks of the add-in to your taste.
  
## And as a bonus...

  - **Weights less than 180 KiB**, saving space on your Prizm's storage memory and speeding up its transfer between calculators.  The PicoC edition has a larger file size, over 250 KiB.
  
  - Really fast to start up - it's usually even faster than the built-in app Run-Mat!
  
  - It's open source (GitHub: [https://github.com/gbl08ma/utilities](https://github.com/gbl08ma/utilities))
  
## Installation instructions
If you use a version of Utilities older than Beta 9, please refer to "Old version users - update instructions" instead.

To install, connect the Prizm calculator to the computer with a miniUSB<->USB cable. On the calculator, press F1 when a pop-up appears on the screen. Wait for the USB connection to be established. When it's finished, your Prizm will appear on your computer as if it were a pendisk.

Copy "utilities.g3a" to the root folder of the "pendisk" (i.e., out of any folders but inside the pendisk; if necessary, overwrite the existing file) and safely remove it. Wait for the calculator to finish "updating the Main Memory". When it does, you should notice a new Main Menu item, called "Utilities".

The first time you run Utilities you'll be presented the Segvault logo and then a first run wizard with some important notes about the add-in, which you should read. You will then be told to adjust the clock, something you'll have to do every time you take the batteries off. Don't worry, Utilities will remind you to do it and guide you through the process.
  
## Old version users - update instructions
(only applies if you are currently using a version Utilities older than Beta 9. If that's not the case, just follow the instalation instructions, copying utilities.g3a over the old one)
 
This bit is really, really important as there are some folder names that changed on newer releases; also, not all files are compatible (calendar events and tasks are compatible, though). You must follow the following instructions for new Utilities versions to work properly: 

  - Delete the @UTILS folder in Main Memory (Memory app in Main Menu, press F1, scroll down to @UTILS, press F1, F6 and then F1); 
  - Now connect the calculator to your computer through USB, as a flash drive, and perform the following steps: 
  - Rename the @CALNDAR folder in the storage memory of your calculator to @UTILS (you must do this on the computer because sometimes the OS won't accept the @ as a valid character); 
  - Inside the newly renamed @UTILS storage memory folder, delete the file Hash.plp in case it exists (the new Hash.plp file is no longer compatible with the ones from previous versions);
  - Copy utilties.g3a to the root folder of the calculator, overwriting the old one;
  - Safely disconnect the calculator from the computer. You are ready to use the new version once your calculator finishes updating the Main Memory...
  
## Usage instructions
Everything should be pretty much self-explanatory - Utilities is made to dispense the Read-Me :). Because of that, only less obvious things will be detailed here. To open the settings menu, press Shift+Menu while Utilities is running.

The calculator lock function will guide you through the process of setting a password, the first time you use it. You can contact gbl08ma if for some reason you wish to disable this function (be prepared to follow instructions in a very exact way).

The Add-In Manager only becomes available when the "Show advanced tools" setting is enabled. The Add-In manager is unstable because of technical limitations, and may cause system errors. If you see a System Error, you should try pressing Menu and then 1 to try to dismiss it, or you can press EXIT to reboot the calculator. Other thing that may happen with the Add-In Manager is that all add-ins will be gone from the Main Menu. If this happens, reboot the calculator to make them appear again.

For information on how to use PicoC on Utilities, please [go here](https://github.com/gbl08ma/utilities/wiki/PicoC-information).

## Checking for updates
Utilities, like most software, receives updates from time to time. Utilities includes a little function that, assuming the calculator's clock is adjusted, will remind you to check for updates after a certain date in the future. However, you do not need to wait for that message to appear. You should check for updates to Utilities periodically, to ensure you have the greatest feature pack and the most stable version. To check for new versions, you should visit the following page:

[http://gbl08ma.com/casio-prizm-software/](http://gbl08ma.com/casio-prizm-software/)

If the above page isn't available, you can try checking for news at the TNY network website:

[http://i.tny.im/](http://i.tny.im/)

Finally, if none of these websites are available, or if you need to contact the authors, this is the email:

gbl08ma@gmail.com

## Changelog
For a detailed changelog, including lots of developer anger and jargon, take a look at the commit history on the GitHub page.

Here is a summary of the changes, in no special order:

From v2.0 to v2.1:
  - Actually complete PicoC support and release PicoC edition;
  - Add support for OS 02.02 (function key color changer was broken);
  - File manager: Pressing Jump (F2) on a file from the search results now moves the selection to the file in question in the file list.

From v1.4 to v2.0:
  - Added balance manager;
  - Added password generator;
  - Added TOTP authenticator;
  - Added "home panels": one can now set a shortcut for when up/down/left/right is pressed on the clock screen;
  - Added JPEG image viewer;
  - Task list: fix bug where a system error could occur if there were no tasks;
  - File manager: allow for compressing and decompressing multiple files at once;
  - File manager: allow for sorting files by type;
  - File manager: file compression library upgraded, should be able to produce smaller files. Old files are still compatible;
  - File manager: allow up/down action on file information screen, moving to the previous/next file in the list, respectively;
  - File manager: add support for overwriting existing files when copying, moving or saving a file over an existing one;
  - File manager clipboard: tweaks to the behavior and item limit increase, from 50 to 75 items;
  - File manager: allow for inverting selection;
  - File manager: show more information in the "Folder statistics" screen;
  - File information: calculate the SHA-1 sum of files along with the SHA-256 sum;
  - Calendar: fix problems with event search;
  - Calendar: show days of week in the busy timetable of the week view;
  - Render newlines correctly in calendar events and tasks;
  - Number selectors (screens where one can select a number by pressing up/down): allow for inserting the number directly with the 0-9 keys;
  - Allow for selecting dates with the graphical calendar, in most places where dates can be entered;
  - Chronometer: added individual chronometer view, with analog dials for the time;
  - Calculator lock: display the clock and, optionally, the user information on the lock screen;
  - Improved navigation in select menus, by using the left/right keys to go back to the previous screen and to enter the current option, respectively;
  - Removed CPU speed selector, as it was obsolete when compared to separated dedicated tools;
  - Improved the inner workings of the Add-in manager;
  - Many internal changes to improve stability and functionality;
  - Many small bugs fixed and other small convenience features introduced.

From v1.3 to v1.4:
  - Show more information on date difference tool ("Count days" on the calendar OPTN menu);
  - Show reminder on how to adjust clock on DST start/end dates for Europe and USA;
  - On calendar week view, the events count for the week is now shown when there's enough space;
  - Fixed bug where chronometers set through the "Built-in" menu would not be saved;
  - Function key labels on the chronometer screen now show and hide depending on the available functions;
  - Added functionality for creating blank g3p files suitable for use on the Geometry add-in (same as saving an empty image through the Geometry add-in);
  - On file manager, the clipboard manager can now be accessed using Shift+8 (CLIP);
  - Allow for aborting file search by pressing AC/ON (you may need to hold the key down for a while);
  - Added option to dismiss "Check for updates" screen, in the case v1.4 is the last version of Utilities;
  - Minor graphical corrections on the UI;
  - Fixed small bugs and changed small details.

From v1.2 to v1.3:
  - Added setting for choosing the type of notification shown when a chronometer finishes;
  - Menu reorganized: now the text editor for a new file is accessible from the F4 menu inside the File Manager;
  - Added folder copying capability to file manager;
  - Added item sorting to file manager (can sort by name A-Z, Z-A, and by size, big-small and small-big);
  - Added file searching capability to file manager (supports filename, content and recursive search);
  - UI change: change some of the backgrounds shown;
  - Added file compression and decompression functionality (accessible from the "File information" screen in the file manager);
  - Added screen (accessible from the "System Information" screen) for seeing the current user information (with passwords) and its history;
  - Display usage of the "user" area (where the user information is stored) on the memory usage screen;
  - Added page up/down functionality to scrollable text views (press Shift then Up or Down);
  - Make scrollable text views faster (including text reader);
  - Noticeably faster calendar and tasks operation, including when searching for events;
  - Fixed file manager crash when system language was Russian;
  - Fixed many small bugs, most of them hard to notice;
  - New main menu icon (again);
  - "Promoted" the Add-In Manager to an advanced tool;
  - Many under-the-hood changes to increase speed and reduce binary size;
  - Changed many small details to improve looks and user experience.

From v1.1 to v1.2:
  - Graphical marker ("timetable") of the busy state for a calendar day or week (can be disabled in the calendar settings);
  - Show icons for the files in the file browser;
  - Added date difference tool;
  - Implement function for repairing the calendar events database, to repair mistakes introduced by versions v1.0 and below;
  - Added tool to prompt the user to repair the database when problems are found;
  - Fixed huge memory leak on event search (noticeable when user has many events stored);
  - Noticeably increased the speed of event search, especially when searching on a year or year range;
  - Higher responsiveness in the event editor and event lists (better user experience);
  - Show a empty square on text inputs where text is mandatory;
  - Better Main Memory space management: reduced space usage when no chronometer is set;
  - Added function for deleting big parts (which the user may no longer need) of the calendar database ("Trim database");
  - Changed the color scheme of the calendar month view;
  - Added "built-in" timers to the chronometer function;
  - Added function for setting a chronometer ending on a set date-time;
  - Added tool for importing calendar events in the iCalendar format;
  - Added simple text editor;
  - Fixed the file copying function and moved it out of the advanced tools category;
  - Redesigned file information screen in the file manager;
  - Added task counts on tasks list;
  - Added system to remind user to check for updates;
  - New main menu logo;
  - Fixed many small bugs, most of them hard to notice;
  - Changed many small details to improve looks and user experience.

From v1.0 to v1.1:
  - Fixed bug where event end date would not be updated when copying or moving;
  - Added setting for showing or hiding seconds in the home screen clock;
  - Added system information screen;
  - Added events home screen pane (press the left key on the home screen) and setting for disabling it;
  - Added different clock styles to home screen, including some with an analog clock;
  - Added calendar event search function;
  - Added a clipboard manager to the file manager;
  - Added functionality for changing calendar events' and tasks' category with Shift+5;
  - Use of more scrollable text areas instead of screens split in steps;
  - Validate time and date input when adding or editing events;
  - Allow for accessing the @MainMem folder with the file browser, after showing a message;
  - Added calendar week view;
  - Changed some of the messages shown in the add-in;
  - Limited dark theme to the home screen and fixed some of the bugs associated with it;
  - Changed many other small things in order to improve looks and user experience.

## License

The add-in is available under the GNU GPL version 2. A license should have come with this read-me and the utilities.g3a file. If not, the license is available online at [https://www.gnu.org/licenses/gpl-2.0.html](https://www.gnu.org/licenses/gpl-2.0.html)

See the disclaimer at the end of this file or the messages on the about screen of the add-in (accessible from the Settings screen).

Distributing this Read-Me along with the g3a binary is not mandatory.

Any derivative work based on this software must clearly state it is not the original Utilities add-in by gbl08ma at Segvault.

## Last words
Utilities is the result of over two years of research, work and extensive testing. That said, we can't guarantee you will have no problems using the add-in or that everything will work as described. You are welcome to report problems to the contacts above, as well as modify the source code to your needs as long as you respect the license.

We hope you enjoy using Utilities as much as we did developing it. And if this add-in ever becomes useful, you just found a secret feature ;)

---
Copyright (C) 2013-2016  Gabriel Maia (gbl08ma) and Segvault (http://segvault.tny.im)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.