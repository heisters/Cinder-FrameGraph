#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"

#include "Cinder-OCIO.h"

// This config is from https://github.com/imageworks/OpenColorIO-Configs
const std::string CONFIG_ASSET_PATH = "aces_1.0.1/config.ocio";

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ocio::operators;

class BasicApp : public App {
public:
	BasicApp();

	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	ocio::Config			mConfig;
	ocio::SurfaceINodeRef	mRootNode;
	ocio::TextureONodeRef	mOutputNode;
	Surface32f				mSurf;
};


BasicApp::BasicApp() :
mConfig( getAssetPath( CONFIG_ASSET_PATH ) )
{

}

void BasicApp::setup()
{
	mSurf = Surface32f( loadImage( getOpenFilePath() ) );
	mRootNode = ocio::SurfaceINode::create( Surface32f::create( mSurf.clone() ) );
	auto process = ocio::ProcessIONode::create(mConfig,
											   ocio::core::ROLE_COMPOSITING_LOG,
											   ocio::core::ROLE_SCENE_LINEAR
											   );
	mOutputNode = ocio::TextureONode::create();

	mRootNode >> process >> mOutputNode;

	mRootNode->update();
}

void BasicApp::mouseDown( MouseEvent event )
{
}

void BasicApp::update()
{
}

void BasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::draw( gl::Texture2d::create( mSurf ), Rectf( vec2( 0, 0 ), (vec2)getWindowSize() * vec2( 1.f, 0.5f ) ) );
	gl::draw( *mOutputNode, Rectf( (vec2)getWindowSize() * vec2( 0.f, 0.5f ), (vec2)getWindowSize() ) );
}

CINDER_APP( BasicApp, RendererGl )
