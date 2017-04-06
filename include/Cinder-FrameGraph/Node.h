#pragma once

#include <cstddef>
#include <tuple>
#include <array>
#include <type_traits>
#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"
#include "cinder/Signals.h"
#include "cinder/Cinder.h"

#include <iostream>

namespace cinder {
namespace frame_graph {

struct Noncopyable
{
protected:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable( const Noncopyable & ) = delete;

    Noncopyable &operator=( const Noncopyable & ) = delete;
};

class HasId
{
public:
    HasId() : mId( sId++ ) {}

    uint64_t id() const { return mId; }

protected:
    static uint64_t sId;
    uint64_t mId;
};

template< typename out_t >
class Outlet;

class AnyNode;


class Visitable
{
public:
};

class VisitorBase
{
public:
    virtual ~VisitorBase() = default;
};

template< typename... T >
class Visitor;

template< typename T >
class Visitor< T > : virtual public VisitorBase
{
public:
    virtual void visit( T & ) = 0;
};

template< typename T, typename... Ts >
class Visitor< T, Ts... > : public Visitor< T >, public Visitor< Ts... >
{
public:
    using Visitor< T >::visit;
    using Visitor< Ts... >::visit;

    virtual void visit( T & ) = 0;
};

template< class ... T >
class NodeVisitor : public Visitor< T... >
{
public:
};

class AnyNode
{
    struct Concept
    {
        virtual ~Concept() = default;

        template< typename V >
        void accept( V & visitor )
        {
            acceptDispatch( &visitor );
        }

        virtual void acceptDispatch( VisitorBase * ) = 0;
    };

    template< typename T >
    struct Model : public Concept
    {
        Model( T &n ) : node( n ) {}


        void acceptDispatch( VisitorBase * v ) override
        {
            auto visitor = dynamic_cast< Visitor< T >* >( v );
            if ( visitor ) {
                node.accept( *visitor );
            }
        }

    private:
        T &node;
    };

    std::unique_ptr< Concept > mConcept;
public:

    template< typename T >
    AnyNode( T &node ) :
            mConcept( new Model< T >( node )) {}


    template< typename V >
    void accept( V & visitor )
    {
        mConcept->accept( visitor );
    }
};


template< class V >
class VisitableNode : public Visitable
{
    template< typename T >
    struct outlet_visitor
    {
        T &visitor;
        outlet_visitor( T &v ) : visitor( v ) {}


        template< typename To >
        void operator()( Outlet< To > &outlet )
        {
            for ( auto &inlet : outlet.connections()) {
                auto n = inlet.get().node();
                if ( n != nullptr ) {
                    n->accept( visitor );
                }
            }
        }
    };

public:
    VisitableNode()
    {
        auto &_this = static_cast< V & >( *this );
        _this.each_in( [&]( auto &i ) {
            i.setNode( _this );
        } );
    }

    template< typename T >
    void accept( T &visitor )
    {
        auto &_this = static_cast< V & >( *this );

        visitor.visit( _this );

        outlet_visitor< T > ov( visitor );
        _this.each_out( ov );
    }
};

//! Abstract base class for all inlets.
class Xlet : private Noncopyable, public HasId
{
public:
    bool operator<( const Xlet &b ) { return mId < b.mId; }
    bool operator==( const Xlet &b ) { return mId == b.mId; }
private:
};


class AbstractInlet : public Xlet
{
public:

    template< typename T >
    void setNode( T &node ) { mNode = new AnyNode( node ); }
    AnyNode *node() { return mNode; }
    const AnyNode *node() const { return mNode; }


protected:
    AnyNode *mNode = nullptr;
};

//! An Inlet accepts \a in_data_ts to its receive method, and returns
//! \a read_t from its read method.
template< typename in_t >
class Inlet : public AbstractInlet
{
public:
    typedef in_t type;
    typedef ci::signals::Signal< void( const in_t & ) > receive_signal;
    typedef Outlet< type > outlet_type;
    friend outlet_type;

    virtual void receive( const in_t &data ) { mReceiveSignal.emit( data ); }

