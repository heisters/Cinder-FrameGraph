#pragma once

namespace cinder { namespace ocio {

template< class node_t >
using ref = std::shared_ptr< node_t >;

template< class o_node_t >
class INode;
template< class o_node_t >
using INodeRef = ref< INode< o_node_t > >;

typedef ref< class ONode >				ONodeRef;
typedef ref< class ImageONode >			ImageONodeRef;
typedef ref< class ImageINode >			ImageINodeRef;
typedef ref< class ImageIONode >		ImageIONodeRef;
typedef ref< class ProcessIONode >		ProcessIONodeRef;
typedef ref< class SurfaceINode >		SurfaceINodeRef;
typedef ref< class TextureINode >		TextureINodeRef;
typedef ref< class TextureONode >		TextureONodeRef;
typedef ref< class ProcessGPUIONode >	ProcessGPUIONodeRef;
typedef ref< class TextureShaderIONode > TextureShaderIONodeRef;

template< typename ... Types >
class CompositeNode;

template< typename ... Types >
using CompositeNodeRef = ref< CompositeNode< Types... > >;

template< class o_node_t >
using MuxONodeRef = ref< o_node_t >;

} }