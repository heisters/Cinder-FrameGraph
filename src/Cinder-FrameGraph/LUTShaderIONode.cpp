#include "Cinder-FrameGraph/LUTShaderIONode.hpp"

using namespace std;
using namespace cinder;
using namespace frame_graph;

static const string VERT = R"EOF(
uniform mat4	ciModelViewProjection;
in vec4			ciPosition;
in vec2			ciTexCoord0;
out vec2		uv;

void main( void ) {
    gl_Position	= ciModelViewProjection * ciPosition;
    uv = ciTexCoord0;
    uv.y = 1. - uv.y;
}
)EOF";


// based on https://github.com/mattdesl/glsl-lut
// (c) @mattdesl, MIT License
static const string FRAG = R"EOF(
#version 150

uniform sampler2D uTexSrc;
uniform sampler2D uTexLUT;

in vec2 uv;
out vec4 oColor;

vec4 lookup(in vec4 textureColor, in sampler2D lookupTable) {
#ifndef LUT_NO_CLAMP
    textureColor = clamp(textureColor, 0.0, 1.0);
#endif

    mediump float blueColor = textureColor.b * 63.0;

    mediump vec2 quad1;
    quad1.y = floor(floor(blueColor) / 8.0);
    quad1.x = floor(blueColor) - (quad1.y * 8.0);

    mediump vec2 quad2;
    quad2.y = floor(ceil(blueColor) / 8.0);
    quad2.x = ceil(blueColor) - (quad2.y * 8.0);

    highp vec2 texPos1;
    texPos1.x = (quad1.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
    texPos1.y = (quad1.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

#ifdef LUT_FLIP_Y
    texPos1.y = 1.0-texPos1.y;
#endif

    highp vec2 texPos2;
    texPos2.x = (quad2.x * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.r);
    texPos2.y = (quad2.y * 0.125) + 0.5/512.0 + ((0.125 - 1.0/512.0) * textureColor.g);

#ifdef LUT_FLIP_Y
    texPos2.y = 1.0-texPos2.y;
#endif

    lowp vec4 newColor1 = texture(lookupTable, texPos1);
    lowp vec4 newColor2 = texture(lookupTable, texPos2);

    lowp vec4 newColor = mix(newColor1, newColor2, fract(blueColor));
    return newColor;
}

void main() {
    vec4 c_src = texture( uTexSrc, uv );
    vec4 c_lut = lookup( c_src, uTexLUT );
    oColor = c_lut;
}
)EOF";

LUTShaderIONode::LUTShaderIONode( const ivec2 & size ) :
TextureShaderIONode( gl::GlslProg::create( gl::GlslProg::Format()
                                           .fragment( FRAG )
                                           .vertex( VERT )
                                           .define( "LUT_FLIP_Y" )
                                         ), size )
{
    setTextureName( 0, "uTexSrc" );
    setTextureName( 1, "uTexLUT" );
}