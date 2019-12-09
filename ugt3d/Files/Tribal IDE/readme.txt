----------------------------------
Tribal IDE
The Tribes 2/V12 Script Development Environment

Version : Beta 3.7
Date    : 08/09/2001
Author  : David Dunscombe 
email   : davdun@barrysworld.com
Website : http://tribes.barrysworld.net/ide/
----------------------------------


The following is from the GNU Public license, and is a general
disclaimer for Tribal IDE. This does not mean that Tribal IDE is
distributed under GPL.

          BECAUSE THE PROGRAM IS LICENSED FREE OF
     CHARGE, THERE IS NO WARRANTY  FOR THE
     PROGRAM, TO THE EXTENT PERMITTED BY
     APPLICABLE LAW.  EXCEPT WHEN  OTHERWISE
     STATED IN WRITING THE COPYRIGHT HOLDERS
     AND/OR OTHER PARTIES  PROVIDE THE PROGRAM
     "AS IS" WITHOUT WARRANTY OF ANY KIND,
     EITHER EXPRESSED  OR IMPLIED, INCLUDING, BUT
     NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
     MERCHANTABILITY AND FITNESS FOR A
     PARTICULAR PURPOSE.  THE ENTIRE RISK AS  TO
     THE QUALITY AND PERFORMANCE OF THE
     PROGRAM IS WITH YOU.  SHOULD THE  PROGRAM
     PROVE DEFECTIVE, YOU ASSUME THE COST OF
     ALL NECESSARY SERVICING,  REPAIR OR
     CORRECTION.

Whats New
---------
	Fixed - current file functions was not updating correctly on file save
	
	Fixed - If connection to debugging server was not disconnected via the IDE subsequant editing
		woudl generate an exception
	
	Fixed - double clicking of find in files messages would not always pick the correct file if there 
		were more than one file of the same name in the project

	Fixed - compile and exec modified will now only compile and exec files which T2/V12 already has
		running, these files have a green file icon next to them while in debugging mode. To compile
		a file that is not currently running in T2/V12 right click on in in the project view and
		click compile. If successfully compiled & execed it will have a green file icon next to it.	


Things to Do
------------
	
	Virtual folders
			

Overview
--------

What is Tribal IDE?

	Tribal IDE is an easy to use environment in which to develop scripts and modifications for 
Tribes 2 and also GarageGames V12 Engine ( www.garagegames.com ). 


Installation
------------
If your reading this you've installed it =D. If you've used the install then Tribal IDE will require 
a restart as the install updates the common controls (although if you have Internet Explorer 5 installed 
you dont have to restart).


Shortcut Keys
-------------
Comment Block   [SHIFT+ALT+C] - Add comment '//' to begining of every selected line
UnComment Block [SHIFT+ALT+U] - Remove comment '//' from begining of every selected line
Indent Block    [TAB] - Indent selected block by specified tab width
UnIndent Block  [SHIFT+TAB] - Unindent block by specified tab width

Connect/Resume Debugging [F9] - Connect to debug server, once connected this will resume.
Step Into [F7] - Step into code on line.
Step Over [F8] - Step over current line.
Toggle Breakpoint [F5] - Toggle breakpoint (if possible) at current line.

Plus typical editor ones.

Creating a project
------------------
To create a new project click on the New Project button. This shows a window with a Project Name and
Base Path edit boxes. Type in a name for your project in the space provided. The base path is used
to work out the relative path names for a script. This is needed as the debugger will send the IDE 
filenames in the format 'scripts\turret.cs' without an absolute path. So this base path needs to be 
something like 'C:\Dynamix\Tribes2\GameData\MyMod\' where mymod is similar to base (ie. you scripts 
will be in C:\Dynamix\Tribes2\GameData\MyMod\scripts\ directory). Click OK and the IDE will ask you
whether you want to to scan for .cs files in all subdirectorys from this path. 

VL2 in Project Support
----------------------
VL2 files (which are zip files) can be added to the project. The files within them can be open within
the editor as read only. If you want to edit them you will have to extract the vl2 manually and add the
files. However find in files, function browser and debugging will work transparently with the files 
within the vl2.

Function Browser
----------------
The function browser initially will parse all the files in your project for functions and the classes 
they are in. It will show them in a large list. As you save files in your project the list will 
instantly update with the functions your have added/edited or deleted. There is also a seperate list of
the functions in the active file. Also when you rigth click on a function in the editor, say for example

      %game.onAIFriendlyFire(%clVictim, %clAttacker, %damageType, %sourceObject);

if you right clicked on the on onAIFriendlyFire bit your would see on the popup menu 5 places where this 
function is defined CnHGame::onAIFriendlyFire, CTFGame::onAIFriendlyFire, etc. This can be clicked on to
jump to the relevant code.