    ci::signals::Connection onReceive( const typename receive_signal::CallbackFn &fn )
    {
        return mReceiveSignal.connect( fn );
    }

protected:
    bool connect( outlet_type &out ) { return mConnections.insert( out ); }
    bool disconnect( outlet_type &out ) { return mConnections.erase( out ); }
    void disconnect() { mConnections.clear(); }

public:
    bool isConnected() const { return !mConnections.empty(); }

    std::size_t numConnections() const { return mConnections.size(); }

private:
    receive_signal mReceiveSignal;
    connection_container< outlet_type > mConnections;
};

class AbstractOutlet : public Xlet
{

};

//! An Outlet connects to an \a out_data_ts Inlet, and is updated with
//! \a update_t.
template< typename out_t >
class Outlet : public AbstractOutlet
{
public:
    typedef out_t type;
    typedef Inlet< type > inlet_type;

    virtual void update( const out_t &in )
    {
        for ( auto &c : mConnections ) {
            c.get().receive( in );
        }
    }

    bool connect( inlet_type &in )
    {
        in.connect( *this );
        return mConnections.insert( in );
    }

    bool disconnect( inlet_type &in )
    {
        in.disconnect( *this );
        return mConnections.erase( in );
    }

    void disconnect()
    {
        for ( auto &i : mConnections ) i.get().disconnect( *this );
        mConnections.clear();
    }

    bool isConnected() const { return !mConnections.empty(); }

    std::size_t numConnections() const { return mConnections.size(); }

    connection_container< inlet_type > &connections() { return mConnections; }
    const connection_container< inlet_type > &connections() const { return mConnections; }

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
    inlet_type< I > &in() { return std::get< I >( mInlets ); }

    template< std::size_t I >
    inlet_type< I > const &in() const { return std::get< I >( mInlets ); }

    template< typename F >
    void each_in( F &fn ) { return algorithms::call( mInlets, fn ); }
    template< typename F >
    void each_in_with_index( F &fn ) { return algorithms::call_with_index( mInlets, fn ); }

    inline inlets_container_type &inlets() { return mInlets; }

    inline const inlets_container_type &inlets() const { return mInlets; }

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
    outlet_type< I > &out() { return std::get< I >( mOutlets ); }

    template< std::size_t I >
    outlet_type< I > const &out() const { return std::get< I >( mOutlets ); }

    template< typename F >
    void each_out( F &fn ) { return algorithms::call( mOutlets, fn ); }
    template< typename F >
    void each_out_with_index( F &fn ) { return algorithms::call_with_index( mOutlets, fn ); }

    inline outlets_container_type &outlets() { return mOutlets; }

    inline const outlets_container_type &outlets() const { return mOutlets; }

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
    Node( const std::string &label )
    {
        setLabel( label );
    }
    Node() : Node( "node" ) {}

    void setLabel( const std::string &label ) { mLabel = label + " (" + std::to_string( mId ) + ")"; }
    std::string getLabel() const { return mLabel; }
    const std::string &label() const { return mLabel; }
    std::string &label() { return mLabel; }

private:
    std::string mLabel;
};

namespace operators {

template<
        typename To,
        typename Ti,
        typename I = typename Ti::type,
        typename O = typename To::type
>
inline const Ti &operator>>( To &outlet, Ti &inlet )
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
inline const ref< No > &operator>>( const ref< Ni > &input, const ref< No > &output )
{
    static_assert( std::is_same< Ti, To >::value, "Cannot connect input to output" );
    input->template out< Ii >() >> output->template in< Io >();
    return output;
}

template<
        typename Ni,
        typename No,
        std::size_t Ii = 0,
        std::size_t Io = 0,
        typename Ti = typename Ni::template outlet_type< Ii >::type,
        typename To = typename No::template inlet_type< Io >::type
>
inline No &operator>>( Ni &input, No &output )
{
    static_assert( std::is_same< Ti, To >::value, "Cannot connect input to output" );
    input.out< Ii >() >> output.in< Io >();
    return output;
}


}


}
}