rtrc_shader("VisualizeMeshWithDistanceField")
{

	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(Texture3D<float>, DistanceTexture)
		rtrc_uniform(float4x4, worldToClip)
		rtrc_uniform(float, distanceScale)
		rtrc_uniform(uint, visualizeSegments)
		rtrc_uniform(float, lineWidthScale)
		rtrc_uniform(float3, positionToUVWScale)
		rtrc_uniform(float3, positionToUVWBias)
	};
	
	rtrc_sampler(TextureSampler, filter = linear, address = clamp)

	struct VSInput
	{
		float3 position : POSITION;
		float3 normal : NORMAL;
	};

	struct VSOutput
	{
		float4 position : SV_Position;
		float3 normal : NORMAL;
		float3 uvw : UWV;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.position = mul(Pass.worldToClip, float4(input.position, 1));
		output.normal = input.normal;
		output.uvw = Pass.positionToUVWScale * input.position + Pass.positionToUVWBias;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		float value = DistanceTexture.SampleLevel(TextureSampler, input.uvw, 0);
		value = frac(value * Pass.distanceScale * Pass.visualizeSegments);
		value = value < clamp(0.00125 * Pass.visualizeSegments * Pass.lineWidthScale, 0.01, 0.5) ? 0 : 1;
		return float4(value.xxx, 0);
	}

} // rtrc_shader("VisualizeMeshWithDistanceField")
