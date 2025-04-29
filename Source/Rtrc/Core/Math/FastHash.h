#pragma once

#include <Rtrc/Core/Math/Vector.h>

RTRC_BEGIN

// See https://www.shadertoy.com/view/XlGcRh
namespace FastHash
{

    using uint = uint32_t;
    using uvec2 = Vector2u;
    using uvec3 = Vector3u;
    using uvec4 = Vector4u;
    using vec2 = Vector2f;
    using vec3 = Vector3f;
    using vec4 = Vector4f;

    auto dot(auto a, auto b) { return ::Rtrc::Dot(a, b); }
    inline float fract(float v) { float d; return std::modf(v, &d); }
    inline vec2 fract(vec2 v) { return vec2(fract(v.x), fract(v.y)); }
    inline vec3 fract(vec3 v) { return vec3(fract(v.x), fract(v.y), fract(v.z)); }
    inline vec4 fract(vec4 v) { return vec4(fract(v.x), fract(v.y), fract(v.z), fract(v.w)); }

    inline uvec2 xxor(uvec2 a, uvec2 b) { return uvec2(a.x ^ b.x, a.y ^ b.y); }
    inline uvec3 xxor(uvec3 a, uvec3 b) { return uvec3(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z); }

    inline uvec2 rshift(uvec2 a, uint b) { return uvec2(a.x >> b, a.y >> b); }
    inline uvec3 rshift(uvec3 a, uint b) { return uvec3(a.x >> b, a.y >> b, a.z >> b); }

    // commonly used constants
    constexpr uint32_t c1 = 0xcc9e2d51u;
    constexpr uint32_t c2 = 0x1b873593u;

    // Helper Functions
    inline uint rotl(uint x, uint r)
    {
        return (x << r) | (x >> (32u - r));
    }

    inline uint rotr(uint x, uint r)
    {
        return (x >> r) | (x << (32u - r));
    }

    inline uint fmix(uint h)
    {
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;
        return h;
    }

    inline uint mur(uint a, uint h)
    {
        // Helper from Murmur3 for combining two 32-bit values.
        a *= c1;
        a = rotr(a, 17u);
        a *= c2;
        h ^= a;
        h = rotr(h, 19u);
        return h * 5u + 0xe6546b64u;
    }

    inline uint bswap32(uint x)
    {
        return (((x & 0x000000ffu) << 24) |
                ((x & 0x0000ff00u) << 8) |
                ((x & 0x00ff0000u) >> 8) |
                ((x & 0xff000000u) >> 24));
    }

    inline uint taus(uint z, int s1, int s2, int s3, uint m)
    {
        uint b = (((z << s1) ^ z) >> s2);
        return (((z & m) << s3) ^ b);
    }

    // convert 2D seed to 1D
    // 2 imad
    inline uint seed(uvec2 p)
    {
        return 19u * p.x + 47u * p.y + 101u;
    }

    // convert 3D seed to 1D
    inline uint seed(uvec3 p)
    {
        return 19u * p.x + 47u * p.y + 101u * p.z + 131u;
    }

    // convert 4D seed to 1D
    inline uint seed(uvec4 p)
    {
        return 19u * p.x + 47u * p.y + 101u * p.z + 131u * p.w + 173u;
    }

    /**********************************************************************
     * Hashes
     **********************************************************************/

     // BBS-inspired hash
     //  - Olano, Modified Noise for Evaluation on Graphics Hardware, GH 2005
    inline uint bbs(uint v)
    {
        v = v % 65521u;
        v = (v * v) % 65521u;
        v = (v * v) % 65521u;
        return v;
    }

    // CityHash32, adapted from Hash32Len0to4 in https://github.com/google/cityhash
    inline uint city(uint s)
    {
        uint len = 4u;
        uint b = 0u;
        uint c = 9u;

        for(uint i = 0u; i < len; i++) {
            uint v = (s >> (i * 8u)) & 0xffu;
            b = b * c1 + v;
            c ^= b;
        }

        return fmix(mur(b, mur(len, c)));
    }

