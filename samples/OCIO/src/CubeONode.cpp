#include "CubeONode.h"

using namespace ci;

#define M_TAU 6.28318530717958647692528676655900576839433879875021

CubeONode::CubeONode()
{
	auto cube = geom::Cube().colors(
									ColorAf( 1.f, 0.f, 0.f ),
									ColorAf( 1.f, 1.f, 0.f ),
									ColorAf( 0.f, 1.f, 0.f ),
									ColorAf( 0.f, 1.f, 1.f ),
									ColorAf( 0.f, 0.f, 1.f ),
									ColorAf( 1.f, 0.f, 1.f )
									);
	mBatch = gl::Batch::create( cube, gl::getStockShader( gl::ShaderDef().color() ) );
}

void CubeONode::resize( const ivec2 &size )
{
	mFbo = gl::Fbo::create( size.x , size.y );
	mFbo->getColorTexture()->setTopDown();
	mCam = CameraPersp( size.x, size.y, 60.f, 0.1f, 10.f );
	mCam = mCam.calcFraming( Sphere( vec3(), 1.f ) );
}

void CubeONode::update()
{
	if ( ! mFbo ) return;

	{
		gl::ScopedFramebuffer	scp_fbo( mFbo );
		gl::ScopedViewport		scp_viewport( mFbo->getSize() );
		gl::ScopedMatrices		scp_mtx;
		gl::ScopedDepth			scp_depth( true );
		gl::ScopedBlendAlpha	scp_alpha;

		gl::setMatrices( mCam );
		gl::multModelMatrix( rotate( (float)( M_TAU * 0.1 * app::getElapsedSeconds() ), vec3( 1.f, 0.f, 1.f ) ) );

		gl::clear();

		mBatch->draw();
	}

	TextureINode::update( mFbo->getColorTexture() );
}
