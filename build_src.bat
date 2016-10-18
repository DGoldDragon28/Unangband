mkdir unangband-064-src
mkdir unangband-064-src\lib
mkdir unangband-064-src\lib\apex
mkdir unangband-064-src\lib\bone
mkdir unangband-064-src\lib\data
mkdir unangband-064-src\lib\docs
mkdir unangband-064-src\lib\edit
mkdir unangband-064-src\lib\file
mkdir unangband-064-src\lib\help
mkdir unangband-064-src\lib\info
mkdir unangband-064-src\lib\pref
mkdir unangband-064-src\lib\save
mkdir unangband-064-src\lib\todo
mkdir unangband-064-src\lib\user
mkdir unangband-064-src\lib\xtra
mkdir unangband-064-src\lib\xtra\font
mkdir unangband-064-src\lib\xtra\graf
mkdir unangband-064-src\lib\xtra\music
mkdir unangband-064-src\lib\xtra\sound
mkdir unangband-064-src\lib\xtra\help
mkdir unangband-064-src\src
mkdir unangband-064-src\osx
mkdir unangband-064-src\osx\English.lproj
mkdir unangband-064-src\osx\English.lproj\main.nib

copy lib\apex\delete.me unangband-064-src\lib\apex
copy lib\bone\delete.me unangband-064-src\lib\bone
copy lib\data\delete.me unangband-064-src\lib\data
copy lib\save\delete.me unangband-064-src\lib\save
copy lib\user\delete.me unangband-064-src\lib\user
copy lib\xtra\font\delete.me unangband-064-src\lib\xtra\font
copy lib\xtra\graf\delete.me unangband-064-src\lib\xtra\graf
copy lib\xtra\music\delete.me unangband-064-src\lib\xtra\music

copy *.txt unangband-064-src
copy *.bat unangband-064-src
copy config*.* unangband-064-src
copy config*.* unangband-064-src

copy Makefile* unangband-064-src
copy *.m4 unangband-064-src

copy lib\Makefile* unangband-064-src\lib
copy lib\apex\Makefile* unangband-064-src\lib\apex
copy lib\bone\Makefile* unangband-064-src\lib\bone
copy lib\data\Makefile* unangband-064-src\lib\data
copy lib\docs\Makefile* unangband-064-src\lib\docs
copy lib\edit\Makefile* unangband-064-src\lib\edit
copy lib\file\Makefile* unangband-064-src\lib\file
copy lib\help\Makefile* unangband-064-src\lib\help
copy lib\info\Makefile* unangband-064-src\lib\info
copy lib\pref\Makefile* unangband-064-src\lib\pref
copy lib\save\Makefile* unangband-064-src\lib\save
copy lib\todo\Makefile* unangband-064-src\lib\todo
copy lib\user\Makefile* unangband-064-src\lib\user
copy lib\xtra\Makefile* unangband-064-src\lib\xtra
copy lib\xtra\font\Makefile* unangband-064-src\lib\xtra\font
copy lib\xtra\graf\Makefile* unangband-064-src\lib\xtra\graf
copy lib\xtra\music\Makefile* unangband-064-src\lib\xtra\music
copy lib\xtra\sound\Makefile* unangband-064-src\lib\xtra\sound
copy lib\xtra\help\Makefile* unangband-064-src\lib\xtra\help
copy src\Makefile* unangband-064-src\src

copy lib\docs\*.rtf unangband-064-src\lib\docs
copy lib\docs\*.txt unangband-064-src\lib\docs
copy lib\edit\*.txt unangband-064-src\lib\edit
copy lib\file\*.txt unangband-064-src\lib\file
copy lib\info\*.txt unangband-064-src\lib\info
copy lib\help\*.txt unangband-064-src\lib\help
copy lib\help\*.hlp unangband-064-src\lib\help
copy lib\pref\*.prf unangband-064-src\lib\pref
copy lib\todo\*.txt unangband-064-src\lib\todo

copy lib\xtra\font\*.fon unangband-064-src\lib\xtra\font
copy lib\xtra\graf\*.bmp unangband-064-src\lib\xtra\graf
copy lib\xtra\graf\*.png unangband-064-src\lib\xtra\graf
copy lib\xtra\sound\*.wav unangband-064-src\lib\xtra\sound

copy lib\xtra\sound\sound.cfg unangband-064-src\lib\xtra\sound
copy lib\xtra\help\angband.hlp unangband-064-src\lib\xtra\help
copy lib\xtra\help\angband.cnt unangband-064-src\lib\xtra\help

copy src\*.h unangband-064-src\src
copy src\*.c unangband-064-src\src
copy src\*.inc unangband-064-src\src
copy src\*.rc unangband-064-src\src
copy src\*.ico unangband-064-src\src
copy src\Makefile.* unangband-064-src\src

copy src\osx\*.icns unangband-064-src\src\osx
copy src\osx\*.xml unangband-064-src\src\osx
copy src\osx\*.h unangband-064-src\src\osx
copy src\osx\English.lproj\*.strings unangband-064-src\src\osx\English.lproj
copy src\osx\English.lproj\main.nib\*.?ib unangband-064-src\src\osx\English.lproj\main.nib

7z a -tzip -r unangband-064-src.zip unangband-064-src

rmdir /q /s unangband-064-src