    // CityHash32, adapted from Hash32Len5to12 in https://github.com/google/cityhash
    inline uint city(uvec2 s)
    {
        uint len = 8u;
        uint a = len, b = len * 5u, c = 9u, d = b;

        a += bswap32(s.x);
        b += bswap32(s.y);
        c += bswap32(s.y);

        return fmix(mur(c, mur(b, mur(a, d))));
    }

    // CityHash32, adapted from Hash32Len5to12 in https://github.com/google/cityhash
    inline uint city(uvec3 s)
    {
        uint len = 12u;
        uint a = len, b = len * 5u, c = 9u, d = b;

        a += bswap32(s.x);
        b += bswap32(s.z);
        c += bswap32(s.y);

        return fmix(mur(c, mur(b, mur(a, d))));
    }

    // CityHash32, adapted from Hash32Len12to24 in https://github.com/google/cityhash
    inline uint city(uvec4 s)
    {
        uint len = 16u;
        uint a = bswap32(s.w);
        uint b = bswap32(s.y);
        uint c = bswap32(s.z);
        uint d = bswap32(s.z);
        uint e = bswap32(s.x);
        uint f = bswap32(s.w);
        uint h = len;

        return fmix(mur(f, mur(e, mur(d, mur(c, mur(b, mur(a, h)))))));
    }

    // Schechter and Bridson hash 
    // https://www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
    inline uint esgtsa(uint s)
    {
        s = (s ^ 2747636419u) * 2654435769u;// % 4294967296u;
        s = (s ^ (s >> 16u)) * 2654435769u;// % 4294967296u;
        s = (s ^ (s >> 16u)) * 2654435769u;// % 4294967296u;
        return s;
    }

    // UE4's RandFast function
    // https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/Random.ush
    inline float fast(vec2 v)
    {
        v = (1.f / 4320.f) * v + vec2(0.25, 0.);
        float state = fract(dot(v * v, vec2(3571)));
        return fract(state * state * (3571.f * 2.f));
    }

    // Hash without Sine
    // https://www.shadertoy.com/view/4djSRW
    inline float hashwithoutsine11(float p)
    {
        p = fract(p * .1031f);
        p *= p + 33.33f;
        p *= p + p;
        return fract(p);
    }

    inline float hashwithoutsine12(vec2 p)
    {
        vec3 p3 = fract(vec3(p.x, p.y, p.x) * .1031f);
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((p3.x + p3.y) * p3.z);
    }

    inline float hashwithoutsine13(vec3 p3)
    {
        p3 = fract(p3 * .1031f);
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((p3.x + p3.y) * p3.z);
    }

