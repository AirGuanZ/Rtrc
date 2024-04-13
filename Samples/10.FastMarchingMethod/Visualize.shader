rtrc_shader("FMM/Visualize")
{

	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float>, Texture)
		rtrc_uniform(float2, lower)
		rtrc_uniform(float2, upper)
		rtrc_uniform(float, scale)
	};

	rtrc_sampler(TextureSampler, filter = point, address = clamp)

	struct VSOutput
	{
		float4 position : SV_Position;
		float2 uv : UV;
	};

	VSOutput VSMain(uint vid : SV_VertexID)
	{
		float2 position, uv;
		if(vid == 0 || vid == 3)
		{
			position = Pass.lower;
			uv = float2(0, 1);
		}
		else if(vid == 1)
		{
			position = float2(Pass.lower.x, Pass.upper.y);
			uv = float2(0, 0);
		}
		else if(vid == 2 || vid == 4)
		{
			position = Pass.upper;
			uv = float2(1, 0);
		}
		else
		{
			position = float2(Pass.upper.x, Pass.lower.y);
			uv = float2(1, 1);
		}
		VSOutput output;
		output.position = float4(position, 0, 1);
		output.uv = uv;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const float value = Texture.SampleLevel(TextureSampler, input.uv, 0);
		return float4(Pass.scale * value.xxx, 1);
	}

} // rtrc_shader("FMM/Visualize")
