#pragma once

#include <cstddef>
#include <type_traits>
#include <tuple>
#include <array>
#include <string>
#include "Cinder-FrameGraph/NodeContainer.h"
#include "Cinder-FrameGraph/Types.h"
#include "cinder/Signals.h"
#include "cinder/Cinder.h"

#include <iostream>

namespace cinder {
namespace frame_graph {


//-----------------------------------------------------------------------------
// Forward declarations

template< typename out_t >
class Outlet;
template< typename in_t >
class Inlet;

class AnyNode;
class NodeBase;

//-----------------------------------------------------------------------------
// Utility base classes

//! makes it illegal to copy a derived class
struct Noncopyable
{
protected:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable( const Noncopyable & ) = delete;

    Noncopyable &operator=( const Noncopyable & ) = delete;
};

//! provides derived classes with automatically assigned, globally unique
//! numeric identifiers
class HasId
{
public:
    HasId() : mId( sId++ ) {}

    uint64_t id() const { return mId; }

protected:
    static uint64_t sId;
    uint64_t mId;
};

//! Declares an interface for anything that acts like a node
class NodeConcept
{
public:
    virtual ~NodeConcept() = default;


    virtual uint64_t id() const = 0;

    virtual void setLabel( const std::string &label ) = 0;
    virtual std::string getLabel() const = 0;
    virtual const std::string &label() const = 0;
    virtual std::string &label() = 0;
};


//-----------------------------------------------------------------------------
// Visitors and Visitables


class VisitableBase
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

//! Base class for custom visitors. T... is a list of node classes it can visit.
template< class ... T >
class NodeVisitor : public Visitor< T... >
{
public:
};

//! Makes it possible to pass and call methods upon nodes without knowing their types.
class AnyNode : virtual public NodeConcept
{
    struct Concept : virtual public NodeConcept
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
            auto typedVisitor = dynamic_cast< Visitor< T >* >( v );
            if ( typedVisitor ) {
                node.accept( *typedVisitor );
            } else {
                auto genericVisitor = dynamic_cast< Visitor< NodeBase >* >( v );
                if ( genericVisitor ) {
                    node.accept( *genericVisitor );
                }
            }
        }

        uint64_t id() const override { return node.id(); }

        void setLabel( const std::string &label ) override { return node.setLabel( label ); }
        std::string getLabel() const override { return node.getLabel(); }
        const std::string &label() const override { return node.label(); }
        std::string &label() override { return node.label(); }


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

    uint64_t id() const override { return mConcept->id(); }

    void setLabel( const std::string &label ) override { return mConcept->setLabel( label ); }
    std::string getLabel() const override { return mConcept->getLabel(); }
    const std::string &label() const override { return mConcept->label(); }
    std::string &label() override { return mConcept->label(); }
};


//! Base class for nodes that can accept visitors. V is the class of the node itself.
template< class V >
class VisitableNode : public VisitableBase
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
    typedef V visitable_type;

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

class NodeBase : private Noncopyable, public HasId, virtual public NodeConcept
{
public:
    NodeBase( const std::string &label )
    {
        setLabel( label );
    }
    NodeBase() : NodeBase( "node" ) {}

    uint64_t id() const override { return HasId::id(); }

    void setLabel( const std::string &label ) override { mLabel = label + " (" + std::to_string( mId ) + ")"; }
    std::string getLabel() const override { return mLabel; }
    const std::string &label() const override { return mLabel; }
    std::string &label() override { return mLabel; }

private:
    std::string mLabel;
};

//! A node has inlets and outlets, specified by its template arguments
template< typename Ti, typename To >
class Node : public NodeBase, public Ti, public To, public VisitableNode< Node< Ti, To > >
{
public:
    Node( const std::string &label ) : NodeBase( label ) {}
    Node() : Node( "node" ) {}
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