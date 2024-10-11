#include <numbers>

#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Geometry/GeodesicPath.h>

RTRC_GEO_BEGIN

GeodesicPathICH::GeodesicPathICH(
    ObserverPtr<const HalfedgeMesh> connectivity, Span<Vector3d> positions, double edgeLengthTolerance)
    : connectivity_(connectivity)
{
    assert(connectivity_ && connectivity_->IsCompacted());
    
    isPseudoSourceVertex_.resize(connectivity_->V(), false);
    for(int v = 0; v < connectivity_->V(); ++v)
    {
        double sumAngle = 0;
        connectivity_->ForEachOutgoingHalfedge(v, [&](int h)
        {
            const Vector3d &p0 = positions[connectivity->VertToOriginalVert(v)];
            const Vector3d &p1 = positions[connectivity->VertToOriginalVert(connectivity_->Tail(h))];
            const Vector3d &p2 = positions[connectivity->VertToOriginalVert(connectivity_->Vert(connectivity_->Prev(h)))];
            const double cosAngle = Cos(p1 - p0, p2 - p0);
            const double angle = std::acos(Clamp(cosAngle, -1.0, 1.0));
            sumAngle += angle;
        });

        if(sumAngle > 2.0 * std::numbers::pi_v<double> * (1.0 - SADDLE_EPS) || connectivity_->IsVertOnBoundary(v))
        {
            isPseudoSourceVertex_[v] = true;
        }
    }

    edgeToLength_.resize(connectivity_->E());
    for(int e = 0; e < connectivity_->E(); ++e)
    {
        const int h = connectivity_->EdgeToHalfedge(e);
        const int head = connectivity_->Head(h);
        const int tail = connectivity_->Tail(h);
        edgeToLength_[e] = Length(
            positions[connectivity->VertToOriginalVert(head)] - positions[connectivity->VertToOriginalVert(tail)]);
    }

    // Bias lengths globally so that for every triangle abc is non-degenerate (ab + bc > ac + edgeLengthTolerance)

    double eps = 0;
    for(int f = 0; f < connectivity_->F(); ++f)
    {
        const int e0 = connectivity_->Edge(3 * f + 0);
        const int e1 = connectivity_->Edge(3 * f + 1);
        const int e2 = connectivity_->Edge(3 * f + 2);
        const double l0 = edgeToLength_[e0];
        const double l1 = edgeToLength_[e1];
        const double l2 = edgeToLength_[e2];
        const double localEps = (std::max)(
        {
            edgeLengthTolerance - l0 - l1 + l2,
            edgeLengthTolerance - l0 - l2 + l1,
            edgeLengthTolerance - l1 - l2 + l0,
        });
        eps = (std::max)(eps, localEps);
    }

    for(double& l : edgeToLength_)
    {
        l += eps;
    }

    // Fill halfedgeToOppositeVertexPositions_

    halfedgeToOppositeVertexPositions_.resize(connectivity_->H());
    for(int h = 0; h < connectivity_->H(); ++h)
    {
        const int e0 = connectivity_->Edge(h);
        const int e1 = connectivity_->Edge(connectivity_->Succ(h));
        const int e2 = connectivity_->Edge(connectivity_->Prev(h));

        const double l0 = edgeToLength_[e0];
        const double l1 = edgeToLength_[e1];
        const double l2 = edgeToLength_[e2];

        const double xb = l0;
        const double a = l1;
        const double b = l2;
        const double x = (xb * xb + a * a - b * b) / (2 * xb);
        const double y = std::max(1e-10, std::sqrt((std::max)(0.0, a * a - x * x)));
        halfedgeToOppositeVertexPositions_[h] = { x, y };
    }
}

