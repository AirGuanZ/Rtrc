rtrc_shader("VisualizeNormal")
{
    rtrc_comp(CSMain)
    #define VIS_MODE VIS_MODE_NORMAL
    #include "./GBufferVisualizer.hlsl"
}