#include <Rtrc/Renderer/GPUScene/MeshRenderingCacheManager.h>

RTRC_RENDERER_BEGIN

MeshRenderingCacheManager::MeshRenderingCacheManager(Ref<Device> device)
    : device_(device), blasBuilder_(device)
{
    bindlessBuffers_ = MakeBox<BindlessBufferManager>(device, 64, GLOBAL_BINDLESS_GEOMETRY_BUFFER_MAX_SIZE);
}

void MeshRenderingCacheManager::Update(const Scene &scene, const Camera &camera)
{
    struct MeshRecord
    {
        const Mesh *mesh = nullptr;
        float buildBlasSortKey = -1; // -1 means no need to build blas
    };
    std::vector<MeshRecord> meshRecords;

    {
        const Vector3f eye = camera.GetPosition();
        scene.ForEachMeshRenderer([&](const MeshRenderer* renderer)
        {
            const float key = ComputeBlasSortKey(eye, renderer);
            meshRecords.push_back(MeshRecord{ renderer->GetMesh().get(), key });
        });

        std::ranges::sort(meshRecords, [](const MeshRecord &a, const MeshRecord &b)
        {
            return a.mesh->GetUniqueID() < b.mesh->GetUniqueID();
        });

        std::vector<MeshRecord> uniqueMeshes;
        uniqueMeshes.reserve(meshRecords.size());
        for(const MeshRecord &mesh : meshRecords)
        {
            if(!uniqueMeshes.empty() && uniqueMeshes.back().mesh == mesh.mesh)
            {
                uniqueMeshes.back().buildBlasSortKey = (std::max)(
                    uniqueMeshes.back().buildBlasSortKey, mesh.buildBlasSortKey);
            }
            else
            {
                uniqueMeshes.push_back(mesh);
            }
        }
        meshRecords = std::move(uniqueMeshes);
    }

    auto it = cachedMeshes_.begin();
    std::vector<Box<MeshRenderingCache>> newCachedMeshes;
    for(const MeshRecord &record : meshRecords)
    {
        // record is not in cached meshes
        if(it == cachedMeshes_.end() || it->get()->mesh->GetUniqueID() > record.mesh->GetUniqueID())
        {
            auto &cache = *newCachedMeshes.emplace_back(MakeBox<MeshRenderingCache>());
            
            cache.buildBlasSortKey = record.buildBlasSortKey;
            cache.mesh = record.mesh;

            if(cache.buildBlasSortKey > 0)
            {
                assert(cache.mesh->GetLayout()->GetVertexBufferLayouts()[0] ==
                       Mesh::BuiltinVertexBufferLayout::Standard);

                auto &vertexBuffer = cache.mesh->GetVertexBuffer(0);
                auto &indexBuffer = cache.mesh->GetIndexBuffer();
                cache.geometryBufferEntry = bindlessBuffers_->Allocate(indexBuffer ? 2 : 1);
                cache.geometryBufferEntry.Set(
                    0, vertexBuffer->GetStructuredSrv(sizeof(Mesh::BuiltinVertexStruct_Default)));

                if(indexBuffer)
                {
                    RHI::Format format;
                    if(cache.mesh->GetIndexFormat() == RHI::IndexFormat::UInt16)
                    {
                        format = RHI::Format::R16_UInt;
                    }
                    else
                    {
                        assert(cache.mesh->GetIndexFormat() == RHI::IndexFormat::UInt32);
                        format = RHI::Format::R32_UInt;
                    }
                    cache.geometryBufferEntry.Set(1, indexBuffer->GetTexelSrv(format));
                    cache.hasIndexBuffer = true;
                }
                else
                {
                    cache.hasIndexBuffer = false;
                }
            }

            continue;
        }

        // reuse exists record
        if(it->get()->mesh->GetUniqueID() == record.mesh->GetUniqueID())
        {
            it->get()->buildBlasSortKey = record.buildBlasSortKey;
            newCachedMeshes.emplace_back(std::move(*it));
            ++it;
            continue;
        }

        // skip outdated record
        assert(it->get()->mesh->GetUniqueID() < record.mesh->GetUniqueID());
        ++it;
    }

    cachedMeshes_ = std::move(newCachedMeshes);
}