std::vector<GeodesicPathICH::PathPoint> GeodesicPathICH::FindShortestPath(
    int sourceVertex, int targetVertex, bool enableXinWangCulling)
{
    assert(sourceVertex != targetVertex);

    Context context;
    context.cullUselessWindowsUsingXinWangConditions = enableXinWangCulling;
    context.source = sourceVertex;
    context.target = targetVertex;
    context.vertexRecords.resize(connectivity_->V());
    context.halfedgeRecords.resize(connectivity_->H());

    // Forward propagation

    auto rootNode = context.NewNode();
    rootNode->isInQueue = true;
    rootNode->depth     = 1;
    rootNode->isVertex  = true;
    rootNode->element   = sourceVertex;
    context.activeNodes.push(rootNode);

    while(!context.activeNodes.empty())
    {
        Node *node = context.activeNodes.top();
        context.activeNodes.pop();

        assert(node->isInQueue);
        node->isInQueue = false;

        // A subtree containing this node was deleted. Recycle the node now since it's not in the queue.
        if(node->element < 0)
        {
            context.RecycleNode(node);
            continue;
        }

        if(node->isVertex)
        {
            ProcessVertexNode(context, node);
        }
        else
        {
            ProcessEdgeNode(context, node);
        }
    }

    // Backtrace

    auto node = context.vertexRecords[targetVertex].vertexNode;
    if(!node)
    {
        return {};
    }

    std::vector<PathPoint> result;

    while(node)
    {
        if(node->isVertex)
        {
            result.emplace_back(VertexPoint{ node->element });
        }
        else
        {
            EdgePoint newEdgePoint;
            newEdgePoint.halfedge = node->element;

            const int hca = newEdgePoint.halfedge;
            const double ac = edgeToLength_[connectivity_->Edge(hca)];
            const Vector2d b = halfedgeToOppositeVertexPositions_[hca];

            result.back().Match(
                [&](const VertexPoint &lastVertexPoint)
                {
                    assert(lastVertexPoint.v == connectivity_->Vert(connectivity_->Prev(hca)));
                    const double positionX = IntersectWithXAxis(b, node->unfoldedSourcePosition);
                    newEdgePoint.normalizedPosition = 1 - Clamp(positionX / ac, 1e-5, 1 - 1e-5);
                },
                [&](const EdgePoint &lastEdgePoint)
                {
                    Vector2d u;
                    if(connectivity_->Twin(connectivity_->Succ(hca)) == lastEdgePoint.halfedge)
                    {
                        u = Lerp({ 0, 0 }, b, 1 - lastEdgePoint.normalizedPosition);
                    }
                    else
                    {
                        assert(connectivity_->Twin(connectivity_->Prev(hca)) == lastEdgePoint.halfedge);
                        u = Lerp(b, { ac, 0 }, 1 - lastEdgePoint.normalizedPosition);
                    }
                    const double positionX = IntersectWithXAxis(u, node->unfoldedSourcePosition);
                    newEdgePoint.normalizedPosition = 1 - Clamp(positionX / ac, 1e-5, 1 - 1e-5);
                });
            result.emplace_back(newEdgePoint);
        }
        node = node->parent;
    }

    std::ranges::reverse(result);
    return result;
}

void GeodesicPathICH::Node::UpdateMinDistance()
{
    if(isVertex)
    {
        minDistance = baseDistance;
        return;
    }

    const double localDistance = ComputeMinimalEdgeWindowDistance(unfoldedSourcePosition, intervalBegin, intervalEnd);
    minDistance = baseDistance + localDistance;
}

bool GeodesicPathICH::ShouldUpdateDistance(double oldDistance, double newDistance)
{
    return newDistance + DISTANCE_UPDATE_EPS < oldDistance;
}

