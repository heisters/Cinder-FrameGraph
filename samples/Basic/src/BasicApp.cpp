#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

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
	void mouseDrag( MouseEvent event ) override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	void						updateColorSpaces();

	ocio::Config				mConfig;
	ocio::QTMovieGlINodeRef		mMovieNode;
	ocio::ProcessGPUIONodeRef	mProcessNode;
	ocio::TextureONodeRef		mRawOutputNode;
	ocio::TextureONodeRef		mProcessedOutputNode;

	int							mSplitX;

	params::InterfaceGlRef		mParams;
	int							mSrcColorSpaceIdx, mDstColorSpaceIdx;
	vector< string>				mColorSpaceNames;
	float						mFPS;
};


BasicApp::BasicApp() :
mConfig( getAssetPath( CONFIG_ASSET_PATH ) ),
mSrcColorSpaceIdx( 0 ),
mDstColorSpaceIdx( 0 )
{
	mColorSpaceNames = mConfig.getAllColorSpaceNames();
	mColorSpaceNames.insert( mColorSpaceNames.begin(), "none" );
}

void BasicApp::setup()
{
	mMovieNode = ocio::QTMovieGlINode::create( getOpenFilePath() );
	mMovieNode->loop();

	mProcessedOutputNode = ocio::TextureONode::create();
	mRawOutputNode = ocio::TextureONode::create();

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
	mParams->addParam( "Source Color Space", mColorSpaceNames, &mSrcColorSpaceIdx )
	.updateFn([&]{ updateColorSpaces(); });
	mParams->addParam( "Destination Color Space", mColorSpaceNames, &mDstColorSpaceIdx )
	.updateFn([&]{ updateColorSpaces(); });
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
		Area src( vec2(), mRawOutputNode->getSize() * scale );
		Rectf dst( vec2(), size * scale );
		gl::draw( *mRawOutputNode, src, dst );
	}
	
	if ( *mProcessedOutputNode ) {
		Area src( mProcessedOutputNode->getSize() * scale * vec2( 1, 0 ), mProcessedOutputNode->getSize() );
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

void BasicApp::updateColorSpaces()
{
	string src = mColorSpaceNames[ mSrcColorSpaceIdx ];
	string dst = mColorSpaceNames[ mDstColorSpaceIdx ];

	if ( mProcessNode )
		mMovieNode->disconnect( mProcessNode );

	if ( src != "none" && dst != "none" ) {
		mProcessNode = ocio::ProcessGPUIONode::create( mConfig, src, dst );
		mMovieNode >> mProcessNode >> mProcessedOutputNode;
	} else {
		mProcessNode = nullptr;
	}
}

CINDER_APP( BasicApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
