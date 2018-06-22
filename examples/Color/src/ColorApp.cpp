#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "cinder/FrameGraph.hpp"
#ifdef USE_GLVIDEO
#include "cinder/framegraph/GLVideo.hpp"
#endif
#include "cinder/framegraph/LUTNode.hpp"
#include "cinder/framegraph/ColorGradeNode.hpp"
#include "cinder/framegraph/VecNode.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace frame_graph;
using namespace frame_graph::operators;

class ColorApp : public App {
public:

    ColorApp();

    void setup() override;
    void resize() override;
    void mouseDrag( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;

private:

#ifdef USE_GLVIDEO
    glvideo::Context::ref           mVideoCtx;
    GLVideoINode                    mSrc;
    GLVideoHapQDecodeShaderIONode   mSrcDecoder;
#else
    TextureINode                    mSrc;
#endif
    TextureINode                    mLUTImage;
    TextureONode                    mOut, mLUTOut;
    LUTNode                         mLUT;
    ColorGradeNode                  mGrader;

    int							    mSplitX;

    params::InterfaceGlRef	        mParams;

    struct grade {
        nodes::ValueNodef exposure{ 0.f };
        Vec3Node<> lgg{ vec3( 0.f, 0.f, 0.f ) };
        nodes::ValueNodef temperature{ 6500.f };
        nodes::ValueNodef contrast{ 0.f };
        nodes::ValueNodef midtone_contrast{ 0.f };
        Vec3Node<> hsv{ vec3( 0.f, 0.f, 0.f ) };
    };
    grade                           mGrade;
};

ColorApp::ColorApp() :
#ifdef USE_GLVIDEO
    mVideoCtx( glvideo::Context::create( 1 ) ),
    mSrc( mVideoCtx, getOpenFilePath( fs::path(), {"mp4", "mov"} ) ),
    mSrcDecoder( mSrc.getSize() ),
#else
    mSrc( loadImage( getOpenFilePath( fs::path(), {"jpg", "png"} ) ) ),
#endif
    mLUTImage( loadImage( getAssetPath( "luts/K_TONE_Kodachrome.png" ) ) ),
    mLUT( getWindowSize() ),
    mGrader( getWindowSize() )
{
    /***************************************************************************
     * Create node graph
     */
#ifdef USE_GLVIDEO
    mSrc.play().loop();
    mSrc >> mSrcDecoder;
    auto & src = mSrcDecoder;
#else
    auto & src = mSrc;
#endif

    src >>                      mGrader >>                              mOut;
    src >>                      mLUT >>                                 mLUTOut;
    mLUTImage >>                mLUT.in< 1 >();

    mGrade.exposure >>          mGrader.in< ColorGradeNode::exposure >();
    mGrade.lgg >>               mGrader.in< ColorGradeNode::LGG >();
    mGrade.temperature >>       mGrader.in< ColorGradeNode::temperature >();
    mGrade.contrast >>          mGrader.in< ColorGradeNode::contrast >();
    mGrade.midtone_contrast >>  mGrader.in< ColorGradeNode::midtone_contrast >();
    mGrade.hsv >>               mGrader.in< ColorGradeNode::HSV >();



    /***************************************************************************
     * Setup split interface
     */


    ivec2 size = mSrc.getSize();
    ivec2 dsize = getDisplay()->getSize();
    ivec2 maxSize = dsize - ivec2( 100 );

    Rectf rSize( vec2( 0 ), (vec2)size );
    Rectf rMax( vec2( 0 ), (vec2)maxSize );
    rSize = rSize.getCenteredFit( rMax, false );

    setWindowSize( rSize.getSize() );
    setWindowPos( (vec2)dsize * 0.5f - (vec2)getWindowSize() * 0.5f );

    mSplitX = (int)( getWindowWidth() * 0.5f );
}

void ColorApp::setup()
{
    mParams = params::InterfaceGl::create( getWindow(), "Color Grading", toPixels( ivec2( 200, 400 ) ) );

    mParams->addParam( "Exposure", &mGrade.exposure.get() )
            .updateFn([&]() { mGrade.exposure.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Lift", &mGrade.lgg.x() )
            .updateFn([&]() { mGrade.lgg.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Gamma", &mGrade.lgg.y() )
            .updateFn([&]() { mGrade.lgg.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Gain", &mGrade.lgg.z() )
            .updateFn([&]() { mGrade.lgg.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Temperature", &mGrade.temperature.get() )
            .updateFn([&]() { mGrade.temperature.update(); })
            .min( 1000 ).max( 40000 )
            .step( 100 )
            ;
    mParams->addParam( "Contrast", &mGrade.contrast.get() )
            .updateFn([&]() { mGrade.contrast.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Midtone Contrast", &mGrade.midtone_contrast.get() )
            .updateFn([&]() { mGrade.midtone_contrast.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Hue", &mGrade.hsv.x() )
            .updateFn([&]() { mGrade.hsv.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Saturation", &mGrade.hsv.y() )
            .updateFn([&]() { mGrade.hsv.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
    mParams->addParam( "Value", &mGrade.hsv.z() )
            .updateFn([&]() { mGrade.hsv.update(); })
            .min( -2.f ).max( 2.f )
            .step( 0.05f )
            ;
}

void ColorApp::resize()
{
    auto size = getWindowSize();
    mLUT.resize( size );
    mGrader.resize( size );
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
    mSrc.update();
    mLUTImage.update();
}

void ColorApp::draw()
{
    gl::clear();
    vec2 size = getWindowSize();
    vec2 scale = vec2( mSplitX, size.y ) / size;

    if ( mOut ) {
        Area src( vec2(), (vec2) mOut.getSize() * scale );
        Rectf dst( vec2(), size * scale );
        gl::draw( mOut, src, dst );
    }

    if ( mLUTOut ) {
        Area src((vec2) mLUTOut.getSize() * scale * vec2( 1, 0 ), mLUTOut.getSize() );
        Rectf dst( size * scale * vec2( 1, 0 ), size );
        gl::draw( mLUTOut, src, dst );
    }


    {
        gl::ScopedLineWidth scp_lineWidth( 5.f );
        gl::ScopedColor scp_color( Color( 1.f, 1.f, 1.f ) );
        gl::drawLine( vec2( mSplitX, 0 ), vec2( mSplitX, size.y ) );
    }

    mParams->draw();
}


CINDER_APP( ColorApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