void GeodesicPathICH::ProcessVertexNode(Context &context, Node *node)
{
    if(node->minDistance > context.vertexRecords[context.target].distanceToSource || node->depth > connectivity_->F())
    {
        return;
    }

    const int v = node->element;
    VertexRecord &record = context.vertexRecords[v];
    if(!ShouldUpdateDistance(record.distanceToSource, node->baseDistance))
    {
        return;
    }

    if(record.vertexNode)
    {
        DeleteSubTree(context, record.vertexNode);
    }
    record.vertexNode = node;
    record.distanceToSource = node->baseDistance;

    if(!isPseudoSourceVertex_[v] && v != context.source)
    {
        return;
    }

    assert(!node->child);
    connectivity_->ForEachNeighbor(v, [&](int nv)
    {
        const int e = connectivity_->FindEdgeByVertex(v, nv);
        const double edgeLength = edgeToLength_[e];
        if(const double newDistance = node->baseDistance + edgeLength;
           ShouldUpdateDistance(context.vertexRecords[nv].distanceToSource, newDistance))
        {
            auto newNode = context.NewNode();
            AddChildNode(node, newNode);
            
            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = true;
            newNode->element      = nv;
            newNode->baseDistance = newDistance;

            newNode->UpdateMinDistance();
            context.activeNodes.push(newNode);
        }
    });

    connectivity_->ForEachOutgoingHalfedge(v, [&](int outgoingHalfedge)
    {
        const int hi = connectivity_->Succ(outgoingHalfedge);
        const int ho = connectivity_->Twin(hi);
        if(ho == HalfedgeMesh::NullID)
        {
            return;
        }

        const double edgeLength = edgeToLength_[connectivity_->Edge(hi)];

        Vector2d pseudoSource = halfedgeToOppositeVertexPositions_[hi];
        pseudoSource.y = -pseudoSource.y;
        pseudoSource.x = edgeLength - pseudoSource.x;

        auto newNode = context.NewNode();
        AddChildNode(node, newNode);
        
        newNode->isInQueue              = true;
        newNode->depth                  = node->depth + 1;
        newNode->isVertex               = false;
        newNode->element                = ho;
        newNode->baseDistance           = node->baseDistance;
        newNode->unfoldedSourcePosition = pseudoSource;
        newNode->intervalBegin          = 0;
        newNode->intervalEnd            = edgeLength;

        newNode->UpdateMinDistance();
        context.activeNodes.push(newNode);
    });
}

