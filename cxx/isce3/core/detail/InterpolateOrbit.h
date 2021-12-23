#pragma once

#include <isce3/error/ErrorCode.h>

#include "../Common.h"
#include "../Orbit.h"
#include "../Vector.h"

namespace isce3 { namespace core { namespace detail {

template<class Orbit>
CUDA_HOSTDEV isce3::error::ErrorCode interpolateOrbit(Vec3* position,
        Vec3* velocity, const Orbit& orbit, double t,
        OrbitInterpBorderMode border_mode);

}}} // namespace isce3::core::detail

#define ISCE_CORE_DETAIL_INTERPOLATEORBIT_ICC
#include "InterpolateOrbit.icc"
#undef ISCE_CORE_DETAIL_INTERPOLATEORBIT_ICC
