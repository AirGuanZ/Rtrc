rtrc_shader("Render")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)
	
	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float4>, ColorTexture)
		rtrc_define(Texture2D<float4>, SpecularTexture)
		rtrc_uniform(float4x4, ObjectToClip)
		rtrc_uniform(float2, FramebufferSize)
	};
	
    rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)

	struct VSInput
	{
		float3 position : POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.clipPosition = mul(Pass.ObjectToClip, float4(input.position, 1));
		output.normal = input.normal;
		output.uv = input.uv;
		return output;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const float4 color = ColorTexture.SampleLevel(Sampler, input.uv, 0);
		const float4 specular = SpecularTexture.SampleLevel(Sampler, input.uv, 0);
		const float3 result = color.rgb * max(specular.a, 0.8);
		return float4(result, color.a);
	}
}

// Note: These shaders are intended solely for testing OIT results and are not optimized.
// Potential optimizations:
// * Fix the maximum layer count at compile time, to write more efficient code.
// * Use non-atomic buffer load to perform a binary search, finding a better insertion starting point.
//   This approach is conservative even without coherence and consistency.

rtrc_shader("RenderLayers")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(RWStructuredBuffer<uint64_t>, LayerBuffer)
		rtrc_define(RWStructuredBuffer<uint>, CounterBuffer)
		rtrc_uniform(uint2, FramebufferSize)
		rtrc_uniform(uint, LayerCount)

		rtrc_define(Texture2D<float4>, ColorTexture)
		rtrc_define(Texture2D<float4>, SpecularTexture)
		rtrc_uniform(float4x4, ObjectToClip)
		rtrc_uniform(float4x4, ObjectToView)
	};
	
    rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)
	
	struct VSInput
	{
		float3 position : POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
		float3 viewPosition : VIEW_POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};
	
	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.clipPosition = mul(Pass.ObjectToClip, float4(input.position, 1));
		output.viewPosition = mul(Pass.ObjectToView, float4(input.position, 1)).xyz;
		output.normal = input.normal;
		output.uv = input.uv;
		return output;
	}

	uint PackUNorm8(float value)
	{
		return uint(clamp(value * 255, 0.0, 255.0));
	}

	uint PackUNorm8x4(float4 value)
	{
		const uint r = PackUNorm8(value.r);
		const uint g = PackUNorm8(value.g);
		const uint b = PackUNorm8(value.b);
		const uint a = PackUNorm8(value.a);
		return (r << 24) | (g << 16) | (b << 8) | a;
	}

	uint FloatToUInt32_OrderPreserving(float f32_val)
	{
		const uint temp = asuint(f32_val);
		const uint i32val = temp ^ ((asint(temp) >> 31) | 0x80000000);
		return i32val;
	}

	void FSMain(VSOutput input)
	{
		const float4 color = ColorTexture.SampleLevel(Sampler, input.uv, 0);
		if(color.a < 0.01)
		{
			return;
		}
		const float4 specular = SpecularTexture.SampleLevel(Sampler, input.uv, 0);
		const float3 result = color.rgb * max(specular.a, 0.8);
		const uint packedResult = PackUNorm8x4(float4(result, color.a));

		const uint2 pixelCoordinate = uint2(input.clipPosition.xy);
		const uint pixelIndex = pixelCoordinate.y * Pass.FramebufferSize.x + pixelCoordinate.x;

		uint layerIndex;
		InterlockedAdd(CounterBuffer[pixelIndex], 1, layerIndex);

		const float dist = length(input.viewPosition);
		const uint encodedDist = FloatToUInt32_OrderPreserving(dist);
		uint64_t layerValue = (uint64_t(encodedDist) << 32) | uint64_t(packedResult);
		
		for(uint i = 0; i < Pass.LayerCount; ++i)
		{
			uint64_t originalValue;
			InterlockedMin(LayerBuffer[pixelIndex * Pass.LayerCount + i], layerValue, originalValue);
			if(originalValue == 0xffffffff)
			{
				return;
			}
			if(originalValue > layerValue)
			{
				// The layerValue must have already successfully replaced the original value at the i'th slot.
				// Now we need to find another slot to accommodate the original value.
				layerValue = originalValue;
			}
			else
			{
				// The insertion was unsuccessful. Keep trying in the next iteration.
			}
		}
	}
}

