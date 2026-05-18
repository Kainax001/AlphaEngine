#include "AlphaGraphic/core/BVHBuilder.h"

#include <algorithm>
#include <limits>

namespace AG {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static float TriCentroid(const TriangleProxy& t, int axis)
{
    return (t.v0[axis] + t.v1[axis] + t.v2[axis]) / 3.0f;
}

static glm::vec3 TriMin(const TriangleProxy& t)
{
    return glm::vec3(
        std::min({ t.v0.x, t.v1.x, t.v2.x }),
        std::min({ t.v0.y, t.v1.y, t.v2.y }),
        std::min({ t.v0.z, t.v1.z, t.v2.z }));
}

static glm::vec3 TriMax(const TriangleProxy& t)
{
    return glm::vec3(
        std::max({ t.v0.x, t.v1.x, t.v2.x }),
        std::max({ t.v0.y, t.v1.y, t.v2.y }),
        std::max({ t.v0.z, t.v1.z, t.v2.z }));
}

// ---------------------------------------------------------------------------
// Recursive build
// ---------------------------------------------------------------------------

int BVHBuilder::BuildRecursive(std::vector<BVHNodeProxy>& nodes,
                                std::vector<TriangleProxy>& tris,
                                int start, int end)
{
    int nodeIdx = (int)nodes.size();
    nodes.emplace_back();   // reserve slot (index stable after this push)

    // Compute AABB over [start, end)
    const float INF = std::numeric_limits<float>::max();
    glm::vec3 bmin = glm::vec3(INF);
    glm::vec3 bmax = glm::vec3(-INF);
    for (int i = start; i < end; i++)
    {
        glm::vec3 lo = TriMin(tris[i]);
        glm::vec3 hi = TriMax(tris[i]);
        bmin = glm::vec3(std::min(bmin.x, lo.x), std::min(bmin.y, lo.y), std::min(bmin.z, lo.z));
        bmax = glm::vec3(std::max(bmax.x, hi.x), std::max(bmax.y, hi.y), std::max(bmax.z, hi.z));
    }
    nodes[nodeIdx].aabbMin = glm::vec4(bmin.x, bmin.y, bmin.z, 0.0f);
    nodes[nodeIdx].aabbMax = glm::vec4(bmax.x, bmax.y, bmax.z, 0.0f);

    int count = end - start;

    // Leaf: 4 triangles or fewer
    if (count <= 4)
    {
        nodes[nodeIdx].left      = -1;
        nodes[nodeIdx].right     = count;
        nodes[nodeIdx].triOffset = start;
        nodes[nodeIdx].pad       = 0;
        return nodeIdx;
    }

    // Choose longest axis and split at centroid midpoint
    glm::vec3 extent = bmax - bmin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    float mid = (bmin[axis] + bmax[axis]) * 0.5f;

    auto pivot = std::partition(
        tris.begin() + start, tris.begin() + end,
        [axis, mid](const TriangleProxy& t)
        {
            return TriCentroid(t, axis) < mid;
        });

    int midIdx = (int)(pivot - tris.begin());

    // Degenerate: all triangles on the same side
    if (midIdx == start || midIdx == end)
        midIdx = start + count / 2;

    int left  = BuildRecursive(nodes, tris, start,  midIdx);
    int right = BuildRecursive(nodes, tris, midIdx, end);

    // NOTE: nodes may have been reallocated during recursion; access by index only
    nodes[nodeIdx].left      = left;
    nodes[nodeIdx].right     = right;
    nodes[nodeIdx].triOffset = 0;
    nodes[nodeIdx].pad       = 0;
    return nodeIdx;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

std::vector<BVHNodeProxy> BVHBuilder::Build(std::vector<TriangleProxy>& tris)
{
    std::vector<BVHNodeProxy> nodes;
    if (tris.empty()) return nodes;
    nodes.reserve(2 * (int)tris.size()); // upper bound: 2n-1 nodes
    BuildRecursive(nodes, tris, 0, (int)tris.size());
    return nodes;
}

} // namespace AG
