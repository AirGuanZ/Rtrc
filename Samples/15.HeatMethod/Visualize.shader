rtrc_shader("VisualizeHeat")
{
	rtrc_vert(VSMain)
	rtrc_frag(FSMain)

	rtrc_group(Pass)
	{
		rtrc_uniform(float4x4, worldToClip)
		rtrc_uniform(uint, mode)
	};

	struct VSInput
	{
		float3 position : POSITION;
		float heat : HEAT;
		float3 gradient : GRADIENT;
		float divergence : DIVERGENCE;
		float distance : DISTANCE;
	};

	struct VSOutput
	{
		float4 position : SV_POSITION;
		float heat : HEAT;
		float3 gradient : GRADIENT;
		float divergence : DIVERGENCE;
		float distance : DISTANCE;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.position = mul(Pass.worldToClip, float4(input.position, 1));
		output.heat = input.heat;
		output.gradient = input.gradient;
		output.divergence = input.divergence;
		output.distance = input.distance;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_TARGET
	{
		float3 color;
		if(Pass.mode == 0)
		{
			color = pow(input.heat, 0.1);
		}
		else if(Pass.mode == 1)
		{
			const float3 direction = any(input.gradient != 0) ? normalize(input.gradient) : 0;
			color = 0.5 + 0.5 * direction;
		}
		else if(Pass.mode == 2)
		{
			color = input.divergence >= 0 ? float3(0.8, 0.1, 0.1) : float3(0.1, 0.1, 0.8);
			color *= abs(input.divergence);
			color = pow(color, 1 / 2.2);
		}
		else
		{
			const bool strip = int(input.distance / 0.02) & 1;
			color = pow(strip ? float3(0.2, 0.8, 0.8) : float3(0.8, 0.2, 0.2), 1 / 2.2);
		}
		return float4(color, 1);
	}
}
