#pragma once

#include <oneapi/tbb/concurrent_queue.h>

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Texture.h>

RTRC_BEGIN

class CommandBuffer;
class Device;
class ImageDynamic;
class Uploader;

class UploadBatch : public Uncopyable
{
public:

    ~UploadBatch();

    UploadBatch(UploadBatch &&other) noexcept;
    UploadBatch &operator=(UploadBatch &&other) noexcept;

    void Swap(UploadBatch &other) noexcept;

    void Record(
        const RC<Buffer> &buffer, // Prev accesses to buffer should be externally synchronized
        const void       *data,
        bool              takeCopyOfData,
        size_t            offset = 0,
        size_t            size = 0);
    void Record(
        const RC<StatefulBuffer> &buffer,
        const void               *data,
        bool                      takeCopyOfData,
        size_t                    offset = 0,
        size_t                    size = 0);

    void Record(
        const RC<Texture> &texture, // Prev accesses to texture should be externally synchronized
        TexSubrsc          subrsc,
        const void        *data,
        size_t             dataRowBytes, // Specify 0 to use packed row bytes
        RHI::TextureLayout afterLayout,
        bool               takeCopyOfData);
    void Record(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const void                *data,
        size_t                     dataRowBytes,
        RHI::TextureLayout         afterLayout,
        bool                       takeCopyOfData);

    void Record(
        const RC<Texture>  &texture,
        TexSubrsc           subrsc,
        const ImageDynamic &image,
        RHI::TextureLayout  afterLayout);
    void Record(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const ImageDynamic        &image,
        RHI::TextureLayout         afterLayout);

    void Cancel();
    void SubmitAndWait();

private:

    friend class Uploader;

    struct BufferTask
    {
        RC<StatefulBuffer>         buffer;
        const void                *data;
        std::vector<unsigned char> ownedData;
        size_t                     offset;
        size_t                     size;
    };

    struct TextureTask
    {
        RC<StatefulTexture>        texture;
        TexSubrsc                  subrsc;
        const void                *data;
        size_t                     dataRowBytes;
        std::vector<unsigned char> ownedData;
        RHI::TextureLayout         afterLayout;
    };

    explicit UploadBatch(Ref<Uploader> uploader);

    std::vector<BufferTask>  bufferTasks_;
    std::vector<TextureTask> textureTasks_;
    Ref<Uploader>            uploader_;
};

class Uploader
{
public:

    Uploader(RHI::DeviceOPtr device, RHI::QueueRPtr queue);

    void Upload(
        const RC<Buffer> &buffer,
        const void       *data,
        size_t            offset = 0,
        size_t            size = 0);
    void Upload(
        const RC<Texture> &texture,
        TexSubrsc          subrsc,
        const void        *data,
        size_t             dataRowBytes,
        RHI::TextureLayout afterLayout);
    void Upload(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const void                *data,
        size_t                     dataRowBytes,
        RHI::TextureLayout         afterLayout);
    void Upload(
        const RC<Texture>  &texture,
        TexSubrsc           subrsc,
        const ImageDynamic &image,
        RHI::TextureLayout  afterLayout);
    void Upload(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const ImageDynamic        &image,
        RHI::TextureLayout         afterLayout);

    UploadBatch CreateBatch();

private:

    friend class UploadBatch;

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
