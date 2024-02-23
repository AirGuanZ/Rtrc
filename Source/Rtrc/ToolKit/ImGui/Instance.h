#pragma once

#include <imgui.h>

#include <Rtrc/RHI/Window/WindowInput.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

struct ImGuiDrawData
{
    struct DrawList
    {
        std::vector<ImDrawCmd>  commandBuffer;
        std::vector<ImDrawIdx>  indexBuffer;
        std::vector<ImDrawVert> vertexBuffer;
        ImDrawListFlags         flags;
    };
    
    void BuildFromImDrawData(const ImDrawData *src);
    
    int totalIndexCount = 0;
    int totalVertexCount = 0;
    std::vector<DrawList> drawLists;

    Vector2f displayPos;
    Vector2f displaySize;
    Vector2f framebufferScale;

    Vector2i         framebufferSize;
    RC<Texture>      fontTexture;
    RC<BindingGroup> fontTextureBindingGroup;
};

class ImGuiRenderer : public Uncopyable
{
public:

    explicit ImGuiRenderer(Ref<Device> device);
    
    RGPass Render(
        const ImGuiDrawData *drawData,
        RGTexture                renderTarget,
        GraphRef             renderGraph);

    // rt must be externally synchronized
    // assert(rtv.size == frameBufferSize)
    void RenderImmediately(
        const ImGuiDrawData *drawData,
        const TextureRtv    &rtv,
        CommandBuffer       &commandBuffer,
        bool                 renderPassMark = true);

private:

    RC<GraphicsPipeline> GetOrCreatePipeline(RHI::Format format);

    Ref<Device>                         device_;
    RC<Shader>                                  shader_;
    RC<BindingGroupLayout>                      cbufferBindingGroupLayout_;
    RC<BindingGroupLayout>                      passBindingGroupLayout_;
    std::map<RHI::Format, RC<GraphicsPipeline>> rtFormatToPipeline_;
};

class ImGuiInstance : public Uncopyable
{
public:

    ImGuiInstance() = default;
    ImGuiInstance(Ref<Device> device, Ref<Window> window);
    ~ImGuiInstance();

    ImGuiInstance(ImGuiInstance &&other) noexcept;
    ImGuiInstance &operator=(ImGuiInstance &&other) noexcept;
    void Swap(ImGuiInstance &other) noexcept;

    void SetInputEnabled(bool enabled);
    bool IsInputEnabled() const;
    void ClearFocus();

    // =========== Frame event ===========

    // Framebuffer size is queried from window by default
    void BeginFrame(const Vector2i &framebufferSize = {});

    Box<ImGuiDrawData> Render();
    
    // =========== GUI ===========

    void UseDarkTheme();
    void UseLightTheme();

    void SetNextWindowPos(const Vector2f &position, ImGuiCond cond = 0);
    void SetNextWindowSize(const Vector2f &size, ImGuiCond cond = 0);

    void SameLine();

    bool Begin(const char *label, bool *open = nullptr, ImGuiWindowFlags flags = 0);
    void End();

    template<typename...Args>
    void Label(const char *label, fmt::format_string<Args...> fmt, Args&&...args);
    void LabelUnformatted(const char *label, const std::string &text);

    bool Button(const char *label, const Vector2f &size = {});
    bool SmallButton(const char *label);
    bool ArrowButton(const char *strId, ImGuiDir dir);
    bool CheckBox(const char *label, bool *value);
    bool RadioButton(const char *label, bool active);
    void ProgressBar(float fraction, const Vector2f &size = { -FLT_MIN, 0 }, const char *overlay = nullptr);
    void Bullet();

    using ItemGetter = std::function<bool(int, const char **)>;
    bool BeginCombo(const char *label, const char *previewValue, ImGuiComboFlags flags = 0);
    void EndCombo();
    void Combo(const char *label, int *curr, Span<std::string> items, int popupMaxHeightInItems = -1);
    void Combo(const char *label, int *curr, Span<const char*> items, int popupMaxHeightInItems = -1);
    void Combo(const char *label, int *curr, std::initializer_list<const char*> items, int popupMaxHeightInItems = -1);
    void Combo(const char *label, int *curr, const char *itemsSeparatedByZeros, int popupMaxHeightInItems = -1);
    void Combo(const char *label, int *curr, const ItemGetter &getItem, int itemCount, int popupMaxHeightInItems = -1);

    bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags = 0, const Vector2f &size = {});

    bool DragFloat (const char *label, float    *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat2(const char *label, float    *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat3(const char *label, float    *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat4(const char *label, float    *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat2(const char *label, Vector2f *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat3(const char *label, Vector3f *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool DragFloat4(const char *label, Vector4f *v, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    
    bool DragInt (const char *label, int      *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt2(const char *label, int      *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt3(const char *label, int      *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt4(const char *label, int      *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt2(const char *label, Vector2i *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt3(const char *label, Vector3i *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool DragInt4(const char *label, Vector4i *v, float vSpeed = 1.0f, int vMin = 0, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);

    bool DragUInt (const char *label, unsigned *v, float vSpeed = 1.0f, int vMax = 0, const char *format = "%d", ImGuiSliderFlags flags = 0);

    bool DragFloatRange2(const char *label, float *currMin, float *currMax, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", const char *formatMax = nullptr, ImGuiSliderFlags flags = 0);
    bool DragIntRange2  (const char *label, int   *currMin, int   *currMax, float vSpeed = 1.0f, int   vMin = 0.0f, int   vMax = 0.0f, const char *format = "%.3f", const char *formatMax = nullptr, ImGuiSliderFlags flags = 0);

    bool SliderFloat (const char *label, float    *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat2(const char *label, float    *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat3(const char *label, float    *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat4(const char *label, float    *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat2(const char *label, Vector2f *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat3(const char *label, Vector3f *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);
    bool SliderFloat4(const char *label, Vector4f *v, float vMin, float vMax, const char *format = "%.3f", ImGuiSliderFlags flags = 0);

    bool SliderInt (const char *label, int      *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt2(const char *label, int      *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt3(const char *label, int      *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt4(const char *label, int      *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt2(const char *label, Vector2i *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt3(const char *label, Vector3i *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);
    bool SliderInt4(const char *label, Vector4i *v, int vMin, int vMax, const char *format = "%d", ImGuiSliderFlags flags = 0);

    bool SliderAngle(const char *label, float *vRad, float vDegreeMin = -360.0f, float vDegreeMax = +360.0f, const char *format = "%.0f deg", ImGuiSliderFlags flags = 0);

    bool InputFloat (const char *label, float  *v, float  step = 0.0f, float  stepFast = 0.0f, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputDouble(const char *label, double *v, double step = 0.0,  double stepFast = 0.0,  const char *format = "%.6f", ImGuiInputTextFlags flags = 0);
    bool InputInt   (const char *label, int    *v, int    step = 1,    int    stepFast = 100, ImGuiInputTextFlags flags = 0);

    bool InputFloat2(const char *label, float    *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputFloat3(const char *label, float    *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputFloat4(const char *label, float    *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputFloat2(const char *label, Vector2f *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputFloat3(const char *label, Vector3f *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    bool InputFloat4(const char *label, Vector4f *v, const char *format = "%.3f", ImGuiInputTextFlags flags = 0);
    
    bool InputInt2(const char *label, int      *v, ImGuiInputTextFlags flags = 0);
    bool InputInt3(const char *label, int      *v, ImGuiInputTextFlags flags = 0);
    bool InputInt4(const char *label, int      *v, ImGuiInputTextFlags flags = 0);
    bool InputInt2(const char *label, Vector2i *v, ImGuiInputTextFlags flags = 0);
    bool InputInt3(const char *label, Vector3i *v, ImGuiInputTextFlags flags = 0);
    bool InputInt4(const char *label, Vector4i *v, ImGuiInputTextFlags flags = 0);

    bool ColorEdit3  (const char *label, float rgb [3], ImGuiColorEditFlags flags = {});
    bool ColorEdit4  (const char *label, float rgba[4], ImGuiColorEditFlags flags = {});
    bool ColorPicker3(const char *label, float rgb [3], ImGuiColorEditFlags flags = {});
    bool ColorPicker4(const char *label, float rgba[4], ImGuiColorEditFlags flags = {});

    template<typename...Args>
    void Text(fmt::format_string<Args...> fmt, Args&&...args);
    void TextUnformatted(std::string_view text);
    bool InputText(const char *label, MutSpan<char> buffer, ImGuiInputTextFlags flags = 0);

    bool IsAnyItemActive() const;

private:

    template<typename F>
    void Do(F &&f); // Call f with thread local ImGui context bounded
    
    void RecreateFontTexture();

    ImGuiContext *GetImGuiContext();

    struct Data;

    class EventReceiver :
        public Receiver<WindowFocusEvent>,
        public Receiver<CursorMoveEvent>,
        public Receiver<WheelScrollEvent>,
        public Receiver<KeyDownEvent>,
        public Receiver<KeyUpEvent>,
        public Receiver<CharInputEvent>
    {
    public:

        EventReceiver(Data *data, Window *window);

        void Handle(const WindowFocusEvent &e) override;
        void Handle(const CursorMoveEvent  &e) override;
        void Handle(const WheelScrollEvent &e) override;
        void Handle(const KeyDownEvent     &e) override;
        void Handle(const KeyUpEvent       &e) override;
        void Handle(const CharInputEvent   &e) override;

    private:

        Data *data_;
    };

    Box<Data> data_;
};

template <typename... Args>
void ImGuiInstance::Label(const char* label, fmt::format_string<Args...> fmt, Args&&... args)
{
    this->LabelUnformatted(label, fmt::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void ImGuiInstance::Text(fmt::format_string<Args...> fmt, Args &&... args)
{
    this->TextUnformatted(fmt::format(fmt, std::forward<Args>(args)...));
}

template<typename F>
void ImGuiInstance::Do(F &&f)
{
    ImGuiContext *oldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(GetImGuiContext());
    RTRC_SCOPE_EXIT{ ImGui::SetCurrentContext(oldContext); };
    std::invoke(std::forward<F>(f));
}

RTRC_END
