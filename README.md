# [F4DConverter] (https://github.com/Gaia3D/NewF4DConverter)

## Summary
This application, F4DConverter, is for converting popular 3D model formats into F4D format which is devised for Mago3D - 3D web geo-platform. (www.mago3d.com). This project is of Multiplatform(UNIX, MAC OSX, Windows) C++ project.

## Getting Started
Start by cloning this repository, making sure to pass the `--recursive` flag to grab all the dependencies.
If you forgot, then you can `git submodule update --init` instead.

```bash
git clone --recursive https://seoul.gaia3d.com:53000/mago3d/NewF4DConverter.git
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
- .gml / .xml / .citygml
  
> Beside above formats, other formats which are supported by Assimp may be supported.(NOT TESTED!!)
>
> In this version, .JT(Jupiter Tessellation, a kind of cad design format) is not included.
>  
> As you know, the file extension .gml and .xml are not only used in citygml. So we are considering if we have to limit the file extension for citygml.

## Dependencies

Functionality           | Version | Library
----------------------- | ------- | ------------------------------------------
Utility                 | 1.71.0 | [boost](https://github.com/boostorg/boost)
Mesh Loading            | 4.1.0 | [assimp](https://github.com/assimp/assimp)
OpenGL Function Loader  | 0.1.11 | [glad](https://github.com/Dav1dde/glad)
Windowing and Input     | 3.2.1 | [glfw](https://github.com/glfw/glfw)
OpenGL Mathematics      | 0.9.9.3 | [glm](https://github.com/g-truc/glm)
Texture Loading         | - | [stb](https://github.com/nothings/stb)
IFC Loading             | - | [ifcplusplus](https://github.com/ifcquery/ifcplusplus)
Json Loading            | 0.10.7 | [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
Projections             | 4.9.3 | [proj.4](https://github.com/OSGeo/proj.4)
XML Parserxerces-c      | 3.2.2-11 | [xerces-c](https://github.com/apache/xerces-c)
CityGML Loading         | 2.0.9 | [libcitygml](https://github.com/jklimke/libcitygml)
GDAL                    | 2.4.1-9 | [gdal](https://github.com/OSGeo/gdal)
GeoTIFF Loading         | 1.4.2-10 | [libgeotiff](https://github.com/OSGeo/libgeotiff)
LAS LiDAR Loading       | 1.8.1-3 | [liblas](https://github.com/libLAS/libLAS)


## License
Copyright (c) 2012-2019, http://www.gaia3d.com
All rights reserved.

mago3D F4DConverter Commercial License for ISVs and VARs:
Gaia3D provides its mago3D F4DConverter under a dual license model designed
to meet the development and distribution needs of both commercial distributors
(such as ISVs and VARs) and open source projects.

For ISVs, VARs and Other Distributors of Commercial Applications:
ISVs (Independent Software Vendors), VARs (Value Added Resellers) and
other distributors that combine and distribute commercially licensed software with
mago3D F4DConverter software and do not wish to distribute the source code
or the commercially licensed software under version 3 of the GNU AFFERO GENERAL PUBLIC LICENSE
(the "AGPL" https://www.gnu.org/licenses/agpl-3.0.en.html ) must enter into
a commercial license agreement with Gaia3D.

For Open Source Projects and Other Developers of Open Source Applications:
For developers of Free Open Source Software ("FOSS") applications under the GPL or AGPL
that want to combine and distribute those FOSS applications with mago3D F4DConverter software,
Gaia3D's open source software licensed under the AGPL is the best option.
