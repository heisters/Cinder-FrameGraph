#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

#include "Cinder-FrameGraph.hpp"
#include "Cinder-FrameGraph/QuickTime.hpp"
#include "Cinder-FrameGraph/OCIO.hpp"

#include "CubeONode.h"


// This config is from https://github.com/imageworks/OpenColorIO-Configs
const std::string CONFIG_ASSET_PATH = "aces_1.0.1/config.ocio";

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace frame_graph::operators;


class OCIOApp : public App {
public:
	OCIOApp();

	void setup() override;
	void resize() override;
	void mouseDrag( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	void updateViewOptions();

	frame_graph::ocio::Config				mConfig;
	frame_graph::QTMovieGlINodeRef			mMovieNode;
	frame_graph::ocio::ProcessGPUIONodeRef	mProcessNode;
	frame_graph::TextureONodeRef			mRawOutputNode;
	frame_graph::TextureONodeRef			mProcessedOutputNode;
	CubeONodeRef							mCubeNode;

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

	vector< string >			mScenes{ "video", "cube" };
	int							mSceneIdx = 0;
};

const static string VIEW_MENU_NAME = "View";

OCIOApp::OCIOApp() :
mConfig( getAssetPath( CONFIG_ASSET_PATH ) )
{
	mInputColorSpaceNames = mConfig.getAllColorSpaceNames();
	mInputColorSpaceNames.insert( mInputColorSpaceNames.begin(), "" );

	mDisplayNames = mConfig.getAllDisplayNames();
	mDisplayNames.insert( mDisplayNames.begin(), "" );

	mLookNames = mConfig.getAllLookNames();
	mLookNames.insert( mLookNames.begin(), "" );
}

void OCIOApp::setup()
{
	/***************************************************************************
	 * Create node graph
	 */


	mMovieNode = frame_graph::QTMovieGlINode::create( getOpenFilePath() );
	mMovieNode->loop();

	mCubeNode = CubeONode::create();

	mProcessNode = frame_graph::ocio::ProcessGPUIONode::create( mConfig );
	mProcessedOutputNode = frame_graph::TextureONode::create();
	mRawOutputNode = frame_graph::TextureONode::create();

	mMovieNode >> mProcessNode;
	mMovieNode >> mRawOutputNode;

	mCubeNode >> mProcessNode;
	mCubeNode >> mRawOutputNode;

	mProcessNode >> mProcessedOutputNode;


	/***************************************************************************
	 * Setup split interface
	 */


	vec2 size = mMovieNode->getSize();
	ivec2 dsize = getDisplay()->getSize();
	vec2 maxSize = dsize - ivec2( 20, 20 );
	maxSize = vec2( maxSize.x, size.y * maxSize.x / size.x );
	setWindowSize( min( size, maxSize ) );
	setWindowPos( (vec2)dsize * 0.5f - (vec2)getWindowSize() * 0.5f );

	mSplitX = getWindowWidth() * 0.5f;


	/***************************************************************************
	 * Create param panel
	 */

	
	mParams = params::InterfaceGl::create( getWindow(), "Parameters", toPixels( ivec2( 400, 200 ) ) );


	mParams->addParam( "FPS", &mFPS, true ).precision( 1 );

	mParams->addParam( "Exposure", &mExposureFStop ).min( -3.f ).max( 3.f ).step( 0.25f )
	.updateFn([&]{ mProcessNode->setExposureFStop( mExposureFStop ); });

	mParams->addParam( "Scene", mScenes, &mSceneIdx );

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


	resize();
}

void OCIOApp::resize()
{
	mCubeNode->resize( getWindowSize() );
}

void OCIOApp::updateViewOptions()
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

void OCIOApp::mouseDrag( MouseEvent event )
{
	if ( event.isLeftDown() ) {
		mSplitX = event.getX();
	}
}

void OCIOApp::mouseDown( MouseEvent event )
{
	mSplitX = event.getX();
}

void OCIOApp::update()
{
	mFPS = getAverageFps();

	if ( mScenes[ mSceneIdx ] == "video" ) {
		mMovieNode->update();
	} else {
		mCubeNode->update();
	}
}

void OCIOApp::draw()
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

CINDER_APP( OCIOApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
