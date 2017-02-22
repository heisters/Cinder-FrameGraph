#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/Signals.h"

#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"

namespace cinder { namespace frame_graph {

//! Abstract base class for all inlets.
class Xlet : private Noncopyable
{
public:
	Xlet() : mId( sId++ ) {}

	bool operator< ( const Xlet & b ) { return mId < b.mId; }

private:
	static size_t sId;
	size_t mId;
};

//! An Inlet accepts \a in_data_ts to its receive method, and returns
//! \a read_t from its read method.
template< typename in_t >
class Inlet : public Xlet
{
public:
	typedef in_t type;
	typedef ci::signals::Signal< void( const in_t & ) > receive_signal;

    virtual void receive( const in_t & data ) { mReceiveSignal.emit( data ); }

	ci::signals::Connection onReceive( const typename receive_signal::CallbackFn & fn )
	{
		return mReceiveSignal.connect( fn );
	}

private:
	receive_signal mReceiveSignal;
};

//! An Outlet connects to an \a out_data_ts Inlet, and is updated with
//! \a update_t.
template< typename out_t >
class Outlet : public Xlet
{
public:
	typedef out_t type;
	typedef Inlet< out_t > inlet_t;
	typedef typename std::reference_wrapper< inlet_t > inlet_ref_t;

    virtual void update( const out_t & in )
    {
        for ( auto & c : mConnections ) {
            c.get().receive( in );
        }
    }
    
    bool connect( const inlet_ref_t & in ) { return mConnections.insert( in ); }
    bool disconnect( const inlet_ref_t & in ) { return mConnections.erase( in ); }
    void disconnect() { mConnections.clear(); }
    bool isConnected() const { return ! mConnections.empty(); }

private:
    connection_container< inlet_ref_t, reference_wrapper_compare > mConnections;
};


template< typename ... in_data_ts >
class Inlets
{
public:
	typedef typename std::tuple< Inlet< in_data_ts >... > in_tuple;

	template< std::size_t I >
	using in_type = typename std::tuple_element< I, in_tuple >::type;


	template< std::size_t I >
	in_type< I > & in() { return std::get< I >( mInlets ); }
	template< std::size_t I >
	in_type< I > const & in() const { return std::get< I >( mInlets ); }

protected:
	in_tuple mInlets;
};

template< typename ... out_data_ts >
class Outlets
{
public:
	typedef std::tuple< Outlet< out_data_ts >... > out_tuple;

	template< std::size_t I >
	using out_type = typename std::tuple_element< I, out_tuple >::type;


	template< std::size_t I >
	out_type< I > & out() { return std::get< I >( mOutlets ); }
	template< std::size_t I >
	out_type< I > const & out() const { return std::get< I >( mOutlets ); }

protected:
	out_tuple mOutlets;
};


template< typename inlets_t, typename outlets_t >
class Node : private Noncopyable, public inlets_t, public outlets_t
{
public:
	//! Default update for one inlet, one outlet
	template<
		typename in_tuple = typename inlets_t::in_tuple,
		typename out_tuple = typename outlets_t::out_tuple
	>
	typename std::enable_if<
		std::tuple_size< in_tuple >::value == 1 &&
		std::tuple_size< out_tuple >::value == 1
	>::type update()
	{
		auto data = this->template in< 0 >()->read();
		this->template out< 0 >()->update( data );
	}

	//! Default update for no inlets, one outlet
	template<
		typename in_tuple = typename inlets_t::in_tuple,
		typename out_tuple = typename outlets_t::out_tuple
	>
	typename std::enable_if<
		std::tuple_size< in_tuple >::value == 0 &&
		std::tuple_size< out_tuple >::value == 1
	>::type update()
	{
		this->template out< 0 >()->update();
	}

};


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

	operator const ci::Surface32fRef & () const { return getSurface(); }

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
	static ref< TextureONode > create()
	{
		return std::make_shared< TextureONode >();
	}

	TextureONode();

	void update( const Surface32fRef & image );
	void update( const gl::Texture2dRef & texture );

	void clear() { mTexture = nullptr; }
	const ci::gl::Texture2dRef getTexture() const { return mTexture; }

	operator const ci::gl::Texture2dRef () const { return getTexture(); }
	operator const bool () const { return mTexture != nullptr; }

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
	typedef std::tuple< Types... > tuple;

	static CompoundNodeRef< Types... > create( const Types& ... nodes )
	{
		return std::make_shared< CompoundNode< Types... > >( nodes... );
	}

	CompoundNode( const Types& ... nodes ) :
		mNodes{ nodes... }
	{}

protected:
	tuple mNodes;
};

namespace operators {
	template< typename in_t, typename out_t,
	typename I = typename in_t::template out_type< 0 >::type,
	typename O = typename out_t::template in_type< 0 >::type
	>
	inline const ref< out_t > & operator>>( const ref< in_t > & input, const ref< out_t > & output )
	{
		static_assert( std::is_same< I, O >::value, "Cannot connect input to output" );
		input->template out< 0 >().connect( std::ref( output->template in< 0 >() ) );
		return output;
	}
}

} }
