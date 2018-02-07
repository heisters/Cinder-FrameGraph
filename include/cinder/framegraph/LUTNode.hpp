#pragma once

#include "cinder/FrameGraph.hpp"

namespace cinder {
namespace frame_graph {

typedef std::shared_ptr< class LUTNode > LUTShaderIONodeRef;

class LUTNode : public TextureShaderIONode< 2 > {
public:

    static LUTShaderIONodeRef create( const ci::ivec2 & size )
    {
        return std::make_shared< LUTNode >( size );
    }

    LUTNode( const ci::ivec2 & size );
};



}
}
