#pragma once

#include <set>

#include <Core/Math/AABB.h>
#include <Rtrc/Scene/Transform.h>
#include <Core/Uncopyable.h>

RTRC_BEGIN

rtrc_struct(PerObjectData)
{
    float4x4 localToWorld;
    float4x4 worldToLocal;
    float4x4 localToCamera;
    float4x4 localToClip;
};

class SceneObject : public Uncopyable
{
public:

    virtual ~SceneObject() = default;

    // Hierarchy
    
    void SetParent(SceneObject *parent);

    // Transform

    const Transform &GetTransform() const;
          Transform &GetMutableTransform();

    void UpdateWorldMatrixRecursively(bool forceUpdate);

    const Matrix4x4f &GetLocalToWorld() const;
    const Matrix4x4f &GetWorldToLocal() const;

    const AABB3f &GetLocalBound() const;
    const AABB3f &GetWorldBound() const;

protected:

    void SetLocalBound(const AABB3f &bound);

private:

    void UpdateWorldBound();

    SceneObject *parent_ = nullptr;
    std::set<SceneObject *> children_;

    Transform transform_;

    bool isWorldBoundDirty_ = false;
    AABB3f localBound_;
    AABB3f worldBound_;

    bool isTransformChanged_ = true;
    Matrix4x4f localToWorld_ = Matrix4x4f::Identity();
    Matrix4x4f worldToLocal_ = Matrix4x4f::Identity();
};

inline void SceneObject::SetParent(SceneObject *parent)
{
    assert(parent != this);
    if(parent_)
    {
        parent_->children_.erase(this);
    }
    parent_ = parent;
    if(parent_)
    {
        parent_->children_.insert(this);
    }
    isTransformChanged_ = true;
}

inline const Transform &SceneObject::GetTransform() const
{
    return transform_;
}

inline Transform &SceneObject::GetMutableTransform()
{
    isTransformChanged_ = true;
    return transform_;
}

inline void SceneObject::UpdateWorldMatrixRecursively(bool forceUpdate)
{
    assert(!parent_ || !parent_->isTransformChanged_);
    if(forceUpdate || isTransformChanged_)
    {
        isWorldBoundDirty_ = true;
        const Matrix4x4f &parentLocalToWorld = parent_ ? parent_->GetLocalToWorld() : Matrix4x4f::Identity();
        localToWorld_ = parentLocalToWorld * transform_.ToMatrix();
        const Matrix4x4f &parentWorldToLocal = parent_ ? parent_->GetWorldToLocal() : Matrix4x4f::Identity();
        worldToLocal_ = transform_.ToInverseMatrix() * parentWorldToLocal;
    }
    forceUpdate |= isTransformChanged_;
    isTransformChanged_ = false;
    for(SceneObject *child : children_)
    {
        child->UpdateWorldMatrixRecursively(forceUpdate);
    }
    if(forceUpdate || isWorldBoundDirty_)
    {
        UpdateWorldBound();
    }
}

inline const Matrix4x4f &SceneObject::GetLocalToWorld() const
{
    assert(!isTransformChanged_);
    return localToWorld_;
}

inline const Matrix4x4f &SceneObject::GetWorldToLocal() const
{
    assert(!isTransformChanged_);
    return worldToLocal_;
}

inline const AABB3f &SceneObject::GetLocalBound() const
{
    return localBound_;
}

inline const AABB3f &SceneObject::GetWorldBound() const
{
    assert(!isWorldBoundDirty_);
    return worldBound_;
}

inline void SceneObject::SetLocalBound(const AABB3f &bound)
{
    localBound_ = bound;
    isWorldBoundDirty_ = true;
}

inline void SceneObject::UpdateWorldBound()
{
    assert(!isTransformChanged_);
    worldBound_ = localToWorld_ * localBound_;
    isWorldBoundDirty_ = false;
}

RTRC_END
