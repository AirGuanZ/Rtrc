#include <Core/Timer.h>
#include <Graphics/ImGui/Instance.h>
#include <Graphics/RenderGraph/Graph.h>
#include <Graphics/Shader/StandaloneCompiler.h>
#include <Graphics/Device/MeshLayout.h>

RTRC_BEGIN

namespace ImGuiDetail
{

    ImGuiKey TranslateKey(KeyCode key)
    {
        using enum KeyCode;
        switch(key)
        {
        case Space:         return ImGuiKey_Space;
        case Apostrophe:    return ImGuiKey_Apostrophe;
        case Comma:         return ImGuiKey_Comma;
        case Minus:         return ImGuiKey_Minus;
        case Period:        return ImGuiKey_Period;
        case Slash:         return ImGuiKey_Slash;
        case D0:            return ImGuiKey_0;
        case D1:            return ImGuiKey_1;
        case D2:            return ImGuiKey_2;
        case D3:            return ImGuiKey_3;
        case D4:            return ImGuiKey_4;
        case D5:            return ImGuiKey_5;
        case D6:            return ImGuiKey_6;
        case D7:            return ImGuiKey_7;
        case D8:            return ImGuiKey_8;
        case D9:            return ImGuiKey_9;
        case Semicolon:     return ImGuiKey_Semicolon;
        case Equal:         return ImGuiKey_Equal;
        case A:             return ImGuiKey_A;
        case B:             return ImGuiKey_B;
        case C:             return ImGuiKey_C;
        case D:             return ImGuiKey_D;
        case E:             return ImGuiKey_E;
        case F:             return ImGuiKey_F;
        case G:             return ImGuiKey_G;
        case H:             return ImGuiKey_H;
        case I:             return ImGuiKey_I;
        case J:             return ImGuiKey_J;
        case K:             return ImGuiKey_K;
        case L:             return ImGuiKey_L;
        case M:             return ImGuiKey_M;
        case N:             return ImGuiKey_N;
        case O:             return ImGuiKey_O;
        case P:             return ImGuiKey_P;
        case Q:             return ImGuiKey_Q;
        case R:             return ImGuiKey_R;
        case S:             return ImGuiKey_S;
        case T:             return ImGuiKey_T;
        case U:             return ImGuiKey_U;
        case V:             return ImGuiKey_V;
        case W:             return ImGuiKey_W;
        case X:             return ImGuiKey_X;
        case Y:             return ImGuiKey_Y;
        case Z:             return ImGuiKey_Z;
        case LeftBracket:   return ImGuiKey_LeftBracket;
        case Backslash:     return ImGuiKey_Backslash;
        case RightBracket:  return ImGuiKey_RightBracket;
        case GraveAccent:   return ImGuiKey_GraveAccent;
        case Escape:        return ImGuiKey_Escape;
        case Enter:         return ImGuiKey_Enter;
        case Tab:           return ImGuiKey_Tab;
        case Backspace:     return ImGuiKey_Backspace;
        case Insert:        return ImGuiKey_Insert;
        case Delete:        return ImGuiKey_Delete;
        case Right:         return ImGuiKey_RightArrow;
        case Left:          return ImGuiKey_LeftArrow;
        case Down:          return ImGuiKey_DownArrow;
        case Up:            return ImGuiKey_UpArrow;
        case Home:          return ImGuiKey_Home;
        case End:           return ImGuiKey_End;
        case F1:            return ImGuiKey_F1;
        case F2:            return ImGuiKey_F2;
        case F3:            return ImGuiKey_F3;
        case F4:            return ImGuiKey_F4;
        case F5:            return ImGuiKey_F5;
        case F6:            return ImGuiKey_F6;
        case F7:            return ImGuiKey_F7;
        case F8:            return ImGuiKey_F8;
        case F9:            return ImGuiKey_F9;
        case F10:           return ImGuiKey_F10;
        case F11:           return ImGuiKey_F11;
        case F12:           return ImGuiKey_F12;
        case NumPad0:       return ImGuiKey_Keypad0;
        case NumPad1:       return ImGuiKey_Keypad1;
        case NumPad2:       return ImGuiKey_Keypad2;
        case NumPad3:       return ImGuiKey_Keypad3;
        case NumPad4:       return ImGuiKey_Keypad4;
        case NumPad5:       return ImGuiKey_Keypad5;
        case NumPad6:       return ImGuiKey_Keypad6;
        case NumPad7:       return ImGuiKey_Keypad7;
        case NumPad8:       return ImGuiKey_Keypad8;
        case NumPad9:       return ImGuiKey_Keypad9;
        case NumPadDemical: return ImGuiKey_KeypadDecimal;
        case NumPadDiv:     return ImGuiKey_KeypadDivide;
        case NumPadMul:     return ImGuiKey_KeypadMultiply;
        case NumPadSub:     return ImGuiKey_KeypadSubtract;
        case NumPadAdd:     return ImGuiKey_KeypadAdd;
        case NumPadEnter:   return ImGuiKey_KeyPadEnter;
        case LeftShift:     return ImGuiKey_LeftShift;
        case LeftCtrl:      return ImGuiKey_LeftCtrl;
        case LeftAlt:       return ImGuiKey_LeftAlt;
        case RightShift:    return ImGuiKey_RightShift;
        case RightCtrl:     return ImGuiKey_RightCtrl;
        case RightAlt:      return ImGuiKey_RightAlt;
        default:            return ImGuiKey_None;
        }
    }

