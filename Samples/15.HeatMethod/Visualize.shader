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
		float source : SOURCE;
		float distance : DISTANCE;
	};

	struct VSOutput
	{
		float4 position : SV_POSITION;
		float heat : HEAT;
		float3 gradient : GRADIENT;
		float divergence : DIVERGENCE;
		float source : SOURCE;
		float distance : DISTANCE;
	};

	float3 HeatColor(float v)
	{
		const float level = saturate(v) * (3.14159265 / 2);
		return float3(sin(level), sin(2 * level), cos(level));
	}

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.position = mul(Pass.worldToClip, float4(input.position, 1));
		output.heat = input.heat;
		output.gradient = input.gradient;
		output.divergence = input.divergence;
		output.source = input.source;
		output.distance = input.distance;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_TARGET
	{
		float3 color;
		if(Pass.mode == 0) // heat
		{
			color = pow(input.heat, 0.1);
		}
		else if(Pass.mode == 1) // gradient
		{
			const float3 direction = any(input.gradient != 0) ? normalize(input.gradient) : 0;
			color = 0.5 + 0.5 * direction;
		}
		else if(Pass.mode == 2) // divergence
		{
			color = input.divergence >= 0 ? float3(1, 0, 0) : float3(0, 0, 1);
			color *= abs(input.divergence);
		}
		else if(Pass.mode == 3) // source
		{
			const float t = saturate(input.source);
			color = HeatColor(t);
			//color = pow(color, 1 / 2.2);
		}
		else if(Pass.mode == 4) // uv
		{
			const int ui = int(input.distance / 0.01);
			const int vi = int(input.source * 100);
			color = (ui & 1) ^ (vi & 1);
		}
		else // distance
		{
			const float strip = input.distance / 0.05;
			color = int(strip) & 1 ? float3(0.2, 0.8, 0.8) : float3(0.8, 0.2, 0.2);
			color *= 1 - frac(strip);
			color = pow(color, 1 / 2.2);
		}
		return float4(color, 1);
	}
}
