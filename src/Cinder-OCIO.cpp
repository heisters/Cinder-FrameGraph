#include "Cinder-OCIO.h"
#include "cinder/Log.h"
#include <algorithm>

using namespace cinder;
using namespace ocio;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
// Config

Config::Config( const ci::fs::path & path )
{
	mConfig = core::Config::CreateFromFile( path.string().c_str() );

	int n = mConfig->getNumColorSpaces();
	for ( int i = 0; i < n; ++i ) {
		mAllColorSpaceNames.push_back( string( mConfig->getColorSpaceNameByIndex( i ) ) );
	}
}

////////////////////////////////////////////////////////////////////////////////
// INode

void INode::update()
{
	for ( auto & output : mOutputs ) output->update();
}

void INode::connect( const ONodeRef & output )
{
	mOutputs.push_back( output );
}

void INode::disconnect( const ONodeRef &output )
{
	mOutputs.erase( remove( mOutputs.begin(), mOutputs.end(), output ), mOutputs.end() );
}

////////////////////////////////////////////////////////////////////////////////
// ImageINode
void ImageINode::connect( const ImageONodeRef & output )
{
	mOutputs.push_back( output );
}

void ImageINode::disconnect( const ImageONodeRef &output )
{
	mOutputs.erase( remove( mOutputs.begin(), mOutputs.end(), output ), mOutputs.end() );
}


////////////////////////////////////////////////////////////////////////////////
// SurfaceINode

SurfaceINode::SurfaceINode( const Surface32fRef & surface ) :
mSurface( surface )
{
}

void SurfaceINode::update()
{
	for ( auto & output : mOutputs ) output->update( mSurface );
}


////////////////////////////////////////////////////////////////////////////////
// TextureINode
void TextureINode::connect( const TextureONodeRef & output )
{
	mOutputs.push_back( output );
}
void TextureINode::disconnect( const TextureONodeRef &output )
{
	mOutputs.erase( remove( mOutputs.begin(), mOutputs.end(), output ), mOutputs.end() );
}

void TextureINode::update()
{
	for ( auto & output : mOutputs ) output->update( mOutTex );
}

void TextureINode::update( const gl::Texture2dRef & texture )
{
	mOutTex = texture;
	for ( auto & output : mOutputs ) output->update( mOutTex );
}

////////////////////////////////////////////////////////////////////////////////
// TextureONode

TextureONode::TextureONode()
{}

void TextureONode::update( const Surface32fRef & image )
{
	mTexture = gl::Texture2d::create( *image );
}

void TextureONode::update( const gl::Texture2dRef & texture )
{
	mTexture = texture;
}



////////////////////////////////////////////////////////////////////////////////
// ProcessIONode


ProcessIONode::ProcessIONode(const Config & config,
							 const string & src,
							 const string & dst) :
mConfig( config )
{
	mProcessor = mConfig->getProcessor( src.c_str(), dst.c_str() );
}

void ProcessIONode::update( const Surface32fRef & image )
{
	core::PackedImageDesc pid( image->getData(), image->getWidth(), image->getHeight(), image->hasAlpha() ? 4 : 3 );

	mProcessor->apply( pid );

	for ( auto & output : mOutputs ) output->update( image );
}

////////////////////////////////////////////////////////////////////////////////
// ProcessGPUIONode

static const std::string FRAG_SHADER_HEADER =
CI_GLSL(150,
		vec4 texture3D( sampler3D s, vec3 p ) { return texture(s, p); }
		vec4 texture2D( sampler2D s, vec2 p ) { return texture(s, p); }
		uniform RAW_SAMPLER uRawTex;
		uniform sampler3D uLUTTex;

		in vec2 vTexCoord0;
		out vec4 oColor;
		);

static const std::string FRAG_SHADER_MAIN =
CI_GLSL(150,
		void main()
		{
			vec4 col = texture( uRawTex, vTexCoord0.st );
			oColor = OCIODisplay( col, uLUTTex );
		}
		);