    const char *SHADER_SOURCE = R"___(
rtrc_vert(VSMain)
rtrc_frag(FSMain)

rtrc_group(CBuffer)
{
    rtrc_uniform(float4x4, Matrix)
};

rtrc_group(Pass)
{
    rtrc_define(Texture2D<float4>, Texture, FS)
};

rtrc_sampler(Sampler, filter = linear, address = repeat)

struct VsInput
{
    float2 position : POSITION;
    float2 uv       : UV;
    float4 color    : COLOR;
};

struct VsToFs
{
    float4 position : SV_POSITION;
    float2 uv       : UV;
    float4 color    : COLOR;
};

VsToFs VSMain(VsInput input)
{
    VsToFs output;
    output.position = mul(CBuffer.Matrix, float4(input.position, 0, 1));
    output.uv       = input.uv;
    output.color    = input.color;
    return output;
}

float4 FSMain(VsToFs input) : SV_TARGET
{
    return input.color * Texture.Sample(Sampler, input.uv);
}
)___";
    
    rtrc_struct(CBuffer)
    {
        float4x4 Matrix;
    };

    class ImGuiContextGuard : public Uncopyable
    {
        ImGuiContext *oldContext_;

    public:

        explicit ImGuiContextGuard(ImGuiContext *context)
        {
            oldContext_ = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(context);
        }

        ~ImGuiContextGuard()
        {
            ImGui::SetCurrentContext(oldContext_);
        }
    };

#define IMGUI_CONTEXT_EXPLICIT(CONTEXT) ImGuiDetail::ImGuiContextGuard guiContextGuard(CONTEXT)
#define IMGUI_CONTEXT IMGUI_CONTEXT_EXPLICIT(data_->context)

} // namespace ImGuiDetail

struct ImGuiInstance::Data
{
    Ref<Device> device = nullptr;
    Ref<Window> window = nullptr;
    ImGuiContext *context = nullptr;
    Timer timer;

    bool enableInput = true;
    Box<EventReceiver> eventReceiver;
    
    RC<BindingGroupLayout> passBindingGroupLayout;

    RC<Texture> fontTexture;
    RC<BindingGroup> fontTextureBindingGroup;

    Vector2i framebufferSize;
    
    void WindowFocus(bool focused);
    void CursorMove(float x, float y);
    void MouseButton(KeyCode button, bool down);
    void WheelScroll(float xoffset, float yoffset);
    void Key(KeyCode key, bool down);
    void Char(unsigned int ch);
};

void ImGuiDrawData::BuildFromImDrawData(const ImDrawData *src)
{
    totalVertexCount = src->TotalVtxCount;
    totalIndexCount = src->TotalIdxCount;
    drawLists.clear();
    drawLists.reserve(src->CmdListsCount);
    for(int i = 0; i < src->CmdListsCount; ++i)
    {
        ImDrawList &in = *src->CmdLists[i];
        DrawList &out = drawLists.emplace_back();
        std::ranges::copy(in.CmdBuffer, std::back_inserter(out.commandBuffer));
        std::ranges::copy(in.IdxBuffer, std::back_inserter(out.indexBuffer));
        std::ranges::copy(in.VtxBuffer, std::back_inserter(out.vertexBuffer));
        out.flags = in.Flags;
    }
    displayPos = { src->DisplayPos.x, src->DisplayPos.y };
    displaySize = { src->DisplaySize.x, src->DisplaySize.y };
    framebufferScale = { src->FramebufferScale.x, src->FramebufferScale.y };
}

