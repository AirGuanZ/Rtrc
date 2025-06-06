rtrc_shader("Compute")
{
	rtrc_compute(CSMain)

	rtrc_group(MainGroup)
	{
		rtrc_define(Texture2D<float4>, InputTexture)
		rtrc_define(RWTexture2D<float4>, OutputTexture)
	    rtrc_uniform(float, scale)
	};
		
	[numthreads(8, 8, 1)]
	void CSMain(uint2 tid : SV_DispatchThreadID)
	{
		float4 input = InputTexture[tid];
		float4 output = MainGroup.scale * input;
		OutputTexture[tid] = output;
	}
}
