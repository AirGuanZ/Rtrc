rtrc_shader("ReSTIR/ClearNewReservoirs")
{
    rtrc_comp(CSMain)
    #include "./ClearNewReservoirs.hlsl"
}

rtrc_shader("ReSTIR/CreateNewReservoirs")
{
    rtrc_comp(CSMain)
    #include "./CreateNewReservoirs.hlsl"
}

rtrc_shader("ReSTIR/TemporalReuse")
{
    rtrc_comp(CSMain)
    #include "./TemporalReuse.hlsl"
}

rtrc_shader("ReSTIR/Resolve")
{
    rtrc_comp(CSMain)
    #include "./Resolve.hlsl"
}
