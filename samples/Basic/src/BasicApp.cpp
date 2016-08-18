#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

#include "Cinder-OCIO.h"
#include "Cinder-OCIO/QuickTime.h"

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
	void mouseDrag( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	void updateViewOptions();

	ocio::Config				mConfig;
	ocio::QTMovieGlINodeRef		mMovieNode;
	ocio::ProcessGPUIONodeRef	mProcessNode;
	ocio::TextureONodeRef		mRawOutputNode;
	ocio::TextureONodeRef		mProcessedOutputNode;

	int							mSplitX;

	params::InterfaceGlRef		mParams;
	int							mInputColorSpaceIdx = 0;
	int							mDisplayIdx = 0;
	int							mViewIdx = 0;
	int							mLookIdx = 0;
	vector< string>				mInputColorSpaceNames;
	vector< string >			mDisplayNames;
	vector< string >			mViewNames;
	vector< string >			mLookNames;
	float						mFPS;

	float						mExposureFStop = 0.f;
};

const static string VIEW_MENU_NAME = "View";
const static string LOOK_MENU_NAME = "Looks";

BasicApp::BasicApp() :
mConfig( getAssetPath( CONFIG_ASSET_PATH ) )
{
	mInputColorSpaceNames = mConfig.getAllColorSpaceNames();
	mInputColorSpaceNames.insert( mInputColorSpaceNames.begin(), "" );

	mDisplayNames = mConfig.getAllDisplayNames();
	mDisplayNames.insert( mDisplayNames.begin(), "" );

	mLookNames = mConfig.getAllLookNames();
	mLookNames.insert( mLookNames.begin(), "" );
}

void BasicApp::setup()
{
	mMovieNode = ocio::QTMovieGlINode::create( getOpenFilePath() );
	mMovieNode->loop();

	mProcessNode = ocio::ProcessGPUIONode::create( mConfig );
	mProcessedOutputNode = ocio::TextureONode::create();
	mRawOutputNode = ocio::TextureONode::create();

	mMovieNode >> mProcessNode >> mProcessedOutputNode;
	mMovieNode >> mRawOutputNode;

	vec2 size = mMovieNode->getSize();
	ivec2 dsize = getDisplay()->getSize();
	vec2 maxSize = dsize - ivec2( 20, 20 );
	maxSize = vec2( maxSize.x, size.y * maxSize.x / size.x );
	setWindowSize( min( size, maxSize ) );
	setWindowPos( (vec2)dsize * 0.5f - (vec2)getWindowSize() * 0.5f );

	mSplitX = getWindowWidth() * 0.5f;


	mParams = params::InterfaceGl::create( getWindow(), "Parameters", toPixels( ivec2( 400, 200 ) ) );


	mParams->addParam( "FPS", &mFPS, true ).precision( 1 );

	mParams->addParam( "Exposure", &mExposureFStop ).min( -3.f ).max( 3.f ).step( 0.25f )
	.updateFn([&]{ mProcessNode->setExposureFStop( mExposureFStop ); });

	mParams->addParam( "Input Color Space", mInputColorSpaceNames, &mInputColorSpaceIdx )
	.group( "Color Spaces" )
	.updateFn([&]{ mProcessNode->setInputColorSpace( mInputColorSpaceNames[ mInputColorSpaceIdx ] ); });

	mParams->addParam( "Display", mDisplayNames, &mDisplayIdx )
	.group( "Color Spaces" )
	.updateFn([&]{ mProcessNode->setDisplayColorSpace( mDisplayNames[ mDisplayIdx ] ); updateViewOptions(); });


	mParams->addParam( "Look", mLookNames, &mLookIdx )
	.group( "Color Spaces" )
	.updateFn([&]{ mProcessNode->setLook( mLookNames[ mLookIdx ] ); });

	mParams->addParam( VIEW_MENU_NAME, {}, &mViewIdx )
	.group( "Color Spaces" );

	updateViewOptions();
}

void BasicApp::updateViewOptions()
{
	mParams->removeParam( VIEW_MENU_NAME );
	mViewIdx = 0;

	if ( mConfig.hasViewsForDisplay( mProcessNode->getDisplayColorSpace() ) ) {
		mViewNames = mConfig.getAllViewNames( mProcessNode->getDisplayColorSpace() );
	} else {
		mViewNames = {};
	}

	mParams->addParam( VIEW_MENU_NAME, mViewNames, &mViewIdx )
	.group( "Color Spaces" )
	.updateFn([&]{ mProcessNode->setViewColorSpace( mViewNames[ mViewIdx ] ); });
}

void BasicApp::mouseDrag( MouseEvent event )
{
	if ( event.isLeftDown() ) {
		mSplitX = event.getX();
	}
}

void BasicApp::mouseDown( MouseEvent event )
{
	mSplitX = event.getX();
}

void BasicApp::update()
{
	mMovieNode->update();
	mFPS = getAverageFps();
}

void BasicApp::draw()
{
	gl::clear();
	vec2 size = getWindowSize();
	vec2 scale = vec2( mSplitX, size.y ) / size;

	if ( *mRawOutputNode ) {
		Area src( vec2(), (vec2)mRawOutputNode->getSize() * scale );
		Rectf dst( vec2(), size * scale );
		gl::draw( *mRawOutputNode, src, dst );
	}
	
	if ( *mProcessedOutputNode ) {
		Area src( (vec2)mProcessedOutputNode->getSize() * scale * vec2( 1, 0 ), mProcessedOutputNode->getSize() );
		Rectf dst( size * scale * vec2( 1, 0 ), size );
		gl::draw( *mProcessedOutputNode, src, dst );
	}


	{
		gl::ScopedLineWidth scp_lineWidth( 5.f );
		gl::ScopedColor scp_color( Color( 1.f, 1.f, 1.f ) );
		gl::drawLine( vec2( mSplitX, 0 ), vec2( mSplitX, size.y ) );
	}

	mParams->draw();
}

CINDER_APP( BasicApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
