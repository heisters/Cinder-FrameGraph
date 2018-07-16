Cinder Frame Graph Example Application: Color Grading 
=====================================================

This application allows rudimentary color grading and LUT application on
images. Support for HAPQ videos is enabled by using libglvideo.

Building
--------

ColorApp uses CMake, and can be built in the standard way:

    cd Cinder/blocks/Cinder-FrameGraph/examples/Color
    mkdir build
    cd build
    cmake ../proj/cmake
    make -j 4

### Building on Windows

Visual Studio Code has great CMake support with the
[CMake tools](https://github.com/vector-of-bool/vscode-cmake-tools) plugin.
You can also use [Visual Studio 2017's built in support for CMake](https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/),
or you can generate a Visual Studio project:

    cd Cinder/blocks/Cinder-FrameGraph/examples/Color
    mkdir build
    cd build
    cmake ../proj/cmake -G "Visual Studio 14 2015 Win64"

Use `cmake --help` to see a full list of the supported Visual Studio generators.

Video
-----

To use video instead of still images in the example app, you need to set the
CMake option ENABLE_GLVIDEO when calling cmake:

    cmake ../proj/cmake -DENABLE_GLVIDEO=ON

or

    cmake ../proj/cmake -DENABLE_GLVIDEO=ON -G "Visual Studio 14 2015 Win64"

This option can also be set in the configurations for VSCode, CLion or whatever
other IDE you may be using.

Once configured, ColorApp will only open MP4 and MOV files with MJPEG or HAPQ
encoding. It will allow you to select MP4s and MOVs with other encodings, but it
will crash with an unsupported codec error.