void GeodesicPathICH::ProcessEdgeNode(Context &context, Node *node)
{
    if(node->minDistance > context.vertexRecords[context.target].distanceToSource || node->depth > connectivity_->F())
    {
        return;
    }

    const int hca = node->element;
    const int va = connectivity_->Tail(hca);
    const int vb = connectivity_->Head(connectivity_->Prev(hca));
    const int vc = connectivity_->Head(hca);
    const Vector2d b = halfedgeToOppositeVertexPositions_[hca];
    const double ac = edgeToLength_[connectivity_->Edge(hca)];

    assert(b.y > 0);
    assert(node->unfoldedSourcePosition.y < 0);
    const double m = IntersectWithXAxis(b, node->unfoldedSourcePosition);

    // Add a new vertex node when the opposite vertex is covered by the shadow of the edge window
    if(node->intervalBegin - COVER_POINT_EPS <= m && m <= node->intervalEnd + COVER_POINT_EPS)
    {
        if(const double newDistance = node->baseDistance + Length(b - node->unfoldedSourcePosition);
           ShouldUpdateDistance(context.vertexRecords[vb].distanceToSource, newDistance))
        {
            auto newNode = context.NewNode();
            AddChildNode(node, newNode);

            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = true;
            newNode->element      = vb;
            newNode->baseDistance = newDistance;

            newNode->UpdateMinDistance();
            context.activeNodes.push(newNode);
        }
    }

    const bool leftChild = node->intervalBegin < m;
    const bool rightChild = node->intervalEnd > m;

    // When 'partial' is false, the right endpoint of the new window must be at b. Otherwise it will be on ab.
    auto AddLeftChild = [&](bool partial)
    {
        const int hab = connectivity_->Succ(hca);
        const int habTwin = connectivity_->Twin(hab);
        if(habTwin != HalfedgeMesh::NullID)
        {
            const double edgeLength = edgeToLength_[connectivity_->Edge(hab)];

            const Vector2d inctLeft = IntersectLineSegment(
                node->unfoldedSourcePosition, Vector2d(node->intervalBegin, 0), { 0, 0 }, b, false);
            const double intervalBegin = Clamp(Length(inctLeft - Vector2d(0, 0)), 0.0, edgeLength);

            double intervalEnd;
            if(partial)
            {
                const Vector2d inctRight = IntersectLineSegment(
                    node->unfoldedSourcePosition, Vector2d(node->intervalEnd, 0), { 0, 0 }, b, true);
                intervalEnd = Length(inctRight - Vector2d(0, 0));
                intervalEnd = Clamp(intervalEnd, intervalBegin, edgeLength);
            }
            else
            {
                intervalEnd = edgeLength;
            }

            if(context.cullUselessWindowsUsingXinWangConditions)
            {
                const Vector2d normalizedAToB = Normalize(b);
                const Vector2d B = intervalEnd * normalizedAToB;
                const double aB = Distance({ 0, 0 }, B);
                const double IB = Distance(node->unfoldedSourcePosition, B);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[va].distanceToSource + aB) < node->baseDistance + IB)
                {
                    return;
                }

                const Vector2d A = intervalBegin * normalizedAToB;
                const double IA = Distance(node->unfoldedSourcePosition, A);
                const double bA = Distance(b, A);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[vb].distanceToSource + bA) < node->baseDistance + IA)
                {
                    return;
                }

                const double cA = Distance({ ac, 0 }, A);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[vc].distanceToSource + cA) < node->baseDistance + IA)
                {
                    return;
                }
            }

            auto newNode = context.NewNode();
            AddChildNode(node, newNode);

            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = false;
            newNode->isLeft       = true;
            newNode->element      = habTwin;
            newNode->baseDistance = node->baseDistance;

            newNode->unfoldedSourcePosition = PropagatePseudoSource(node->unfoldedSourcePosition, b, ac, true);
            newNode->intervalBegin          = intervalBegin;
            newNode->intervalEnd            = intervalEnd;

            newNode->UpdateMinDistance();
            context.activeNodes.push(newNode);
        }
    };

    auto AddRightChild = [&](bool partial)
    {
        const int hbc = connectivity_->Prev(hca);
        const int hbcTwin = connectivity_->Twin(hbc);
        if(hbcTwin != HalfedgeMesh::NullID)
        {
            const double edgeLength = edgeToLength_[connectivity_->Edge(hbc)];

            const Vector2d inctRight = IntersectLineSegment(
                node->unfoldedSourcePosition, Vector2d(node->intervalEnd, 0), { ac, 0 }, b, false);
            const double intervalEnd = Clamp(Length(inctRight - b), 0.0, edgeLength);

            double intervalBegin;
            if(partial)
            {
                const Vector2d inctLeft = IntersectLineSegment(
                    node->unfoldedSourcePosition, Vector2d(node->intervalBegin, 0), { ac, 0 }, b, true);
                intervalBegin = Length(inctLeft - b);
                intervalBegin = Clamp(intervalBegin, 0.0, intervalEnd);
            }
            else
            {
                intervalBegin = 0;
            }

            if(context.cullUselessWindowsUsingXinWangConditions)
            {
                const Vector2d c = { ac, 0 };
                const Vector2d normalizedBToC = Normalize(c - b);
                const Vector2d B = b + normalizedBToC * intervalBegin;
                const double IB = Distance(node->unfoldedSourcePosition, B);
                const double cB = Distance(c, B);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[vc].distanceToSource + cB) < node->baseDistance + IB)
                {
                    return;
                }

                const Vector2d A = b + normalizedBToC * intervalEnd;
                const double IA = Distance(node->unfoldedSourcePosition, A);
                const double bA = Distance(b, A);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[vb].distanceToSource + bA) < node->baseDistance + IA)
                {
                    return;
                }

                const double aA = Distance({ 0, 0 }, A);
                if((1 + XIN_WANG_EPS) * (context.vertexRecords[va].distanceToSource + aA) < node->baseDistance + IA)
                {
                    return;
                }
            }

            auto newNode = context.NewNode();
            AddChildNode(node, newNode);

            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = false;
            newNode->isLeft       = false;
            newNode->element      = hbcTwin;
            newNode->baseDistance = node->baseDistance;

            newNode->unfoldedSourcePosition = PropagatePseudoSource(node->unfoldedSourcePosition, b, ac, false);
            newNode->intervalBegin          = intervalBegin;
            newNode->intervalEnd            = intervalEnd;

            newNode->UpdateMinDistance();
            context.activeNodes.push(newNode);
        }
    };

    if(leftChild && !rightChild)
    {
        AddLeftChild(true);
    }
    else if(!leftChild && rightChild)
    {
        AddRightChild(true);
    }
    else
    {
        assert(leftChild && rightChild);

        HalfedgeRecord &halfedgeRecord = context.halfedgeRecords[hca];
        if(const double occupierDistance = node->baseDistance + Length(node->unfoldedSourcePosition - b);
           occupierDistance < halfedgeRecord.occupierDistance)
        {
            if(halfedgeRecord.occupier)
            {
                assert(!halfedgeRecord.occupier->isVertex);
                if(halfedgeRecord.occupier->child)
                {
                    if(halfedgeRecord.occupierM > m + REMOVE_CHILD_EPS)
                    {
                        // Delete the left subtree of the original occupier

                        Node *childNodeToBeDeleted = nullptr, *currentNode = halfedgeRecord.occupier->child;
                        for(int i = 0; i < 3; ++i)
                        {
                            if(!currentNode->isVertex && currentNode->isLeft)
                            {
                                childNodeToBeDeleted = currentNode;
                                break;
                            }
                            currentNode = currentNode->succ;
                        }

                        if(childNodeToBeDeleted)
                        {
                            DeleteSubTree(context, childNodeToBeDeleted);
                        }
                    }
                    else if(halfedgeRecord.occupierM < m - REMOVE_CHILD_EPS)
                    {
                        // Delete the right subtree

                        Node *childNodeToBeDeleted = nullptr, *currentNode = halfedgeRecord.occupier->child;
                        for(int i = 0; i < 3; ++i)
                        {
                            if(!currentNode->isVertex && !currentNode->isLeft)
                            {
                                childNodeToBeDeleted = currentNode;
                                break;
                            }
                            currentNode = currentNode->succ;
                        }

                        if(childNodeToBeDeleted)
                        {
                            DeleteSubTree(context, childNodeToBeDeleted);
                        }
                    }
                }
            }

            halfedgeRecord.occupier         = node;
            halfedgeRecord.occupierDistance = occupierDistance;
            halfedgeRecord.occupierM        = m;

            AddLeftChild(false);
            AddRightChild(false);
        }
        else
        {
            if(m < halfedgeRecord.occupierM)
            {
                AddLeftChild(false);
            }
            else
            {
                AddRightChild(false);
            }
        }
    }
}

