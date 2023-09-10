Shader "Builtin/Debug/VisualizeNormal"
{
    #comp CSMain
    #define VIS_MODE VIS_MODE_NORMAL
    #include "./GBufferVisualizer.hlsl"
}