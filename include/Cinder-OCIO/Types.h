#pragma once

namespace cinder { namespace ocio {

template< class node_t >
using ref = std::shared_ptr< node_t >;

typedef ref< class ImageONode >			ImageONodeRef;
typedef ref< class ImageINode >			ImageINodeRef;
typedef ref< class ImageIONode >		ImageIONodeRef;
typedef ref< class ProcessIONode >		ProcessIONodeRef;
typedef ref< class SurfaceINode >		SurfaceINodeRef;
typedef ref< class TextureONode >		TextureONodeRef;
typedef ref< class ProcessGPUIONode >	ProcessGPUIONodeRef;
template< class o_node_t >
using MuxONodeRef = ref< o_node_t >;

} }