Using the debugger
------------------
Tribes 2 has implemented a remote debugger. You can connect to it via a Tribes 2 client with debugger();
but I thought it'd be nice to have a windows app be able to do this. The first thing you need to do
is start up Tribes 2 either 

a) on another PC which is on the same network as you
b) run Tribes 2 in a window on same PC
c) run it as a dedicated server on same PC. 

For the debugging feature to work correctly you need to have a project with the same script files as 
Tribes 2 is running. Also the path for each file needs to be the same relative path as tribes 2, 
eg the path next to the script 'defaultGame.cs' in Tribal IDE project view needs to be 'scripts\'. 
The relative path for each file in a project is worked out from the base path you set when you 
created the project.

Once you have Tribes 2 is running you need to type in the following command in the Tribes 2 console to 
enable the debugger

			dbgsetparameters(PORT_NUMBER,PASSWORD);

Where PORT_NUMBER is a value such as 28040 and PASSWORD is a password that you will use to only
allow valid people to connect to the debugger. Once you have typed that in you get a message back
something like this

Binding server port to default IP

The debugger is now running. Now start Tribal IDE and go to the preferences window. Click on the 
debugger icon in the preferences window. There are three values you need to enter to into Tribal 
IDE to connect to the debugger. The first is the IP address of where the Tribes 2 running the debugger
is. If its on the same PC as the IDE type in 127.0.0.1 for the address. Now enter the port value you
entered earlier in Tribes 2 in the Server Port box. Finally enter the password you set also earlier.
Click OK to save the changes. If all things are correctly setup you should be able to click on the
PLAY button and the following messages should appear in the message window at the bottom of the IDE.

Connecting to debug server
Connected to debug server
Sending Password
Password Accepted
Requesting Server Script List

Some buttons on the toolbar will also become active. If you click on the pause button Tribes 2 will 
stop and the IDE will jump to the line that you paused on. Breakpoints can be toggled by clicking on
the little icons in the editor gutter. 

BreakPoints
-----------
Once your connected to the debugger, you may notice some little blue dots down the side of an open file.
These indicate points at which you can tell Tribes 2 to stop if it hits that line. Just click the blue dot,
the line will then turn red to indicate this is now a breakpoint. All breakpoints you set are shown in a 
breakpoint window. These can be group or changed to have certain conditions, for example when a variable
is a certain number you might to want to pause at the breakpoint. The breakpoints can be assigned into
groups and disabled/enabled.


Watches
-------
Watches can be set for variables. There are a few methods to see what value a variable has. One is to 
just move the mouse cursor over the variable, the value that variable has will appear in the status bar 
at the bottom of the IDE. Another way is to select the variable or combination of variable and method,
so for example if there was a line such as

	%turretTarg = %turret.getTarget();
   	
You could highlight %turret.getTarget() and do [CTRL+F5] and the %turret.getTarget() and its value would
appear in the watch window at the bottom. Another method is to just type in a variable at the bottom
of the watch list and hit ENTER.

You can change the values of variables by double clicking on the watch item in the watch list.

Call Stack
----------
The callstack shows a list the preceeding calls that resulted in the location you are currently at. To jump 
to the file and line of the functions shown, just double click on it.


Compile and exec Single file
----------------------------
For this to work you need to be connected to the Tribes 2 debugger. It is available as a right click 
option on the project view. 


Compile and Exec Modified files
-------------------------------
For this to work you need to be connected to the Tribes 2 debugger.

This feature allows you do syntax checking and see the changes you have made instantly within Tribes 2.
It only attempts to compile and exec files already running in T2/V12, these are indicated by the green
file icon in the project view. To compile a file not currently running in T2/V12 right click on the file
in the project view and click on compile. A green file icon should appear next to it if it compiled and 
execed sucessfully.

Tribal IDE keeps track of the files you have modified and havn't been compiled. So when you click the
Compile and Exec button it will first ask you to save any unsaved files, and then tell Tribes 2 to
compile specific scripts. Any syntax errors that Tribes 2 generates will be appear in the messages
window, these can be double clicked on to goto the specific file and line. Once completed compiling and
execing the IDE will report how many files it has managed to compiled and how many it has failed to
compile. The list of failed to compile files will appear also in the messages window. 

Runtime Errors
--------------
Runtime errors will appear in the console window, these can be also double clicked on to go to the file
and line number.


I know this documentation is rubbish =D, more next version.... honest =D

David Dunscombe.


------------------------------------------------------------------------------------

History
-------

Beta 3.7 08-09-2001
-------------------
	Fixed - current file functions was not updating correctly on file save
	
	Fixed - If connection to debugging server was not disconnected via the IDE subsequant editing
		woudl generate an exception
	
	Fixed - double clicking of find in files messages would not always pick the correct file if there 
		were more than one file of the same name in the project

	Fixed - compile and exec modified will now only compile and exec files which T2/V12 already has
		running, these files have a green file icon next to them while in debugging mode. To compile
		a file that is not currently running in T2/V12 right click on in in the project view and
		click compile. If successfully compiled & execed it will have a green file icon next to it.	