rtrc_shader("ResolveLayers")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)
	
	rtrc_group(Pass)
	{
		rtrc_define(StructuredBuffer<uint64_t>, LayerBuffer)
		rtrc_define(StructuredBuffer<uint>, CounterBuffer)
		rtrc_uniform(uint2, FramebufferSize)
		rtrc_uniform(uint, LayerCount)
	};

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
	};

	VSOutput VSMain(uint vertexID : SV_VertexID)
	{
		VSOutput output;
		output.clipPosition.x = vertexID == 2 ? 3.0 : -1.0;
		output.clipPosition.y = vertexID == 1 ? 3.0 : -1.0;
		output.clipPosition.z = 0.5;
		output.clipPosition.w = 1;
		return output;
	}
	
	float UInt32ToFloat_OrderPreserving(uint u32val)
	{
		const int temp1 = u32val ^ 0x80000000;
		const int temp2 = temp1 ^ (temp1 >> 31);
		const float f32val = asfloat(temp2);
		return f32val;
	}

	float DecodeUNorm8(uint v)
	{
		return v / 255.0;
	}

	float4 DecodeUNorm8x4(uint v)
	{
		const uint r = v >> 24;
		const uint g = (v >> 16) & 0xff;
		const uint b = (v >> 8) & 0xff;
		const uint a = v & 0xff;
		return float4(DecodeUNorm8(r), DecodeUNorm8(g), DecodeUNorm8(b), DecodeUNorm8(a));
	}

	void Swap(inout uint a, inout uint b)
	{
		const uint t = a;
		a = b;
		b = t;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const uint2 pixelCoordinate = uint2(input.clipPosition.xy);
		const uint pixelIndex = pixelCoordinate.y * Pass.FramebufferSize.x + pixelCoordinate.x;
		const uint layerCount = min(CounterBuffer[pixelIndex], Pass.LayerCount);
		if(layerCount == 0)
		{
			clip(-1);
		}

		float4 result = float4(0, 0, 0, 1);
		for(int layerIndex = int(layerCount) - 1; layerIndex >= 0; --layerIndex)
		{
			const uint loadIndex = Pass.LayerCount * pixelIndex + layerIndex;
			const uint64_t layerValue = LayerBuffer[loadIndex];
			const float4 value = DecodeUNorm8x4(uint(layerValue & 0xffffffff));
			result.rgb = value.rgb * value.a + result.rgb * (1 - value.a);
			result.a *= 1 - value.a;
		}

		return float4(result.rgb, result.a);
	}
}

rtrc_shader("RenderAlphaLayers")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(RWStructuredBuffer<uint>, LayerBuffer)
		rtrc_uniform(uint2, FramebufferSize)
		rtrc_uniform(uint, LayerCount)
		
		rtrc_define(Texture2D<float4>, ColorTexture)
		rtrc_uniform(float4x4, ObjectToClip)
		rtrc_uniform(float4x4, ObjectToView)
	};
	
    rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)
	
	struct VSInput
	{
		float3 position : POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
		precise float3 viewPosition : VIEW_POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.clipPosition = mul(Pass.ObjectToClip, float4(input.position, 1));
		output.viewPosition = mul(Pass.ObjectToView, float4(input.position, 1)).xyz;
		output.normal = input.normal;
		output.uv = input.uv;
		return output;
	}
	
	uint PackUNorm8(float value)
	{
		return uint(clamp(value * 255, 0.0, 255.0));
	}

	uint EncodeFloat24(float f)
	{
		const float v = saturate(f) * 16777215u;
		return uint(v) << 8;
	}

	void FSMain(VSOutput input)
	{
		const float4 color = ColorTexture.SampleLevel(Sampler, input.uv, 0);
		if(color.a < 0.01)
		{
			return;
		}
		const uint encodedAlpha = PackUNorm8(color.a);

		const float dist = length(input.viewPosition) / 15;
		const uint encodedDist = EncodeFloat24(dist);
		uint layerValue = (encodedDist & 0xffffff00) | encodedAlpha;

		const uint2 pixelCoordinate = uint2(input.clipPosition.xy);
		const uint pixelIndex = pixelCoordinate.y * Pass.FramebufferSize.x + pixelCoordinate.x;
		
		for(uint i = 0; i < Pass.LayerCount; ++i)
		{
			uint originalValue;
			InterlockedMin(LayerBuffer[pixelIndex * Pass.LayerCount + i], layerValue, originalValue);
			if(originalValue == 0xffffffff)
			{
				return;
			}
			if(originalValue > layerValue)
			{
				layerValue = originalValue;
			}
		}
	}
}

