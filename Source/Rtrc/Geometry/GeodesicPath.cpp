#include <numbers>

#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Geometry/GeodesicPath.h>

RTRC_GEO_BEGIN

GeodesicPathICH::GeodesicPathICH(ObserverPtr<const HalfedgeMesh> connectivity, Span<Vector3d> positions)
    : connectivity_(connectivity)
    , positions_(positions)
{
    assert(connectivity_ && connectivity_->IsCompacted() && connectivity_->IsInputManifold());
    
    isPseudoSourceVertex_.resize(connectivity_->V(), false);
    for(int v = 0; v < connectivity_->V(); ++v)
    {
        double sumAngle = 0;
        connectivity_->ForEachOutgoingHalfedge(v, [&](int h)
        {
            const Vector3d &p0 = positions[v];
            const Vector3d &p1 = positions[connectivity_->Tail(h)];
            const Vector3d &p2 = positions[connectivity_->Vert(connectivity_->Prev(h))];
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
        edgeToLength_[e] = Length(positions_[head] - positions_[tail]);
    }

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
        const double y = std::sqrt(std::max(0.0, a * a - x * x));
        halfedgeToOppositeVertexPositions_[h] = { x, y };
    }
}

std::vector<GeodesicPathICH::PathPoint> GeodesicPathICH::FindShortestPath(int sourceVertex, int targetVertex)
{
    assert(sourceVertex != targetVertex);

    Context context;
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
                    newEdgePoint.normalizedPosition = 1 - Saturate(positionX / ac);
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
                    newEdgePoint.normalizedPosition = 1 - Saturate(positionX / ac);
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
        if(const double newDistance = node->baseDistance + Length(positions_[v] - positions_[nv]);
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
    const int vb = connectivity_->Head(connectivity_->Prev(hca));
    const Vector2d b = halfedgeToOppositeVertexPositions_[hca];
    const double ac = edgeToLength_[connectivity_->Edge(hca)];

    assert(b.y > 0);
    assert(node->unfoldedSourcePosition.y < 0);
    const double m = IntersectWithXAxis(b, node->unfoldedSourcePosition);

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

    auto AddLeftChild = [&](bool partial)
    {
        const int hab = connectivity_->Succ(hca);
        const int habTwin = connectivity_->Twin(hab);
        if(habTwin != HalfedgeMesh::NullID)
        {
            const Vector2d inctLeft = IntersectLineSegment(
                node->unfoldedSourcePosition, Vector2d(node->intervalBegin, 0), { 0, 0 }, b, false);
            const double intervalBegin = Length(inctLeft - Vector2d(0, 0));

            double intervalEnd;
            if(partial)
            {
                const Vector2d inctRight = IntersectLineSegment(
                    node->unfoldedSourcePosition, Vector2d(node->intervalEnd, 0), { 0, 0 }, b, true);
                intervalEnd = Length(inctRight - Vector2d(0, 0));
            }
            else
            {
                intervalEnd = edgeToLength_[connectivity_->Edge(hab)];
            }

            auto newNode = context.NewNode();
            AddChildNode(node, newNode);

            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = false;
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
            const Vector2d inctRight = IntersectLineSegment(
                node->unfoldedSourcePosition, Vector2d(node->intervalEnd, 0), { ac, 0 }, b, false);
            const double intervalEnd = Length(inctRight - b);

            double intervalBegin;
            if(partial)
            {
                const Vector2d inctLeft = IntersectLineSegment(
                    node->unfoldedSourcePosition, Vector2d(node->intervalBegin, 0), { ac, 0 }, b, true);
                intervalBegin = Length(inctLeft - b);
            }
            else
            {
                intervalBegin = 0;
            }

            auto newNode = context.NewNode();
            AddChildNode(node, newNode);

            newNode->isInQueue    = true;
            newNode->depth        = node->depth + 1;
            newNode->isVertex     = false;
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
                assert(!halfedgeRecord.occupier->isVertex && halfedgeRecord.occupier->child);
                assert(halfedgeRecord.occupier->child->succ->succ == halfedgeRecord.occupier->child);
                if(halfedgeRecord.occupierM > m)
                {
                    // Delete the left subtree of the original occupier
                    DeleteSubTree(context, halfedgeRecord.occupier->child);
                }
                else
                {
                    // Delete the right subtree
                    DeleteSubTree(context, halfedgeRecord.occupier->child->succ);
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
        auto a = parent->child->prev;
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

RTRC_GEO_END
