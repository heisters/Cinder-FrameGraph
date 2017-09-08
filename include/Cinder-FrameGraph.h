#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/GeomIo.h"
#include "cinder/gl/gl.h"
#include "cinder/Signals.h"
#include "libnodes/Node.h"
#include "Cinder-FrameGraph/Types.h"

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
    TextureINode( ImageSourceRef img );

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
class TextureShaderIONode : public Node< UniformInlets< gl::Texture2dRef, I >, Outlets< gl::Texture2dRef > >
{
public:
    static TextureShaderIONodeRef< I > create( const ci::gl::GlslProgRef & shader, const ci::ivec2 & size )
    {
        return std::make_shared< TextureShaderIONode >( shader, size );
    }
    TextureShaderIONode( const ci::gl::GlslProgRef & shader, const ci::ivec2 & size ) :
            mBatch( ci::gl::Batch::create( ci::geom::Rect() >> ci::geom::Translate( 0.5f, 0.5f ), shader ) )
    {
        using namespace ci;

        resize( size );
        mTextures.fill( nullptr );
        for ( size_t i = 0; i < mTextureNames.size(); ++i ) {
            mTextureNames[i] = "uTexture" + std::to_string( i );
        }

        this->each_in_with_index( [&]( auto & inlet, size_t i ) {
            inlet.onReceive( [&, i]( const gl::Texture2dRef & tex ) {
                update( i, tex );
            } );
        } );
    }


    virtual void resize( const ci::ivec2 & size )
    {
        using namespace ci;
        mFbo = gl::Fbo::create( size.x, size.y );
        mModelMatrix = scale( vec3( mFbo->getSize(), 1.f ) );
    }

    void setTextureName( std::size_t i, const std::string & name )
    {
        mTextureNames[ i ] = name;
    }

    virtual void update( std::size_t i, const ci::gl::Texture2dRef & texture )
    {
        mTextures[ i ] = texture;
        if ( i == ( TextureShaderIONode< I >::in_size - 1 ) ) update();
    }

    virtual void update()
    {
        using namespace ci;

        if ( ! mFbo ) return;

        {
            gl::ScopedFramebuffer	scp_fbo( mFbo );
            gl::ScopedViewport		scp_viewport( mFbo->getSize() );
            gl::ScopedMatrices		scp_mtx;
            gl::ScopedColor         scp_color( ColorAf( 1.f, 1.f, 1.f, 1.f ) );

            for ( uint8_t i = 0; i < mTextures.size(); ++i ) {
                mTextures.at( i )->bind( i );
                mBatch->getGlslProg()->uniform( mTextureNames.at( i ), i );
            }


            gl::setMatricesWindow( mFbo->getSize() );
            gl::multModelMatrix( mModelMatrix );

            mBatch->getGlslProg()->uniform( "uSize", vec2( mFbo->getSize() ) );

            gl::clear();

            mBatch->draw();

            for ( uint8_t i = 0; i < mTextures.size(); ++i ) {
                mTextures.at( i )->unbind();
            }
        }

        auto tex = mFbo->getColorTexture();
        tex->setTopDown( true );

        this->template out< 0 >().update( tex );
    }

private:
    std::array< gl::Texture2dRef, I > mTextures;
    std::array< std::string, I > mTextureNames;
    ci::gl::BatchRef mBatch;
    ci::gl::FboRef mFbo = nullptr;
    ci::mat4 mModelMatrix;
};


}
}