void GeodesicPathICH::AddChildNode(Node *parent, Node *child)
{
    assert(!child->parent);
    child->parent = parent;

    if(!parent->child)
    {
        parent->child = child;
        child->prev = child;
        child->succ = child;
    }
    else
    {
        auto a = parent->child;
        auto b = parent->child->succ;
        a->succ = child;
        b->prev = child;
        child->prev = a;
        child->succ = b;
        parent->child = child;
    }
}

void GeodesicPathICH::DeleteSubTree(Context &context, Node *node, bool isSubTreeRoot)
{
    // There is no need to break the internal parent/child/brother links in the subtree since after the
    // deletion, all of these nodes will be no longer accessable from other nodes.
    if(isSubTreeRoot)
    {
        // Break the parent link
        if(Node *parent = node->parent)
        {
            if(parent->child == node)
            {
                if(node->succ == node) // This is the only child
                {
                    parent->child = nullptr;
                }
                else
                {
                    parent->child = node->succ;
                }
            }
        }

        assert(node->prev && node->succ);
        // Break the brother link
        {
            auto prev = node->prev;
            auto succ = node->succ;
            prev->succ = succ;
            succ->prev = prev;
        }
    }

    // Break the occupier link
    if(node->isVertex)
    {
        if(auto &record = context.vertexRecords[node->element]; record.vertexNode == node)
        {
            record.vertexNode = nullptr;
        }
    }
    else
    {
        auto &record = context.halfedgeRecords[node->element];
        if(record.occupier == node)
        {
            record.occupier = nullptr;
        }
    }

    if(node->child)
    {
        auto child = node->child;
        do
        {
            auto succ = child->succ;
            DeleteSubTree(context, child, false);
            child = succ;
        }
        while(child != node->child);
    }

    if(node->isInQueue)
    {
        node->element = -1;
    }
    else
    {
        context.RecycleNode(node);
    }
}

