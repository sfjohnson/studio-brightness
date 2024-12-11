# studio-brightness

studio-brightness is a small Windows utility that sits in the taskbar notification area and changes the brightness of a connected Apple Studio Display.

- To increase brightness: left shift + left windows (or command) + right arrow
- To reduce brightness: left shift + left windows (or command) + left arrow

To exit, left or right click on the icon in the taskbar notification area and click Exit.

## Building

Pick either one of `build.bat` or CMake to build the executable "bin/studio-brightness.exe":

* run `build.bat` in the Developer Command Prompt where cl.exe and rc.exe are in PATH.
* CMake works with compilers including Visual Studio, Intel oneAPI, MinGW GCC, Clang

    ```sh
    cmake -B bin

    cmake --build bin
    ```
