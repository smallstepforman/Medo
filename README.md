# Medo
![Alt text](/Docs/Medo_Logo.png?raw=true "Medo Logo")

Haiku Media Editor

Copyright Zen Yes Pty Ltd 2019-2021

Melbourne, Australia.

Released under Open Source MIT license.


Written by Zenja Solaja


Medo is a modern Media Editor built exclusively for the open source Haiku OS (aka the Media OS).  Medo allows customisation using OpenGL based GLSL plugins, and allows 3rd party developers to create dynamically loaded Addons and Plugins.

There are many bundled media effects, including:
- Colour grading effects (Saturation/Exposure/Temperature etc)
- Colour correction curves and white balance.
- Support for Adobe LUT (Look Up Tables)
- Create masks and keyframes
- Several bundled transitions
- Several special effects (Blur, Night Vision, Chroma Key etc)
- Spatial tools to transform media (scale/rotate/position/crop)
- Multiple text effects, including 3D extruded fonts.
- Audio effects (20 band equaliser, fade)

Medo can edit UHD 4K videos, export to any Haiku supported codec, is optimised for many CPU-core systems, and has low system resource requirements. Medo is multilingual (currently supports 10 user languages), and is themable.  The Medo.HPKG (Haiku package file) can  fit on a floppy disk with no external dependancies other than what ships with a default Haiku release (it's less than 1.44Mb).


![Alt text](/Docs/Medo.jpeg?raw=true "Medo Screenshot")

# Development Requirements
- Haiku development environment, 64 bit only (c++17 minimum standard)
- QtCreator as the IDE for code navigation and code completion
- ffmpeg development package
- cmake (optional)

# Build Scripts
1. Clean all object files, remove temporary build directories 

./clean_all

2. Build the source (either with jam or cmake)

a) jam build (default primary build method), debug build (-g -O0)

jam -j16 (option -j16 is parallel build, replace 16 with number of CPU cores, eg. -j8)

./build_addons

b) cmake + make (optional secondary build method), debug build (-g -O0)

cmake CMakeLists.txt

make -j16 (option -j16 is parallel build, replace 16 with number of CPU cores, eg. -j8)

./build_addons

3. Setup attributes, file icon

./setup_attributes

4. Build HPKG (release), which will build an installer package, (no debug code, -03)

./create_package

The Medo.hpkg file will be created in the Package directory.


# Source code layout
3rdparty/   - external 3rd party code

Actor/      - Actor programming model framework

AddOns/     - dynamically loaded add-ons.  Each add-on must be compiled separately.

Docs/       - developer documentation

Editor/     - the core application

Effects/    - bundled effects

Gui/        - custom GUI widgets

Plugins/    - dynamically loaded GLSL plugins.

rapidjson/  - embedded library

Resources/  - application icons etc

Yarra/      - OpenGL based rendering library

# Software Architecture
The BeAPI is based on C++ pre-ISO/IEC 14882:1998 standard (published in 1998, ie. pre Standard Template Library).  The API uses the Actor programming model (it's interesting that nowhere in the BeBook do they mention the term Actor, so it's quite likely that they independantly implemented the Actor model).  Each BWindow object runs in its own thread with a dedicated message queue.  Medo also incorporates a proper Actor library for parallel processing (eg. thumbnail generation, OpenGL rendering, audio buffer creation, exporting etc).  

Medo is a 64 bit application, compiled with C++20 gcc compile flag (c++2a).
Medo uses OpenGL 3.3 Core profile, with GLSL 330 for video effects rendering.
An OpenGL wrapper called Yarra is used to abstract the raw OpenGL calls.

Users can implement custom GLSL shader effects by creating their own plugins.  Effects from websites like ShaderToy can easily be converted to work with Medo (please use the developer guide in the Docs/ directory for more information).  Please look at the Plugins/ directory for some simple examples.

Medo also supports external binary addons, which allows 3rd party developers to add complex audio and video effects.  Please use the developer guide (Docs/ directory) and see the examples in the AddOns directory. 

Media decoding uses the native Haiku MediaKit.  As of Dec 2020, the MediaKit encoders still have stability issues, therefore, Medo also provides an ffmpeg Encoder based code path (which is the default encoder).

