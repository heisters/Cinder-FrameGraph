#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"

#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"
#include "mapbox/variant.hpp"
#include "mapbox/variant_io.hpp"

namespace cinder { namespace frame_graph {

//! Abstract base class for all inlets.
class Xlet : private Noncopyable
{
public:
};

//! An Inlet accepts \a in_data_ts to its receive method, and returns
//! \a read_t from its read method.
template< typename ... in_data_ts, typename read_t = typename std::tuple< ... in_data_ts > >
class Inlet : public Xlet
{
public:
    typedef connection_container< typename Inlet< ... in_data_ts > > container;
    typedef ref< typename Inlet< ... in_data_ts > > ref;

    virtual void receive( in_data_ts ... data ) { mData = std::make_tuple( data... ); }
    virtual const read_t & read() const { return mData; }

private:
    read_t mData;
};

//! An Outlet connects to an \a out_data_ts Inlet, and is updated with
//! \a update_t.
template< typename update_t, typename ... out_data_ts >
class Outlet : public Xlet
{
public:
    typedef typename Inlet< ... out_data_ts > inlet_t;

    virtual void update( update_t data )
    {
        for ( auto & c : mConnections ) {
            c->receive( data );
        }
    }
    
    bool connect( const inlet_t::ref & in ) { return mConnections.insert( in ); }
    bool disconnect( const inlet_t::ref & in ) { return mConnections.erase( in ); }
    void disconnect() { mConnections.clear(); }
    bool isConnected() const { return ! mConnections.empty(); }

private:
    typename inlet_t::container mConnections;
};

template< typename xlet_t, typename ... data_ts >
class Xlets
{
public:
    typedef std::tuple< xlet_t< in_data_ts > ... > tuple;

    template< size_t I >
    auto get() const { return std::get< I >( mTuple ); }

    template< typename T >
    auto get() const { return std::get< T >( mTuple ); }

private:
    tuple mTuple;
};

template< typename ... in_data_ts >
class Inlets : public Xlets< Inlet, ... in_data_ts >
{
public:
};

template< typename ... out_data_ts >
class Outlets : public Xlets< Outlet, ... out_data_ts >
{
public:
};


template< typename inlets_t, typename outlets_t >
class Node : private Noncopyable
{
public:
    template< size_t I >
    auto in() const { return mInlets.get< I >(); }

    template< typename T >
    auto in() const { return mInlets.get< T >(); }

    template< size_t I >
    auto out() const { return mOutlets.get< I >(); }

    template< typename T >
    auto out() const { return mOutlets.get< T >(); }

    virtual void update()
    {
        for ( const auto & in : mInlets ) {
            out< 0 >()->update( in.read() );
        }
    }
private:
    inlets_t    mInlets;
    outlets_t   mOutlets;
};


//! A node that accepts an image input.
class ImageONode : public Node< Inlets< Surface32fRef >, Outlets<> >
{
public:
};
typedef ref< ImageONode > ImageONodeRef;

//! A node that provides an image as input to its outputs.
class ImageINode : public Node< Inlets<>, Outlets< Surface32fRef > >
{
public:
};

//! A node that accepts an image input, and then provides an image to its outputs.
class ImageIONode : public ImageINode, public ImageONode
{
public:
};

//! A node that represents a Cinder Surface32f.
class SurfaceINode : public ImageINode
{
public:
	static ref< SurfaceINode > create( const Surface32fRef & surface )
	{
		return std::make_shared< SurfaceINode >( surface );
	}

	SurfaceINode( const Surface32fRef & surface );

	virtual void update() override;

	const ci::Surface32fRef & getSurface() const { return mSurface; }

	operator const ci::Surface32fRef & () const { return getSurface(); }

private:
	Surface32fRef mSurface;
};


//! A node that represents a Cinder gl::Texture2d, useful for displaying
//! results.
class TextureONode : public ImageONode
{
public:
	static ref< TextureONode > create()
	{
		return std::make_shared< TextureONode >();
	}

	TextureONode();


	void clear() { mTexture = nullptr; }
	const ci::gl::Texture2dRef getTexture() const { return mTexture; }

	operator const ci::gl::Texture2dRef () const { return getTexture(); }
	operator const bool () const { return mTexture != nullptr; }

	ci::ivec2 getSize() const { return mTexture->getSize(); }
private:

	ci::gl::Texture2dRef	mTexture = nullptr;
};
typedef ref< TextureONode > TextureONodeRef;


//! A node that emits OpenGL textures.
class TextureINode : public INode< TextureONodeRef >
{
public:
	virtual void update() override;
protected:
	virtual void update( const ci::gl::Texture2dRef & texture );

private:
	ci::gl::Texture2dRef			mOutTex;
};


class TextureIONode : public TextureINode, public TextureONode
{
public:
    virtual void update( const ci::gl::Texture2dRef & texture ) override;
};


//! A node that applies a shader to a texture input.
class TextureShaderIONode : public TextureIONode
{
public:
	static ref< TextureShaderIONode > create( const ci::gl::GlslProgRef & shader )
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


//! A node that is comprised of other nodes
template < typename ... Types >
class CompoundNode
{
public:
	typedef mapbox::util::variant< Types... > variant;

	static CompoundNodeRef< Types... > create( const Types& ... nodes )
	{
		return std::make_shared< CompoundNode< Types... > >( nodes... );
	}

	CompoundNode( const Types& ... nodes ) :
		mNodes{ nodes... }
	{}

protected:
	std::vector< variant > mNodes;
};

namespace operators {
	template< typename in_t, typename out_t >
	inline const ref< out_t > & operator>>( const ref< in_t > & input, const ref< out_t > & output )
	{
		input->connect( output );
		return output;
	}
}

} }
