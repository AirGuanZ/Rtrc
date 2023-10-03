rtrc_shader("PathTracing/Trace")
{
    rtrc_comp(CSMain)
    #include "./Trace.hlsl"
}

rtrc_shader("PathTracing/TemporalFilter")
{
    rtrc_comp(CSMain)
    #include "./TemporalFilter.hlsl"
}

rtrc_shader("PathTracing/SpatialFilter")
{
    rtrc_comp(CSMain)
    rtrc_keyword(IS_FILTER_DIRECTION_Y)
    #include "./SpatialFilter.hlsl"
}
