#pragma once

#include <queue>

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Memory/Arena.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Geometry/HalfedgeMesh.h>

RTRC_GEO_BEGIN

// See
//  [The Discrete Geodesic Problem]
//  [Shortest Paths on a Polyhedron]
//  [Improving Chen and Han's algorithm on the discrete geodesic problem]
class GeodesicPathICH
{
public:

    struct EdgePoint
    {
        int halfedge;
        double normalizedPosition;
    };

    struct VertexPoint
    {
        int v;
    };

    using PathPoint = Variant<EdgePoint, VertexPoint>;

    // Note: positions should be indexed by 'vertex in connectivity', instead of 'original vertex in connectivity'.
    GeodesicPathICH(
        ObserverPtr<const HalfedgeMesh> connectivity, Span<Vector3d> positions, double edgeLengthTolerance = -1);

    // Returns Empty vector if the algorithm fails to find any path. Otherwise, the frist element in the returned path
    // must be a vertex point corresponding to sourceVertex, and the last element must be targetVertex.
    std::vector<PathPoint> FindShortestPath(int sourceVertex, int targetVertex, bool enableXinWangCulling = true);

private:

    static constexpr double SADDLE_EPS          = 1e-4;
    static constexpr double DISTANCE_UPDATE_EPS = 1e-5;
    static constexpr double COVER_POINT_EPS     = 1e-5;
    static constexpr double XIN_WANG_EPS        = 1e-3;
    static constexpr double REMOVE_CHILD_EPS    = 1e-5;

    struct Node
    {
        bool isInQueue = false;
        int depth = 0;

        Node *parent = nullptr;
        Node *prev   = nullptr;
        Node *succ   = nullptr;
        Node *child  = nullptr;

        bool     isVertex     = false;
        bool     isLeft       = false; // Is this node the left child of its parent node. Meaningful only for edge node created by another edge node.
        int      element      = 0; // Vertex id for vertex node; halfedge id for edge node; -1 for pending delete node.
        double   baseDistance = 0;
        double   minDistance          = 0;

        // Edge nodes only. intervalBegin/End are stored as distance to tail(element).

        Vector2d unfoldedSourcePosition;
        double   intervalBegin = 0;
        double   intervalEnd   = 0;

        void UpdateMinDistance();
    };

    struct NodeKeyGreater
    {
        bool operator()(const Node *lhs, const Node *rhs) const
        {
            return lhs->minDistance > rhs->minDistance;
        }
    };

    struct VertexRecord
    {
        double distanceToSource = DBL_MAX;
        Node *vertexNode = nullptr;
    };

    struct HalfedgeRecord
    {
        Node *occupier = nullptr;
        double occupierDistance = DBL_MAX;
        double occupierM = 0;
    };

    struct Context
    {
        bool cullUselessWindowsUsingXinWangConditions = false;

        int source = 0;
        int target = 0;

        LinearAllocator nodeAllocator;
        std::vector<Node *> freeNodes;

        std::priority_queue<Node *, std::vector<Node*>, NodeKeyGreater> activeNodes;

        std::vector<VertexRecord> vertexRecords;
        std::vector<HalfedgeRecord> halfedgeRecords;

        Node *NewNode();
        void RecycleNode(Node *node);
    };

    static bool ShouldUpdateDistance(double oldDistance, double newDistance);

    void ProcessVertexNode(Context &context, Node *node);
    void ProcessEdgeNode(Context &context, Node *node);

    static void AddChildNode(Node *parent, Node *child);
    static void DeleteSubTree(Context &context, Node *node, bool isSubTreeRoot = true);

    // Compute distance between pseudoSource and segment (0, intervalbegin)-(0, intervalEnd)
    static double ComputeMinimalEdgeWindowDistance(
        const Vector2d &pseudoSource, double intervalBegin, double intervalEnd);

    // Returns the x-coordinate of the intersection point between the line segment ab and the x-axis,
    // given that point a is above the x-axis and point b is below the x-axis.
    static double IntersectWithXAxis(const Vector2d &a, const Vector2d &b);

    // Returns the intersection point between line ab and segment cd, assuming they intersect.
    // If ab and cd are collinear, point c is returned as the intersection point.
    static Vector2d IntersectLineSegment(
        const Vector2d &a, const Vector2d &b, const Vector2d &c, const Vector2d &d, bool mapNegativeToMax);

    static Vector2d PropagatePseudoSource(const Vector2d &o, const Vector2d &b, double ac, bool left);

    ObserverPtr<const HalfedgeMesh> connectivity_;

    std::vector<bool> isPseudoSourceVertex_;
    std::vector<double> edgeToLength_;

    // For each halfedge h, we transform its corresponding triangle s.t. tail(h) is at the origin, head(h) is on the
    // +x axis, and tail(succ(h)) is in the y > 0 area.
    // Store the transformed position of tail(succ(h)) in halfedgeToOppositeVertexPositions_[h] .
    std::vector<Vector2d> halfedgeToOppositeVertexPositions_;
};

template<typename T>
Vector3<T> EvaluateGeodesicPathPoint(
    const HalfedgeMesh               &connectivity,
    Span<Vector3<T>>                  positions,
    const GeodesicPathICH::PathPoint &pathPoint);

// Embed a geodesic path onto the given mesh, ensuring each segment aligns with a mesh edge.
std::vector<int> EmbedGeodesicPath(
    HalfedgeMesh                    &connectivity,
    std::vector<Vector3d>           &positions,
    Span<GeodesicPathICH::PathPoint> path);

RTRC_GEO_END
