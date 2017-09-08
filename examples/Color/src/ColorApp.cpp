#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rect.h"

#include "Cinder-FrameGraph.hpp"
#include "Cinder-FrameGraph/LUTShaderIONode.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace frame_graph;
using namespace frame_graph::operators;

class ColorApp : public App {
public:

    ColorApp();

    void setup() override {};
    void resize() override;
    void mouseDrag( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;

private:

    TextureINode                mSrcImage, mLUTImage;
    TextureONode                mRawOut, mProcessedOut;
    LUTShaderIONode             mLUT;

    int							mSplitX;

};

ColorApp::ColorApp() :
    mSrcImage( loadImage( getOpenFilePath() ) ),
    mLUTImage( loadImage( getAssetPath( "luts/K_TONE_Kodachrome.png" ) ) ),
    mLUT( getWindowSize() )
{
    /***************************************************************************
     * Create node graph
     */


    mSrcImage >>    mLUT;
    mSrcImage >>                    mRawOut;
    mLUTImage >>    mLUT.in< 1 >();
                    mLUT >>         mProcessedOut;


    /***************************************************************************
     * Setup split interface
     */


    ivec2 size = mSrcImage.getSize();
    ivec2 dsize = getDisplay()->getSize();
    ivec2 maxSize = dsize - ivec2( 100 );

    Rectf rSize( vec2( 0 ), (vec2)size );
    Rectf rMax( vec2( 0 ), (vec2)maxSize );
    rSize = rSize.getCenteredFit( rMax, false );

    setWindowSize( rSize.getSize() );
    setWindowPos( (vec2)dsize * 0.5f - (vec2)getWindowSize() * 0.5f );

    mSplitX = (int)( getWindowWidth() * 0.5f );
}

void ColorApp::resize()
{
    mLUT.resize( getWindowSize() );
}

void ColorApp::mouseDrag( MouseEvent event )
{
    if ( event.isLeftDown() ) {
        mSplitX = event.getX();
    }
}

void ColorApp::mouseDown( MouseEvent event )
{
    mSplitX = event.getX();
}

void ColorApp::update()
{
    mSrcImage.update();
    mLUTImage.update();
}

void ColorApp::draw()
{
    gl::clear();
    vec2 size = getWindowSize();
    vec2 scale = vec2( mSplitX, size.y ) / size;

    if ( mRawOut ) {
        Area src( vec2(), (vec2) mRawOut.getSize() * scale );
        Rectf dst( vec2(), size * scale );
        gl::draw( mRawOut, src, dst );
    }

    if ( mProcessedOut ) {
        Area src((vec2) mProcessedOut.getSize() * scale * vec2( 1, 0 ), mProcessedOut.getSize() );
        Rectf dst( size * scale * vec2( 1, 0 ), size );
        gl::draw( mProcessedOut, src, dst );
    }


    {
        gl::ScopedLineWidth scp_lineWidth( 5.f );
        gl::ScopedColor scp_color( Color( 1.f, 1.f, 1.f ) );
        gl::drawLine( vec2( mSplitX, 0 ), vec2( mSplitX, size.y ) );
    }
}


CINDER_APP( ColorApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
