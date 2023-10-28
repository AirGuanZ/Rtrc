rtrc_shader("SVGF/ClearPrevData")
{
    rtrc_comp(CSMain)
    #include "./ClearPrevData.hlsl"
}

rtrc_shader("SVGF/TemporalFilter")
{
    rtrc_comp(CSMain)
    #include "./TemporalFilter.hlsl"
}

rtrc_shader("SVGF/VarianceEstimate")
{
    rtrc_comp(CSMain)
    #include "./VarianceEstimate.hlsl"
}

rtrc_shader("SVGF/VariancePrefilter")
{
    rtrc_comp(CSMain)
    #include "./VariancePrefilter.hlsl"
}

rtrc_shader("SVGF/SpatialFilter")
{
    rtrc_keyword(IS_DIRECTION_Y)
    rtrc_comp(CSMain)
    #include "./SpatialFilter.hlsl"
}
