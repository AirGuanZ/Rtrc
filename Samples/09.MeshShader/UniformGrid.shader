rtrc_shader("MeshShader/UniformGrid")
{

	rtrc_mesh(MSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_uniform(float2, clipLower)
		rtrc_uniform(float2, clipUpper)
	};

	struct MSOutput
	{
		float4 position : SV_Position;
	};

	[outputtopology("triangle")]
	[numthreads(8, 8, 1)]
	void MSMain(
		uint2 tid : SV_DispatchThreadID,
		out vertices MSOutput outputVertices[81],
		out indices uint3 outputTriangles[128])
	{
		SetMeshOutputCounts(81, 128);

		const float u0 = tid.x / 8.0;
		const float v0 = tid.y / 8.0;
		const float u1 = (tid.x + 1) / 8.0;
		const float v1 = (tid.y + 1) / 8.0;
		
		outputVertices[9 * tid.y + tid.x].position = float4(lerp(Pass.clipLower, Pass.clipUpper, float2(u0, v0)), 0, 1);
		if(tid.x == 7)
		{
			outputVertices[9 * tid.y + 8].position = float4(lerp(Pass.clipLower, Pass.clipUpper, float2(u1, v0)), 0, 1);
		}
		if(tid.y == 7)
		{
			outputVertices[9 * 8 + tid.x].position =  float4(lerp(Pass.clipLower, Pass.clipUpper, float2(u0, v1)), 0, 1);
		}
		if(tid.x == 7 && tid.y == 7)
		{
			outputVertices[9 * 8 + 8].position =  float4(lerp(Pass.clipLower, Pass.clipUpper, float2(u1, v1)), 0, 1);
		}

		outputTriangles[2 * (8 * tid.y + tid.x) + 0] = uint3(
			9 * (tid.y + 0) + (tid.x + 0),
			9 * (tid.y + 1) + (tid.x + 0),
			9 * (tid.y + 1) + (tid.x + 1));
		outputTriangles[2 * (8 * tid.y + tid.x) + 1] = uint3(
			9 * (tid.y + 0) + (tid.x + 0),
			9 * (tid.y + 1) + (tid.x + 1),
			9 * (tid.y + 0) + (tid.x + 1));
	}

	float4 FSMain() : SV_Target
	{
		return float4(1, 1, 1, 1);
	}

} // rtrc_shader("MeshShader/UniformGrid")