double GeodesicPathICH::ComputeMinimalEdgeWindowDistance(
    const Vector2d &pseudoSource, double intervalBegin, double intervalEnd)
{
    const double dx = Clamp(pseudoSource.x, intervalBegin, intervalEnd) - pseudoSource.x;
    const double dy = pseudoSource.y;
    return std::sqrt(dx * dx + dy * dy);
}

double GeodesicPathICH::IntersectWithXAxis(const Vector2d &a, const Vector2d &b)
{
    assert(a.y > 0 && b.y < 0);
    const double t = a.y / (a.y - b.y);
    return a.x + t * (b.x - a.x);
}

Vector2d GeodesicPathICH::IntersectLineSegment(
    const Vector2d &a, const Vector2d &b, const Vector2d &c, const Vector2d &d, bool mapNegativeToMax)
{
    const Vector2d A = b - a;
    const Vector2d B = c - d;
    const Vector2d C = c - a;

    const double det = Det2x2(A.x, B.x,
                              A.y, B.y);
    if(std::abs(det) < 1e-10)
    {
        return c;
    }

    const double detB = Det2x2(A.x, C.x,
                               A.y, C.y);
    double t = detB / det;
    if(mapNegativeToMax && t < 0)
    {
        t = 1;
    }
    t = Saturate(t);
    return c + t * (d - c);
}

Vector2d GeodesicPathICH::PropagatePseudoSource(const Vector2d &o, const Vector2d &b, double ac, bool left)
{
    constexpr double MAX_Y = -1e-6;
    if(left)
    {
        const double angle = std::atan2(b.y, b.x);
        const double s = std::sin(angle);
        const double c = std::cos(angle);
        return { c * o.x + s * o.y, (std::min)(-s * o.x + c * o.y, MAX_Y) };
    }
    const Vector2d to = o - b;
    const double angle = std::atan2(-b.y, ac - b.x);
    const double s = std::sin(angle);
    const double c = std::cos(angle);
    return { c * to.x + s * to.y, (std::min)(-s * to.x + c * to.y, MAX_Y) };
}

GeodesicPathICH::Node *GeodesicPathICH::Context::NewNode()
{
    if(freeNodes.empty())
    {
        return nodeAllocator.Create<Node>();
    }
    auto ret = freeNodes.back();
    freeNodes.pop_back();
    *ret = Node();
    return ret;
}

void GeodesicPathICH::Context::RecycleNode(Node *node)
{
    freeNodes.push_back(node);
}

template<typename T>
Vector3<T> EvaluateGeodesicPathPoint(
    const HalfedgeMesh               &connectivity,
    Span<Vector3<T>>                  positions,
    const GeodesicPathICH::PathPoint &pathPoint)
{
    assert(connectivity.IsCompacted() && connectivity.IsInputManifold());
    return pathPoint.Match(
        [&](const GeodesicPathICH::VertexPoint &vertexPoint)
        {
            return positions[vertexPoint.v];
        },
        [&](const GeodesicPathICH::EdgePoint &edgePoint)
        {
            const Vector3<T> &a = positions[connectivity.Head(edgePoint.halfedge)];
            const Vector3<T> &b = positions[connectivity.Tail(edgePoint.halfedge)];
            return Lerp(a, b, static_cast<T>(edgePoint.normalizedPosition));
        });
}