ImGuiRenderer::ImGuiRenderer(Ref<Device> device)
    : device_(device)
{
    StandaloneShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device_);
    
    shader_ = shaderCompiler.Compile(ImGuiDetail::SHADER_SOURCE, {}, RTRC_DEBUG);
    cbufferBindingGroupLayout_ = shader_->GetBindingGroupLayoutByName("CBuffer");
    passBindingGroupLayout_ = shader_->GetBindingGroupLayoutByName("Pass");
}

RG::Pass *ImGuiRenderer::Render(
    const ImGuiDrawData *drawData,
    RG::TextureResource *renderTarget,
    RG::RenderGraph     *renderGraph)
{
    auto pass = renderGraph->CreatePass("Render ImGui");
    pass->Use(renderTarget, RG::ColorAttachment);
    pass->SetCallback([this, drawData, renderTarget]
    {
        RenderImmediately(drawData, renderTarget->GetRtvImm(), RG::GetCurrentCommandBuffer(), false);
    });
    return pass;
}

void ImGuiRenderer::RenderImmediately(
    const ImGuiDrawData *drawData,
    const TextureRtv    &rtv,
    CommandBuffer       &commandBuffer,
    bool                 renderPassMark)
{
    if(drawData->drawLists.empty())
    {
        return;
    }

    const Vector2u rtSize = rtv.GetTexture()->GetSize();
    if(rtSize.x != drawData->framebufferSize.x || rtSize.y != drawData->framebufferSize.y)
    {
        return;
    }

    // Upload vertex/index data

    RC<Buffer> vertexBuffer, indexBuffer;
    if(drawData->totalVertexCount > 0)
    {
        vertexBuffer = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = drawData->totalVertexCount * sizeof(ImDrawVert),
                .usage = RHI::BufferUsage::VertexBuffer,
                .hostAccessType = RHI::BufferHostAccessType::Upload
            });
        auto vertexData = static_cast<ImDrawVert *>(vertexBuffer->GetRHIObject()->Map(0, vertexBuffer->GetSize(), {}));
        size_t vertexOffset = 0;
        for(const ImGuiDrawData::DrawList &drawList : drawData->drawLists)
        {
            std::memcpy(vertexData + vertexOffset, drawList.vertexBuffer.data(), drawList.vertexBuffer.size() * sizeof(ImDrawVert));
            vertexOffset += drawList.vertexBuffer.size();
        }
        vertexBuffer->GetRHIObject()->Unmap(0, vertexBuffer->GetSize(), true);
    }
    if(drawData->totalIndexCount > 0)
    {
        indexBuffer = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = drawData->totalIndexCount * sizeof(ImDrawIdx),
                .usage = RHI::BufferUsage::IndexBuffer,
                .hostAccessType = RHI::BufferHostAccessType::Upload
            });
        auto indexData = static_cast<ImDrawIdx *>(indexBuffer->GetRHIObject()->Map(0, indexBuffer->GetSize(), {}));
        size_t indexOffset = 0;
        for(const ImGuiDrawData::DrawList &drawList : drawData->drawLists)
        {
            std::memcpy(indexData + indexOffset, drawList.indexBuffer.data(), drawList.indexBuffer.size() * sizeof(ImDrawIdx));
            indexOffset += drawList.indexBuffer.size();
        }
        indexBuffer->GetRHIObject()->Unmap(0, indexBuffer->GetSize(), true);
    }

    // Render

    if(renderPassMark)
    {
        commandBuffer.BeginDebugEvent("Render ImGui");
    }
    RTRC_SCOPE_EXIT
    {
        if(renderPassMark)
        {
            commandBuffer.EndDebugEvent();
        }
    };

    const RHI::Format format = rtv.GetRHIObject()->GetDesc().format;
    RC<GraphicsPipeline> pipeline = GetOrCreatePipeline(format);

    commandBuffer.BeginRenderPass(ColorAttachment
    {
        .renderTargetView = rtv,
        .loadOp = AttachmentLoadOp::Load,
        .storeOp = AttachmentStoreOp::Store
    });
    RTRC_SCOPE_EXIT{ commandBuffer.EndRenderPass(); };

    commandBuffer.BindGraphicsPipeline(pipeline);
    commandBuffer.SetViewports(rtv.GetTexture()->GetViewport());

    // Vertex & Index buffer

    if(vertexBuffer)
    {
        commandBuffer.SetVertexBuffer(0, vertexBuffer, sizeof(ImDrawVert));
    }

    if(indexBuffer)
    {
        static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t) || sizeof(ImDrawIdx) == sizeof(uint32_t));
        if constexpr(sizeof(ImDrawIdx) == sizeof(uint32_t))
        {
            commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt32);
        }
        else
        {
            commandBuffer.SetIndexBuffer(indexBuffer, RHI::IndexFormat::UInt16);
        }
    }
    
    // Constant buffer

    {
        const float L = drawData->displayPos.x;
        const float R = drawData->displayPos.x + drawData->displaySize.x;
        const float T = drawData->displayPos.y;
        const float B = drawData->displayPos.y + drawData->displaySize.y;

        ImGuiDetail::CBuffer cbufferData;
        cbufferData.Matrix = Matrix4x4f(
            2.0f / (R - L), 0.0f,           0.0f, (R + L) / (L - R),
            0.0f,           2.0f / (T - B), 0.0f, (T + B) / (B - T),
            0.0f,           0.0f,           0.5f, 0.5f,
            0.0f,           0.0f,           0.0f, 1.0f);
        auto cbuffer = device_->CreateConstantBuffer(cbufferData);
        auto cbufferGroup = cbufferBindingGroupLayout_->CreateBindingGroup();
        cbufferGroup->Set(0, cbuffer);
        commandBuffer.BindGraphicsGroup(0, cbufferGroup);
    }
    
    uint32_t vertexOffset = 0, indexOffset = 0;
    for(const ImGuiDrawData::DrawList &drawList : drawData->drawLists)
    {
        for(const ImDrawCmd &drawCmd : drawList.commandBuffer)
        {
            if(drawCmd.UserCallback)
            {
                //drawCmd.UserCallback(&drawList, &drawCmd);
                LogError("User callback is not supported");
                continue;
            }

            // Scissor

            ImVec2 clipMin(drawCmd.ClipRect.x - drawData->displayPos.x, drawCmd.ClipRect.y - drawData->displayPos.y);
            ImVec2 clipMax(drawCmd.ClipRect.z - drawData->displayPos.x, drawCmd.ClipRect.w - drawData->displayPos.y);
            if(clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
            {
                continue;
            }
            commandBuffer.SetScissors(Scissor{
                { static_cast<int>(clipMin.x), static_cast<int>(clipMin.y) },
                { static_cast<int>(clipMax.x - clipMin.x), static_cast<int>(clipMax.y - clipMin.y) }
            });

            // Texture

            if(drawCmd.GetTexID() == drawData->fontTexture.get())
            {
                commandBuffer.BindGraphicsGroup(1, drawData->fontTextureBindingGroup);
            }
            else
            {
                auto texture = static_cast<Texture *>(drawCmd.GetTexID());
                auto group = passBindingGroupLayout_->CreateBindingGroup();
                group->Set(0, texture->GetSrv());
                commandBuffer.BindGraphicsGroup(1, group);
            }

            // Draw

            commandBuffer.DrawIndexed(
                static_cast<int>(drawCmd.ElemCount), 1,
                static_cast<int>(indexOffset + drawCmd.IdxOffset),
                static_cast<int>(vertexOffset + drawCmd.VtxOffset), 0);
        }
        vertexOffset += drawList.vertexBuffer.size();
        indexOffset += drawList.indexBuffer.size();
    }
}

