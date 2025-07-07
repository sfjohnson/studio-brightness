# studio-brightness

studio-brightness is a small Windows utility that sits in the taskbar notification area and changes the brightness of a connected Apple Studio Display.

- To increase brightness: left shift + left windows (or command) + right arrow
- To reduce brightness: left shift + left windows (or command) + left arrow

To exit, left or right click on the icon in the taskbar notification area and click Exit.

## Building

Pick either one of `build.bat` or CMake to build the executable "bin/studio-brightness.exe":

Run `build.bat` in the Developer Command Prompt where cl.exe and rc.exe are in PATH.

Alternatively, CMake works with compilers including Visual Studio, Intel oneAPI, MinGW GCC, Clang, and more.
By default, the debug popup windows that appear on errors are disabled.
To enable them, set CMAKE_BUILD_TYPE=Debug in the CMake command line or in the CMake GUI.

Default release build, no popup windows on error:

```sh
cmake -B bin

cmake --build bin
```

Debug build, popup windows on error:

```sh
cmake -B bin -DCMAKE_BUILD_TYPE=Debug

cmake --build bin
```

Because the main entry point is wWinMain, the return code is always 0.
Thus if studio-brightness fails to start or run correctly, try building in CMake Debug build type to see the error message in a popup window.
