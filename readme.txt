fix for visual studio 6 to properly terminate a process that is being debugged under 64bit windows

under:
C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Bin
rename TLLOC.DLL TLLOC.old.DLL
The naming is important since the new dll acts as a shim and loads the old one by that exact name


VS6Help.reg to point to the .hlp files for win32 api when pressing F1