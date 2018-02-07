#include "Cinder-FrameGraph/GLVideo.hpp"

using namespace glvideo;
using namespace cinder;
using namespace std;
using namespace frame_graph;


GLVideoINode::GLVideoINode( const glvideo::Context::ref & context, const fs::path & path, bool playImmediately ) :
	mMovie( Movie::create( context, path.string(), Movie::Options().cpuBufferSize( 2 ).prebuffer( false ) ) )
{
	if ( playImmediately ) mMovie->play();

}

GLVideoINode::GLVideoINode( const glvideo::Movie::ref & movie ) :
    mMovie( movie )
{
}

void GLVideoINode::update()
{
    mMovie->update();
	auto frame = mMovie->getCurrentFrame();
	if ( frame ) {
		auto tex = gl::Texture2d::create( frame->getTextureTarget(), frame->getTextureId(), mMovie->getWidth(), mMovie->getHeight(), true /* doNotDispose */ );
		tex->setTopDown( true );
		//frame->setOwnsTexture( false );
		TextureINode::update( tex );
	}
}


///////////////////////////////////////////////////////////////////////////////
// Vertex shader
static string vertex = CI_GLSL( 410,
uniform mat4	        ciModelViewProjection;
in vec4			        ciPosition;
in vec2			        ciTexCoord0;

out vec2		        vTexCoord;

void main( void ) {
    vTexCoord = ciTexCoord0;
    gl_Position = ciModelViewProjection * ciPosition;
}
);

///////////////////////////////////////////////////////////////////////////////
// YCoCg fragment shader

static string fragmentYCoCg = CI_GLSL( 410,
uniform sampler2D       uTexture0;
uniform vec2            uSize;
out vec4			    oColor;
in vec2				    vTexCoord;

void main( void ) {
   vec4 color = texture( uTexture0, vTexCoord );
   float Co = color.x - ( 0.5 * 256.0 / 255.0 );
   float Cg = color.y - ( 0.5 * 256.0 / 255.0 );
   float Y = color.w;
   oColor = vec4( Y + Co - Cg, Y + Cg, Y - Co - Cg, 1.0 );
}
);


GLVideoHapQDecodeShaderIONode::GLVideoHapQDecodeShaderIONode( const ci::ivec2 & size ) :
TextureShaderIONode( gl::GlslProg::create( gl::GlslProg::Format().vertex( vertex ).fragment( fragmentYCoCg ) ), size )
{
}