#include <Rtrc/Core/Timer.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Graphics/Shader/StandaloneCompiler.h>
#include <Rtrc/Graphics/Device/MeshLayout.h>
#include <Rtrc/ToolKit/ImGui/ImGuiInstance.h>

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
        rtrc_var(float4x4, Matrix);
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

    template<typename T>
    struct TypeToImGuiDataType;

    template<> struct TypeToImGuiDataType<int8_t>  { static constexpr ImGuiDataType Type = ImGuiDataType_S8;  };
    template<> struct TypeToImGuiDataType<int16_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S16; };
    template<> struct TypeToImGuiDataType<int32_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S32; };
    template<> struct TypeToImGuiDataType<int64_t> { static constexpr ImGuiDataType Type = ImGuiDataType_S64; };
    
    template<> struct TypeToImGuiDataType<uint8_t>  { static constexpr ImGuiDataType Type = ImGuiDataType_U8;  };
    template<> struct TypeToImGuiDataType<uint16_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U16; };
    template<> struct TypeToImGuiDataType<uint32_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U32; };
    template<> struct TypeToImGuiDataType<uint64_t> { static constexpr ImGuiDataType Type = ImGuiDataType_U64; };

    template<> struct TypeToImGuiDataType<float>  { static constexpr ImGuiDataType Type = ImGuiDataType_Float;  };
    template<> struct TypeToImGuiDataType<double> { static constexpr ImGuiDataType Type = ImGuiDataType_Double; };

    template<> struct TypeToImGuiDataType<char>
    {
        static constexpr ImGuiDataType Type = std::is_signed_v<char> ? ImGuiDataType_S8 : ImGuiDataType_U8;
    };

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

