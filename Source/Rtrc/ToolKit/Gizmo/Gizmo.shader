rtrc_shader("Rtrc/GizmoRenderer/RenderColoredPrimitives")
{

	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_uniform(float4x4, WorldToClip)
		rtrc_uniform(float, gamma)
	};

	struct VSInput
	{
		float3 position : POSITION;
		float3 color    : COLOR;
	};

	struct VS2FS
	{
		float4 position : SV_POSITION;
		float3 color	: COLOR;
	};

	VS2FS VSMain(VSInput input)
	{
		VS2FS output;
		output.position = mul(Pass.WorldToClip, float4(input.position, 1));
		output.color = input.color;
		return output;
	}

	float4 FSMain(VS2FS input) : SV_TARGET
	{
		return float4(pow(input.color, Pass.gamma), 1);
	}

} // rtrc_shader("Rtrc/GizmoRenderer/RenderColoredPrimitives")
