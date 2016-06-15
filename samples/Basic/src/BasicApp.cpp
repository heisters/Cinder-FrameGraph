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

class BasicApp : public App {
public:
	BasicApp();

	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	ocio::Config	mConfig;
	Surface32f		mSurf, mSurfProcessed;
};


BasicApp::BasicApp() :

mConfig( getAssetPath( CONFIG_ASSET_PATH ) )
{

}

void BasicApp::setup()
{
	mSurf = Surface32f( loadImage( getOpenFilePath() ) );
	mSurfProcessed = mSurf.clone();
	ocio::process( mConfig, &mSurfProcessed );
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
	gl::draw( gl::Texture2d::create( mSurfProcessed ), Rectf( (vec2)getWindowSize() * vec2( 0.f, 0.5f ), (vec2)getWindowSize() ) );
}

CINDER_APP( BasicApp, RendererGl )
