#include <queue>

#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

void ResolveNonManifoldConnectivity(std::vector<uint32_t> &indices, std::vector<uint32_t> &newVertexToOldVertex)
{
    const HalfedgeMesh connectivity = HalfedgeMesh::Build(indices, HalfedgeMesh::SplitNonManifoldInput);
    assert(connectivity.H() == static_cast<int>(indices.size()));

    for(int h = 0; h < connectivity.H(); ++h)
    {
        indices[h] = connectivity.Vert(h);
    }

    newVertexToOldVertex.resize(connectivity.V());
    for(int v = 0; v < connectivity.V(); ++v)
    {
        newVertexToOldVertex[v] = connectivity.VertToOriginalVert(v);
    }
}

bool IsOrientable(const HalfedgeMesh &connectivity)
{
    if(!connectivity.IsInputManifold())
    {
        return false;
    }

    // 0: unknown; 1: pos; 2: neg
    std::vector<int> faceToOrientation(connectivity.F());
    std::queue<int> activeFaces;

    for(int seedFace = 0; seedFace < connectivity.F(); ++seedFace)
    {
        if(!connectivity.IsFaceValid(seedFace))
        {
            continue;
        }

        if(faceToOrientation[seedFace])
        {
            continue;
        }
        faceToOrientation[seedFace] = 1;

        assert(activeFaces.empty());
        activeFaces.push(seedFace);

        while(!activeFaces.empty())
        {
            const int f = activeFaces.front();
            activeFaces.pop();

            assert(faceToOrientation[f] == 1 || faceToOrientation[f] == 2);
            const int neighborOrientation = 3 - faceToOrientation[f];

            for(int i = 0; i < 3; ++i)
            {
                const int h = 3 * f + i;
                const int t = connectivity.Twin(h);
                if(t == HalfedgeMesh::NullID)
                {
                    continue;
                }

                const int ft = connectivity.Face(t);
                if(!faceToOrientation[ft])
                {
                    faceToOrientation[ft] = neighborOrientation;
                    activeFaces.push(ft);
                }
                else if(faceToOrientation[ft] != neighborOrientation)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

RTRC_GEO_END