RC<GraphicsPipeline> ImGuiRenderer::GetOrCreatePipeline(RHI::Format format)
{
    static const MeshLayout *meshLayout = RTRC_MESH_LAYOUT(Buffer(
        Attribute("POSITION", 0, Float2),
        Attribute("UV",       0, Float2),
        Attribute("COLOR",    0, UChar4UNorm)));
    if(auto it = rtFormatToPipeline_.find(format); it != rtFormatToPipeline_.end())
    {
        return it->second;
    }
    auto pipeline = device_->CreateGraphicsPipeline(GraphicsPipeline::Desc
    {
        .shader                 = shader_,
        .meshLayout             = meshLayout,
        .primitiveTopology      = RHI::PrimitiveTopology::TriangleList,
        .fillMode               = RHI::FillMode::Fill,
        .cullMode               = RHI::CullMode::DontCull,
        .enableBlending         = true,
        .blendingSrcColorFactor = RHI::BlendFactor::SrcAlpha,
        .blendingDstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha,
        .blendingColorOp        = RHI::BlendOp::Add,
        .blendingSrcAlphaFactor = RHI::BlendFactor::One,
        .blendingDstAlphaFactor = RHI::BlendFactor::OneMinusSrcAlpha,
        .blendingAlphaOp        = RHI::BlendOp::Add,
        .colorAttachmentFormats = { format }
    });
    rtFormatToPipeline_.insert({ format, pipeline });
    return pipeline;
}

