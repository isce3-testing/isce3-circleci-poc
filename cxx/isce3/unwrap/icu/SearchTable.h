#pragma once

#include <cstddef> // size_t

#include "ICU.h" // offset2_t

namespace isce3::unwrap::icu {

// \brief Lookup table of points to search for residues or neutrons
//
// Search points are stored in order of increasing distance with elliptical
// distance contours determined by the relative pixel spacing in x & y.
class SearchTable {
public:
    // Constructor
    SearchTable(const int maxdist, const float ratioDxDy = 1.f);
    // Destructor
    ~SearchTable();
    // Access specified element.
    offset2_t& operator[](const size_t pos) const;
    // Get number of search points within the ellipse with semi-major axis a.
    size_t numPtsInEllipse(const int a) const;

private:
    // Array of search points
    offset2_t* _searchpts;
    // Number of search points within each integer distance
    size_t* _npts;
};

} // namespace isce3::unwrap::icu

// Get inline implementations.
#define ISCE_UNWRAP_ICU_SEARCHTABLE_ICC
#include "SearchTable.icc"
#undef ISCE_UNWRAP_ICU_SEARCHTABLE_ICC
