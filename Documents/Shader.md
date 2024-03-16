# Custom Shader Syntax

## Shader Entry

```cpp
// Vertex shader
rtrc_vertex(VertexShaderEntryName)
rtrc_vert(VertexShaderEntryName)

// Fragment/pixel shader
rtrc_fragment(FragmentShaderEntryName)
rtrc_frag(FragmentShaderEntryName)
rtrc_pixel(FragmentShaderEntryName)

// Compute shader
rtrc_compute(ComputeShaderEntryName)
rtrc_comp(ComputeShaderEntryName)

// Mesh shader
rtrc_mesh(MeshShaderEntryName)

// Declare this as a ray tracing shader, which can contain multiple shader entries.
rtrc_raytracing()

// Ray generation shader
[shader("raygeneration")]
void RayGenerationShaderEntryName() { ... }

// Closest hit shader
[shader("closesthit")]
void ClosestHitShaderEntryName() { ... }

// Miss shader
[shader("miss")]
void MissShaderEntryName() { ... }

// Any hit shader
[shader("anyhit")]
void AnyHitShaderEntryName() { ... }

// Intersection shader
[shader("intersection")]
void IntersectionShaderEntryName() { ... }

// Callable shader
[shader("callable")]
void CallableShaderEntryName() { ... }

// Ray tracing shader group
rtrc_rt_group(RayGen)
rtrc_rt_group(ClosestHit, AnyHit, Intersection)
```

## Binding Group

```cpp
// Define a resource
rtrc_define(Buffer<uint>, ForwardDeclaredResource)

// Define a binding group
rtrc_group(GroupName)
{
    // Reference the previously forward declared resource in the binding group.
    // Statically used resources must be included in exactly one binding group.
    rtrc_ref(ForwardDeclaredResource)

    // Define a Buffer<uint>, which can only be used in the compute stage (CS).
    // Multiple stages can be specified using '|' (VS | FS, for example).
    rtrc_define(Buffer<uint>, Resource, CS)

    // Define a resource array with 4 elements.
    rtrc_define(Texture2D<float4>[4], ResourceArray)

    // Define a bindless resource array with 64 elements.
    // 'bindless' indicates that it allows partial binding and updating while dynamically unused.
    rtrc_bindless(Texture2D<float>[64], BindlessResourceArray)

    // Define a var-sized bindless resource array with with a maximal size of 512.
    rtrc_bindless_varsize(Buffer<float>[512], VarSizedBindlessResourceArray)

    // Define a uniform value parameter.
    // Uniform parameters within a binding group are automatically packed into a constant buffer with the same name as the binding group.
    rtrc_uniform(float4x4, Value)
};

// Define a new resource array that is bound to the same register as 'BindlessResourceArray'.
// This allows the user to access the same descriptor array with different resource types.
// Note that it is not possible to alias a texture array and a buffer array or vice versa.
rtrc_alias(Texture3D<float2>, AliasedResourceName, BindlessResourceArray)
```

## Push Constant

```cpp
// Define a push constant block accessible in the fragment shader
rtrc_push_constant(Main, FS)
{
    // Define the push constant value 'slots' in the 'Main' block
    // Supported types: float/int/uint + 1/2/3/4
    uint4 slots;
};

uint4 foo()
{
    // Read the value of the push constant 'slots' from the 'Main' block
    return rtrc_get_push_constant(Main, slots);
}
```

## Inline Sampler

```cpp
// Define an inline sampler. Identical inline samplers are automatically merged by the rtrc shader preprocessor.
// All properties are optional.
// Use 'filter' to set 'filter_mag', 'filter_min', and 'filter_mip' to the same value.
// Use 'address' to set 'address_u', 'address_v', and 'address_w' to the same value.
rtrc_sampler(
    SamplerName,
    filter_mag = nearest/point/linear,
    filter_min = nearest/point/linear,
    filter_mip = nearest/point/linear,
    filter = nearest/point/linear,
    anisotropy = 1/2/4/8/16,
    address_u = repeat/mirror/clamp/border,
    address_v = repeat/mirror/clamp/border,
    address_w = repeat/mirror/clamp/border,
    address = repeat/mirror/clamp/border)
```

