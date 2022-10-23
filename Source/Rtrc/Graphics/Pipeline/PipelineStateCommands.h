#pragma once

#include <Rtrc/Graphics/Object/Pipeline.h>

RTRC_BEGIN

class PipelineStateCommands
{
public:

    struct SetCullMode
    {
        RHI::CullMode mode;

        void Apply(GraphicsPipeline::Desc &desc) const { desc.cullMode = mode; }
    };

    struct SetFillMode
    {
        RHI::FillMode mode;

        void Apply(GraphicsPipeline::Desc &desc) const { desc.fillMode = mode; }
    };

    using Command = Variant<SetCullMode, SetFillMode>;

    void AddCommand(Command command)
    {
        commands_.push_back(std::move(command));
    }

    void Apply(GraphicsPipeline::Desc &desc) const
    {
        for(const Command &command : commands_)
        {
            command.Match([&](const auto &c) { c.Apply(desc); });
        }
    }

private:

    std::vector<Command> commands_;
};

RTRC_END
