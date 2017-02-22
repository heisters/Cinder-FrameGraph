#pragma once

namespace cinder { namespace frame_graph {

template< class T >
using ref = std::shared_ptr< T >;

struct reference_wrapper_compare {
	template< typename A, typename B >
	bool operator() ( const std::reference_wrapper< A > & lhs, const std::reference_wrapper< B > & rhs) const {
		return lhs.get() < rhs.get();
	}
};


template< typename value_t, typename compare = std::less< value_t > >
class connection_container {
public:
    typedef value_t value_type;
	typedef std::vector< value_type > vector_type;
	typedef std::set< value_type, compare > set_type;

    bool insert( const value_t & member ) {
        if ( mSet.insert( member ).second ) {
            mVector.push_back( member );
            return true;
        }
        return false;
    }

    bool erase( const value_t & member ) {
        if ( mSet.erase( member ) ) {
            mVector.erase( remove( mVector.begin(), mVector.end(), member ), mVector.end() );
            return true;
        }
        return false;
    }

	typename vector_type::iterator begin() { return mVector.begin(); }
	typename vector_type::iterator end() { return mVector.end(); }
	typename vector_type::const_iterator cbegin() const { return mVector.cbegin(); }
    typename vector_type::const_iterator cend() const { return mVector.cend(); }

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
	template< class value_t, class return_t, class ref_t = ref< value_t > >
	void call(connection_container< value_t > & container,
			  return_t (ref_t::element_type::*fn)(void))
	{
		for ( auto & c : container ) ((*c).*fn)();
	}

	template<
	class a_t,
	class b_t,
	class a_ref_t = ref< a_t >,
	class b_ref_t = const ref< b_t >
	>
	void call(connection_container< a_t > & a,
			  const connection_container< b_t > & b,
			  void (a_t::*fn)(const b_t &))
	{
		auto a_it		= a.begin();
		auto a_end		= a.end();
		auto b_it		= b.begin();
		auto b_end		= b.end();

		while ( a_it != a_end && b_it != b_end ) {
			((**a_it).*fn)( *b_it );
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
	void call(connection_container< a_t > & a,
			  const b_ref_t &b,
			  void (a_t::*fn)(const b_t &))
	{
		for ( auto & aa : a ) ((*aa).*fn)( b );
	}


	template< std::size_t I = 0, typename FuncT, typename... Tp >
	inline typename std::enable_if< I == sizeof...(Tp), void >::type
	call( std::tuple< Tp... > &, FuncT ) // Unused arguments are given no names.
	{ }

	template< std::size_t I = 0, typename FuncT, typename... Tp >
	inline typename std::enable_if< I < sizeof...(Tp), void >::type
	call( std::tuple< Tp... >& t, FuncT f )
	{
		f( std::get< I >( t ) );
		call< I + 1, FuncT, Tp... >( t, f );
	}


	template< class T, typename ref_t = ref< T > >
	void update( connection_container< T > & connections )
	{
		call( connections, &ref_t::element_type::update );
	}
}

namespace operators {
    template<
        typename in_t,
        typename out_t
    >
        inline const connection_container< out_t >& operator >> ( connection_container< in_t > & inputs,
            const connection_container< out_t > & outputs )
    {
        algorithms::call( inputs, outputs, &in_t::connect );
        return outputs;
    }

    template<
        typename in_t,
        typename out_t,
        typename out_ref_t = ref< out_t >
    >
        inline const out_ref_t & operator >> ( connection_container< in_t > & inputs,
            const out_ref_t & output )
    {
        algorithms::call( inputs, output, &in_t::connect );
        return output;
    }
}

} };
