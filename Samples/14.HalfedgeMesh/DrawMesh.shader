rtrc_shader("Draw")
{
	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_uniform(float2, lower)
		rtrc_uniform(float2, upper)
	};

	struct VSInput
	{
		float2 position : POSITION;
		float3 color    : COLOR;
	};

	struct VSOutput
	{
		float4 position : SV_POSITION;
		float3 color    : COLOR;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.position = float4(lerp(Pass.lower, Pass.upper, 0.5 + 0.5 * input.position), 0, 1);
		output.color = input.color;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_TARGET
	{
		return float4(pow(input.color, 1 / 2.2), 1);
	}
}
