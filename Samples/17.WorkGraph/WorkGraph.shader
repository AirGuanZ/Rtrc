rtrc_shader("WorkGraph")
{
	rtrc_entry(EntryNode)

	rtrc_group(Pass)
	{
		rtrc_define(RWTexture2D<float4>, Texture)
		rtrc_uniform(uint2, resolution)
	};

	struct EntryRecord
	{
		uint2 gridSize : SV_DispatchGrid;
	};

	[Shader("node")]
	[NodeLaunch("broadcasting")]
	[NodeMaxDispatchGrid(256, 256, 1)]
	[NumThreads(8, 8, 1)]
	void EntryNode(DispatchNodeInputRecord<EntryRecord> input, uint2 tid : SV_DispatchThreadID)
	{
		if(any(tid >= Pass.resolution))
		{
			return;
		}
		Texture[tid] = 2.0 * Texture[tid];
	}
}
