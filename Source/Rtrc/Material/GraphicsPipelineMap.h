#pragma once

#include <Rtrc/Material/Material.h>

RTRC_BEGIN

template<typename...OverridablePipelineStates>
class GraphicsPipelineMap
{
public:

    struct Key
    {
        std::weak_ptr<Shader> shader;
        std::tuple<OverridablePipelineStates...> overridenPipelineStates;

        bool operator<(const Key &rhs) const
        {
            struct WeakPtrCompWrapper
            {
                const std::weak_ptr<Shader> &ptr;
                bool operator<(const WeakPtrCompWrapper &rhs) const
                {
                    return std::owner_less<std::weak_ptr<Shader>>{}(ptr, rhs.ptr);
                }
            };
            return std::tuple_cat(std::make_tuple(WeakPtrCompWrapper{ shader }), overridenPipelineStates) <
                   std::tuple_cat(std::make_tuple(WeakPtrCompWrapper{ rhs.shader }), rhs.overridenPipelineStates);
        }
    };

private:

    std::map<Key, RHI::GraphicsPipelinePtr> pipelines_;
};

RTRC_END