ImGuiInstance::ImGuiInstance(Ref<Device> device, Ref<Window> window)
{
    data_ = MakeBox<Data>();
    data_->device = device;
    data_->window = window;
    auto oldContext = ImGui::GetCurrentContext();
    data_->context = ImGui::CreateContext();
    ImGui::SetCurrentContext(oldContext);
    Do([&]
    {
        ImGui::GetIO().IniFilename = nullptr;
    });

    data_->enableInput = true;
    data_->eventReceiver = MakeBox<EventReceiver>(data_.get(), window);
    Do([&]
    {
        ImGui::GetIO().AddFocusEvent(data_->window->HasFocus());
    });
    
    BindingGroupLayout::Desc bindingGroupLayoutDesc;
    bindingGroupLayoutDesc.bindings.push_back(BindingGroupLayout::BindingDesc
    {
        .type = RHI::BindingType::Texture,
        .stages = RHI::ShaderStage::FS,
    });
    data_->passBindingGroupLayout = device->CreateBindingGroupLayout(bindingGroupLayoutDesc);

    RecreateFontTexture();
}

ImGuiInstance::~ImGuiInstance()
{
    if(data_)
    {
        data_->eventReceiver.reset();
        ImGui::DestroyContext(data_->context);
    }
}

ImGuiInstance::ImGuiInstance(ImGuiInstance &&other) noexcept
    : ImGuiInstance()
{
    Swap(other);
}

ImGuiInstance &ImGuiInstance::operator=(ImGuiInstance &&other) noexcept
{
    Swap(other);
    return *this;
}

void ImGuiInstance::Swap(ImGuiInstance &other) noexcept
{
    data_.swap(other.data_);
}

void ImGuiInstance::SetInputEnabled(bool enabled)
{
    IMGUI_CONTEXT;
    data_->enableInput = enabled;
    ImGui::GetIO().SetAppAcceptingEvents(enabled);
}

bool ImGuiInstance::IsInputEnabled() const
{
    return data_->enableInput;
}

void ImGuiInstance::Data::WindowFocus(bool focused)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    ImGui::GetIO().AddFocusEvent(focused);
}

void ImGuiInstance::ClearFocus()
{
    IMGUI_CONTEXT;
    ImGui::SetWindowFocus(nullptr);
}

void ImGuiInstance::Data::CursorMove(float x, float y)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    if(enableInput)
    {
        ImGui::GetIO().AddMousePosEvent(x, y);
    }
}

void ImGuiInstance::Data::MouseButton(KeyCode button, bool down)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    assert(button == KeyCode::MouseLeft || button == KeyCode::MouseMiddle || button == KeyCode::MouseRight);
    int translatedButton;
    if(button == KeyCode::MouseLeft)
    {
        translatedButton = 0;
    }
    else if(button == KeyCode::MouseMiddle)
    {
        translatedButton = 2;
    }
    else
    {
        translatedButton = 1;
    }
    ImGui::GetIO().AddMouseButtonEvent(translatedButton, down);
}

void ImGuiInstance::Data::WheelScroll(float xoffset, float yoffset)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    ImGui::GetIO().AddMouseWheelEvent(xoffset, yoffset);
}

void ImGuiInstance::Data::Key(KeyCode key, bool down)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    ImGui::GetIO().AddKeyEvent(ImGuiDetail::TranslateKey(key), down);
    if(key == KeyCode::LeftCtrl || key == KeyCode::RightCtrl)
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_ModCtrl, down);
    }
    else if(key == KeyCode::LeftShift || key == KeyCode::RightShift)
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_ModShift, down);
    }
    else if(key == KeyCode::LeftAlt || key == KeyCode::RightAlt)
    {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_ModAlt, down);
    }
}

void ImGuiInstance::Data::Char(unsigned ch)
{
    IMGUI_CONTEXT_EXPLICIT(context);
    ImGui::GetIO().AddInputCharacter(ch);
}

void ImGuiInstance::BeginFrame(const Vector2i &framebufferSize)
{
    IMGUI_CONTEXT;
    data_->timer.BeginFrame();
    ImGui::GetIO().DeltaTime = data_->timer.GetDeltaSecondsF();
    const Vector2i windowSize = data_->window->GetWindowSize();
    const Vector2i tFramebufferSize = framebufferSize.x > 0 && framebufferSize.y > 0 ? framebufferSize : data_->window->GetFramebufferSize();
    data_->framebufferSize = tFramebufferSize;
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(
        static_cast<float>(tFramebufferSize.x) / static_cast<float>(windowSize.x),
        static_cast<float>(tFramebufferSize.y) / static_cast<float>(windowSize.y));
    ImGui::NewFrame();
}

