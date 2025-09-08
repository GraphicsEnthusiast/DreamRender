#ifndef _MATERIAL__GLSL__
#define _MATERIAL__GLSL__

#include "spectrum/spectrum.h"

struct MaterialEvalInfo {
    SampledSpectrum bsdf;
    float pdf;
};

#endif // _MATERIAL__GLSL__