static const std::string VERT_SHADER =
CI_GLSL(150,
		uniform mat4	ciModelViewProjection;
		uniform vec2	uTexCoordScale;
		in vec4			ciPosition;
		in vec2			ciTexCoord0;
		out vec2		vTexCoord0;

		void main( void ) {
			gl_Position	= ciModelViewProjection * ciPosition;
			vTexCoord0 = ciTexCoord0 * uTexCoordScale;
		}
		);


ProcessGPUIONode::ProcessGPUIONode(const Config & config,
								   const string & src,
								   const string & dst) :
mConfig( config )
{
	static const int sSize = 32;

	mProcessor = mConfig->getProcessor( src.c_str(), dst.c_str() );


	mShaderDesc.setLanguage( core::GPU_LANGUAGE_GLSL_1_3 );
	mShaderDesc.setFunctionName( "OCIODisplay" );
	mShaderDesc.setLut3DEdgeLen( sSize );


	int num3Dentries = 3 * sSize * sSize * sSize;
	vector< float > g_lut3d;
	g_lut3d.resize(num3Dentries);
	mProcessor->getGpuLut3D( &g_lut3d[0], mShaderDesc );

	{
		gl::Texture3d::Format fmt;
		fmt.minFilter( GL_LINEAR ).magFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE );
		fmt.setDataType( GL_FLOAT );
		fmt.setInternalFormat( GL_RGB16F_ARB );
		mLUTTex = gl::Texture3d::create( g_lut3d.data(), GL_RGB, sSize, sSize, sSize, fmt );
	}
}

void ProcessGPUIONode::updateBatch( const BatchFormat & fmt )
{
	if ( mBatchFormat == fmt ) return;
	mBatchFormat = fmt;


	std::ostringstream os;
	os << FRAG_SHADER_HEADER << mProcessor->getGpuShaderText( mShaderDesc ) << FRAG_SHADER_MAIN;

	gl::GlslProg::Format shaderFmt;
	shaderFmt.vertex( VERT_SHADER ).fragment( os.str() );

	string sampler( "sampler2D" );
	if ( mBatchFormat.getTextureTarget() == GL_TEXTURE_RECTANGLE_ARB )
		sampler = "sampler2DRect";

	shaderFmt.define( "RAW_SAMPLER", sampler );


	auto shader = gl::GlslProg::create( shaderFmt );
	shader->uniform( "uRawTex", 0 );
	shader->uniform( "uLUTTex", 1 );

	vec2 texCoordScale( 1.f, 1.f );
	if ( mBatchFormat.getTextureTarget() == GL_TEXTURE_RECTANGLE_ARB )
		texCoordScale = mBatchFormat.getTextureSize();

	shader->uniform( "uTexCoordScale", texCoordScale );

	mBatch = gl::Batch::create( geom::Rect(), shader );
}

void ProcessGPUIONode::update( const gl::Texture2dRef & texture )
{
	if ( ! mFbo || mFbo->getWidth() != texture->getWidth() || mFbo->getHeight() != texture->getHeight() ) {
		mFbo = gl::Fbo::create( texture->getWidth(), texture->getHeight() );
		mModelMatrix = scale( vec3( mFbo->getSize(), 1.f ) ) * translate( vec3( 0.5f, 0.5f, 0.f ) );
	}

	updateBatch( BatchFormat().textureTarget( texture->getTarget() ).textureSize( texture->getSize() ) );

	{
		gl::ScopedFramebuffer scp_fbo( mFbo );
		gl::ScopedViewport scp_viewport( mFbo->getSize() );
		gl::ScopedMatrices scp_matrices;
		gl::ScopedTextureBind scp_rawTex( texture, 0 );
		gl::ScopedTextureBind scp_lutTex( mLUTTex, 1 );

		gl::setMatricesWindow( mFbo->getWidth(), mFbo->getHeight(), false );
		gl::multModelMatrix( mModelMatrix );
		gl::clear();

		mBatch->draw();
	}

	TextureINode::update( mFbo->getColorTexture() );
}

////////////////////////////////////////////////////////////////////////////////
// QTMovieGlINode

QTMovieGlINode::QTMovieGlINode( const fs::path & path ) :
mMovie( qtime::MovieGl::create( path ) )
{
	mMovie->play();
}

void QTMovieGlINode::update()
{
	auto tex = mMovie->getTexture();
	if ( tex ) TextureINode::update( tex );
}