Box<ImGuiDrawData> ImGuiInstance::Render()
{
    IMGUI_CONTEXT;
    ImGui::Render();
    auto ret = MakeBox<ImGuiDrawData>();
    ret->BuildFromImDrawData(ImGui::GetDrawData());
    ret->framebufferSize = data_->framebufferSize;
    ret->fontTexture = data_->fontTexture;
    ret->fontTextureBindingGroup = data_->fontTextureBindingGroup;
    return ret;
}

void ImGuiInstance::RecreateFontTexture()
{
    IMGUI_CONTEXT;
    unsigned char *data;
    int width, height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&data, &width, &height);
    data_->fontTexture = data_->device->CreateAndUploadTexture2D(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::R8G8B8A8_UNorm,
            .width                = static_cast<uint32_t>(width),
            .height               = static_cast<uint32_t>(height),
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::ShaderResource,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent
        }, data, RHI::TextureLayout::ShaderTexture);
    data_->fontTextureBindingGroup = data_->passBindingGroupLayout->CreateBindingGroup();
    data_->fontTextureBindingGroup->Set(0, data_->fontTexture->GetSrv());
    ImGui::GetIO().Fonts->SetTexID(data_->fontTexture.get());
}

ImGuiContext *ImGuiInstance::GetImGuiContext()
{
    return data_->context;
}

ImGuiInstance::EventReceiver::EventReceiver(Data *data, Window *window)
    : data_(data)
{
    window->Attach(static_cast<Receiver<WindowFocusEvent>*>(this));
    window->GetInput().Attach(static_cast<Receiver<CursorMoveEvent> *>(this));
    window->GetInput().Attach(static_cast<Receiver<WheelScrollEvent> *>(this));
    window->GetInput().Attach(static_cast<Receiver<KeyDownEvent> *>(this));
    window->GetInput().Attach(static_cast<Receiver<KeyUpEvent> *>(this));
    window->GetInput().Attach(static_cast<Receiver<CharInputEvent> *>(this));
}

void ImGuiInstance::EventReceiver::Handle(const WindowFocusEvent &e)
{
    data_->WindowFocus(e.hasFocus);
}

void ImGuiInstance::EventReceiver::Handle(const CursorMoveEvent &e)
{
    data_->CursorMove(e.absoluteX, e.absoluteY);
}

void ImGuiInstance::EventReceiver::Handle(const WheelScrollEvent &e)
{
    data_->WheelScroll(0, static_cast<float>(e.relativeOffset));
}

void ImGuiInstance::EventReceiver::Handle(const KeyDownEvent &e)
{
    if(e.key == KeyCode::MouseLeft || e.key == KeyCode::MouseMiddle || e.key == KeyCode::MouseRight)
    {
        data_->MouseButton(e.key, true);
    }
    else
    {
        data_->Key(e.key, true);
    }
}

void ImGuiInstance::EventReceiver::Handle(const KeyUpEvent &e)
{
    if(e.key == KeyCode::MouseLeft || e.key == KeyCode::MouseMiddle || e.key == KeyCode::MouseRight)
    {
        data_->MouseButton(e.key, false);
    }
    else
    {
        data_->Key(e.key, false);
    }
}

void ImGuiInstance::EventReceiver::Handle(const CharInputEvent &e)
{
    data_->Char(e.charCode);
}

void ImGuiInstance::SetNextWindowPos(const Vector2f &position, ImGuiCond cond)
{
    IMGUI_CONTEXT;
    ImGui::SetNextWindowPos({ position.x, position.y }, cond);
}

void ImGuiInstance::SetNextWindowSize(const Vector2f &size, ImGuiCond cond)
{
    IMGUI_CONTEXT;
    ImGui::SetNextWindowSize({ size.x, size.y }, cond);
}

bool ImGuiInstance::Begin(const char *label, bool *open, ImGuiWindowFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::Begin(label, open, flags);
}

void ImGuiInstance::End()
{
    IMGUI_CONTEXT;
    return ImGui::End();
}

bool ImGuiInstance::Button(const char *label, const Vector2f &size)
{
    IMGUI_CONTEXT;
    return ImGui::Button(label, size);
}

bool ImGuiInstance::SmallButton(const char *label)
{
    IMGUI_CONTEXT;
    return ImGui::SmallButton(label);
}

bool ImGuiInstance::ArrowButton(const char *strId, ImGuiDir dir)
{
    IMGUI_CONTEXT;
    return ImGui::ArrowButton(strId, dir);
}

