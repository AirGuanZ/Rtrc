struct ScaleSettingCStruct
{
    float Factor;
};

ConstantBuffer<ScaleSettingCStruct> ScaleSetting;
Texture2D<float4>                   InputTexture;
RWTexture2D<float4>                 OutputTexture;

group ScaleGroup
{
    ScaleSetting, InputTexture, OutputTexture
}

float4 CalculateResult(float4 input)
{
    return float4(ScaleSetting.Factor * input.rgb, input.a);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    float4 input = InputTexture[id.xy];
    OutputTexture[id.xy] = CalculateResult(input);
}
