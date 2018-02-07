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
    std::array< std::string, I >            mTextureMatrixNames;
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
        for ( size_t i = 0; i < I; ++i ) {
            mTextureNames[i] = "uTexture" + std::to_string( i );
            mTextureMatrixNames[i] = "uTexture" + std::to_string( i ) + "Mtx";
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

    void setTextureMatrixName( std::size_t i, const std::string & name )
    {
        mTextureMatrixNames[ i ] = name;
    }

protected:

    virtual void setTexture( std::size_t i, const ci::gl::Texture2dRef & texture )
    {
        mTextures[ i ] = texture;
    }

public:

    template< typename T >
    void setUniform( const std::string & name, const T & v )
    {
        mBatch->getGlslProg()->uniform( name, v );
    }

protected:

    ci::gl::BatchRef batch() { return mBatch; }
    ci::gl::FboRef fbo() { return mFbo; }

    virtual void prepareRender() {}

    virtual ci::gl::Texture2dRef render()
    {
        using namespace ci;

        if ( ! mFbo ) return nullptr;

        {
            gl::ScopedFramebuffer	scp_fbo( mFbo );
            gl::ScopedViewport		scp_viewport( mFbo->getSize() );
            gl::ScopedMatrices		scp_mtx;
            gl::ScopedColor         scp_color( ColorAf( 1.f, 1.f, 1.f, 1.f ) );

            gl::setMatricesWindow( mFbo->getSize() );
            gl::multModelMatrix( mModelMatrix );
            auto shader = mBatch->getGlslProg();

            for ( uint8_t i = 0; i < mTextures.size(); ++i ) {
                auto tex = mTextures.at( i );
                if ( ! tex ) continue;
                auto name = mTextureNames.at( i );
                auto mtxName = mTextureMatrixNames.at( i );

                tex->bind( i );

                shader->uniform( name, i );

                int loc;
                if ( shader->findUniform( mtxName, &loc ) != nullptr ) {
                    mat4 m;
                    if ( !tex->isTopDown()) {
                        m = translate( vec3( 0.f, 1.f, 0.f )) *
                            scale( vec3( 1.f, -1.f, 1.f ));
                    }
                    shader->uniform( loc, m );
                }
            }


            int loc;
            if ( shader->findUniform( "uSize", &loc ) != nullptr ) {
                shader->uniform( loc, vec2( mFbo->getSize()));
            }

            prepareRender();

            gl::clear();

            mBatch->draw();

            for ( uint8_t i = 0; i < mTextures.size(); ++i ) {
                auto tex = mTextures.at( i );
                if ( ! tex ) continue;
                tex->unbind();
            }
        }

        auto tex = mFbo->getColorTexture();
        tex->setTopDown( true );
        return tex;
    }

    ci::gl::Texture2dRef getTexture() { return mFbo->getColorTexture(); }

};

}
}