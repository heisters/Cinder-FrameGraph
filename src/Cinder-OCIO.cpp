#include "Cinder-OCIO.h"
#include <algorithm>
#include "cinder/Log.h"
#include "cinder/GeomIo.h"
#include "cinder/gl/gl.h"

using namespace cinder;
using namespace ocio;
using namespace std;

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
// TextureShaderIONode

TextureShaderIONode::TextureShaderIONode( const gl::GlslProgRef & shader )
{
	mBatch = gl::Batch::create( geom::Rect() >> geom::Translate( 0.5f, 0.5f ), shader );
}

void TextureShaderIONode::update( const ci::gl::Texture2dRef & texture )
{
	ivec2 size = texture->getSize();


	if ( ! mFbo || mFbo->getSize() != size ) {
		mFbo = gl::Fbo::create( size.x, size.y );
		mModelMatrix = scale( vec3( mFbo->getSize(), 1.f ) );
	}

	{
		gl::ScopedFramebuffer	scp_fbo( mFbo );
		gl::ScopedViewport		scp_viewport( mFbo->getSize() );
		gl::ScopedMatrices		scp_mtx;
		gl::ScopedTextureBind	scp_tex( texture );

		gl::setMatricesWindow( mFbo->getSize() );
		gl::multModelMatrix( mModelMatrix );

		gl::clear();

		mBatch->draw();
	}

	auto tex = mFbo->getColorTexture();
	tex->setTopDown( true );
	TextureINode::update( tex );
}