template Vector3<float>  EvaluateGeodesicPathPoint(const HalfedgeMesh &, Span<Vector3<float>>,  const GeodesicPathICH::PathPoint &);
template Vector3<double> EvaluateGeodesicPathPoint(const HalfedgeMesh &, Span<Vector3<double>>, const GeodesicPathICH::PathPoint &);

namespace EmbedGeodesicPathDetail
{

    // A sub path is a geodesic path which contains only two vertex points (start and end).
    // All other intermediate points must be edge points.
    struct SubPathPoint
    {
        // When the tail is -1, this is a vertex point, otherwise an edge point.
        int head, tail;
        double normalizedPosition;
    };

    std::vector<int> EmbedEdgePointsInGeodesicSubPath(
        HalfedgeMesh           &connectivity,
        std::vector<Vector3d>  &positions,
        Span<SubPathPoint>  edgePoints)
    {
        std::vector<int> result;
        result.reserve(edgePoints.size());

        for(const SubPathPoint &edgePoint : edgePoints)
        {
            const Vector3d &pHead = positions[edgePoint.head];
            const Vector3d &pTail = positions[edgePoint.tail];
            const Vector3d newPoint = Lerp(pHead, pTail, edgePoint.normalizedPosition);

            const int h = connectivity.FindHalfedgeByVertex(edgePoint.head, edgePoint.tail);
            assert(h >= 0);

            const int v = connectivity.V();
            result.push_back(v);

            connectivity.SplitEdge(h);
            positions.push_back(newPoint);
        }

        return result;
    }

} // namespace EmbedGeodesicPathDetail

std::vector<int> EmbedGeodesicPath(
    HalfedgeMesh                    &connectivity,
    std::vector<Vector3d>           &positions,
    Span<GeodesicPathICH::PathPoint> path)
{
    using namespace EmbedGeodesicPathDetail;

    assert(path.first().Is<Geo::GeodesicPathICH::VertexPoint>());
    assert(path.last().Is<Geo::GeodesicPathICH::VertexPoint>());

    // Store halfedges using heads and tails, since heads and tails are stable
    // during the embedding process, while halfedge indices are not.

    std::vector<SubPathPoint> convertedPathPoints;
    convertedPathPoints.reserve(path.size());
    for(auto &p : path)
    {
        p.Match(
            [&](const GeodesicPathICH::VertexPoint &vp)
            {
                convertedPathPoints.push_back({ vp.v, -1, 0 });
            },
            [&](const GeodesicPathICH::EdgePoint &ep)
            {
                const int head = connectivity.Head(ep.halfedge);
                const int tail = connectivity.Tail(ep.halfedge);
                convertedPathPoints.push_back({ head, tail, ep.normalizedPosition });
            });
    }

    // Subdivide the whole path into multiple sub paths and embed them one-by-one.

    std::vector<int> result;
    for(uint32_t subPathStart = 0, subPathEnd; subPathStart < convertedPathPoints.size() - 1; subPathStart = subPathEnd)
    {
        assert(convertedPathPoints[subPathStart].tail == -1);
        result.push_back(convertedPathPoints[subPathStart].head);

        subPathEnd = subPathStart + 1;
        while(convertedPathPoints[subPathEnd].tail != -1)
        {
            ++subPathEnd;
            assert(subPathEnd < convertedPathPoints.size());
        }

        const uint32_t edgePointStart = subPathStart + 1;
        const uint32_t edgePointEnd = subPathEnd;
        if(edgePointStart < edgePointEnd)
        {
            const auto edgePoints = EmbedEdgePointsInGeodesicSubPath(
                connectivity, positions,
                Span(convertedPathPoints).GetSubSpan(edgePointStart, edgePointEnd - edgePointStart));
            result.append_range(edgePoints);
        }
    }
    result.push_back(convertedPathPoints.back().head);

    return result;
}

RTRC_GEO_END
