#include "Cinder-FrameGraph.hpp"
#include <algorithm>
#include "cinder/Log.h"

using namespace cinder;
using namespace frame_graph;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// SurfaceINode

SurfaceINode::SurfaceINode( const Surface32fRef & surface ) :
    mSurface( surface )
{
}

void SurfaceINode::update()
{
    out< 0 >().update( mSurface );
}

////////////////////////////////////////////////////////////////////////////////
// TextureINode

TextureINode::TextureINode() :
mTexture( nullptr )
{}

TextureINode::TextureINode( ImageSourceRef img, const ci::gl::Texture2d::Format & fmt ) :
mTexture( gl::Texture2d::create( img, fmt ) )
{

}

void TextureINode::update()
{
    out< 0 >().update( mTexture );
}

void TextureINode::update( const gl::Texture2dRef & texture )
{
    mTexture = texture;
    out< 0 >().update( mTexture );
}

////////////////////////////////////////////////////////////////////////////////
// TextureONode

TextureONode::TextureONode()
{
    in< 0 >().onReceive( [&]( const gl::Texture2dRef & tex ) {
        update( tex );
    } );
    in< 1 >().onReceive( [&]( const Surface32fRef & img ) {
        update( img );
    } );
}

void TextureONode::update( const Surface32fRef & image )
{
    update( gl::Texture2d::create( *image ) );
}

void TextureONode::update( const gl::Texture2dRef & texture )
{
    mTexture = texture;
}

////////////////////////////////////////////////////////////////////////////////
// TextureIONode

TextureIONode::TextureIONode()
{
    in< 0 >().onReceive( [&]( const gl::Texture2dRef & tex ) {
        update( tex );
    } );
}

void TextureIONode::update( const gl::Texture2dRef & texture )
{
    mTexture = texture;
    out< 0 >().update( mTexture );
}
