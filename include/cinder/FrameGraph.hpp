#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Signals.h"
#include "libnodes/Node.h"
#include "libnodes/operators.h"
#include "cinder/framegraph/Types.hpp"
#include "cinder/framegraph/FullScreenQuadRenderer.hpp"

namespace cinder{
namespace frame_graph {

using namespace nodes;

//! A node that inputs a Cinder Surface32f.
class SurfaceINode : public Node< Inlets<>, Outlets< Surface32fRef > >
{
public:
    static ref< SurfaceINode > create( const Surface32fRef & surface )
    {
        return std::make_shared< SurfaceINode >( surface );
    }

    SurfaceINode( const Surface32fRef & surface );

    virtual void update();

    const ci::Surface32fRef & getSurface() const { return mSurface; }

    operator const ci::Surface32fRef & ( ) const { return getSurface(); }

private:
    Surface32fRef mSurface;
};

//! A node that emits OpenGL textures.
class TextureINode : public Node< Inlets<>, Outlets< gl::Texture2dRef > >
{
public:
    static TextureINodeRef create( ImageSourceRef img, const ci::gl::Texture2d::Format & fmt = ci::gl::Texture2d::Format() )
    {
        return std::make_shared< TextureINode >( img, fmt );
    }

    TextureINode();
    TextureINode( ImageSourceRef img, const ci::gl::Texture2d::Format & fmt = ci::gl::Texture2d::Format() );

    inline ci::ivec2 getSize() const { return mTexture->getSize(); }

    virtual void update();
protected:
    virtual void update( const ci::gl::Texture2dRef & texture );

private:
    ci::gl::Texture2dRef			mTexture = nullptr;
};

//! A node that represents a Cinder gl::Texture2d, useful for displaying
//! results.
class TextureONode : public Node< Inlets< gl::Texture2dRef, Surface32fRef >, Outlets<> >
{
public:
    static TextureONodeRef create()
    {
        return std::make_shared< TextureONode >();
    }

    TextureONode();

    void update( const Surface32fRef & image );
    void update( const gl::Texture2dRef & texture );

    void clear() { mTexture = nullptr; }
    const ci::gl::Texture2dRef getTexture() const { return mTexture; }

    operator const ci::gl::Texture2dRef() const { return getTexture(); }
    operator const bool() const { return mTexture != nullptr; }

    ci::ivec2 getSize() const { return mTexture->getSize(); }
private:

    ci::gl::Texture2dRef	mTexture = nullptr;
};

class TextureIONode : public Node< Inlets< gl::Texture2dRef >, Outlets< gl::Texture2dRef > >
{
public:
    TextureIONode();
    virtual void update( const ci::gl::Texture2dRef & texture );
private:
    ci::gl::Texture2dRef			mTexture = nullptr;
};

//! A node that applies a shader to a texture input.
template< std::size_t I = 1 >
class TextureShaderIONode :
        public Node< UniformInlets< gl::Texture2dRef, I >, Outlets< gl::Texture2dRef > >,
        public FullScreenQuadRenderer< I >
{
public:
    typedef FullScreenQuadRenderer< I >::WATCH WATCH;

    static TextureShaderIONodeRef< I > create( const ci::gl::GlslProgRef & shader, const ci::ivec2 & size )
    {
        return std::make_shared< TextureShaderIONode >( shader, size );
    }

    static TextureShaderIONodeRef< I > create( const ci::gl::GlslProg::Format & format, const ci::ivec2 & size )
    {
        return std::make_shared< TextureShaderIONode >( format, size );
    }

    static TextureShaderIONodeRef< I > create( ci::DataSourceRef vertexShader, ci::DataSourceRef fragmentShader, const ci::ivec2 & size )
    {
        return std::make_shared< TextureShaderIONode >( vertexShader, fragmentShader, size );
    }

    static TextureShaderIONodeRef< I > create( ci::DataSourceRef vertexShader, ci::DataSourceRef fragmentShader, const ci::ivec2 & size, WATCH watch, std::function< void( const WatchEvent& ) > watchCb = [](const WatchEvent&){} )
    {
        return std::make_shared< TextureShaderIONode >( vertexShader, fragmentShader, size, watch, watchCb );
    }

    TextureShaderIONode( const ci::gl::GlslProgRef & shader, const ci::ivec2 & size ) :
        FullScreenQuadRenderer< I >( shader, size )
    {
        listen();
    }

    TextureShaderIONode( const ci::gl::GlslProg::Format & format, const ci::ivec2 & size ) :
        FullScreenQuadRenderer< I >( format, size )
    {
        listen();
    }

    TextureShaderIONode( ci::DataSourceRef vertexShader, ci::DataSourceRef fragmentShader, const ci::ivec2 & size ) :
        FullScreenQuadRenderer< I >( vertexShader, fragmentShader, size )
    {
        listen();
    }

    TextureShaderIONode( ci::DataSourceRef vertexShader, ci::DataSourceRef fragmentShader, const ci::ivec2 & size, WATCH watch, std::function< void( const WatchEvent& ) > watchCb = [](const WatchEvent&){} ) :
        FullScreenQuadRenderer< I >( vertexShader, fragmentShader, size, watch, watchCb )
    {
        listen();
    }

protected:
    void listen()
    {
        this->inlets().each_with_index( [&]( auto & inlet, size_t i ) {
            inlet.onReceive( [&, i]( const ci::gl::Texture2dRef & tex ) {
                update( i, tex );
            } );
        } );
    }

public:

    virtual void update( std::size_t i, const ci::gl::Texture2dRef & texture )
    {
        this->setTexture( i, texture );
        if ( i == ( TextureShaderIONode< I >::in_size - 1 ) ) update();
    }

    virtual void update()
    {
        this->template out< 0 >().update( this->render() );
    }

private:
};


}
}