RG::Pass *MeshRenderingCacheManager::Build(RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount)
{
    std::vector<MeshRenderingCache *> meshesNeedBlas;
    for(auto &record : cachedMeshes_)
    {
        if(record->buildBlasSortKey >= 0 && !record->blas)
        {
            meshesNeedBlas.push_back(record.get());
        }
    }
    std::ranges::sort(meshesNeedBlas, [](const MeshRenderingCache *a, const MeshRenderingCache *b)
    {
        return a->buildBlasSortKey > b->buildBlasSortKey;
    });

    std::vector<MeshRenderingCache *> meshesToBuildBlas;
    {
        int buildCount = 0, buildPrimitiveCount = 0;
        auto it = meshesNeedBlas.begin();
        while(buildCount < maxBuildCount && buildPrimitiveCount < maxPrimitiveCount && it != meshesNeedBlas.end())
        {
            auto mesh = *it++;
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
    
    auto buildBlasPass = renderGraph.CreatePass("Build blas for meshes");
    if(!meshesToBuildBlas.empty())
    {
        std::vector<BlasBuilder::BuildInfo> blasBuildInfo;
        for(auto mesh : meshesToBuildBlas)
        {
            assert(!mesh->blas);
            mesh->blas = device_->CreateBlas();
            blasBuildInfo.push_back(blasBuilder_.Prepare(
                mesh->mesh, 
                RHI::RayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                mesh->blas));
            mesh->blas->SetBuffer(device_->CreateBuffer(RHI::BufferDesc
            {
                .size           = blasBuildInfo.back().prebuildInfo->GetAccelerationStructureBufferSize(),
                .usage          = RHI::BufferUsage::AccelerationStructure,
                .hostAccessType = RHI::BufferHostAccessType::None
            }));
        }
        buildBlasPass->SetCallback([builds = std::move(blasBuildInfo), this]
        {
            auto &commandBuffer = RG::GetCurrentCommandBuffer();
            for(const BlasBuilder::BuildInfo &build : builds)
            {
                blasBuilder_.Build(commandBuffer, build);
            }
            blasBuilder_.Finalize(commandBuffer);
        });
    }

    return buildBlasPass;
}

RC<BindingGroup> MeshRenderingCacheManager::GetBindlessBufferBindingGroup() const
{
    return bindlessBuffers_->GetBindingGroup();
}

const MeshRenderingCache *MeshRenderingCacheManager::FindMeshRenderingCache(const Mesh *mesh) const
{
    size_t beg = 0, end = cachedMeshes_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        MeshRenderingCache *data = cachedMeshes_[mid].get();
        if(data->mesh->GetUniqueID() == mesh->GetUniqueID())
        {
            return cachedMeshes_[mid].get();
        }
        if(data->mesh->GetUniqueID() < mesh->GetUniqueID())
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

float MeshRenderingCacheManager::ComputeBlasSortKey(const Vector3f &eye, const MeshRenderer *meshRenderer)
{
    if(!meshRenderer->GetFlags().Contains(MeshRendererFlagBit::OpaqueTlas))
    {
        return -1;
    }

    const MeshLayout *meshLayout = meshRenderer->GetMesh()->GetLayout();
    const VertexBufferLayout *firstVertexBufferLayout = meshLayout->GetVertexBufferLayouts()[0];
    if(firstVertexBufferLayout != Mesh::BuiltinVertexBufferLayout::Standard)
    {
        throw Exception(
            "Static mesh render object with ray tracing flag enabled must have a mesh "
            "using default builtin vertex buffer layout as the first vertex buffer layout");
    }

    const AABB3f &worldBound = meshRenderer->GetWorldBound();
    const Vector3f nearestPointToEye = Max(worldBound.lower, Min(eye, worldBound.upper));
    const float distanceSquare = LengthSquare(nearestPointToEye - eye);
    const float radiusSquare = LengthSquare(worldBound.ComputeExtent());
    return radiusSquare / (std::max)(distanceSquare, 1e-5f);
}

RTRC_RENDERER_END
