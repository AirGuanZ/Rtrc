#pragma once

#include <Rtrc/Renderer/Scene/Transform.h>

RTRC_BEGIN

class SceneObject
{
public:

    // Hierarchy
    
    void SetParent(SceneObject *parent);

    // Transform

    const Transform &GetTransform() const;
    Transform &GetMutableTransform();

    void UpdateWorldMatrixRecursively(bool forceUpdate);

    const Matrix4x4f &GetLocalToWorld() const;
    const Matrix4x4f &GetWorldToLocal() const;

private:

    SceneObject *parent_ = nullptr;
    std::set<SceneObject *> children_;

    Transform transform_;

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
        const Matrix4x4f &parentLocalToWorld = parent_ ? parent_->GetLocalToWorld() : Matrix4x4f::Identity();
        localToWorld_ = parentLocalToWorld * transform_.ToMatrix();
        const Matrix4x4f &parentWorldToLocal = parent_ ? parent_->GetWorldToLocal() : Matrix4x4f::Identity();
        worldToLocal_ = transform_.ToInverseMatrix() * parentWorldToLocal;
    }
    forceUpdate |= isTransformChanged_;
    isTransformChanged_ = false;
    for(auto child : children_)
    {
        child->UpdateWorldMatrixRecursively(forceUpdate);
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

RTRC_END
