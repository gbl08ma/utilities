# Utilities add-in for the Casio Prizm (fx-CG 10 and 20)

The Utilities add-in enhances your Casio Prizm calculator by providing functionality not originally found in the OS, or by providing enhanced alternatives to existing functionality.

## Features

  - **Clock functionality** - see and set the current time and date. Multiple time and date formats supported, and you can choose to show an analog clock.

  - **Chronometer functionality** - 20 timers that can go upwards or downwards, and be paused (accuracy: +/- 7 ms)
  
  - **Calendar** - besides seeing any calendar month, you can add events on specific days (useful for storing exam dates and assignment due dates). You can choose to use the week view, which has the advantage of showing more event details. Events can be edited, deleted and copied or moved to other days. You can easily set a stopwatch to remind you about a certain event. Searching for events is also supported.
  
  - **Task list** - like calendar events, but with no set date. You can mark tasks as complete.
  
  - **Power management** - fine adjustment of the power off and backlight timeouts, as well as brightness level. Advanced users can also set the CPU clock speed (overclock or underclock).
  
  - **File manager** - more advanced than the one provided by Casio in the Memory menu, with Utilities you can move files between folders, rename files and folders, calculate the SHA-256 sum of files and view text ones. Long file names supported. Users willing to take some risk can also copy files (still a unstable feature).
  
  - **Lantern** - turn your Prizm into a expensive lantern! Modes supported: steady light, flashing light, morse light (screen lightens up while you press a key) and color light.
  
  - **Memory usage viewer** - graphic display of the memory usage (main and storage) of the calculator.
  
  - **Function key color adjuster** - change the color of the function key labels throughout the OS.
  
  - **System information viewer** - view the OS and bootloader version and timestamps, calculator model information, Renesas CPU version registers and the unique device ID.
  
  - **Calculator lock** - "lock" the calculator with a password to avoid having curious people messing around with your calculator. Not suitable as a high-security lock (for now, it's easy to bypass). You can set Utilities to turn off the calculator after locking. And after unlocking, you can set it to open Run-Mat or ask you if you want. To return from Run-Mat back to Utilities, press Shift and Exit at the same time.
  
    - If you fear that the lock function may cause problems at some point (e.g. locking yourself out during an exam), contact gbl08ma for instructions on how to disable it.
  
  - **Add-in manager** - allows for hiding add-ins from the Main Menu without uninstalling them, when they are not necessary. Currently it is an unstable feature due to OS limitations, sometimes causing system errors.
  
  - Save the current time to the alpha/Basic variable T. This is useful for doing calculations around a certain time. Press the XoT key on the home screen to call the functionality.
  
  - Many settings to customize the behavior and looks of the add-in to your taste.
  
## And as a bonus...

  - **Weights less than 155 KiB**, saving space on your Prizm's storage memory and speeding up its transfer between calculators.
  
  - Really fast to start up - it's usually even faster than the built-in app Run-Mat!
  
  - It's open source (GitHub: [https://github.com/gbl08ma/utilities](https://github.com/gbl08ma/utilities))
  
## Installation instructions
If you use a version of Utilities older than Beta 9, please refer to "Update Instructions" instead.

To install, connect the Prizm calculator to the computer with a miniUSB<->USB cable. On the calculator, press F1 when a pop-up appears on the screen. Wait for the USB connection to be established. When it's finished, your Prizm will appear on your computer as if it were a pendisk.

Copy "utilities.g3a" to the root folder of the pendisk (i.e., out of any folders but inside the pendisk) and safely remove it. Wait for the calculator to finish "updating the Main Memory". When it does, you should notice a new Menu item, with a Clock icon, called "Utilities".

The first time you run Utilities you'll be presented with the tny. internet media group logo and then a first run wizard with some important notes about the add-in. You should definitely read it. You will then be guided to adjust the clock, something you'll have to do every time you take the batteries off. Don't worry, Utilities will remind you to do it and guide you through the process.
  
## Update instructions
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

The CPU clock adjustment tool, as well as the file copying tool, only appear when the "Show advanced tools" setting is enabled.

The Add-In manager and the file copying function (the latter is not shown by default) are still unstable and may cause system errors. If you see a System Error, you should try pressing Menu and then 1 to try to dismiss it, or you can press EXIT to reboot the calculator. 99% of the times no data is lost or serious damage is done in the event of System Errors.

## Checking for updates
Utilities, like most software, receives updates from time to time. You should check for updates to Utilities periodically, to ensure you have the greatest feature pack and the most stable version. To check for new versions, you should visit the following page:

[http://gbl08ma.com/casio-prizm-software/](http://http://gbl08ma.com/casio-prizm-software/)

If the above page isn't available, you can try checking for news at the tny. internet media website:

[http://i.tny.im/](http://i.tny.im/)

Finally, if none of these websites are available, or if you need to contact the authors, this is the email:

gbl08ma@gmail.com

## License

The add-in is available under the GNU GPL version 2. A license should have come with this read-me and the utilities.g3a file. If not, the license is available online at [https://www.gnu.org/licenses/gpl-2.0.html](https://www.gnu.org/licenses/gpl-2.0.html)

See the disclaimer at the end of this file or the messages on the about screen of the add-in (accessible from the Settings screen).

Distributing this Read-Me along with the g3a binary is not mandatory.

Any derivative work based on this software must clearly state it is not the original Utilities add-in by gbl08ma at tny. internet media.

## Last words
Utilities is the result of over one year of research, work and extensive testing. That said, we can't guarantee you will have no problems using the add-in or that everything will work as described. You are welcome to report problems to the contacts above, as well as modify the source code to your needs as long as you respect the license.

We hope you enjoy using Utilities as much as we did developing it. And if this add-in ever becomes useful, you just found a secret feature ;)

---
Copyright (C) 2013  Gabriel Maia (gbl08ma) and the tny. internet media group

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