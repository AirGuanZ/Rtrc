#pragma once

#include <imgui.h>

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Window/Input.h>

RTRC_BEGIN

class ImGuiInstance : public Uncopyable
{
public:

    ImGuiInstance() = default;
    ImGuiInstance(Device &device, Window &window);
    ~ImGuiInstance();

    ImGuiInstance(ImGuiInstance &&other) noexcept;
    ImGuiInstance &operator=(ImGuiInstance &&other) noexcept;
    void Swap(ImGuiInstance &other) noexcept;

    // Input event

    void WindowFocus(bool focused);
    void CursorMove(float x, float y);
    void MouseButton(KeyCode button, bool down);
    void WheelScroll(float xoffset, float yoffset);
    void Key(KeyCode key, bool down);
    void Char(unsigned int ch);

    // Frame event

    void BeginFrame();

    RG::Pass *AddToRenderGraph(RG::TextureResource *renderTarget, RG::RenderGraph *renderGraph);

    // rt must be externally synchronized
    // assert(rtv.size == frameBufferSize)
    void RenderImmediately(const TextureRTV &rtv, CommandBuffer &commandBuffer, bool renderPassMark = true);

    // GUI

    template<typename F>
    void WithRawContext(F &&f);

    bool Begin(const char *name, bool *open = nullptr, ImGuiWindowFlags flags = 0);
    void End();

    bool Button(const char *name, const Vector2f &size = {});

    template<typename...Args>
    void Text(fmt::format_string<Args...> fmt, Args&&...args);
    void TextUnformatted(std::string_view text);

private:

    RC<GraphicsPipeline> GetOrCreatePipeline(RHI::Format format);

    void RecreateFontTexture();

    ImGuiContext *GetImGuiContext();

    struct Data;

    Box<Data> data_;
};

template<typename F>
void ImGuiInstance::WithRawContext(F &&f)
{
    auto oldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(GetImGuiContext());
    RTRC_SCOPE_EXIT{ ImGui::SetCurrentContext(oldContext); };
    std::invoke(std::forward<F>(f));
}

template<typename... Args>
void ImGuiInstance::Text(fmt::format_string<Args...> fmt, Args &&... args)
{
    this->TextUnformatted(fmt::format(fmt, std::forward<Args>(args)...));
}

RTRC_END