    inline vec2 hashwithoutsine21(float p)
    {
        vec3 p3 = fract(vec3(p, p, p) * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((vec2(p3.x, p3.x) + vec2(p3.y, p3.z)) * vec2(p3.z, p3.y));
    }

    inline vec2 hashwithoutsine22(vec2 p)
    {
        vec3 p3 = fract(vec3(p.x, p.y, p.x) * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((vec2(p3.x, p3.x) + vec2(p3.y, p3.z)) * vec2(p3.z, p3.y));
    }

    inline vec2 hashwithoutsine23(vec3 p3)
    {
        p3 = fract(p3 * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((vec2(p3.x, p3.x) + vec2(p3.y, p3.z)) * vec2(p3.z, p3.y));
    }

    inline vec3 hashwithoutsine31(float p)
    {
        vec3 p3 = fract(vec3(p, p, p) * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.z, p3.x) + vec3(33.33f)));
        return fract((vec3(p3.x, p3.x, p3.y) + vec3(p3.y, p3.z, p3.z)) * vec3(p3.z, p3.y, p3.x));
    }

    inline vec3 hashwithoutsine32(vec2 p)
    {
        vec3 p3 = fract(vec3(vec3(p.x, p.y, p.x)) * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.x, p3.z) + vec3(33.33f)));
        return fract((vec3(p3.x, p3.x, p3.y) + vec3(p3.y, p3.z, p3.z)) * vec3(p3.z, p3.y, p3.x));
    }

    inline vec3 hashwithoutsine33(vec3 p3)
    {
        p3 = fract(p3 * vec3(.1031f, .1030f, .0973f));
        p3 += vec3(dot(p3, vec3(p3.y, p3.x, p3.z) + vec3(33.33f)));
        return fract((vec3(p3.x, p3.x, p3.y) + vec3(p3.y, p3.x, p3.x)) * vec3(p3.z, p3.y, p3.x));
    }

    inline vec4 hashwithoutsine41(float p)
    {
        vec4 p4 = fract(vec4(p, p, p, p) * vec4(.1031f, .1030f, .0973f, .1099f));
        p4 += vec4(dot(p4, vec4(p4.w, p4.z, p4.x, p4.y) + vec4(33.33f)));
        return fract((vec4(p4.x, p4.x, p4.y, p4.z) + vec4(p4.y, p4.z, p4.z, p4.w)) * vec4(p4.z, p4.y, p4.w, p4.x));
    }

    inline vec4 hashwithoutsine42(vec2 p)
    {
        vec4 p4 = fract(vec4(vec4(p.x, p.y, p.x, p.y)) * vec4(.1031f, .1030f, .0973f, .1099f));
        p4 += vec4(dot(p4, vec4(p4.w, p4.z, p4.x, p4.y) + vec4(33.33f)));
        return fract((vec4(p4.x, p4.x, p4.y, p4.z) + vec4(p4.y, p4.z, p4.z, p4.w)) * vec4(p4.z, p4.y, p4.w, p4.x));
    }

    inline vec4 hashwithoutsine43(vec3 p)
    {
        vec4 p4 = fract(vec4(vec4(p.x, p.y, p.z, p.x)) * vec4(.1031f, .1030f, .0973f, .1099f));
        p4 += vec4(dot(p4, vec4(p4.w, p4.z, p4.x, p4.y) + vec4(33.33f)));
        return fract((vec4(p4.x, p4.x, p4.y, p4.z) + vec4(p4.y, p4.z, p4.z, p4.w)) * vec4(p4.z, p4.y, p4.w, p4.x));
    }

    inline vec4 hashwithoutsine44(vec4 p4)
    {
        p4 = fract(p4 * vec4(.1031f, .1030f, .0973f, .1099f));
        p4 += vec4(dot(p4, vec4(p4.w, p4.z, p4.x, p4.y) + vec4(33.33f)));
        return fract((vec4(p4.x, p4.x, p4.y, p4.z) + vec4(p4.y, p4.z, p4.z, p4.w)) * vec4(p4.z, p4.y, p4.w, p4.x));
    }

    // Hybrid Taus
    // https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch37.html
    inline uint hybridtaus(uvec4 z)
    {
        z.x = taus(z.x, 13, 19, 12, 0xfffffffeu);
        z.y = taus(z.y, 2, 25, 4, 0xfffffff8u);
        z.z = taus(z.z, 3, 11, 17, 0xfffffff0u);
        z.w = z.w * 1664525u + 1013904223u;

        return z.x ^ z.y ^ z.z ^ z.w;
    }

    // Interleaved Gradient Noise
    //  - Jimenez, Next Generation Post Processing in Call of Duty: Advanced Warfare
    //    Advances in Real-time Rendering, SIGGRAPH 2014
    inline float ign(vec2 v)
    {
        vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
        return fract(magic.z * fract(dot(v, vec2(magic.x, magic.y))));
    }

    // Integer Hash - I
    // - Inigo Quilez, Integer Hash - I, 2017
    //   https://www.shadertoy.com/view/llGSzw
    inline uint iqint1(uint n)
    {
        // integer hash copied from Hugo Elias
        n = (n << 13U) ^ n;
        n = n * (n * n * 15731U + 789221U) + 1376312589U;

        return n;
    }

    // Integer Hash - II
    // - Inigo Quilez, Integer Hash - II, 2017
    //   https://www.shadertoy.com/view/XlXcW4
    inline uvec3 iqint2(uvec3 x)
    {
        const uint k = 1103515245u;

        auto yzx = [](uvec3 v) { return uvec3(v.y, v.z, v.x); };

        x = xxor(rshift(x, 8U), yzx(x)) * k;
        x = xxor(rshift(x, 8U), yzx(x)) * k;
        x = xxor(rshift(x, 8U), yzx(x)) * k;

        return x;
    }

    // Integer Hash - III
    // - Inigo Quilez, Integer Hash - III, 2017
    //   https://www.shadertoy.com/view/4tXyWN
    inline uint iqint3(uvec2 x)
    {
        uvec2 q = 1103515245U * xxor(rshift(x, 1U), uvec2(x.y, x.x));
        uint  n = 1103515245U * ((q.x) ^ (q.y >> 3U));

        return n;
    }

    inline uint jkiss32(uvec2 p)
    {
        uint x = p.x;//123456789;
        uint y = p.y;//234567891;

        uint z = 345678912u, w = 456789123u, c = 0u;
        int t;
        y ^= (y << 5); y ^= (y >> 7); y ^= (y << 22);
        t = int(z + w + c);
        w = uint(t & 2147483647);
        x += 1411392427u;
        return x + y + w;
    }

    // linear congruential generator
    inline uint lcg(uint p)
    {
        return p * 1664525u + 1013904223u;
    }

    // MD5GPU
    // https://www.microsoft.com/en-us/research/wp-content/uploads/2007/10/tr-2007-141.pdf
    constexpr uint A0 = 0x67452301u;
    constexpr uint B0 = 0xefcdab89u;
    constexpr uint C0 = 0x98badcfeu;
    constexpr uint D0 = 0x10325476u;

    inline uint F(uvec3 v) { return (v.x & v.y) | (~v.x & v.z); }
    inline uint G(uvec3 v) { return (v.x & v.z) | (v.y & ~v.z); }
    inline uint H(uvec3 v) { return v.x ^ v.y ^ v.z; }
    inline uint I(uvec3 v) { return v.y ^ (v.x | ~v.z); }

    inline void FF(uvec4 &v, uvec4 &rotate, uint x, uint ac)
    {
        v.x = v.y + rotl(v.x + F({ v.y, v.z, v.w }) + x + ac, rotate.x);

        rotate = { rotate.y, rotate.z, rotate.w, rotate.x };
        v = { v.y, v.z, v.w, v.x };
    }

    inline void GG(uvec4 &v, uvec4 &rotate, uint x, uint ac)
    {
        v.x = v.y + rotl(v.x + G({ v.y, v.z, v.y }) + x + ac, rotate.x);

        rotate = { rotate.y, rotate.z, rotate.w, rotate.x };
        v = { v.y, v.z, v.w, v.x };
    }

    inline void HH(uvec4 &v, uvec4 &rotate, uint x, uint ac)
    {
        v.x = v.y + rotl(v.x + H({ v.y, v.z, v.w }) + x + ac, rotate.x);

        rotate = { rotate.y, rotate.z, rotate.w, rotate.x };
        v = { v.y, v.z, v.w, v.x };
    }

    inline void II(uvec4 &v, uvec4 &rotate, uint x, uint ac)
    {
        v.x = v.y + rotl(v.x + I({ v.y, v.z, v.w }) + x + ac, rotate.x);

        rotate = { rotate.y, rotate.z, rotate.w, rotate.x };
        v = { v.y, v.z, v.w, v.x };
    }

    inline uint K(uint i)
    {
        return uint(std::abs(std::sin(float(i) + 1.)) * float(0xffffffffu));
    }

    inline uvec4 md5(uvec4 u)
    {
        uvec4 digest = uvec4(A0, B0, C0, D0);
        uvec4 r, v = digest;
        uint i = 0u;

        uint M[16];
        M[0] = u.x; M[1] = u.y;	M[2] = u.z;	M[3] = u.w;
        M[4] = 0u; M[5] = 0u; M[6] = 0u; M[7] = 0u; M[8] = 0u;
        M[9] = 0u; M[10] = 0u; M[11] = 0u; M[12] = 0u; M[13] = 0u;
        M[14] = 0u; M[15] = 0u;

        r = uvec4(7, 12, 17, 22);
        FF(v, r, M[0], K(i++));
        FF(v, r, M[1], K(i++));
        FF(v, r, M[2], K(i++));
        FF(v, r, M[3], K(i++));
        FF(v, r, M[4], K(i++));
        FF(v, r, M[5], K(i++));
        FF(v, r, M[6], K(i++));
        FF(v, r, M[7], K(i++));
        FF(v, r, M[8], K(i++));
        FF(v, r, M[9], K(i++));
        FF(v, r, M[10], K(i++));
        FF(v, r, M[11], K(i++));
        FF(v, r, M[12], K(i++));
        FF(v, r, M[13], K(i++));
        FF(v, r, M[14], K(i++));
        FF(v, r, M[15], K(i++));

        r = uvec4(5, 9, 14, 20);
        GG(v, r, M[1], K(i++));
        GG(v, r, M[6], K(i++));
        GG(v, r, M[11], K(i++));
        GG(v, r, M[0], K(i++));
        GG(v, r, M[5], K(i++));
        GG(v, r, M[10], K(i++));
        GG(v, r, M[15], K(i++));
        GG(v, r, M[4], K(i++));
        GG(v, r, M[9], K(i++));
        GG(v, r, M[14], K(i++));
        GG(v, r, M[3], K(i++));
        GG(v, r, M[8], K(i++));
        GG(v, r, M[13], K(i++));
        GG(v, r, M[2], K(i++));
        GG(v, r, M[7], K(i++));
        GG(v, r, M[12], K(i++));

        r = uvec4(4, 11, 16, 23);
        HH(v, r, M[5], K(i++));
        HH(v, r, M[8], K(i++));
        HH(v, r, M[11], K(i++));
        HH(v, r, M[14], K(i++));
        HH(v, r, M[1], K(i++));
        HH(v, r, M[4], K(i++));
        HH(v, r, M[7], K(i++));
        HH(v, r, M[10], K(i++));
        HH(v, r, M[13], K(i++));
        HH(v, r, M[0], K(i++));
        HH(v, r, M[3], K(i++));
        HH(v, r, M[6], K(i++));
        HH(v, r, M[9], K(i++));
        HH(v, r, M[12], K(i++));
        HH(v, r, M[15], K(i++));
        HH(v, r, M[2], K(i++));

        r = uvec4(6, 10, 15, 21);
        II(v, r, M[0], K(i++));
        II(v, r, M[7], K(i++));
        II(v, r, M[14], K(i++));
        II(v, r, M[5], K(i++));
        II(v, r, M[12], K(i++));
        II(v, r, M[3], K(i++));
        II(v, r, M[10], K(i++));
        II(v, r, M[1], K(i++));
        II(v, r, M[8], K(i++));
        II(v, r, M[15], K(i++));
        II(v, r, M[6], K(i++));
        II(v, r, M[13], K(i++));
        II(v, r, M[4], K(i++));
        II(v, r, M[11], K(i++));
        II(v, r, M[2], K(i++));
        II(v, r, M[9], K(i++));

        return digest + v;
    }

    // Adapted from MurmurHash3_x86_32 from https://github.com/aappleby/smhasher
    inline uint murmur3(uint seed)
    {
        uint h = 0u;
        uint k = seed;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        h ^= 4u;

        return fmix(h);
    }

    // Adapted from MurmurHash3_x86_32 from https://github.com/aappleby/smhasher
    inline uint murmur3(uvec2 seed)
    {
        uint h = 0u;
        uint k = seed.x;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.y;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        h ^= 8u;

        return fmix(h);
    }

    // Adapted from MurmurHash3_x86_32 from https://github.com/aappleby/smhasher
    inline uint murmur3(uvec3 seed)
    {
        uint h = 0u;
        uint k = seed.x;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.y;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.z;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        h ^= 12u;

        return fmix(h);
    }

    // Adapted from MurmurHash3_x86_32 from https://github.com/aappleby/smhasher
    inline uint murmur3(uvec4 seed)
    {
        uint h = 0u;
        uint k = seed.x;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.y;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.z;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        k = seed.w;

        k *= c1;
        k = rotl(k, 15u);
        k *= c2;

        h ^= k;
        h = rotl(h, 13u);
        h = h * 5u + 0xe6546b64u;

        h ^= 16u;

        return fmix(h);
    }

    // https://www.pcg-random.org/
    inline uint pcg(uint v)
    {
        uint state = v * 747796405u + 2891336453u;
        uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }

    inline uvec2 pcg2d(uvec2 v)
    {
        v = v * 1664525u + uvec2(1013904223u);

        v.x += v.y * 1664525u;
        v.y += v.x * 1664525u;

        v.x = v.x ^ (v.x >> 16u);
        v.y = v.y ^ (v.y >> 16u);

        v.x += v.y * 1664525u;
        v.y += v.x * 1664525u;

        v.x = v.x ^ (v.x >> 16u);
        v.y = v.y ^ (v.y >> 16u);

        return v;
    }

    // http://www.jcgt.org/published/0009/03/02/
    inline uvec3 pcg3d(uvec3 v) {

        v = v * 1664525u + uvec3(1013904223u);

        v.x += v.y * v.z;
        v.y += v.z * v.x;
        v.z += v.x * v.y;

        v.x ^= v.x >> 16u;
        v.y ^= v.y >> 16u;
        v.z ^= v.z >> 16u;

        v.x += v.y * v.z;
        v.y += v.z * v.x;
        v.z += v.x * v.y;

        return v;
    }

    // http://www.jcgt.org/published/0009/03/02/
    inline uvec3 pcg3d16(uvec3 v)
    {
        v = v * 12829u + uvec3(47989u);

        v.x += v.y * v.z;
        v.y += v.z * v.x;
        v.z += v.x * v.y;

        v.x += v.y * v.z;
        v.y += v.z * v.x;
        v.z += v.x * v.y;

        v.x >>= 16u;
        v.y >>= 16u;
        v.z >>= 16u;

        return v;
    }

    // http://www.jcgt.org/published/0009/03/02/
    inline uvec4 pcg4d(uvec4 v)
    {
        v = v * 1664525u + uvec4(1013904223u);

        v.x += v.y * v.w;
        v.y += v.z * v.x;
        v.z += v.x * v.y;
        v.w += v.y * v.z;

        v.x ^= v.x >> 16u;
        v.y ^= v.y >> 16u;
        v.z ^= v.z >> 16u;
        v.w ^= v.w >> 16u;

        v.x += v.y * v.w;
        v.y += v.z * v.x;
        v.z += v.x * v.y;
        v.w += v.y * v.z;

        return v;
    }

    // UE4's PseudoRandom function
    // https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/Random.ush
    inline float pseudo(vec2 v)
    {
        v = fract(v / 128.f) * 128.f + vec2(-64.340622f, -72.465622f);
        return fract(dot(vec3(v.x, v.y, v.x) * vec3(v.x, v.y, v.y), vec3(20.390625f, 60.703125f, 2.4281209f)));
    }

    // Numerical Recipies 3rd Edition
    inline uint ranlim32(uint j)
    {
        uint u, v, w1, w2, x, y;

        v = 2244614371U;
        w1 = 521288629U;
        w2 = 362436069U;

        u = j ^ v;

        u = u * 2891336453U + 1640531513U;
        w1 = 33378u * (w1 & 0xffffu) + (w1 >> 16);
        w2 = 57225u * (w2 & 0xffffu) + (w2 >> 16);

        v = u;

        u = u * 2891336453U + 1640531513U;
        v ^= v >> 13; v ^= v << 17; v ^= v >> 5;
        w1 = 33378u * (w1 & 0xffffu) + (w1 >> 16);
        w2 = 57225u * (w2 & 0xffffu) + (w2 >> 16);

        x = u ^ (u << 9); x ^= x >> 17; x ^= x << 6;
        y = w1 ^ (w1 << 17); y ^= y >> 15; y ^= y << 5;

        return (x + v) ^ (y + w2);
    }

    // SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
    inline uint superfast(uint data)
    {
        uint hash = 4u, tmp;

        hash += data & 0xffffu;
        tmp = (((data >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }

    // SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
    inline uint superfast(uvec2 data)
    {
        uint hash = 8u, tmp;

        hash += data.x & 0xffffu;
        tmp = (((data.x >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.y & 0xffffu;
        tmp = (((data.y >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }

    // SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
    inline uint superfast(uvec3 data)
    {
        uint hash = 8u, tmp;

        hash += data.x & 0xffffu;
        tmp = (((data.x >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.y & 0xffffu;
        tmp = (((data.y >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.z & 0xffffu;
        tmp = (((data.z >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }

    // SuperFastHash, adapated from http://www.azillionmonkeys.com/qed/hash.html
    inline uint superfast(uvec4 data)
    {
        uint hash = 8u, tmp;

        hash += data.x & 0xffffu;
        tmp = (((data.x >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.y & 0xffffu;
        tmp = (((data.y >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.z & 0xffffu;
        tmp = (((data.z >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        hash += data.w & 0xffffu;
        tmp = (((data.w >> 16) & 0xffffu) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        hash += hash >> 11;

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }

    // Tiny Encryption Algorithm
    //  - Zafar et al., GPU random numbers via the tiny encryption algorithm, HPG 2010
    inline uvec2 tea(int tea, uvec2 p)
    {
        uint s = 0u;

        for(int i = 0; i < tea; i++) {
            s += 0x9E3779B9u;
            p.x += (p.y << 4u) ^ (p.y + s) ^ (p.y >> 5u);
            p.y += (p.x << 4u) ^ (p.x + s) ^ (p.x >> 5u);
        }
        return { p.x, p.y };
    }

    // common GLSL hash
    //  - Rey, On generating random numbers, with help of y= [(a+x)sin(bx)] mod 1,
    //    22nd European Meeting of Statisticians and the 7th Vilnius Conference on
    //    Probability Theory and Mathematical Statistics, August 1998
    /*
    uvec2 trig(uvec2 p) {
        return uvec2(float(0xffffff)*fract(43757.5453*sin(dot(vec2(p),vec2(12.9898,78.233)))));
    }
    */
    inline float trig(vec2 p)
    {
        return fract(43757.5453f * std::sin(dot(p, vec2(12.9898f, 78.233f))));
    }

    // Wang hash, described on http://burtleburtle.net/bob/hash/integer.html
    // original page by Thomas Wang 404
    inline uint wang(uint v)
    {
        v = (v ^ 61u) ^ (v >> 16u);
        v *= 9u;
        v ^= v >> 4u;
        v *= 0x27d4eb2du;
        v ^= v >> 15u;
        return v;
    }

    // 128-bit xorshift
    //  - Marsaglia, Xorshift RNGs, Journal of Statistical Software, v8n14, 2003
    inline uint xorshift128(uvec4 v)
    {
        v.w ^= v.w << 11u;
        v.w ^= v.w >> 8u;
        v = { v.w, v.x, v.y, v.z };
        v.x ^= v.y;
        v.x ^= v.y >> 19u;
        return v.x;
    }

    // 32-bit xorshift
    //  - Marsaglia, Xorshift RNGs, Journal of Statistical Software, v8n14, 2003
    inline uint xorshift32(uint v)
    {
        v ^= v << 13u;
        v ^= v >> 17u;
        v ^= v << 5u;
        return v;
    }

    // xxhash (https://github.com/Cyan4973/xxHash)
    //   From https://www.shadertoy.com/view/Xt3cDn
    inline uint xxhash32(uint p)
    {
        const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
        const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
        uint h32 = p + PRIME32_5;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
        h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
        return h32 ^ (h32 >> 16);
    }

    inline uint xxhash32(uvec2 p)
    {
        const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
        const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
        uint h32 = p.y + PRIME32_5 + p.x * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
        h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
        return h32 ^ (h32 >> 16);
    }

    inline uint xxhash32(uvec3 p)
    {
        const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
        const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
        uint h32 = p.z + PRIME32_5 + p.x * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 += p.y * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
        h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
        return h32 ^ (h32 >> 16);
    }

    inline uint xxhash32(uvec4 p)
    {
        const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
        const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
        uint h32 = p.w + PRIME32_5 + p.x * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 += p.y * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 += p.z * PRIME32_3;
        h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
        h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
        h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
        return h32 ^ (h32 >> 16);
    }

} // namespace FastHash

RTRC_END
