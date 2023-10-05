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
