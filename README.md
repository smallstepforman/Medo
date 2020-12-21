# Medo
Medo is a modern Media Editor built exclusively for the open source Haiku OS (aka the Media OS).  Medo allows customisation using OpenGL based GLSL plugins, and allows 3rd party developers to create dynamically loaded Addons and Plugins.

There are many bundled media effects, including:
- Colour grading effects (Saturation/Exposure/Temperature etc)
- Colour correction curves and white balance.
- Support for Adobe LUT (Look Up Tables)
- Create masks and keyframes
- Several bundled transitions
- Several special effects (Blur, Night Vision, Chroma Key etc)
- Spatial tools to transform media (scale/rotate/position/crop)
- Audio effects (20 band equaliser, fade)

# Development Requirements
Haiku development environment.
QtCreator as the IDE for code navigation and code completion
ffmpeg development package
cmake (optional)

# Build Scripts
1) Clean all object files, remove temporary build directories 
./clean_all

2a) Build using jam (default primary build method)
jam -j16 (option -j16 is parallel build, replace 16 with number of CPU cores, eg. -j8)
./build_addons

2b) Build using cmake + make (optional secondary build method)
cmake CMakeLists.txt
make -j16 (

3) Setup attributes, file icon
./setup_attributes

4) Build HPKG (release), which will build an installer package
./create_package

# Soure code layout
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
