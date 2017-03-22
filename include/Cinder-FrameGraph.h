#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/Signals.h"
#include "Cinder-FrameGraph/Node.h"

namespace cinder{
namespace frame_graph {

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
class TextureShaderIONode : public TextureIONode
{
public:
    static TextureShaderIONodeRef create( const ci::gl::GlslProgRef & shader )
    {
        return std::make_shared< TextureShaderIONode >( shader );
    }
    TextureShaderIONode( const ci::gl::GlslProgRef & shader );
    virtual void update( const ci::gl::Texture2dRef & texture ) override;

private:
    ci::gl::BatchRef mBatch;
    ci::gl::FboRef mFbo;
    ci::mat4 mModelMatrix;
};


}
}
