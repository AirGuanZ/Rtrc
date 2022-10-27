#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>

RTRC_BEGIN

class StaticMesh : public Uncopyable
{
public:

    StaticMesh();
    StaticMesh(StaticMesh &&other) noexcept;
    StaticMesh &operator=(StaticMesh &&other) noexcept;

    void Swap(StaticMesh &other) noexcept;

    void SetMesh(RC<Mesh> mesh);
    void SetMaterial(RC<MaterialInstance> matInst);

private:

    RC<Mesh> mesh_;
    RC<MaterialInstance> matInst_;
};

RTRC_END