rtrc_shader("BlendBackground")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)
	
	rtrc_group(Pass)
	{
		rtrc_define(StructuredBuffer<uint>, LayerBuffer)
		rtrc_uniform(uint2, FramebufferSize)
		rtrc_uniform(uint, LayerCount)
	};
	
	struct VSOutput
	{
		float4 clipPosition : SV_Position;
	};
	
	VSOutput VSMain(uint vertexID : SV_VertexID)
	{
		VSOutput output;
		output.clipPosition.x = vertexID == 2 ? 3.0 : -1.0;
		output.clipPosition.y = vertexID == 1 ? 3.0 : -1.0;
		output.clipPosition.z = 0.5;
		output.clipPosition.w = 1;
		return output;
	}
	
	float DecodeUNorm8(uint v)
	{
		return v / 255.0;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const uint2 pixelCoordinate = uint2(input.clipPosition.xy);
		const uint pixelIndex = pixelCoordinate.y * Pass.FramebufferSize.x + pixelCoordinate.x;

		float alpha = 1;
		for(uint layerIndex = 0; layerIndex < Pass.LayerCount; ++layerIndex)
		{
			const uint layerValue = LayerBuffer[pixelIndex * Pass.LayerCount + layerIndex];
			if(layerValue == 0xffffffff)
			{
				break;
			}
			const float layerAlpha = DecodeUNorm8(layerValue & 0xff);
			alpha *= 1 - layerAlpha;
		}

		return float4(0, 0, 0, alpha);
	}
}

rtrc_shader("ResolveAlphaLayers")
{
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)
	
	rtrc_vertex(VSMain)
	rtrc_fragment(FSMain)

	rtrc_group(Pass)
	{
		rtrc_define(StructuredBuffer<uint>, LayerBuffer)
		rtrc_uniform(uint2, FramebufferSize)
		rtrc_uniform(uint, LayerCount)
		
		rtrc_define(Texture2D<float4>, ColorTexture)
		rtrc_define(Texture2D<float4>, SpecularTexture)
		rtrc_uniform(float4x4, ObjectToClip)
		rtrc_uniform(float4x4, ObjectToView)
	};
	
    rtrc_sampler(Sampler, filter = linear, address_u = repeat, address_v = clamp)
	
	struct VSInput
	{
		float3 position : POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	struct VSOutput
	{
		float4 clipPosition : SV_Position;
		precise float3 viewPosition : VIEW_POSITION;
		float3 normal : NORMAL;
		float2 uv : UV;
	};

	VSOutput VSMain(VSInput input)
	{
		VSOutput output;
		output.clipPosition = mul(Pass.ObjectToClip, float4(input.position, 1));
		output.viewPosition = mul(Pass.ObjectToView, float4(input.position, 1)).xyz;
		output.normal = input.normal;
		output.uv = input.uv;
		return output;
	}

	uint EncodeFloat24(float f)
	{
		const float v = saturate(f) * 16777215u;
		return uint(v) << 8;
	}
	
	uint PackUNorm8(float value)
	{
		return uint(clamp(value * 255, 0.0, 255.0));
	}
	
	float DecodeUNorm8(uint v)
	{
		return v / 255.0;
	}

	float4 FSMain(VSOutput input) : SV_Target
	{
		const float4 color = ColorTexture.SampleLevel(Sampler, input.uv, 0);
		if(color.a < 0.01)
		{
			clip(-1);
		}
		const uint encodedAlpha = PackUNorm8(color.a);

		const float dist = length(input.viewPosition) / 15;
		const uint encodedDist = EncodeFloat24(dist) & 0xffffff00;
		const uint currentLayerValue = encodedDist | encodedAlpha;
		
		const uint2 pixelCoordinate = uint2(input.clipPosition.xy);
		const uint pixelIndex = pixelCoordinate.y * Pass.FramebufferSize.x + pixelCoordinate.x;
		
		float accumulatedAlpha = 1; bool found = false;
		for(uint i = 0; i < Pass.LayerCount; ++i)
		{
			const uint layerValue = LayerBuffer[pixelIndex * Pass.LayerCount + i];
			const uint encodedLayerDist = layerValue & 0xffffff00;
			if(encodedLayerDist > encodedDist)
			{
				clip(-1);
			}

			const float alpha = DecodeUNorm8(layerValue & 0xff);
			if(layerValue == currentLayerValue)
			{
				accumulatedAlpha *= alpha;
				found = true;
				break;
			}
			else
			{
				accumulatedAlpha *= 1 - alpha;
			}
		}

		if(!found)
		{
			clip(-1);
		}
		
		const float4 specular = SpecularTexture.SampleLevel(Sampler, input.uv, 0);
		const float3 result = color.rgb * max(specular.a, 0.8);
		return float4(accumulatedAlpha * result, 1);
	}
}
