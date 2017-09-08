#pragma once

#include "Cinder-FrameGraph.hpp"

namespace cinder {
namespace frame_graph {

typedef std::shared_ptr< class LUTShaderIONode > LUTShaderIONodeRef;

class LUTShaderIONode : public TextureShaderIONode< 2 > {
public:

    static LUTShaderIONodeRef create( const ci::ivec2 & size )
    {
        return std::make_shared< LUTShaderIONode >( size );
    }

    LUTShaderIONode( const ci::ivec2 & size );
};



}
}