bool ImGuiInstance::CheckBox(const char *label, bool *value)
{
    IMGUI_CONTEXT;
    return ImGui::Checkbox(label, value);
}

bool ImGuiInstance::RadioButton(const char *label, bool active)
{
    IMGUI_CONTEXT;
    return ImGui::RadioButton(label, active);
}

void ImGuiInstance::ProgressBar(float fraction, const Vector2f &size, const char *overlay)
{
    IMGUI_CONTEXT;
    return ImGui::ProgressBar(fraction, size, overlay);
}

void ImGuiInstance::Bullet()
{
    IMGUI_CONTEXT;
    return ImGui::Bullet();
}

bool ImGuiInstance::BeginCombo(const char *label, const char *previewValue, ImGuiComboFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::BeginCombo(label, previewValue, flags);
}

void ImGuiInstance::EndCombo()
{
    IMGUI_CONTEXT;
    return ImGui::EndCombo();
}

void ImGuiInstance::Combo(const char *label, int *curr, Span<std::string> items, int popupMaxHeightInItems)
{
    const int itemCount = static_cast<int>(items.size());
    auto getItem = [&](int idx, const char **outItem)
    {
        if(idx < itemCount)
        {
            *outItem = items[idx].c_str();
            return true;
        }
        return false;
    };
    Combo(label, curr, getItem, itemCount, popupMaxHeightInItems);
}

void ImGuiInstance::Combo(const char *label, int *curr, Span<const char*> items, int popupMaxHeightInItems)
{
    IMGUI_CONTEXT;
    ImGui::Combo(label, curr, items.GetData(), static_cast<int>(items.size()), popupMaxHeightInItems);
}

void ImGuiInstance::Combo(const char *label, int *curr, std::initializer_list<const char*> items, int popupMaxHeightInItems)
{
    return Combo(label, curr, Span(items), popupMaxHeightInItems);
}

void ImGuiInstance::Combo(const char *label, int *curr, const char *itemsSeparatedByZeros, int popupMaxHeightInItems)
{
    IMGUI_CONTEXT;
    ImGui::Combo(label, curr, itemsSeparatedByZeros, popupMaxHeightInItems);
}

void ImGuiInstance::Combo(const char *label, int *curr, const ItemGetter &getItem, int itemCount, int popupMaxHeightInItems)
{
    IMGUI_CONTEXT;
    auto cGetter = +[](void *userData, int idx, const char **outItem)
    {
        auto &getter = *static_cast<const ItemGetter *>(userData);
        return getter(idx, outItem);
    };
    auto getterData = const_cast<void *>(static_cast<const void *>(&getItem));
    ImGui::Combo(label, curr, cGetter, getterData, itemCount, popupMaxHeightInItems);
}

bool ImGuiInstance::Selectable(const char *label, bool selected, ImGuiSelectableFlags flags, const Vector2f &size)
{
    IMGUI_CONTEXT;
    return ImGui::Selectable(label, selected, flags, size);
}

