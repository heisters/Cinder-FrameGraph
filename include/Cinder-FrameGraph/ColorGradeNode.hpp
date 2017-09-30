#pragma once

#include "Cinder-FrameGraph.hpp"

namespace cinder {
namespace frame_graph {

//! A node that provides basic real-time color grading.
//! Inlets except temperature expect values in the range of roughly -2 to +2.
//! 0 means no change, > 0 is more of the effect, and < 0 is less of the effect.
//! Temperature takes degrees Kelvin, and is designed to be used with values
//! between 1000K and 40,000K.
class ColorGradeNode :
        public FullScreenQuadRenderer< 1 >,
        public Node< Inlets<
                gl::Texture2dRef,
                float, // exposure
                vec3,  // lift, gamma, gain
                float, // temperature (Kelvin)
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


    explicit ColorGradeNode( const ci::ivec2 & size );

};

}
}
