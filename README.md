Cinder Frame Graph
==================

Cinder Frame Graph is a block for [libcinder](https://libcinder.org/) that
allows node-based image and video processing. It provides a number of nodes for
loading images and videos to textures and processing with shaders. It
has experimental support for [OpenColorIO](http://opencolorio.org/), a [node for
applying LUTs](include/cinder/framegraph/LUTNode.hpp) to texture sources, and
[a color grading node](include/cinder/framegraph/ColorGradeNode.hpp). It is also
integrated with [Cinder's vector types](include/cinder/framegraph/VecNode.hpp),
making it easy to apply transformations to vectors and use them as
inputs/outputs with other nodes. 

Frame Graph also makes it easy to define your own node-based data-flow
applications. It is based on [libnodes](https://github.com/heisters/libnodes),
which provides a type-safe node architecture. New node classes can easily be
defined to process any data type, whether text, JSON, or OpenCV matrices. While
it works well with external libraries like [OpenCV](http://opencv.org/), direct
support is currently omitted in the interest of reducing dependencies.

Getting Cinder Frame Graph
--------------------------

Frame Graph depends on libnodes and, optionally, libglvideo. To install Frame
Graph and its dependencies, clone the Frame Graph repo into your `Cinder/blocks`
directory:

    cd Cinder/blocks
    # on older versions of git, use --recursive instead of --recurse-submodules
    git clone --recurse-submodules git@github.com:heisters/Cinder-FrameGraph

libglvideo support is not enabled by default. See below for details on how to
use it.

A Brief Example
---------------

```C++
#include "cinder/FrameGraph.hpp"
#include "cinder/framegraph/LUTNode.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace frame_graph;
using namespace frame_graph::operators;

// Use a TextureINode to feed an image-based texture into the node graph
TextureINode n_image( loadImage( getOpenFilePath() ) );
// Use another TextureINode to load the LUT
TextureINode n_lutImage( getAssetPath( "luts/K_TONE_Kodachrome.png" ) );
// This will provide a texture to draw to the screen
TextureONode n_out;
// This applies the LUT to the input texture
LUTNode n_lut( getWindowSize() );

// connect the input image to the LUT applicator using the >> operator. When no
// inlet or outlet is specified, >> defaults to the first (index 0) outlet and
// inlet on the nodes.
n_image >>      n_lut >>            n_out;
// now feed the LUT itself into the LUT applicator's second inlet.
n_lutImage >>   n_lut.in< 1 >();

// Call update on the root nodes to propagate their messages through the graph.
n_image.update();
n_lutImage.update();

// TextureONode provides a coercion operator to bool, which exposes the
// underlying ci::gl::Texture2dRef's bool coercion operator.
if ( n_out ) {
    // TextureONode can also be implicity coerced to the underlying Texture2dRef
    gl::draw( n_out );
}
```

For more examples, see the [examples folder](examples/) and
[libnodes](https://github.com/heisters/libnodes) itself.

Building
--------

Cinder Frame Graph uses CMake. It has a
[Cinder Block CMake config](proj/cmake/Cinder-FrameGraphConfig.cmake), you can
see an [example of its usage](examples/Color/proj/cmake/CMakeLists.txt) in the
Color example project.

libglvideo support
------------------

To enable [libglvideo](https://github.com/heisters/libglvideo) support, set
ENABLE_FRAMEGRAPH_LIBGLVIDEO in your CMakeLists.txt file before calling
`ci_make_app()`:

    set(ENABLE_FRAMEGRAPH_LIBGLVIDEO ON CACHE BOOL "Enable FrameGraph libglvideo support")

Provided you have properly cloned Frame Graph's submodule dependencies, this
will automatically find libglvideo, build it, and link it.

With libglvideo support enabled, you will need two nodes to read and decode
a HAPQ video:

```C++
#include "cinder/framegraph/GLVideo.hpp"

// ...

// Create a glvideo context that will be shared by all movies. The number
// of worker threads depends on the number of available CPU cores and the
// number of videos you anticipate to need to decode simultaneously.
glvideo::Context::ref ctx = glvideo::Context::create( 4 /* worker threads */ );

// Create the movie control node
GLVideoINode movie( ctx, "path/to/a/hapq_video.mp4" );

// Create the YUV -> RGB decoding node
GLVideoHapQDecodeShaderIONode decoder( movie.getSize() );

// Create a texture output node
TextureONode out;

// Start the movie and loop it
movie.play().loop();

// Setup the node graph
movie >> decoder >> out;

// update the movie once per frame:
movie.update();

// draw the movie
if ( out ) gl::draw( out );
```

See the [Color example project](examples/Color/README.md) for a full example.


MIT License
-----------

Copyright (c) 2016-2017 Ian Heisters

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

