#pragma once

#include <concepts>

#include <imgui.h>
#include <imfilebrowser.h>

#include <Rtrc/RHI/Window/WindowInput.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

namespace ImGuiDetail
{

    enum class FlagBit : uint32_t
    {
        None = 0,
        EnableDocking = 1u << 0
    };

    RTRC_DEFINE_ENUM_FLAGS(FlagBit);
    using Flags = EnumFlagsFlagBit;

} // namespace ImGuiDetail

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
        RGTexture            renderTarget,
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

    Ref<Device>                                 device_;
    RC<Shader>                                  shader_;
    RC<BindingGroupLayout>                      cbufferBindingGroupLayout_;
    RC<BindingGroupLayout>                      passBindingGroupLayout_;
    std::map<RHI::Format, RC<GraphicsPipeline>> rtFormatToPipeline_;
};

template<typename T>
concept ImGuiScalar = std::integral<T> || std::floating_point<T>;

class ImGuiInstance : public Uncopyable
{
public:

    using Flags = ImGuiDetail::Flags;

    ImGuiInstance() = default;
    ImGuiInstance(Ref<Device> device, Ref<Window> window);
    ~ImGuiInstance();

    ImGuiInstance(ImGuiInstance &&other) noexcept;
    ImGuiInstance &operator=(ImGuiInstance &&other) noexcept;
    void Swap(ImGuiInstance &other) noexcept;

    void AddFlags(Flags flags);
    void ClearFlags(Flags flags);

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

    ImGuiID DockSpaceOverMainViewport(ImGuiID dockspaceID);

    bool Begin(const char *label, bool *open = nullptr, ImGuiWindowFlags flags = 0);
    void End();

    void OpenPopup(const char *label);
    bool BeginPopup(const char *label);
    void EndPopup();

    void BeginDisabled();
    void EndDisabled();

    template<typename...Args>
    void Label(const char *label, StdFormatString<Args...> fmt, Args&&...args);
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
    bool Combo(const char *label, int *curr, Span<std::string> items, int popupMaxHeightInItems = -1);
    bool Combo(const char *label, int *curr, Span<const char*> items, int popupMaxHeightInItems = -1);
    bool Combo(const char *label, int *curr, std::initializer_list<const char*> items, int popupMaxHeightInItems = -1);
    bool Combo(const char *label, int *curr, const char *itemsSeparatedByZeros, int popupMaxHeightInItems = -1);
    bool Combo(const char *label, int *curr, const ItemGetter &getItem, int itemCount, int popupMaxHeightInItems = -1);

    bool Selectable(const char *label, bool selected, ImGuiSelectableFlags flags = 0, const Vector2f &size = {});

    // Set 'format' to nullptr to use the default format string.
    // Default format strings for following types are defined in ImGui::DataTypeGetInfo:
    //     { S8, S16, S32, S64, U8, U16, U32, U64, F32, F64 }
    template<ImGuiScalar T>
    bool Drag(const char *label, T *value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);

