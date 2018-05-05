# [F4DConverter] (https://github.com/Gaia3D/F4DConverter)

## Summary
This application, F4DConverter, is for converting popular 3D model formats into F4D format which is devised for Mago3D - 3D web geo-platform. (www.mago3d.com). This project is of Multiplatform(UNIX, MAC OSX, Windows) C++ project.

## Getting Started
Start by cloning this repository, making sure to pass the `--recursive` flag to grab all the dependencies.
If you forgot, then you can `git submodule update --init` instead.

```bash
git clone --recursive https://github.com/Gaia3D/F4DConverter.git
cd F4DConverter
cd build
```

Now generate a project file or makefile for your platform.

```bash
# UNIX Makefile
cmake ..

# Mac OSX
cmake -G "Xcode" ..

# Microsoft Windows
cmake -G "Visual Studio 15 2017" ..
cmake -G "Visual Studio 15 2017 Win64" ..
...
```

## supported input formats ##
- .ifc
- .3ds
- .obj
- .dae

> Beside above formats, other formats which are supported by Assimp may be supported.(NOT TESTED!!)
>
> In this version, .JT(Jupiter Tessellation, a kind of cad design format) is not included.

## Dependencies

Functionality           | Library
----------------------- | ------------------------------------------
Mesh Loading            | [assimp](https://github.com/assimp/assimp)
OpenGL Function Loader  | [glad](https://github.com/Dav1dde/glad)
Windowing and Input     | [glfw](https://github.com/glfw/glfw)
OpenGL Mathematics      | [glm](https://github.com/g-truc/glm)
Texture Loading         | [stb](https://github.com/nothings/stb)

## License
