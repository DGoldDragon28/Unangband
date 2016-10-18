mkdir ext-win
mkdir ext-win\src
mkdir ext-win\lib
mkdir ext-win\lib\xtra
mkdir ext-win\lib\xtra\font
mkdir ext-win\lib\xtra\graf
mkdir ext-win\lib\xtra\sound

copy lib\xtra\font\*.fon ext-win\lib\xtra\font\
copy lib\xtra\graf\*.bmp ext-win\lib\xtra\graf\
copy lib\xtra\sound\sound.cfg ext-win\lib\xtra\sound\
copy lib\xtra\sound\*.wav ext-win\lib\xtra\sound\

copy src\angband.rc ext-win\src\
copy src\angband.ico ext-win\src\
copy src\readdib.c ext-win\src\
copy src\readdib.h ext-win\src\

cd ext-win
7z a -tzip ..\ext-win-306.zip *

cd ..
rmdir /q /s ext-win