    template<ImGuiScalar T>
    bool DragVector2(const char *label, T *value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool DragVector3(const char *label, T *value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool DragVector4(const char *label, T *value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);

    template<ImGuiScalar T>
    bool DragVector2(const char *label, Vector2<T> &value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool DragVector3(const char *label, Vector3<T> &value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool DragVector4(const char *label, Vector4<T> &value, float vSpeed = 1.0f, T vMin = 0, T vMax = 0, const char *format = nullptr, ImGuiSliderFlags flags = 0);

    bool DragFloatRange2(const char *label, float *currMin, float *currMax, float vSpeed = 1.0f, float vMin = 0.0f, float vMax = 0.0f, const char *format = "%.3f", const char *formatMax = nullptr, ImGuiSliderFlags flags = 0);
    bool DragIntRange2  (const char *label, int   *currMin, int   *currMax, float vSpeed = 1.0f, int   vMin = 0.0f, int   vMax = 0.0f, const char *format = "%.3f", const char *formatMax = nullptr, ImGuiSliderFlags flags = 0);

    template<ImGuiScalar T>
    bool Slider(const char *label, T *value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector2(const char *label, T *value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector3(const char *label, T *value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector4(const char *label, T *value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector2(const char *label, Vector2<T> &value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector3(const char *label, Vector3<T> &value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);
    template<ImGuiScalar T>
    bool SliderVector4(const char *label, Vector4<T> &value, T vMin, T vMax, const char *format = nullptr, ImGuiSliderFlags flags = 0);

    bool SliderAngle(const char *label, float *vRad, float vDegreeMin = -360.0f, float vDegreeMax = +360.0f, const char *format = "%.0f deg", ImGuiSliderFlags flags = 0);

    template<ImGuiScalar T>
    bool Input(const char *label, T *value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool Input(const char *label, T *value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector2(const char *label, T *value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector2(const char *label, T *value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector3(const char *label, T *value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector3(const char *label, T *value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector4(const char *label, T *value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector4(const char *label, T *value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector2(const char *label, Vector2<T> &value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector2(const char *label, Vector2<T> &value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector3(const char *label, Vector3<T> &value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector3(const char *label, Vector3<T> &value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector4(const char *label, Vector4<T> &value, const char *format = nullptr, ImGuiInputFlags flags = 0);
    template<ImGuiScalar T>
    bool InputVector4(const char *label, Vector4<T> &value, T step, T stepFast, const char *format = nullptr, ImGuiInputFlags flags = 0);

    bool ColorEdit3  (const char *label, float rgb [3], ImGuiColorEditFlags flags = {});
    bool ColorEdit4  (const char *label, float rgba[4], ImGuiColorEditFlags flags = {});
    bool ColorPicker3(const char *label, float rgb [3], ImGuiColorEditFlags flags = {});
    bool ColorPicker4(const char *label, float rgba[4], ImGuiColorEditFlags flags = {});

    template<typename...Args>
    void Text(StdFormatString<Args...> fmt, Args&&...args);
    template<typename...Args>
    void TextWrapped(StdFormatString<Args...> fmt, Args&&...args);
    void TextUnformatted(std::string_view text);
    void TextUnformattedWrapped(const std::string &text);
    bool InputText(const char *label, MutSpan<char> buffer, ImGuiInputTextFlags flags = 0);

    bool BeginMainMenuBar();
    void EndMainMenuBar();
    bool BeginMenu(const char *name);
    void EndMenu();
    bool MenuItem(const char *name);

    float GetWindowWidth() const;
    float GetWindowHeight() const;

    bool IsAnyItemActive() const;
    bool IsMouseClicked(KeyCode keyCode) const;
    bool IsWindowHovered() const;

    template<typename F>
    decltype(auto) Do(F &&f); // Call f with thread local ImGui context bounded

    void BindContext();
    void UnbindContext();
    
    void RecreateFontTexture();

private:

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
void ImGuiInstance::Label(const char* label, StdFormatString<Args...> fmt, Args&&... args)
{
    this->LabelUnformatted(label, std::format(fmt, std::forward<Args>(args)...));
}

template<typename... Args>
void ImGuiInstance::Text(StdFormatString<Args...> fmt, Args &&... args)
{
    this->TextUnformatted(std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void ImGuiInstance::TextWrapped(StdFormatString<Args...> fmt, Args &&... args)
{
    this->TextUnformattedWrapped(std::format(fmt, std::forward<Args>(args)...));
}

template<typename F>
decltype(auto) ImGuiInstance::Do(F &&f)
{
    ImGuiContext *oldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(GetImGuiContext());
    RTRC_SCOPE_EXIT{ ImGui::SetCurrentContext(oldContext); };
    return std::invoke(std::forward<F>(f));
}

RTRC_END
