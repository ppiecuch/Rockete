<<<<<<< HEAD
# Rockete - libRocket files editor

## New changelog and development progress

1. New project management options: the project can now be created, saved and open. It contains informations about paths and cutting info for every image.
2. Texture atlas support: if along with a texture, there is a texture info file (in my own format or cocos2d plist) it is used and you can see all elements of the atlas in the file tree view. Additionaly, for every atlas, an rcss file is created with proper decorators.
3. 9-patch creation support.
4. New texture atlas preview: images and sprites from atlas can be preview now.
5. New fonts preview: read fonts has a preview panel now.

Latest OSX binary build from 25/09/2013 can be found at: [code.google.com](https://code.google.com/p/ppiecuch-projects/downloads/detail?name=Rockete-25092013.zip).

-- ORIGINAL readme.md --
## Status
[![Build Status](https://secure.travis-ci.org/FishingCactus/Rockete.png?branch=master)](http://travis-ci.org/FishingCactus/Rockete)

## Description
Rockete is a Qt-based libRocket RML/RCSS files editor.
The main purpose is to provide a useful tool to create game graphical user interfaces.
The second 'e' means "Editor".
It is pronunced like "Machete" ;)

## Requirements

#### Linux
- Qt4 library
- libRocket
- ${LIBROCKET} environment variable if libRocket is not in /usr/include and /usr/lib

#### Windows
- Qt4 library
- libRocket
- ${LIBROCKET} environment variable referring to libRocket root directory

## Build

#### Linux
- qmake
- make

#### Windows (Visual Studio project)
- qmake -tp vc rockete.pro
- Open VS and build project...

## License (MIT)
 
 Copyright (c) 2011-2012 Fishing Cactus SA
=======
# libRocket - The HTML/CSS User Interface Library

http://librocket.com

libRocket is the C++ user interface package based on the HTML and CSS standards. It is 
designed as a complete solution for any project's interface needs.

libRocket uses the time-tested open standards XHTML1.0 and CSS2.0 (while borrowing features from 
HTML5 and CSS3), and extends them with features suited towards real-time applications. Because of 
this, you don't have to learn a whole new proprietary technology like other libraries in this space.

## Features

- Cross platform architecture: Windows, Mac, Linux, iPhone, etc.
- Dynamic layout system.
- Efficient application-wide styling, with a custom-built templating engine.
- Fully featured control set: buttons, sliders, drop-downs, etc.
- Runtime visual debugging suite.
- Easily integrated and extensible with Python scripting.

## Extensible
- Abstracted interfaces for plugging in to any game engine.
- Decorator engine allowing custom application-specific effects that can be applied to any element.
- Generic event system that binds seamlessly into existing projects.

## License (MIT)
 
 Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
>>>>>>> 4d553b6bb519c7f5e4f4496de71ad7430ea7269b
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
  
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
