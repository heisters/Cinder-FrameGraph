#include "catch.hpp"
#include "Cinder-FrameGraph/Node.h"

using namespace cinder;
using namespace frame_graph;
using namespace std;
using namespace Catch;
using namespace frame_graph::operators;

class Int_IONode : public Node< Inlets< int >, Outlets< int > >
{
public:
    Int_IONode( const string & label ) : Node< Inlets< int >, Outlets< int > >( label )
    {
        in< 0 >().onReceive( [&] ( const int & i ) {
            received.push_back( i );
            this->out< 0 >().update( i );
        } );
    }

    std::vector< int > received;
};

TEST_CASE( "Node interface", "[nodes]" ) {
    Int_IONode n( "a label" );

    SECTION( "id" ) {
        REQUIRE( n.id() >= 0 );
    }

    SECTION( "label" ) {
        REQUIRE_THAT( n.label(), StartsWith( "a label" ) );
        REQUIRE_THAT( n.getLabel(), StartsWith( "a label" ) );
        n.label() = "another label (0)";
        REQUIRE_THAT( n.getLabel(), StartsWith( "another label" ) );
    }

    SECTION( "inlets and outlets ") {
        Int_IONode n2( "node b" );

        n >> n2;

        n.in< 0 >().receive( 1 );
        n.in< 0 >().receive( 2 );

        REQUIRE( n.received.size() == 2 );
        REQUIRE( n2.received.size() == 2 );

        REQUIRE( n.received[0] == 1 );
        REQUIRE( n2.received[0] == 1 );
        REQUIRE( n.received[1] == 2 );
        REQUIRE( n2.received[1] == 2 );
    }
}