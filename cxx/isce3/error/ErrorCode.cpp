#include "ErrorCode.h"

#include <isce3/except/Error.h>

namespace isce3 { namespace error {

std::string getErrorString(ErrorCode status)
{
    switch (status) {
    case ErrorCode::Success: return "the operation completed without errors";
    case ErrorCode::OrbitInterpSizeError:
        return "insufficient orbit state vectors to form interpolant";
    case ErrorCode::OrbitInterpDomainError:
        return "interpolation point outside orbit domain";
    case ErrorCode::OrbitInterpUnknownMethod:
        return "unexpected orbit interpolation method";
    case ErrorCode::OutOfBoundsDem: return "out of bounds DEM";
    case ErrorCode::FailedToConverge:
        return "optimization routine failed to converge within the maximum "
               "number of iterations";
    case ErrorCode::WrongLookSide: return "wrong look side";
    case ErrorCode::OutOfBoundsLookup: return "out of bounds LUT lookup";
    }

    throw isce3::except::RuntimeError(ISCE_SRCINFO(), "unknown error code");
}

}} // namespace isce3::error
