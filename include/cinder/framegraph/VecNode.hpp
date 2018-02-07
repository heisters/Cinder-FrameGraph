#pragma once

#include "cinder/CinderGlm.h"
#include "libnodes/Node.h"
#include "libnodes/ValueNode.h"

namespace cinder {
namespace frame_graph {

template< typename T = vec2, typename V = typename T::value_type >
class Vec2Node : public nodes::ValueNode< T, V, V >
{
public:
    Vec2Node( const std::string &label, const T & v ) :
            nodes::ValueNode< T, V, V >( label, v )
    {
//        listen();
    }

    Vec2Node( const T & v ) :
            nodes::ValueNode< T, V, V >( v )
    {
//        listen();
    }

    V & x() { return this->get().x; }
    const V & x() const { return this->get().x; }
    V & y() { return this->get().y; }
    const V & y() const { return this->get().y; }


protected:
    void listen() override
    {
        nodes::ValueNode< T, V, V >::listen();

        this->template in< 1 >().onReceive( [&] ( const V & v ) {
            T cp = (*this)();
            cp.x = v;
            this->set( cp );
            this->template out< 1 >().update( v );
        });

        this->template in< 2 >().onReceive( [&] ( const V & v ) {
            T cp = (*this)();
            cp.y = v;
            this->set( cp );
            this->template out< 2 >().update( v );
        });
    }
};

template< typename T = vec3, typename V = typename T::value_type >
class Vec3Node : public nodes::ValueNode< T, V, V, V >
{
public:
    Vec3Node( const std::string &label, const T & v ) :
            nodes::ValueNode< T, V, V, V >( label, v )
    {
//        listen();
    }

    Vec3Node( const T & v ) :
            nodes::ValueNode< T, V, V, V >( v )
    {
//        listen();
    }

    V & x() { return this->get().x; }
    const V & x() const { return this->get().x; }
    V & y() { return this->get().y; }
    const V & y() const { return this->get().y; }
    V & z() { return this->get().z; }
    const V & z() const { return this->get().z; }

protected:
    void listen() override
    {
        nodes::ValueNode< T, V, V, V >::listen();

        this->template in< 1 >().onReceive( [&] ( const V & v ) {
            T cp = (*this)();
            cp.x = v;
            this->set( cp );
            this->template out< 1 >().update( v );
        });

        this->template in< 2 >().onReceive( [&] ( const V & v ) {
            T cp = (*this)();
            cp.y = v;
            this->set( cp );
            this->template out< 2 >().update( v );
        });

        this->template in< 3 >().onReceive( [&] ( const V & v ) {
            T cp = (*this)();
            cp.z = v;
            this->set( cp );
            this->template out< 3 >().update( v );
        });
    }
};


}
}


