#pragma once
#include <cstddef>
#include <functional>
#include <algorithm>
#include <memory>
#include <vector>
#include <set>

namespace cinder {
namespace frame_graph {
template< class T >
using ref = std::shared_ptr< T >;

struct reference_wrapper_compare {
    template< typename A, typename B >
    bool operator() ( const std::reference_wrapper< A > & lhs, const std::reference_wrapper< B > & rhs ) const {
        return lhs.get() < rhs.get();
    }
};


template< typename V >
class connection_container {
public:
    typedef reference_wrapper_compare           compare;
    typedef std::reference_wrapper< V >         value_type;
    typedef std::vector< value_type >           vector_type;
    typedef std::set< value_type, compare >     set_type;

    bool insert( const value_type & member ) {
        if ( mSet.insert( member ).second ) {
            mVector.push_back( member );
            return true;
        }
        return false;
    }

    bool erase( const value_type & member ) {
        if ( mSet.erase( member ) ) {
            mVector.erase( std::remove_if( mVector.begin(), mVector.end(), [&]( const value_type & m ) {
                return m.get() == member.get();
            } ), mVector.end() );
            return true;
        }
        return false;
    }

    typename vector_type::iterator begin() { return mVector.begin(); }
    typename vector_type::iterator end() { return mVector.end(); }
    typename vector_type::const_iterator cbegin() const { return mVector.cbegin(); }
    typename vector_type::const_iterator cend() const { return mVector.cend(); }

    bool empty() const { return mSet.empty(); }

    std::size_t size() const { return mSet.size(); }

    void clear()
    {
        mVector.clear();
        mSet.clear();
    }

private:
    vector_type mVector;
    set_type mSet;
};

namespace algorithms {
///////////////////////////////////////////////////////////////////////////
// connection_container algorithms
///////////////////////////////////////////////////////////////////////////
template< class value_t, class return_t, class ref_t = ref< value_t > >
void call( connection_container< value_t > & container,
    return_t( ref_t::element_type::*fn )( void ) )
{
    for ( auto & c : container ) ( ( *c ).*fn )( );
}

template<
    class a_t,
    class b_t,
    class a_ref_t = ref< a_t >,
    class b_ref_t = const ref< b_t >
>
void call( connection_container< a_t > & a,
    const connection_container< b_t > & b,
    void ( a_t::*fn )( const b_t & ) )
{
    auto a_it = a.begin();
    auto a_end = a.end();
    auto b_it = b.begin();
    auto b_end = b.end();

    while ( a_it != a_end && b_it != b_end ) {
        ( ( **a_it ).*fn )( *b_it );
        ++a_it;
        ++b_it;
    }
}


template<
    class a_t,
    class b_t,
    class a_ref_t = ref< a_t >,
    class b_ref_t = const ref< b_t >
>
void call( connection_container< a_t > & a,
    const b_ref_t &b,
    void ( a_t::*fn )( const b_t & ) )
{
    for ( auto & aa : a ) ( ( *aa ).*fn )( b );
}


template< class T, typename ref_t = ref< T > >
void update( connection_container< T > & connections )
{
    call( connections, &ref_t::element_type::update );
}

///////////////////////////////////////////////////////////////////////////
// std::tuple algorithms
///////////////////////////////////////////////////////////////////////////

template< typename V >
using iterator_function = std::function< void( V& ) >;
template< typename V >
using iterator_with_index_function = std::function< void( V&, std::size_t ) >;

// call a function on each member of a tuple
template<
    typename V,
    std::size_t I = 0,
    typename... Tp
>
inline typename std::enable_if< I == sizeof...( Tp ) >::type
call( std::tuple< Tp... > &, iterator_function< V > ) // Unused arguments are given no names.
{ }

template<
    typename V,
    std::size_t I = 0,
    typename... Tp
>
inline typename std::enable_if < I < sizeof...( Tp ) >::type
    call( std::tuple< Tp... >& t, iterator_function< V > f )
{
    f( std::get< I >( t ) );
    call< V, I + 1, Tp... >( t, f );
}


// call a function on each member of a tuple, passing the member index as the second
// parameter
template<
    typename V,
    std::size_t I = 0,
    typename... Tp
>
inline typename std::enable_if< I == sizeof...( Tp ) >::type
call( std::tuple< Tp... > &, iterator_with_index_function< V > ) // Unused arguments are given no names.
{ }

template<
    typename V,
    std::size_t I = 0,
    typename... Tp
>
inline typename std::enable_if < I < sizeof...( Tp ) >::type
    call( std::tuple< Tp... >& t, iterator_with_index_function< V > f )
{
    f( std::get< I >( t ), I );
    call< V, I + 1, Tp... >( t, f );
}

///////////////////////////////////////////////////////////////////////////
// std::array algorithms
///////////////////////////////////////////////////////////////////////////

template<
    typename V,
    std::size_t I
>
void call( std::array< V, I > & a, iterator_function< V > f )
{
    for ( auto & v : a ) f( v );
}

template<
    typename V,
    std::size_t I
>
void call( std::array< V, I > & a, iterator_with_index_function< V > f )
{
    std::size_t i = 0;
    for ( auto & v : a ) f( v, i++ );
}
}
}
};
