#pragma once

#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/GeomIo.h"
#include "cinder/gl/gl.h"

namespace cinder {
namespace frame_graph {

template< std::size_t I >
class FullScreenQuadRenderer {
    std::array< ci::gl::Texture2dRef, I >   mTextures;
    std::array< std::string, I >            mTextureNames;
    ci::gl::BatchRef                        mBatch;
    ci::gl::FboRef                          mFbo = nullptr;
    ci::mat4                                mModelMatrix;

public:

    FullScreenQuadRenderer( const ci::gl::GlslProgRef &shader,
                            const ci::ivec2 &size ) :
            mBatch( ci::gl::Batch::create(
                    ci::geom::Rect() >> ci::geom::Translate( 0.5f, 0.5f ),
                    shader )
            )
    {
        resize( size );
        mTextures.fill( nullptr );
        for ( size_t i = 0; i < mTextureNames.size(); ++i ) {
            mTextureNames[i] = "uTexture" + std::to_string( i );
        }


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

protected:

    virtual void setTexture( std::size_t i, const ci::gl::Texture2dRef & texture )
    {
        mTextures[ i ] = texture;
    }

    virtual void render()
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

        mFbo->getColorTexture()->setTopDown( true );
    }

    ci::gl::Texture2dRef getTexture() { return mFbo->getColorTexture(); }

};

}
}