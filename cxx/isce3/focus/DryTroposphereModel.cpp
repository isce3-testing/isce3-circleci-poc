#include "DryTroposphereModel.h"

#include <isce3/except/Error.h>

using isce3::except::InvalidArgument;

namespace isce3 { namespace focus {

std::string toString(DryTroposphereModel m)
{
    switch (m) {
    case DryTroposphereModel::NoDelay: return "nodelay";
    case DryTroposphereModel::TSX: return "tsx";
    }

    throw InvalidArgument(ISCE_SRCINFO(), "unexpected dry troposphere model");
}

DryTroposphereModel parseDryTropoModel(const std::string& s)
{
    if (s == "nodelay") {
        return DryTroposphereModel::NoDelay;
    }
    if (s == "tsx") {
        return DryTroposphereModel::TSX;
    }

    std::string errmsg =
            "expected one of {'nodelay', 'tsx'}, instead got '" + s + "'";
    throw InvalidArgument(ISCE_SRCINFO(), errmsg);
}

}} // namespace isce3::focus
