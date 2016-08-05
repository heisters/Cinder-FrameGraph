#pragma once

namespace cinder { namespace ocio {

template< class node_t >
using ref = std::shared_ptr< node_t >;

//! A container for nodes.
template< class node_ref_t >
using NodeContainer = std::vector< node_ref_t >;

	namespace algorithms {
		template< class node_ref_t, class return_t >
		void call(NodeContainer< node_ref_t > & nodes,
				  return_t (node_ref_t::element_type::*fn)(void))
		{
			for ( auto & node : nodes ) ((*node).*fn)();
		}

		template<
		class a_node_t,
		class b_node_t,
		class a_node_ref_t = ref< a_node_t >,
		class b_node_ref_t = const ref< b_node_t >
		>
		void call(NodeContainer< a_node_ref_t > & anodes,
				  const NodeContainer< b_node_ref_t > & bnodes,
				  void (a_node_t::*fn)(const b_node_t &))
		{
			auto a_it		= anodes.begin();
			auto a_end		= anodes.end();
			auto b_it		= bnodes.begin();
			auto b_end		= bnodes.end();

			while ( a_it != a_end && b_it != b_end ) {
				((**a_it).*fn)( *b_it );
				++a_it;
				++b_it;
			}
		}


		template<
		class a_node_t,
		class b_node_t,
		class a_node_ref_t = ref< a_node_t >,
		class b_node_ref_t = const ref< b_node_t >
		>
		void call(NodeContainer< a_node_ref_t > & anodes,
				  const b_node_ref_t &bnode,
				  void (a_node_t::*fn)(const b_node_t &))
		{
			for ( auto & a : anodes ) ((*a).*fn)( bnode );
		}


		template< class node_ref_t >
		void update( NodeContainer< node_ref_t > & nodes )
		{
			call( nodes, &node_ref_t::element_type::update );
		}
	}

	namespace operators {
		template<
		typename in_ref_t,
		typename out_ref_t,
		typename in_t = typename in_ref_t::element_type
		>
		inline const NodeContainer< out_ref_t >& operator >>(NodeContainer< in_ref_t > & inputs,
															 const NodeContainer< out_ref_t > & outputs)
		{
			algorithms::call( inputs, outputs, &in_t::connect );
			return outputs;
		}

		template<
		typename in_ref_t,
		typename out_ref_t,
		typename in_t = typename in_ref_t::element_type
		>
		inline const out_ref_t & operator >> (NodeContainer< in_ref_t > & inputs,
											  const out_ref_t & output)
		{
			algorithms::call( inputs, output, &in_t::connect );
			return output;
		}
	}
} };