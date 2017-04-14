#include "catch.hpp"
#include "Cinder-FrameGraph/Node.h"
#include <iostream>

using namespace cinder;
using namespace frame_graph;
using namespace std;
using namespace Catch;
using namespace frame_graph::operators;

class Int_IONode : public Node< Inlets< int >, Outlets< int > > {
public:
    Int_IONode( const string &label ) : Node< Inlets< int >, Outlets< int > >( label ) {
        in< 0 >().onReceive( [&]( const int &i ) {
            received.push_back( i );
            this->out< 0 >().update( i );
        } );
    }

    std::vector< int > received;
};

SCENARIO( "With a single node", "[nodes]" ) {
    Int_IONode n( "a label" );

    THEN( "it has a numeric id" ) {
        REQUIRE( n.id() >= 0 );
    }

    THEN( "it has a label with getters and setters" ) {
        REQUIRE( n.label() == "a label" );
        REQUIRE( n.getLabel() == "a label" );
        n.label() = "another label";
        REQUIRE( n.getLabel() == "another label" );
    }
}

SCENARIO( "With an AnyNode", "[nodes]" ) {
    Int_IONode concrete( "node 1" );
    AnyNode any( (Int_IONode::visitable_type &)concrete );

    THEN( "it has the same interface" ) {
        REQUIRE( any.id() == concrete.id() );
        REQUIRE( any.getLabel() == "node 1" );
    }

    THEN( "it maintains a reference to the original node" ) {
        any.setLabel( "something else" );
        REQUIRE( concrete.label() == "something else" );
    }

    THEN( "it is easily convertable to a NodeBase" ) {
        NodeBase & base = any;

        REQUIRE( base.label() == "node 1" );
        base.setLabel( "another something else" );
        REQUIRE( concrete.label() == "another something else" );
    }
}

SCENARIO( "With two connected nodes", "[nodes]" ) {
    Int_IONode n1( "label 1" );
    Int_IONode n2( "label 2" );
    n1 >> n2;

    THEN( "the nodes propagate signals" ) {
        n1.in< 0 >().receive( 1 );
        n1.in< 0 >().receive( 2 );

        REQUIRE( n1.received.size() == 2 );
        REQUIRE( n2.received.size() == 2 );

        REQUIRE( n1.received[ 0 ] == 1 );
        REQUIRE( n2.received[ 0 ] == 1 );
        REQUIRE( n1.received[ 1 ] == 2 );
        REQUIRE( n2.received[ 1 ] == 2 );
    }

    THEN( "the inlets are iterable" ) {
        int num = 0;
        n1.each_in( [&]( auto &i ) {
            num++;
        } );

        REQUIRE( num == 1 );
    }

    THEN( "the outlets are iterable" ) {
        int num = 0;
        n1.each_out( [&]( auto &o ) {
            num++;
        } );

        REQUIRE( num == 1 );
    }

    GIVEN( "a visitor" ) {
        class Int_IONodeVisitor : public NodeVisitor< Int_IONode::visitable_type >, private frame_graph::Noncopyable {
        public:
            void visit( Int_IONode::visitable_type &n ) {
                visited.push_back( n.getLabel());
            }

            std::vector< std::string > visited;
        };

        Int_IONodeVisitor v;

        THEN( "the hierarchy can be visited" ) {
            n1.accept( v );

            REQUIRE( v.visited.size() == 2 );
            REQUIRE( v.visited[ 0 ] == "label 1" );
            REQUIRE( v.visited[ 1 ] == "label 2" );
        }
    }
}

