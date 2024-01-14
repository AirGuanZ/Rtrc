#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Texture.h>

RTRC_BEGIN

class CommandBuffer;
class Device;
class Downloader;
class ImageDynamic;

class DownloadBatch : public Uncopyable
{
public:

    ~DownloadBatch();

    DownloadBatch(DownloadBatch &&other) noexcept;
    DownloadBatch &operator=(DownloadBatch &&other) noexcept;

    void Swap(DownloadBatch &other) noexcept;

    void Record(const RC<StatefulBuffer> &buffer,
                void                     *data,
                size_t                    offset = 0,
                size_t                    size = 0);

    void Record(const RC<StatefulTexture> &texture,
                TextureSubresource         subrsc,
                void                      *outputData,
                size_t                     dataRowBytes = 0);

    void Cancel();
    void SubmitAndWait();

private:

    friend class Downloader;

    struct BufferTask
    {
        RC<StatefulBuffer> buffer;
        void              *outputData;
        size_t             offset;
        size_t             size;
    };

    struct TextureTask
    {
        RC<StatefulTexture> texture;
        TextureSubresource  subrsc;
        void               *outputData;
        size_t              dataRowBytes;
        size_t              stagingRowBytes;
        size_t              packedRowBytes;
    };

    explicit DownloadBatch(Ref<Downloader> downloader);

    std::vector<BufferTask>   bufferTasks_;
    std::vector<TextureTask>  textureTasks_;
    Ref<Downloader>           downloader_;
};

class Downloader : public Uncopyable
{
public:

    Downloader(RHI::DeviceOPtr device, RHI::QueueRPtr queue);
    
    void Download(const RC<StatefulBuffer> &buffer,
                  void                     *data,
                  size_t                    offset = 0,
                  size_t                    size = 0);

    void Download(const RC<StatefulTexture> &texture,
                  TextureSubresource         subrsc,
                  void                      *outputData,
                  size_t                     dataRowBytes = 0);

    DownloadBatch CreateBatch();

private:

    friend class DownloadBatch;

    struct Batch
    {
        RHI::CommandPoolUPtr commandPool;
        RHI::FenceUPtr       fence;
    };

    Batch GetBatch();
    void FreeBatch(Batch batch);

    RHI::DeviceOPtr              device_;
    RHI::QueueRPtr               queue_;
    tbb::concurrent_queue<Batch> batches_;
};

RTRC_END
