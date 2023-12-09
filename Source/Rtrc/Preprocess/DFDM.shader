rtrc_shader("Rtrc/Builtin/DFDM")
{

	rtrc_comp(CSMain)

	rtrc_keyword(WRAP_MODE, CLAMP, REPEAT)
	rtrc_keyword(ITERATION, i0, i1, i2, i3)
	
	rtrc_group(Pass)
	{
		rtrc_define(Texture2D<float3>,   DisplacementMap)
		rtrc_define(Texture2D<float2>,   OldCorrectionMap)
		rtrc_define(RWTexture2D<float2>, NewCorrectionMap)

		rtrc_uniform(int2, resolution)
		rtrc_uniform(float, areaPreservation)
		rtrc_uniform(float3, displacementScale)
		rtrc_uniform(float2, correctionScale)
	};

	bool IsOnBoundary(int2 coord)
	{
		#if WRAP_MODE == WRAP_MODE_CLAMP
			return any(coord <= 0) || any(coord >= Pass.resolution - 1);
		#elif WRAP_MODE == WRAP_MODE_REPEAT
			return false;
		#endif
	}

	// =============================== Compute energy & gradient ===============================

	float LengthSquare(float2 v)
	{
		return dot(v, v);
	}

	float AreaPreservePowDeriv(float x)
	{
		return Pass.areaPreservation * pow(x, Pass.areaPreservation - 1);
	}

	float2 ComputeGradient(float3 dnpos1, float3 dnpos2, float2 dntex1, float2 dntex2, out float energy)
	{
		const float adpos = length(cross(dnpos1, dnpos2));
		const float adtex = dntex1.x * dntex2.y - dntex2.x * dntex1.y;

		const float energy1 = 1.0 / (adpos * adtex);
		const float energy2 =
			LengthSquare(dntex1 - dntex2) * dot(dnpos1, dnpos2) +
			LengthSquare(dntex1) * dot(dnpos2 - dnpos1, dnpos2) +
			LengthSquare(dntex2) * dot(dnpos1, dnpos1 - dnpos2);
		const float areaPreservationFactor = adpos / adtex + adtex / adpos;
		const float energy3 = pow(areaPreservationFactor, Pass.areaPreservation);
		energy = energy1 + energy2 * energy3;

		return
			energy3 * (
				energy2 * energy1 / adtex * -float2(dntex2.y - dntex1.y, dntex1.x - dntex2.x) +
				energy1 * 2.0 * (
					dntex1 * dot(dnpos2 - dnpos1, dnpos2) +
					dntex2 * dot(dnpos1, dnpos1 - dnpos2)
				)
			) +
			energy1 * energy2 * (
				AreaPreservePowDeriv(areaPreservationFactor) *
				float2(dntex2.y - dntex1.y, dntex1.x - dntex2.x) *
				(1.0 / adpos - adpos / (adtex * adtex))
			)
	}

	float ComputeEnergy(float3 dnpos1, float3 dnpos2, float2 dntex1, float2 dntex2)
	{
		float energy;
		ComputeGradient(energy);
		return energy;
	}
	
	// =============================== Input data ===============================

	struct Node
	{
		float3 position;
		float2 correction;
		float2 uv;
	};

	void WrapCoord(inout int2 coord)
	{
		coord -= int2(floor(float2(coord) / Pass.resolution)) * Pass.resolution;
	}

	bool LoadNode(int2 coord, out Node node)
	{
		WrapCoord(coord);

		const float3 displacementVector = Pass.displacementScale * DisplacementMap[coord];
		const float2 positionXZ = float2(coord) / Pass.resolution;
		const float3 position = float3(positionXZ.x, 0, positionXZ.z) + displacementVector;

		const float2 correction = Pass.correctionScale * OldCorrectionMap[coord];
		const float2 uv = positionXZ + correction;

		node.position    = position;
		node.correction  = correction;
		node.uv			 = uv;
		
		return true;
	}

	uint GetIterationIndex()
	{
		return
			ITERATION == ITERATION_i0 ? 0 :
			ITERATION == ITERATION_i1 ? 1 :
			ITERATION == ITERATION_i2 ? 2 :
			ITERATION == ITERATION_i3 ? 3 ;
	}

	[numthreads(8, 8, 1)]
	void CSMain(uint2 tid : SV_DispatchThreadID)
	{
		const int2 coord = int2(tid);
		if(any(coord >= Pass.resolution))
			return;

		Node currentNode;
		LoadNode(coord, currentNode);

		// Collect neighbor nodes

		const int2 neighborOffsets[6] =
		{
			int2(-1, +0),
			int2(-1, +1),
			int2(+0, -1),
			int2(+0, +1),
			int2(+1, -1),
			int2(+1, +0),
		};
		const int neighborIndices[6] =
		{
			0,
			1,
			5,
			2,
			4,
			3
		};
		float3 dnpos[6]; float2 dntex[6];
		for(int i = 0; i < 6; ++i)
		{
			const int neighborIndex = neighborIndices[i];
			const Node neighborNode = LoadNode(coord + neighborOffsets[i]);
			dnpos[neighborIndex] = currentNode.position - neighborNode.position;
			dntex[neighborIndex] = currentNode.uv - neighborNode.uv;
		}

		float energy = 0;
		float2 gradient = 0;
		for(int i = 0; j = 5; i < 6; j = i++)
		{
			float neighborEnergy;
			gradient += ComputeGradient(dnpos[i], dnpos[j], dntex[i], dntex[j], neighborEnergy);
			energy += neighborEnergy;
		}
		energy *= 0.5 / 6;

		float minStepSize = 0, maxStepSize = 2.0e32;
		for(int i = 0, j = 5; i < 6; j = i++)
		{
			const float adtex = dntex[j].x * dntex[i].y - dntex[i].x * dntex[j].y;
			const float djXg = dntex[j].x * gradient.y - gradient.x * dntex[j].y;
			const float dgXi = gradient.x * dntex[i].y - dntex[i].x * gradient.y;
			const float den = djXg + dgXi;
			const float adtex0Offset = adtex / den;
			if(adtex0Offset > 0)
			{
				if(adtex > 0)
					maxStepSize = min(maxStepSize, adtex0Offset);
				else
					minStepSize = max(minStepSize, adtex0Offset);
			}
		}

		if(minStepSize > maxStepSize)
			minStepSize = maxStepSize = 0;

		int searchSteps = 32;
		while((maxStepSize - minStepSize > 1e-6) && --searchSteps != 0)
		{
			const float third1 = lerp(minStepSize, maxStepSize, 1.0 / 3.0);
			const float third2 = lerp(minStepSize, maxStepSize, 2.0 / 3.0);
			const float2 dtex1 = third1 * gradient;
			const float2 dtex2 = third2 * gradient;
			
			float e1 = 0, e2 = 0;
			for(int j = 5, i = 0; i < 6; j = i++)
			{
				e1 += ComputeEnergy(dnpos[j], dnpos[i], dntex[j] - dtex1, dntex[i] - dtex1);
				e2 += ComputeEnergy(dnpos[j], dnpos[i], dntex[j] - dtex2, dntex[i] - dtex2);
			}
			e1 *= 0.5 / 6;
			e2 *= 0.5 / 6;

			if(e1 > e2)
				minStepSize = third1;
			else
				maxStepSize = third2;
		}

		float stepSize = 0.5 * (minStepSize + maxStepSize);
		if(IsOnBoundary(coord))
			stepSize = 0;

		const uint iterationIndex == GetIterationIndex();
		float2 newCorrection = currentNode.correction;
		if((tid.x & 1) == (iterationIndex & 1) && (tid.y & 1) == ((iterationIndex >> 1) & 1))
		   newCorrection -= stepSize * gradient;
		NewCorrectionMap[tid] = newCorrection;
	}

} // rtrc_shader("Rtrc/Builtin/DFDM")