Beta 3.6 21-05-2001
-------------------

	Fixed - drill down wasnt working correctly after a few levels

Beta 3.5 18-05-2001
-------------------

	Added - drill down watches thanks Tinman! =D

	Added - vl2 caching, file in vl2 is cached on first load - this speeds up find in files

	Fixed - vl2 .dso files not included in project

Beta 3.2 11-05-2001 :
---------------------

	Fixed - Current file functions for non-vl2(zip) files was not working =/ oops

Beta 3.1 10-05-2001 :
---------------------

	Fixed - Breakpointable points not being cleared in certain situations

	Fixed - Dosn't request breakpoint list every time active editor is changed

	Fixed - Now only scans a zip once for functions after it is changed, was rescaning every project load

	Fixed - Breakpoint line not being coloured red in certain situations

	Added - Goto definition, right click on function in editor and jump to definition

Beta 3 06-05-2001 : 
-------------------

	Added - Dockable windows - move into center of another window to create tabs

	Added - VL2 in project support, find in files will search in vl2, open files in read only	

	Added - Breakpoint list with conditional breakpoints, and enabling/disabling of groups

	Added - Call stack

	Changed - Changed indent/unindent block to TAB and SHIFT TAB when there is selected text

	Added - Find drop downs now remember previous searchs after editor is closed

	Added - Added current file function list - shows functions that in current active file

	Fixed - Hitting return on quick find edit box now repeats search - also removed beep

	Fixed - Doing close all whilst in debugging caused incoming breakpoint list to re-open
		the file.


Beta 2 22-04-2001 : 
-------------------
	
	Bug fixes and small feature improvements

	Some situations close project would return false, causing closing of app to
	incorrectly fail. 

	Fixed 'Save As' erm not working =D oops

	Fixed status bar jumping above watch/console/message window

	Fixed watch window now will ask for all watches when a single watch variable is changed,
	as it might affect the other watches

	Now Checking filelist debugger sends me for all querys to debugger, before sending 		
	breakpoint list requests out - fixes breakpoints not appearing on files.

	Added .gui to open/save file dialogs
	
	Added seperate font/other options for printing out

	Added read-only checks, will ask to clear read only flag on creation of new project. On
	every save it will check if the file has attribute set, if so asks whether the IDE should
	clear it

	Saves main window position when not maximized

	Now asks before making .cs and .gui file associations


Beta 1 21-04-2001 : 
-------------------
	Limited first release of Tribal IDE


------------------------------------------------------------------------------------


Copyrights and Trademarks
-------------------------

All copyrights and trademarks acknowleged.

The Program ("Tribal IDE") is owned by David Dunscombe ("the
Owner") and is protected by the United States copyright laws and
international treaty provisions. The Owner retains all rights not
expressly granted. None of the components of the Program
(including the documentation) may be copied, removed or altered,
in whole or part, for any unauthorized use.

LICENSE
The owner grants you non-exclusive license to:
* Use the Program free of charge, for an unlimited time in 
it's present state (Version Beta 3.7) as found in this installtion.

You may not:
* Remove or alter any of the copyright notices from any
components of the Program.
* Sub license, rent or lease all or part of the Program.
* Use the Program with the intent to violate any licensing
agreements or contracts.
* Modify, adapt, translate, create derivative works, decompile,
disassemble, or otherwise reverse engineer or attempt to reverse
engineer or derive source code from all or any portion of the
Program or anything incorporated therein or permit or encourage
any third party to do so.



Distribution
------------

You may freely distribute the Program in its original form,
including all documentation and copyright information, by any
online means. You may not charge, receive donation for, or
otherwise profit from the distribution of this product.
You may NOT distribute this program on ANY media (floppy disk,
cdrom, tape, or other magnetic storage device) except for personal
use.
Commercial distribution, for free or for profit, must be arranged
through the Owner first.



Disclaimer
----------

BECAUSE THE PROGRAM IS LICENSED FREE OF
CHARGE, THERE IS NO WARRANTY  FOR THE
PROGRAM, TO THE EXTENT PERMITTED BY
APPLICABLE LAW.  EXCEPT WHEN  OTHERWISE
STATED IN WRITING THE COPYRIGHT HOLDERS
AND/OR OTHER PARTIES  PROVIDE THE PROGRAM
"AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED  OR IMPLIED, INCLUDING, BUT
NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.  THE ENTIRE RISK AS  TO
THE QUALITY AND PERFORMANCE OF THE
PROGRAM IS WITH YOU.  SHOULD THE PROGRAM
PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING,  REPAIR OR
CORRECTION.
