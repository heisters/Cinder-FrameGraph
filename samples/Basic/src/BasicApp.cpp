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
	ocio::QTMovieGlINodeRef	mMovieNode;
	ocio::TextureONodeRef	mRawOutputNode;
	ocio::TextureONodeRef	mProcessedOutputNode;
};


BasicApp::BasicApp() :
mConfig( getAssetPath( CONFIG_ASSET_PATH ) )
{

}

void BasicApp::setup()
{
	mMovieNode = ocio::QTMovieGlINode::create( getOpenFilePath() );
	mMovieNode->loop();

	auto process = ocio::ProcessGPUIONode::create(mConfig,
												  ocio::core::ROLE_COMPOSITING_LOG,
												  ocio::core::ROLE_SCENE_LINEAR
												  );
	mProcessedOutputNode = ocio::TextureONode::create();
	mRawOutputNode = ocio::TextureONode::create();

	mMovieNode >> process >> mProcessedOutputNode;
	mMovieNode >> mRawOutputNode;
}

void BasicApp::mouseDown( MouseEvent event )
{
}

void BasicApp::update()
{
	mMovieNode->update();
}

void BasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::draw( *mRawOutputNode, Rectf( vec2( 0, 0 ), (vec2)getWindowSize() * vec2( 1.f, 0.5f ) ) );
	gl::draw( *mProcessedOutputNode, Rectf( (vec2)getWindowSize() * vec2( 0.f, 0.5f ), (vec2)getWindowSize() ) );
}

CINDER_APP( BasicApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
