mkdir unangband-064-win
mkdir unangband-064-win\lib
mkdir unangband-064-win\lib\apex
mkdir unangband-064-win\lib\bone
mkdir unangband-064-win\lib\data
mkdir unangband-064-win\lib\docs
mkdir unangband-064-win\lib\edit
mkdir unangband-064-win\lib\file
mkdir unangband-064-win\lib\help
mkdir unangband-064-win\lib\info
mkdir unangband-064-win\lib\pref
mkdir unangband-064-win\lib\save
mkdir unangband-064-win\lib\script
mkdir unangband-064-win\lib\user
mkdir unangband-064-win\lib\xtra
mkdir unangband-064-win\lib\xtra\font
mkdir unangband-064-win\lib\xtra\graf
mkdir unangband-064-win\lib\xtra\music
mkdir unangband-064-win\lib\xtra\sound
mkdir unangband-064-win\lib\xtra\help

copy lib\apex\delete.me unangband-064-win\lib\apex
copy lib\bone\delete.me unangband-064-win\lib\bone
copy lib\data\delete.me unangband-064-win\lib\data
copy lib\save\delete.me unangband-064-win\lib\save
copy lib\user\delete.me unangband-064-win\lib\user
copy lib\xtra\music\delete.me unangband-064-win\lib\xtra\music

copy unangband.exe unangband-064-win
copy readme.txt unangband-064-win
copy changes.txt unangband-064-win
copy faq.txt unangband-064-win
copy bugs.txt unangband-064-win
copy nocompile.txt unangband-064-win\compile.txt

copy lib\docs\*.rtf unangband-064-win\lib\docs
copy lib\edit\*.txt unangband-064-win\lib\edit

copy lib\file\*.txt unangband-064-win\lib\file
copy lib\info\*.txt unangband-064-win\lib\info

copy lib\help\*.txt unangband-064-win\lib\help
copy lib\help\*.hlp unangband-064-win\lib\help

copy lib\pref\*.prf unangband-064-win\lib\pref

copy lib\xtra\font\*.fon unangband-064-win\lib\xtra\font

copy lib\xtra\graf\*.bmp unangband-064-win\lib\xtra\graf

copy lib\xtra\sound\sound.cfg unangband-064-win\lib\xtra\sound
copy lib\xtra\sound\*.wav unangband-064-win\lib\xtra\sound

copy lib\xtra\help\angband.hlp unangband-064-win\lib\xtra\help
copy lib\xtra\help\angband.cnt unangband-064-win\lib\xtra\help

7z a -tzip -r unangband-064-win.zip unangband-064-win

rmdir /q /s unangband-064-win

