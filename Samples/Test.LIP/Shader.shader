rtrc_shader("Draw")
{

	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	#define SEGMENTS 1024
	#define LEFT_U 0.45 //0.25
	#define RIGHT_U 0.55 //0.75
	#define Y_SCALE 5 //1

	rtrc_group(Pass)
	{
		rtrc_define(Texture1D<float>, Texture)
	};
	
	rtrc_sampler(TextureSampler, filter = linear, address = clamp)

	float4 VSMain(uint vid : SV_VertexID) : SV_Position
	{
		const float step = 2.0 / SEGMENTS;
		const float x = -1.0 + (vid / 2) * step + (vid & 1 ? step : 0);
		const float u = lerp(LEFT_U, RIGHT_U, 0.5 * x + 0.5);
		const float y = Texture.SampleLevel(TextureSampler, u, 0);
		return float4(x, Y_SCALE * (y - 0.5), 0, 1);
	}

	float4 FSMain(float4 pos : SV_Position) : SV_Target
	{
		return float4(1, 1, 1, 1);
	}

} // rtrc_shader("Draw")