SCENARIO( "With heterogenous nodes", "[nodes]" ) {
    class IntString_IONode
            : public Node< Inlets< int, string >, Outlets< int, string > > {
    public:
        IntString_IONode( const string &label ) : Node< Inlets< int, string >, Outlets< int, string > >( label ) {
            in< 0 >().onReceive( [&]( const int &i ) {
                receivedInts.push_back( i );
                this->out< 0 >().update( i );
            } );
            in< 1 >().onReceive( [&]( const string &s ) {
                receivedStrings.push_back( s );
                this->out< 1 >().update( s );
            } );
        }

        std::vector< int > receivedInts;
        std::vector< string > receivedStrings;
    };

    Int_IONode n1( "node 1" );
    Int_IONode n2( "node 2" );
    IntString_IONode n3( "node 3" );
    IntString_IONode n4( "node 4" );
    n1 >> n2;
    n2.out< 0 >() >> n3.in< 0 >();
    n1.out< 0 >() >> n4.in< 0 >();
    n3.out< 1 >() >> n4.in< 1 >();

    THEN( "the nodes propagate signals" ) {
        n1.in< 0 >().receive( 1 );
        n1.in< 0 >().receive( 2 );
        n3.in< 1 >().receive( "message 1" );
        n3.in< 1 >().receive( "message 2" );

        REQUIRE( n1.received.size() == 2 );
        REQUIRE( n2.received.size() == 2 );
        REQUIRE( n3.receivedInts.size() == 2 );
        REQUIRE( n4.receivedInts.size() == 2 );
        REQUIRE( n3.receivedStrings.size() == 2 );
        REQUIRE( n4.receivedStrings.size() == 2 );

        REQUIRE( n1.received[ 0 ] == 1 );
        REQUIRE( n2.received[ 0 ] == 1 );
        REQUIRE( n1.received[ 1 ] == 2 );
        REQUIRE( n2.received[ 1 ] == 2 );

        REQUIRE( n3.receivedInts[ 0 ] == 1 );
        REQUIRE( n4.receivedInts[ 0 ] == 1 );
        REQUIRE( n3.receivedInts[ 1 ] == 2 );
        REQUIRE( n4.receivedInts[ 1 ] == 2 );

        REQUIRE( n3.receivedStrings[ 0 ] == "message 1" );
        REQUIRE( n4.receivedStrings[ 0 ] == "message 1" );
        REQUIRE( n3.receivedStrings[ 1 ] == "message 2" );
        REQUIRE( n4.receivedStrings[ 1 ] == "message 2" );
    }

    THEN( "the inlets are iterable" ) {
        int num = 0;
        n3.each_in( [&]( auto &i ) {
            num++;
        } );

        REQUIRE( num == 2 );
    }

    THEN( "the outlets are iterable" ) {
        int num = 0;
        n3.each_out( [&]( auto &o ) {
            num++;
        } );

        REQUIRE( num == 2 );
    }

    THEN( "the inlets are iterable with indices" ) {
        vector< size_t > indices;
        n3.each_in_with_index( [&]( auto &inlet, size_t i ) {
            indices.push_back( i );
        } );

        REQUIRE( indices.size() == 2 );
        REQUIRE( indices[ 0 ] == 0 );
        REQUIRE( indices[ 1 ] == 1 );
    }

    THEN( "the outlets are iterable with indices" ) {
        vector< size_t > indices;
        n3.each_out_with_index( [&]( auto &outlet, size_t i ) {
            indices.push_back( i );
        } );

        REQUIRE( indices.size() == 2 );
        REQUIRE( indices[ 0 ] == 0 );
        REQUIRE( indices[ 1 ] == 1 );
    }

    THEN( "the outlets are iterable with type information" ) {
        struct outlet_iterator {
            int gotInt = 0;
            int gotString = 0;

            void operator()( const Outlet< int > &o ) { gotInt++; }
            void operator()( const Outlet< string > &o ) { gotString++; }
        };

        outlet_iterator it;
        n3.each_out( it );
        REQUIRE( it.gotInt == 1 );
        REQUIRE( it.gotString == 1 );
    }


    GIVEN( "a visitor" ) {
        class V : public NodeVisitor< Int_IONode::visitable_type, IntString_IONode::visitable_type > {
        public:

            void visit( Int_IONode::visitable_type &n ) {
                visited.push_back( n.getLabel());
            }

            void visit( IntString_IONode::visitable_type &n ) {
                visited.push_back( n.getLabel());
            }

            std::vector< std::string > visited;
        };

        V v;

        THEN( "the hierarchy can be visited" ) {
            n1.accept( v );

            REQUIRE( v.visited.size() == 5 );
            REQUIRE( v.visited[ 0 ] == "node 1" );
            REQUIRE( v.visited[ 1 ] == "node 2" );
            REQUIRE( v.visited[ 2 ] == "node 3" );
            REQUIRE( v.visited[ 3 ] == "node 4" );
            REQUIRE( v.visited[ 4 ] == "node 4" );
        }
    }

    GIVEN( "a generic visitor using NodeBase" ) {
        class V : public NodeVisitor< NodeBase > {
        public:

            void visit( NodeBase & n ) {
                visited.push_back( n.getLabel() );
            }

            std::vector< std::string > visited;
        };

        V v;

        THEN( "the hierarchy can be visited" ) {
            n1.accept( v );

            REQUIRE( v.visited.size() == 5 );
            REQUIRE( v.visited[ 0 ] == "node 1" );
            REQUIRE( v.visited[ 1 ] == "node 2" );
            REQUIRE( v.visited[ 2 ] == "node 3" );
            REQUIRE( v.visited[ 3 ] == "node 4" );
            REQUIRE( v.visited[ 4 ] == "node 4" );
        }
    }

    GIVEN( "a connection-aware visitor" ) {
        class V : public NodeVisitor< NodeBase > {
        public:

            void visit( NodeBase & n ) {
                ss << "[" << n.getLabel() << "] ";
            }

            void visit( OutletBase & o, InletBase & i ) {
                ss << o.node()->getLabel() << " (" << o.index() << ") -> " << i.node()->getLabel() << " (" << i.index() << "), ";
            };

            std::stringstream ss;
        };

        V v;

        THEN( "the hierarchy can be visited" ) {
            n1.accept( v );

            REQUIRE( v.ss.str() == "[node 1] node 1 (0) -> node 2 (0), [node 2] node 2 (0) -> node 3 (0), [node 3] node 3 (1) -> node 4 (1), [node 4] node 1 (0) -> node 4 (0), [node 4] " );
        }
    }
}