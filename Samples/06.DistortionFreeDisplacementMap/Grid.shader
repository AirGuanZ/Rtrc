rtrc_shader("DFDM/Render")
{

	rtrc_vertex(VSMain)
	rtrc_pixel(PSMain)

	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float>,  HeightMap)
		rtrc_define(Texture2D<float3>,  NormalMap)
		rtrc_define(Texture2D<float2>, CorrectionMap)
		rtrc_define(Texture2D<float3>, ColorMap)
		rtrc_uniform(float4x4, WorldToClip)
		rtrc_uniform(float, WorldSize)
		rtrc_uniform(uint, N)
		rtrc_uniform(uint, EnableCorrection)
		rtrc_uniform(uint, Checkboard)
	};
	
    rtrc_sampler(Sampler, filter = linear, address_u = clamp, address_v = clamp)
    rtrc_sampler(SamplerRepeat, filter = linear, address_u = repeat, address_v = repeat)

	struct VSInput
	{
		float2 position : Position;
	};

	struct VS2PS
	{
		float4 position : SV_Position;
		float2 uv : UV;
	};

	VS2PS VSMain(VSInput input)
	{
		const float2 uv = input.position / Pass.N;
		const float height = HeightMap.SampleLevel(Sampler, uv, 0);
		const float3 worldPosition = float3(uv.x * Pass.WorldSize, height, uv.y * Pass.WorldSize);

		VS2PS result;
		result.position = mul(Pass.WorldToClip, float4(worldPosition, 1));
		result.uv = uv;
		return result;
	}

	float3 CheckboardColor(float2 uv)
	{
		const int2 grid = int2(floor(uv * 128));
		return (grid.x % 2 == grid.y % 2) ? float3(0.2, 0.2, 0.2) : float3(0.8, 0.8, 0.8);
	}

	float4 PSMain(VS2PS input) : SV_Target
	{
		const float2 correction = CorrectionMap.SampleLevel(Sampler, input.uv, 0);
		const float2 correctedUV = input.uv + (Pass.EnableCorrection ? correction : float2(0, 0));

		float3 color;
		if(Pass.Checkboard)
			color = CheckboardColor(correctedUV);
		else
			color = ColorMap.SampleLevel(SamplerRepeat, 10 * correctedUV, 0);

		const float3 normal = NormalMap.SampleLevel(Sampler, input.uv, 0);
		return float4(pow(color * max(0.2 + normal.y * 0.8, 0), 1 / 2.2), 1);
	}

} // rtrc_shader("TerrainDemo/Render")
