md bin
md obj
set CXXFLAGS=-MD -O2 -W4 -Iinclude /std:c++20
cl %CXXFLAGS% -c -Foobj/main.obj src/main.cpp
cl %CXXFLAGS% -c -Foobj/hid.obj src/hid.cpp
rc -Iinclude -foobj/studio-brightness.res studio-brightness.rc
cl -MD -Fe./bin/studio-brightness.exe obj/main.obj obj/hid.obj obj/studio-brightness.res -link hid.lib setupapi.lib wbemuuid.lib comctl32.lib User32.lib Shell32.lib Gdi32.lib
