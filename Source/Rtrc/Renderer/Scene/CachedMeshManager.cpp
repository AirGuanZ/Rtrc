#include <Rtrc/Renderer/Scene/CachedMeshManager.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_RENDERER_BEGIN

CachedMeshManager::CachedMeshManager(const Config &config, ObserverPtr<Device> device)
    : config_(config), device_(device), blasBuilder_(device)
{
    bindlessBufferManager_ = MakeBox<BindlessBufferManager>(device);
}

void CachedMeshManager::UpdateCachedMeshData(const RenderCommand_RenderStandaloneFrame &frame)
{
    struct MeshRecord
    {
        const Mesh::SharedRenderingData *mesh = nullptr;
        float buildBlasSortKey = -1; // -1 means no need to build blas
    };
    std::vector<MeshRecord> meshRecords;

    {
        const Vector3f eye = frame.camera.position;
        const SceneProxy &scene = *frame.scene;

        if(config_.rayTracing)
        {
            std::ranges::transform(
                scene.GetStaticMeshRenderObjects(), std::back_inserter(meshRecords),
                [&eye](const StaticMeshRenderProxy *staticMeshRenderer)
                {
                    const float blasSortKey = ComputeBuildBlasSortKey(eye, staticMeshRenderer);
                    return MeshRecord{ staticMeshRenderer->meshRenderingData.Get(), blasSortKey };
                });
        }
        else
        {
            std::ranges::transform(
                scene.GetStaticMeshRenderObjects(), std::back_inserter(meshRecords),
                [&eye](const StaticMeshRenderProxy *staticMeshRenderer)
                {
                    return MeshRecord{ staticMeshRenderer->meshRenderingData.Get(), -1 };
                });
        }

        std::ranges::sort(meshRecords, [](const MeshRecord &a, const MeshRecord &b)
        {
            return a.mesh->GetUniqueID() < b.mesh->GetUniqueID();
        });

        std::vector<MeshRecord> uniqueMeshes;
        uniqueMeshes.reserve(meshRecords.size());
        for(const MeshRecord &mesh : meshRecords)
        {
            if(uniqueMeshes.back().mesh == mesh.mesh)
            {
                uniqueMeshes.back().buildBlasSortKey = (std::max)(
                    uniqueMeshes.back().buildBlasSortKey, mesh.buildBlasSortKey);
            }
            else
            {
                uniqueMeshes.push_back(mesh);
            }
        }
    }

    auto it = cachedMeshes_.begin();
    for(const MeshRecord &meshRecord : meshRecords)
    {
        if(it == cachedMeshes_.end() || it->get()->meshId > meshRecord.mesh->GetUniqueID())
        {
            auto &data = *cachedMeshes_.emplace(it, MakeBox<CachedMesh>());
            data->buildBlasSortKey = meshRecord.buildBlasSortKey;
            data->mesh             = meshRecord.mesh;
            data->meshId           = meshRecord.mesh->GetUniqueID();
            if(data->buildBlasSortKey > 0)
            {
                assert(data->mesh->GetLayout()->GetVertexBufferLayouts()[0] == Mesh::BuiltinVertexBufferLayout::Default);
                auto &buffer = data->mesh->GetVertexBuffer(0);
                data->geometryBufferEntry = bindlessBufferManager_->Allocate();
                data->geometryBufferEntry.Set(buffer->GetStructuredSrv(sizeof(Mesh::BuiltinVertexStruct_Default)));
            }
            it = cachedMeshes_.end();
            continue;
        }

        if(it->get()->meshId == meshRecord.mesh->GetUniqueID())
        {
            it->get()->buildBlasSortKey = meshRecord.buildBlasSortKey;
            ++it;
            continue;
        }

        assert(it->get()->meshId < meshRecord.mesh->GetUniqueID());
        it = cachedMeshes_.erase(it);
    }

    linearCachedMeshes_.clear();
    std::ranges::transform(
        cachedMeshes_, std::back_inserter(linearCachedMeshes_),
        [](const Box<CachedMesh> &data) { return data.get(); });
}

