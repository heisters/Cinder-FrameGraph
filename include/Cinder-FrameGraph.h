#pragma once

#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Batch.h"
#include "cinder/Signals.h"

#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"

namespace cinder {
namespace frame_graph {
class HasId
{
public:
    HasId() : mId( sId++ ) {}

protected:
    static uint64_t sId;
    uint64_t mId;
};

//! Abstract base class for all inlets.
class Xlet : private Noncopyable, public HasId
{
public:
    bool operator< ( const Xlet & b ) { return mId < b.mId; }
    bool operator== ( const Xlet& b ) { return mId == b.mId; }

private:
};

template< typename out_t >
class Outlet;

//! An Inlet accepts \a in_data_ts to its receive method, and returns
//! \a read_t from its read method.
template< typename in_t >
class Inlet : public Xlet
{
public:
    typedef in_t type;
    typedef ci::signals::Signal< void( const in_t & ) > receive_signal;
    typedef Outlet< type > outlet_type;
    friend outlet_type;

    virtual void receive( const in_t & data ) { mReceiveSignal.emit( data ); }

    ci::signals::Connection onReceive( const typename receive_signal::CallbackFn & fn )
    {
        return mReceiveSignal.connect( fn );
    }

protected:
    bool connect( outlet_type & out ) { return mConnections.insert( out ); }
    bool disconnect( outlet_type & out ) { return mConnections.erase( out ); }
    void disconnect() { mConnections.clear(); }
public:
    bool isConnected() const { return ! mConnections.empty(); }

private:
    receive_signal mReceiveSignal;
    connection_container< outlet_type > mConnections;
};

//! An Outlet connects to an \a out_data_ts Inlet, and is updated with
//! \a update_t.
template< typename out_t >
class Outlet : public Xlet
{
public:
    typedef out_t type;
    typedef Inlet< type > inlet_type;

    virtual void update( const out_t & in )
    {
        for ( auto & c : mConnections ) {
            c.get().receive( in );
        }
    }

    bool connect( inlet_type & in )
    {
        in.connect( *this );
        return mConnections.insert( in );
    }
    bool disconnect( inlet_type & in )
    {
        in.disconnect( *this );
        return mConnections.erase( in );
    }
    void disconnect()
    {
        for ( auto & i : mConnections ) i.get().disconnect( *this );
        mConnections.clear();
    }
    bool isConnected() const { return ! mConnections.empty(); }

private:
    connection_container< inlet_type > mConnections;
};

//! Provides a common interface for hetero- and homo-geneous inlets.
template< typename T >
class AbstractInlets
{
public:
    typedef T inlets_container_type;

    static constexpr std::size_t in_size = std::tuple_size< inlets_container_type >::value;

    template< std::size_t I >
    using inlet_type = typename std::tuple_element< I, inlets_container_type >::type;

    template< std::size_t I >
    inlet_type< I > & in() { return std::get< I >( mInlets ); }
    template< std::size_t I >
    inlet_type< I > const & in() const { return std::get< I >( mInlets ); }

    template< typename V >
    void each_in( algorithms::iterator_function< V > fn ) { return algorithms::call< V >( mInlets, fn ); }

    template< typename V >
    void each_in( algorithms::iterator_with_index_function< V > fn ) { return algorithms::call< V >( mInlets, fn ); }

    inline inlets_container_type & inlets() { return mInlets; }
    inline const inlets_container_type & inlets() const { return mInlets; }

protected:
    inlets_container_type mInlets;
};

//! Provides a common interface for hetero- and homo-geneous outlets.
template< typename T >
class AbstractOutlets
{
public:
    typedef T outlets_container_type;
    static constexpr std::size_t out_size = std::tuple_size< outlets_container_type >::value;

    template< std::size_t I >
    using outlet_type = typename std::tuple_element< I, outlets_container_type >::type;

    template< std::size_t I >
    outlet_type< I > & out() { return std::get< I >( mOutlets ); }
    template< std::size_t I >
    outlet_type< I > const & out() const { return std::get< I >( mOutlets ); }

    template< typename V >
    void each_out( algorithms::iterator_function< V > fn ) { return algorithms::call< V >( mOutlets, fn ); }

    template< typename V >
    void each_out( algorithms::iterator_with_index_function< V > fn ) { return algorithms::call< V >( mOutlets, fn ); }

    inline outlets_container_type & outlets() { return mOutlets; }
    inline const outlets_container_type & outlets() const { return mOutlets; }

protected:
    outlets_container_type mOutlets;
};

//! A collection of heterogeneous Inlets
template< typename ... T >
class Inlets : public AbstractInlets< std::tuple< Inlet< T >... > >
{
public:
};

//! A collection of homogeneous Inlets
template< typename T, std::size_t I >
class UniformInlets : public AbstractInlets< std::array< Inlet< T >, I > >
{
public:
};

//! A collection of heterogeneous Outlets
template< typename ... T >
class Outlets : public AbstractOutlets< std::tuple< Outlet< T >... > >
{
public:
};

//! A collection of homogeneous Outlets
template< typename T, std::size_t I >
class UniformOutlets : public AbstractOutlets< std::array< Outlet< T >, I > >
{
public:
};

//! A node has inlets and outlets, specified by its template arguments
template< typename Ti, typename To >
class Node : private Noncopyable, public HasId, public Ti, public To
{
public:
    Node() { setLabel( "node" ); }
    Node( const std::string & label ) { setLabel( label ); }

    void setLabel( const std::string & label ) { mLabel = label + " (" + std::to_string( mId ) + ")"; }
    std::string getLabel() const { return mLabel; }
    const std::string & label() const { return mLabel; }
    std::string & label() { return mLabel; }

private:
    std::string mLabel;
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

namespace operators {
template<
    typename To,
    typename Ti,
    typename I = Ti::type,
    typename O = To::type
>
inline const Ti & operator >> ( To & outlet, Ti & inlet )
{
    static_assert( std::is_same< I, O >::value, "Cannot connect outlet to inlet" );
    outlet.connect( inlet );
    return inlet;
}

template<
    typename Ni,
    typename No,
    std::size_t Ii = 0,
    std::size_t Io = 0,
    typename Ti = typename Ni::template outlet_type< Ii >::type,
    typename To = typename No::template inlet_type< Io >::type
>
inline const ref< No > & operator >> ( const ref< Ni > & input, const ref< No > & output )
{
    static_assert( std::is_same< Ti, To >::value, "Cannot connect input to output" );
    input->template out< Ii >() >> output->template in< Io >();
    return output;
}
}
}
}
