rtrc_shader("Render")
{
	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float>,  HeightMap)
		rtrc_define(Texture2D<float3>, NormalMap)
		rtrc_define(Texture2D<float>,  HorizonMap)
		rtrc_uniform(float4x4, worldToClip)
		rtrc_uniform(float2, horizontalLower)
		rtrc_uniform(float2, horizontalUpper)
		rtrc_uniform(uint, N)
		rtrc_uniform(float, sunTheta)
		rtrc_uniform(float, softness)
	};
	
	rtrc_sampler(TextureSampler, filter = linear, address = clamp)

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
		float2 uv			: UV;
	};

	VSOutput VSMain(uint vid : SV_VertexID)
	{
		const uint N = Pass.N;
		const uint vy = vid / (N + 1);
		const uint vx = vid - vy * (N + 1);
		const float2 uv = float2(vx, vy) / (N + 1);

		const float height = HeightMap.SampleLevel(TextureSampler, uv, 0);
		const float2 horizontalPosition = lerp(Pass.horizontalLower, Pass.horizontalUpper, uv);
		const float3 worldPosition = float3(horizontalPosition.x, height, horizontalPosition.y);

		VSOutput output;
		output.clipPosition = mul(Pass.worldToClip, float4(worldPosition, 1));
		output.uv = uv;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const float3 lightDirection = -float3(cos(Pass.sunTheta), sin(Pass.sunTheta), cos(Pass.sunTheta));
		const float3 normal = normalize(NormalMap.SampleLevel(TextureSampler, input.uv, 0));
		const float normalFactor = max(dot(normal, -lightDirection), 0);

		const float visibleTheta = HorizonMap.SampleLevel(TextureSampler, input.uv, 0);
		const float minVisibleTheta = visibleTheta - 0.5 * Pass.softness;
		const float shadowFactor = saturate((Pass.sunTheta - minVisibleTheta) / (1.5 * Pass.softness));

		const float lightFactor = saturate(normalFactor + 0.05) * saturate(shadowFactor + 0.05);
		return float4(pow(lightFactor.rrr, 1 / 2.), 1);
	}
}
