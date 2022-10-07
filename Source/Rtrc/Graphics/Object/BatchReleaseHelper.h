#pragma once

#include <mutex>
#include <tbb/spin_mutex.h>

#include <Rtrc/Graphics/Object/HostSynchronizer.h>
#include <Rtrc/Utils/SlotVector.h>

RTRC_BEGIN

/*
    Usage:

        helper.SetPreNewBatchCallback(f);
        helper.SetPostNewBatchCallback(g);
        helper.SetReleaseCallback(h);

        During render loop:

            while(true):
                h(batch used in several frames ago);
                f();
                CreateNewBatch();
                g();
*/
template<typename T>
class BatchReleaseHelper : public Uncopyable
{
public:

    using DataList = std::list<T>;
    using DataListIterator = typename DataList::iterator;

    struct ReleaseBatch
    {
        DataList data;
        Box<tbb::spin_mutex> appendMutex;

        ReleaseBatch() = default;
        ReleaseBatch(ReleaseBatch &&) noexcept = default;
    };

    explicit BatchReleaseHelper(HostSynchronizer &sync)
        : hostSync_(sync)
    {
        auto NewBatch = [this]
        {
            if(preNewBatchCallback_)
            {
                preNewBatchCallback_();
            }
            currentBatchIndex_ = batches_.New();
            batches_.At(currentBatchIndex_).appendMutex = MakeBox<tbb::spin_mutex>();
            if(postNewBatchCallback_)
            {
                postNewBatchCallback_();
            }
            hostSync_.OnCurrentFrameComplete([this, batchIndex = currentBatchIndex_]
            {
                if(releaseCallback_)
                {
                    releaseCallback_(batchIndex, batches_.At(batchIndex).data);
                }
                batches_.Delete(batchIndex);
            });
        };
        NewBatch();
        newBatchEventKey_ = hostSync_.RegisterNewBatchEvent(NewBatch);
    }

    ~BatchReleaseHelper()
    {
        hostSync_.UnregisterNewBatchEvent(newBatchEventKey_);
    }

    void SetReleaseCallback(std::function<void(int, DataList &)> callback)
    {
        releaseCallback_ = std::move(callback);
    }

    void SetPreNewBatchCallback(std::function<void()> callback)
    {
        preNewBatchCallback_ = std::move(callback);
    }

    void SetPostNewBatchCallback(std::function<void()> callback)
    {
        postNewBatchCallback_ = std::move(callback);
    }

    int GetCurrentBatchIndex() const
    {
        return currentBatchIndex_;
    }

    DataListIterator AddToCurrentBatch(T data)
    {
        ReleaseBatch &batch = batches_.At(currentBatchIndex_);
        DataListIterator ret;
        {
            std::lock_guard lock(*batch.appendMutex);
            batch.data.push_back(std::move(data));
            ret = batch.data.end();
            --ret;
        }
        return ret;
    }

    HostSynchronizer &GetHostSynchronizer()
    {
        return hostSync_;
    }

private:

    HostSynchronizer &hostSync_;

    int currentBatchIndex_;
    SlotVector<ReleaseBatch> batches_;
    HostSynchronizer::RegisterKey newBatchEventKey_;

    std::function<void(int, DataList &)> releaseCallback_;
    std::function<void()> postNewBatchCallback_;
    std::function<void()> preNewBatchCallback_;
};

RTRC_END