bool ImGuiInstance::DragFloat(const char *label, float *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragFloat(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat2(const char *label, float *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragFloat2(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat3(const char *label, float *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragFloat3(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat4(const char *label, float *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragFloat4(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat2(const char *label, Vector2f *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragFloat2(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat3(const char *label, Vector3f *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragFloat3(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragFloat4(const char *label, Vector4f *v, float vSpeed, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragFloat4(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt(const char *label, int *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragInt(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt2(const char *label, int *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragInt2(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt3(const char *label, int *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragInt3(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt4(const char *label, int *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragInt4(label, v, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt2(const char *label, Vector2i *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragInt2(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt3(const char *label, Vector3i *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragInt3(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragInt4(const char *label, Vector4i *v, float vSpeed, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return DragInt4(label, &v->x, vSpeed, vMin, vMax, format, flags);
}

bool ImGuiInstance::DragUInt(const char *label, unsigned *v, float vSpeed, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    int vbar = *v;
    const bool ret = DragInt(label, &vbar, vSpeed, 0, vMax, format, flags);
    *v = vbar;
    return ret;
}

bool ImGuiInstance::DragFloatRange2(const char *label, float *currMin, float *currMax, float vSpeed, float vMin, float vMax, const char *format, const char *formatMax, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragFloatRange2(label, currMin, currMax, vSpeed, vMin, vMax, format, formatMax, flags);
}

bool ImGuiInstance::DragIntRange2(const char *label, int *currMin, int *currMax, float vSpeed, int vMin, int vMax, const char *format, const char *formatMax, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::DragIntRange2(label, currMin, currMax, vSpeed, vMin, vMax, format, formatMax, flags);
}

bool ImGuiInstance::SliderFloat(const char *label, float *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderFloat(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat2(const char *label, float *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderFloat2(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat3(const char *label, float *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderFloat3(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat4(const char *label, float *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderFloat4(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat2(const char *label, Vector2f *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderFloat2(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat3(const char *label, Vector3f *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderFloat3(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderFloat4(const char *label, Vector4f *v, float vMin, float vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderFloat4(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt(const char *label, int *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderInt(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt2(const char *label, int *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderInt2(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt3(const char *label, int *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderInt3(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt4(const char *label, int *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderInt4(label, v, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt2(const char *label, Vector2i *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderInt2(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt3(const char *label, Vector3i *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderInt3(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderInt4(const char *label, Vector4i *v, int vMin, int vMax, const char *format, ImGuiSliderFlags flags)
{
    return SliderInt4(label, &v->x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderAngle(const char *label, float *vRad, float vDegreeMin, float vDegreeMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderAngle(label, vRad, vDegreeMin, vDegreeMax, format, flags);
}

bool ImGuiInstance::InputFloat(const char *label, float *v, float step, float stepFast, const char *format, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputFloat(label, v, step, stepFast, format, flags);
}

bool ImGuiInstance::InputDouble(const char *label, double *v, double step, double stepFast, const char *format, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputDouble(label, v, step, stepFast, format, flags);
}

bool ImGuiInstance::InputInt(const char *label, int *v, int step, int stepFast, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputInt(label, v, step, stepFast, flags);
}

bool ImGuiInstance::InputFloat2(const char *label, float *v, const char *format, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputFloat2(label, v, format, flags);
}

bool ImGuiInstance::InputFloat3(const char *label, float *v, const char *format, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputFloat3(label, v, format, flags);
}

bool ImGuiInstance::InputFloat4(const char *label, float *v, const char *format, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputFloat4(label, v, format, flags);
}

bool ImGuiInstance::InputFloat2(const char *label, Vector2f *v, const char *format, ImGuiInputTextFlags flags)
{
    return InputFloat2(label, &v->x, format, flags);
}

bool ImGuiInstance::InputFloat3(const char *label, Vector3f *v, const char *format, ImGuiInputTextFlags flags)
{
    return InputFloat3(label, &v->x, format, flags);
}

bool ImGuiInstance::InputFloat4(const char *label, Vector4f *v, const char *format, ImGuiInputTextFlags flags)
{
    return InputFloat4(label, &v->x, format, flags);
}

bool ImGuiInstance::InputInt2(const char *label, int *v, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputInt2(label, v, flags);
}

bool ImGuiInstance::InputInt3(const char *label, int *v, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputInt3(label, v, flags);
}

bool ImGuiInstance::InputInt4(const char *label, int *v, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputInt4(label, v, flags);
}

bool ImGuiInstance::InputInt2(const char *label, Vector2i *v, ImGuiInputTextFlags flags)
{
    return InputInt2(label, &v->x, flags);
}

bool ImGuiInstance::InputInt3(const char *label, Vector3i *v, ImGuiInputTextFlags flags)
{
    return InputInt3(label, &v->x, flags);
}

bool ImGuiInstance::InputInt4(const char *label, Vector4i *v, ImGuiInputTextFlags flags)
{
    return InputInt4(label, &v->x, flags);
}

bool ImGuiInstance::ColorEdit3(const char *label, float rgb[3], ImGuiColorEditFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::ColorEdit3(label, rgb, flags);
}

bool ImGuiInstance::ColorEdit4(const char *label, float rgba[4], ImGuiColorEditFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::ColorEdit4(label, rgba, flags);
}

bool ImGuiInstance::ColorPicker3(const char *label, float rgb[3], ImGuiColorEditFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::ColorPicker3(label, rgb, flags);
}

bool ImGuiInstance::ColorPicker4(const char *label, float rgba[4], ImGuiColorEditFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::ColorPicker4(label, rgba, flags);
}

void ImGuiInstance::TextUnformatted(std::string_view text)
{
    IMGUI_CONTEXT;
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
}

bool ImGuiInstance::InputText(const char *label, MutSpan<char> buffer, ImGuiInputTextFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::InputText(label, buffer.GetData(), buffer.GetSize(), flags);
}

bool ImGuiInstance::IsAnyItemActive() const
{
    IMGUI_CONTEXT;
    return ImGui::IsAnyItemActive();
}

RTRC_END

constexpr ImVec2::ImVec2(const Rtrc::Vector2<float> &v)
    : ImVec2(v.x, v.y)
{

}