CachedMeshManager::CachedMesh *CachedMeshManager::FindCachedMesh(UniqueId meshId)
{
    size_t beg = 0, end = linearCachedMeshes_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        CachedMesh *data = linearCachedMeshes_[mid];
        if(data->meshId == meshId)
        {
            return linearCachedMeshes_[mid];
        }
        if(data->meshId < meshId)
        {
            beg = mid;
        }
        else
        {
            end = mid;
        }
    }
    return nullptr;
}

const CachedMeshManager::CachedMesh *CachedMeshManager::FindCachedMesh(UniqueId meshId) const
{
    size_t beg = 0, end = linearCachedMeshes_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        CachedMesh *data = linearCachedMeshes_[mid];
        if(data->meshId == meshId)
        {
            return linearCachedMeshes_[mid];
        }
        if(data->meshId < meshId)
        {
            beg = mid;
        }
        else
        {
            end = mid;
        }
    }
    return nullptr;
}

CachedMeshManager::RenderGraphOutput CachedMeshManager::BuildBlasForMeshes(
    RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount)
{
    if(!config_.rayTracing)
    {
        return {};
    }

    std::vector<CachedMesh *> meshesNeedBlas;
    for(auto &meshData : cachedMeshes_)
    {
        if(meshData->buildBlasSortKey >= 0 && !meshData->blas)
        {
            meshesNeedBlas.push_back(meshData.get());
        }
    }
    std::ranges::sort(meshesNeedBlas, [](const CachedMesh *a, const CachedMesh *b)
    {
        return a->buildBlasSortKey > b->buildBlasSortKey;
    });

    std::vector<CachedMesh *> meshesToBuildBlas;
    {
        int buildCount = 0, buildPrimitiveCount = 0;
        auto it = meshesNeedBlas.begin();
        while(buildCount < maxBuildCount && buildPrimitiveCount < maxPrimitiveCount && it != meshesNeedBlas.end())
        {
            auto mesh = *it;
            const int newPrimitiveCount = buildPrimitiveCount + mesh->mesh->GetPrimitiveCount();
            if(newPrimitiveCount > buildPrimitiveCount)
            {
                if(meshesToBuildBlas.empty())
                {
                    meshesToBuildBlas.push_back(mesh);
                    break;
                }
                continue;
            }

            meshesToBuildBlas.push_back(mesh);
            ++buildCount;
            buildPrimitiveCount = newPrimitiveCount;
        }
    }

    auto pass = renderGraph.CreatePass("Build blas for meshes");
    if(!meshesToBuildBlas.empty())
    {
        for(auto mesh : meshesToBuildBlas)
        {
            assert(!mesh->blas);
            mesh->blas = device_->CreateBlas();
        }
        pass->SetCallback([meshes = std::move(meshesToBuildBlas), this](RG::PassContext &context)
        {
            for(auto mesh : meshes)
            {
                blasBuilder_.Build(
                    context.GetCommandBuffer(), *mesh->mesh,
                    RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace,
                    RHI::PipelineStage::All, mesh->blas);
            }
            blasBuilder_.Finalize(context.GetCommandBuffer());
        });
    }

    return { pass };
}

float CachedMeshManager::ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer)
{
    if(renderer->rayTracingFlags == StaticMeshRendererRayTracingFlagBit::None)
    {
        return -1;
    }

    const MeshLayout *meshLayout = renderer->meshRenderingData->GetLayout();
    const VertexBufferLayout *firstVertexBufferLayout = meshLayout->GetVertexBufferLayouts()[0];
    if(firstVertexBufferLayout != Mesh::BuiltinVertexBufferLayout::Default)
    {
        throw Exception(
            "Static mesh render object with ray tracing flag enabled must have a mesh "
            "using default builtin vertex buffer layout as the first vertex buffer layout");
    }

    const Vector3f nearestPointToEye = Max(renderer->worldBound.lower, Min(eye, renderer->worldBound.upper));
    const float distanceSquare = LengthSquare(nearestPointToEye - eye);
    const float radiusSquare = LengthSquare(renderer->worldBound.ComputeExtent());
    return radiusSquare / (std::max)(distanceSquare, 1e-5f);
}

RTRC_RENDERER_END
