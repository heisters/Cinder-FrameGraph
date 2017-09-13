#pragma once

#include "Cinder-FrameGraph.hpp"

namespace cinder {
namespace frame_graph {

class ColorGradeShaderIONode :
        public FullScreenQuadRenderer< 1 >,
        public Node< Inlets<
                gl::Texture2dRef,
                float, // exposure
                vec3,  // lift, gamma, gain
                int,   // temperature (Kelvin)
                float, // contrast
                float, // midtone_contrast
                vec3   // hue, saturation, value
        >, Outlets< gl::Texture2dRef > >
{
public:
    enum inlet_names {
        first_inlet = 1,
        exposure = 1,
        LGG,
        temperature,
        contrast,
        midtone_contrast,
        HSV,
        last_inlet
    };


    ColorGradeShaderIONode( const ci::ivec2 & size );

};

}
}
