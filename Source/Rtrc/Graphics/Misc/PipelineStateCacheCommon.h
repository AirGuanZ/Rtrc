#pragma once

#include <map>

#include <tbb/concurrent_vector.h>

#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

template<typename Desc>
class PipelineStateCache;

template<typename Desc>
class PipelineStateCacheKey
{
public:

    bool IsValid() const { return value_ != InvalidValue; }

    const Desc &GetDesc() const;

    size_t Hash() const { return ::Rtrc::Hash(value_); }

    bool operator==(const PipelineStateCacheKey &) const = default;

private:

    friend class PipelineStateCache<Desc>;

    using Value = uint32_t;

    static constexpr Value InvalidValue = (std::numeric_limits<Value>::max)();

    Value value_ = InvalidValue;
};

template<typename Desc>
class PipelineStateCache : public Uncopyable
{
public:

    using Key = PipelineStateCacheKey<Desc>;

    static PipelineStateCache &GetInstance()
    {
        static PipelineStateCache ret;
        return ret;
    }

    Key Register(const Desc &desc)
    {
        std::lock_guard lock(mutex_);
        if(auto it = map_.find(desc); it != map_.end())
        {
            Key ret;
            ret.value_ = it->second;
            return ret;
        }
        const auto value = static_cast<typename Key::Value>(descs_.size());
        descs_.push_back(new Desc(desc));
        map_.insert({ desc, value });
        Key ret;
        ret.value_ = value;
        return ret;
    }

    const Desc &Query(const Key &key)
    {
        if(!key.IsValid())
        {
            static const Desc defaultDesc;
            return defaultDesc;
        }
        return *descs_[key.value_];
    }

private:

    PipelineStateCache() = default;

    ~PipelineStateCache()
    {
        for(auto p : descs_)
        {
            delete p;
        }
    }

    std::mutex                          mutex_;
    std::map<Desc, typename Key::Value> map_;
    tbb::concurrent_vector<Desc*>       descs_;
};

template<typename Desc>
const Desc &PipelineStateCacheKey<Desc>::GetDesc() const
{
    return PipelineStateCache<Desc>::GetInstance().Query(*this);
}

RTRC_END