RGPass ImGuiRenderer::Render(
    const ImGuiDrawData *drawData,
    RGTexture                renderTarget,
    GraphRef             renderGraph)
{
    auto pass = renderGraph->CreatePass("Render ImGui");
    pass->Use(renderTarget, RG::ColorAttachment);
    pass->SetCallback([this, drawData, renderTarget]
    {
        RenderImmediately(drawData, renderTarget->GetRtvImm(), RGGetCommandBuffer(), false);
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

    commandBuffer.BeginRenderPass(RHI::ColorAttachment
    {
        .renderTargetView = rtv,
        .loadOp = RHI::AttachmentLoadOp::Load,
        .storeOp = RHI::AttachmentStoreOp::Store
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
            commandBuffer.SetScissors(RHI::Scissor{
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
    if(auto it = rtFormatToPipeline_.find(format); it != rtFormatToPipeline_.end())
    {
        return it->second;
    }
    auto pipeline = device_->CreateGraphicsPipeline(GraphicsPipelineDesc
    {
        .shader     = shader_,
        .meshLayout = RTRC_STATIC_MESH_LAYOUT(
            Buffer(
                Attribute("POSITION", 0, Float2),
                Attribute("UV",       0, Float2),
                Attribute("COLOR",    0, UChar4UNorm))),
        .rasterizerState = RTRC_STATIC_RASTERIZER_STATE(
        {
            .primitiveTopology = RHI::PrimitiveTopology::TriangleList,
            .fillMode          = RHI::FillMode::Fill,
            .cullMode          = RHI::CullMode::DontCull
        }),
        .blendState = RTRC_STATIC_BLEND_STATE(
        {
            .enableBlending         = true,
            .blendingSrcColorFactor = RHI::BlendFactor::SrcAlpha,
            .blendingDstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha,
            .blendingColorOp        = RHI::BlendOp::Add,
            .blendingSrcAlphaFactor = RHI::BlendFactor::One,
            .blendingDstAlphaFactor = RHI::BlendFactor::OneMinusSrcAlpha,
            .blendingAlphaOp        = RHI::BlendOp::Add,
        }),
        .attachmentState = RTRC_ATTACHMENT_STATE
        {
            .colorAttachmentFormats = { format }
        }
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
    data_->fontTexture = data_->device->CreateAndUploadTexture(RHI::TextureDesc
        {
            .format               = RHI::Format::R8G8B8A8_UNorm,
            .width                = static_cast<uint32_t>(width),
            .height               = static_cast<uint32_t>(height),
            .usage                = RHI::TextureUsage::ShaderResource
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

void ImGuiInstance::UseDarkTheme()
{
    IMGUI_CONTEXT;
    ImGui::StyleColorsDark();
}

void ImGuiInstance::UseLightTheme()
{
    IMGUI_CONTEXT;
    ImGui::StyleColorsLight();
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

void ImGuiInstance::SameLine()
{
    IMGUI_CONTEXT;
    ImGui::SameLine();
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

void ImGuiInstance::BeginDisabled()
{
    IMGUI_CONTEXT;
    ImGui::BeginDisabled();
}

void ImGuiInstance::EndDisabled()
{
    IMGUI_CONTEXT;
    ImGui::EndDisabled();
}

void ImGuiInstance::LabelUnformatted(const char *label, const std::string &text)
{
    IMGUI_CONTEXT;
    ImGui::LabelText(label, "%s", text.c_str());
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

template <ImGuiScalar T>
bool ImGuiInstance::Drag(const char* label, T* value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::DragScalar(label, dataType, value, vSpeed, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector2(const char* label, T* value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::DragScalarN(label, dataType, value, 2, vSpeed, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector3(const char* label, T* value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::DragScalarN(label, dataType, value, 3, vSpeed, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector4(const char* label, T *value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::DragScalarN(label, dataType, value, 4, vSpeed, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector2(const char* label, Vector2<T>& value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    return this->DragVector2(label, &value.x, vSpeed, vMin, vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector3(const char* label, Vector3<T>& value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    return this->DragVector3(label, &value.x, vSpeed, vMin, vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::DragVector4(const char* label, Vector4<T>& value, float vSpeed, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    return this->DragVector4(label, &value.x, vSpeed, vMin, vMax, format, flags);
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

template <ImGuiScalar T>
bool ImGuiInstance::Slider(const char* label, T* value, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::SliderScalar(label, dataType, value, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector2(const char* label, T* value, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::SliderScalarN(label, dataType, value, 2, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector3(const char *label, T *value, T vMin, T vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::SliderScalarN(label, dataType, value, 3, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector4(const char *label, T *value, T vMin, T vMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::SliderScalarN(label, dataType, value, 4, &vMin, &vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector2(const char* label, Vector2<T>& value, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    return ImGuiInstance::SliderVector2(label, &value.x, vMin, vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector3(const char* label, Vector3<T>& value, T vMin, T vMax, const char* format, ImGuiSliderFlags flags)
{
    return ImGuiInstance::SliderVector3(label, &value.x, vMin, vMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::SliderVector4(const char *label, Vector4<T> &value, T vMin, T vMax, const char *format, ImGuiSliderFlags flags)
{
    return ImGuiInstance::SliderVector4(label, &value.x, vMin, vMax, format, flags);
}

bool ImGuiInstance::SliderAngle(const char *label, float *vRad, float vDegreeMin, float vDegreeMax, const char *format, ImGuiSliderFlags flags)
{
    IMGUI_CONTEXT;
    return ImGui::SliderAngle(label, vRad, vDegreeMin, vDegreeMax, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::Input(const char* label, T* value, const char* format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalar(label, dataType, value, nullptr, nullptr, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::Input(const char* label, T* value, T step, T stepFast, const char* format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalar(label, dataType, value, &step, &stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector2(const char* label, T* value, const char* format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 2, nullptr, nullptr, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector2(const char* label, T* value, T step, T stepFast, const char* format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 2, &step, &stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector3(const char *label, T *value, const char *format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 3, nullptr, nullptr, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector3(const char *label, T *value, T step, T stepFast, const char *format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 3, &step, &stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector4(const char *label, T *value, const char *format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 4, nullptr, nullptr, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector4(const char *label, T *value, T step, T stepFast, const char *format, ImGuiInputFlags flags)
{
    IMGUI_CONTEXT;
    constexpr ImGuiDataType dataType = ImGuiDetail::TypeToImGuiDataType<T>::Type;
    return ImGui::InputScalarN(label, dataType, value, 4, &step, &stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector2(const char* label, Vector2<T>& value, const char* format, ImGuiInputFlags flags)
{
    return this->InputVector2(label, &value.x, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector2(const char* label, Vector2<T>& value, T step, T stepFast, const char* format, ImGuiInputFlags flags)
{
    return this->InputVector2(label, &value.x, step, stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector3(const char *label, Vector3<T> &value, const char *format, ImGuiInputFlags flags)
{
    return this->InputVector3(label, &value.x, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector3(const char *label, Vector3<T> &value, T step, T stepFast, const char *format, ImGuiInputFlags flags)
{
    return this->InputVector3(label, &value.x, step, stepFast, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector4(const char *label, Vector4<T> &value, const char *format, ImGuiInputFlags flags)
{
    return this->InputVector4(label, &value.x, format, flags);
}

template <ImGuiScalar T>
bool ImGuiInstance::InputVector4(const char *label, Vector4<T> &value, T step, T stepFast, const char *format, ImGuiInputFlags flags)
{
    return this->InputVector4(label, &value.x, step, stepFast, format, flags);
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

#define RTRC_INST_IMGUI_VECTOR_WIDGETS(TYPE)                                                                                                                             \
template bool ImGuiInstance::Drag<TYPE>(const char *label, TYPE *value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                 \
template bool ImGuiInstance::DragVector2<TYPE>(const char *label, TYPE *value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);          \
template bool ImGuiInstance::DragVector3<TYPE>(const char *label, TYPE *value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);          \
template bool ImGuiInstance::DragVector4<TYPE>(const char *label, TYPE *value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);          \
template bool ImGuiInstance::DragVector2<TYPE>(const char *label, Vector2<TYPE> &value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags); \
template bool ImGuiInstance::DragVector3<TYPE>(const char *label, Vector3<TYPE> &value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags); \
template bool ImGuiInstance::DragVector4<TYPE>(const char *label, Vector4<TYPE> &value, float vSpeed, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags); \
template bool ImGuiInstance::Slider(const char *label, TYPE *value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                                   \
template bool ImGuiInstance::SliderVector2(const char *label, TYPE *value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                            \
template bool ImGuiInstance::SliderVector3(const char *label, TYPE *value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                            \
template bool ImGuiInstance::SliderVector4(const char *label, TYPE *value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                            \
template bool ImGuiInstance::SliderVector2(const char *label, Vector2<TYPE> &value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                   \
template bool ImGuiInstance::SliderVector3(const char *label, Vector3<TYPE> &value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                   \
template bool ImGuiInstance::SliderVector4(const char *label, Vector4<TYPE> &value, TYPE vMin, TYPE vMax, const char *format, ImGuiSliderFlags flags);                   \
template bool ImGuiInstance::Input(const char *label, TYPE *value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                                \
template bool ImGuiInstance::InputVector2(const char *label, TYPE *value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                         \
template bool ImGuiInstance::InputVector3(const char *label, TYPE *value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                         \
template bool ImGuiInstance::InputVector4(const char *label, TYPE *value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                         \
template bool ImGuiInstance::InputVector2(const char *label, Vector2<TYPE> &value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                \
template bool ImGuiInstance::InputVector3(const char *label, Vector3<TYPE> &value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                \
template bool ImGuiInstance::InputVector4(const char *label, Vector4<TYPE> &value, TYPE step, TYPE stepFast, const char *format, ImGuiSliderFlags flags);                \
template bool ImGuiInstance::Input(const char *label, TYPE *value, const char *format, ImGuiSliderFlags flags);                                                          \
template bool ImGuiInstance::InputVector2(const char *label, TYPE *value, const char *format, ImGuiSliderFlags flags);                                                   \
template bool ImGuiInstance::InputVector3(const char *label, TYPE *value, const char *format, ImGuiSliderFlags flags);                                                   \
template bool ImGuiInstance::InputVector4(const char *label, TYPE *value, const char *format, ImGuiSliderFlags flags);                                                   \
template bool ImGuiInstance::InputVector2(const char *label, Vector2<TYPE> &value, const char *format, ImGuiSliderFlags flags);                                          \
template bool ImGuiInstance::InputVector3(const char *label, Vector3<TYPE> &value, const char *format, ImGuiSliderFlags flags);                                          \
template bool ImGuiInstance::InputVector4(const char *label, Vector4<TYPE> &value, const char *format, ImGuiSliderFlags flags);                                          \

RTRC_INST_IMGUI_VECTOR_WIDGETS(float)
RTRC_INST_IMGUI_VECTOR_WIDGETS(double)
RTRC_INST_IMGUI_VECTOR_WIDGETS(char)
RTRC_INST_IMGUI_VECTOR_WIDGETS(int8_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(int16_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(int32_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(int64_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(uint8_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(uint16_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(uint32_t)
RTRC_INST_IMGUI_VECTOR_WIDGETS(uint64_t)

#undef RTRC_INST_IMGUI_VECTOR_WIDGETS

RTRC_END

constexpr ImVec2::ImVec2(const Rtrc::Vector2<float> &v)
    : ImVec2(v.x, v.y)
{

}
