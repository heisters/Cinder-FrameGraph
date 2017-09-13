#include "Cinder-FrameGraph/ColorGradeShaderIONode.hpp"

using namespace ci;
using namespace frame_graph;
using namespace std;

static const std::map< ColorGradeShaderIONode::inlet_names, std::string > INLET_NAME_STRINGS{
        {ColorGradeShaderIONode::exposure,          "Exposure"},
        {ColorGradeShaderIONode::LGG,               "LGG"},
        {ColorGradeShaderIONode::temperature,       "Temperature"},
        {ColorGradeShaderIONode::contrast,          "Contrast"},
        {ColorGradeShaderIONode::midtone_contrast,  "MidtoneContrast"},
        {ColorGradeShaderIONode::HSV,               "HSV"}
};


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


static const string FRAG = R"EOF(
#version 150

uniform sampler2D uTex;

uniform bool uEnableExposure;
uniform bool uEnableLGG;
uniform bool uEnableTemperature;
uniform bool uEnableContrast;
uniform bool uEnableMidtoneContrast;
uniform bool uEnableHSV;

uniform float   uExposure;
uniform vec3    uLGG;
uniform float   uTemperature;
uniform float   uContrast;
uniform float   uMidtoneContrast;
uniform vec3    uHSV;

in vec2 uv;
out vec4 oColor;

//
// Support functions
//

vec3 rgb2hsv( in vec3 c )
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb( in vec3 c )
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float bezier( float t, vec2 P0, vec2 P1, vec2 P2, vec2 P3 )
{
    float t2 = t * t;
    float tinv = 1.0 - t;
    float tinv2 = tinv * tinv;
    return (P0 * tinv2 * tinv + P1 * 3.0 * t * tinv2 + P2 * 3.0 * t2 * tinv + P3 * t2 * t).y;
}

float bezier( float t, vec2 P1, vec2 P2 )
{
    return bezier( t, vec2( 0. ), P1, P2, vec2( 1. ) );
}

vec3 curve( in vec3 c_in, in vec2 p1, in vec2 p2 )
{
    vec3 o;

    o.r = bezier( c_in.r, p1, p2 );
    o.g = bezier( c_in.g, p1, p2 );
    o.b = bezier( c_in.b, p1, p2 );

    return o;
}

vec3 kelvin2rgb( in float K )
{
    float t = K / 100.0;
    vec3 o1, o2;

    float tg1 = t - 2.;
    float tb1 = t - 10.;
    float tr2 = t - 55.0;
    float tg2 = t - 50.0;

    o1.r = 1.;
    o1.g = ( -155.25485562709179 - 0.44596950469579133 * tg1 + 104.49216199393888 * log( tg1 ) ) / 255.;
    o1.b = ( -254.76935184120902 + 0.8274096064007395 * tb1 + 115.67994401066147 * log( tb1 ) ) / 255.;
    o1.b = mix( 0., o1.b, step( 2001., K ) );

    o2.r = ( 351.97690566805693 + 0.114206453784165 * tr2 - 40.25366309332127 * log( tr2 ) ) / 255.;
    o2.g = ( 325.4494125711974 + 0.07943456536662342 * tg2 - 28.0852963507957 * log( tg2 ) ) / 255.;
    o2.b = 1.;

    o1 = clamp( o1, 0., 1. );
    o2 = clamp( o2, 0., 1. );

    return mix( o1, o2, step( 66., t ) );
}

//
// Grading functions
//

vec3 liftGammaGain( in vec3 c_in, in vec3 lgg )
{
    return pow( ( c_in + lgg.x * ( 1. - c_in ) ) * lgg.z, vec3( 1. / lgg.y ) );
}

vec3 exposure( in vec3 c_in, in float x )
{
    return c_in * pow( 2., x );
}

vec3 contrast( in vec3 c_in, in float x )
{
    return ( c_in - .5 ) * x + .5;
}

vec3 hsv( in vec3 c_in, in vec3 x )
{
    return hsv2rgb( rgb2hsv( c_in ) + x );
}

// like "clarity", not the same
vec3 midToneContrast( in vec3 c_in, in float x )
{
    return mix( c_in, curve( c_in, vec2( .05, 1. ), vec2( .95, 0. ) ), x );
}

vec3 temperature( in vec3 c_in, in float K )
{
    vec3 chsv_in = rgb2hsv( c_in );
    vec3 chsv_mult = rgb2hsv( kelvin2rgb( K ) * c_in );

    return hsv2rgb( vec3( chsv_mult.x, chsv_mult.y, chsv_in.z ) );
}


// tint
// highlights
// shadows
// whites
// blacks
// vibrance

void main() {
    vec4 c = texture( uTex, uv );

    if ( uEnableExposure )          c.rgb = exposure( c.rgb, uExposure );
    if ( uEnableLGG )               c.rgb = liftGammaGain( c.rgb, uLGG );
    if ( uEnableTemperature )       c.rgb = temperature( c.rgb, uTemperature );
    if ( uEnableContrast )          c.rgb = contrast( c.rgb, uContrast );
    if ( uEnableMidtoneContrast )   c.rgb = midToneContrast( c.rgb, uMidtoneContrast );
    if ( uEnableHSV )               c.rgb = hsv( c.rgb, uHSV );

    oColor = c;
}
)EOF";


ColorGradeShaderIONode::ColorGradeShaderIONode( const ci::ivec2 & size ) :
        FullScreenQuadRenderer< 1 >( gl::GlslProg::create( gl::GlslProg::Format()
                                                                   .fragment( FRAG )
                                                                   .vertex( VERT )
        ), size )
{
    setTextureName( 0, "uTex" );

    in< 0 >().onReceive( [&]( const gl::Texture2dRef & tex ) {
        setTexture( 0, tex );
        out< 0 >().update( render() );
    } );

    each_in_with_index( [&]( auto & inlet, size_t i ) {
        string n = INLET_NAME_STRINGS.at( (inlet_names)i );
        setUniform( "uEnable" + n, false );

        inlet.onReceive( [&, n]( const auto & v ) {
            setUniform( "uEnable" + n, true );
            setUniform( "u" + n, v );
        });
    }, index_constant< (size_t)first_inlet >{} );
}
