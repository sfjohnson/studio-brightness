md bin
md obj 
cl -MD -O2 -W4 -Iinclude -c -Foobj/main.obj src/main.cpp
cl -MD -O2 -W4 -Iinclude -c -Foobj/hid.obj src/hid.cpp
rc -Iinclude -foobj/studio-brightness.res studio-brightness.rc
cl -MD -Fe./bin/studio-brightness.exe obj/main.obj obj/hid.obj obj/studio-brightness.res -link hid.lib setupapi.lib wbemuuid.lib comctl32.lib User32.lib Shell32.lib Gdi32.lib
