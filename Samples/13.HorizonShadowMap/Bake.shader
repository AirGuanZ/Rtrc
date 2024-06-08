rtrc_shader("Bake")
{
	rtrc_comp(CSMain)

	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float>, HeightMap)
		rtrc_define(Texture2D<float3>, NormalMap)
		rtrc_define(RWTexture2D<float>, MinVisibleAngleMap)
		rtrc_uniform(uint2, resolution)
		rtrc_uniform(float, horizontalScale)
		rtrc_uniform(float, sunPhi)
	};
		
	rtrc_sampler(TextureSampler, filter = linear, address = clamp)

	float ComputeAngle(int2 tid)
	{
		tid = clamp(tid, 0, Pass.resolution - 1);

		const float2 origin = tid + 0.5;
		const float2 direction = float2(cos(Pass.sunPhi), sin(Pass.sunPhi));
		const float2 step = 0.4 * direction;

		const float2 uvOrigin = origin / Pass.resolution;
		const float heightOrigin = HeightMap.SampleLevel(TextureSampler, uvOrigin, 0);
		
		const float3 normalOrigin = normalize(NormalMap.SampleLevel(TextureSampler, uvOrigin, 0));
		const float directionTangent = -dot(direction, normalOrigin.xz) / max(normalOrigin.y, 1e-5);

		float occlusionTangent = 0;
		for(uint i = 0; i < 1024; ++i)
		{
			const float2 position = origin + step * i;
			const float2 uv = position / Pass.resolution;
			if(any(uv != saturate(uv)))
				break;

			const float height = HeightMap.SampleLevel(TextureSampler, uv, 0);
			const float dist = 0.4 * i;
			const float tangent = (height - heightOrigin) / dist;
			occlusionTangent = max(occlusionTangent, tangent);
		}
		occlusionTangent /= Pass.horizontalScale;

		return atan(max(occlusionTangent, max(directionTangent, 0)));
	}

	[numthreads(8, 8, 1)]
	void CSMain(uint2 tid : SV_DispatchThreadID)
	{
		if(any(tid >= Pass.resolution))
			return;
		
		const int2 offsets[9] =
		{
			int2(-1, -1), int2(-1, +0), int2(-1, +1),
			int2(+0, -1), int2(+0, +0), int2(+0, +1),
			int2(+1, -1), int2(+1, +0), int2(+1, +1)
		};
		float sum = 0;
		for(int i = 0; i < 9; ++i)
		{
			sum += ComputeAngle(int2(tid) + offsets[i]);
		}

		const float minVisibleAngle = sum / 9; //ComputeAngle(tid);
		MinVisibleAngleMap[tid] = minVisibleAngle;
	}
}
