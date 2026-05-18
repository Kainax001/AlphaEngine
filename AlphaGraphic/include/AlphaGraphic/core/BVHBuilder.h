#pragma once

#include "AlphaGraphic/core/RenderProxy.h"
#include <vector>

namespace AG {

// Builds a flat BVH (binary bounding-volume hierarchy) over a triangle list.
// The triangle array is reordered in-place for cache-friendly leaf traversal.
// Root node is always at index 0 in the returned node array.
class BVHBuilder
{
public:
    static std::vector<BVHNodeProxy> Build(std::vector<TriangleProxy>& tris);

private:
    static int BuildRecursive(std::vector<BVHNodeProxy>& nodes,
                               std::vector<TriangleProxy>& tris,
                               int start, int end);
};

} // namespace AG
