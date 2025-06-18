/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vec.h                                             :+:      :+:    :+:    */
/*                                                    +:+ +:+         +:+     */
/*   By: lfiestas <lfiestas@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/30 14:32:49 by lfiestas          #+#    #+#             */
/*   Updated: 2025/04/22 10:15:37 by lfiestas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VEC_INCLUDED
#define VEC_INCLUDED 1

#include <tgmath.h> // TODO not widely supported so we need our own s_ macros!

#if __GNUC__
#define VEC_FUNC __attribute__((warn_unused_result, nonnull())) static inline
#elif _MSC_VER
#define VEC_FUNC _Check_return_ static inline
#else
#define VEC_FUNC static inline
#endif

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) && \
    !defined(__TINYC__) && \
    !defined(__MSP430__) && \
    !defined(__cplusplus) && \
    !defined(__COMPCERT__)
#define VEC_STATIC static
#else
#define VEC_STATIC
#endif

#if __GNUG__ || _MSC_VER
#define MAT_RESTRICT __restrict
#elif __cplusplus
#define MAT_RESTRICT
#else
#define MAT_RESTRICT restrict
#endif

#define VEC_OVERLOAD(_0, _1, _2, _3, RESOLVED, ...) RESOLVED
#define MAT_INIT_OVERLOAD(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13_, _14, _15, RESOLVED, ...) RESOLVED

// ----------------------------------------------------------------------------
// Types

#ifdef USER_SCALAR_TYPE
typedef USER_SCALAR_TYPE Scalar;
#else
typedef double Scalar;
#endif

// ------------------------------------
// Vectors

typedef struct Vec2
{
    union {
        Scalar x;
        Scalar r;
    };
    union {
        Scalar y;
        Scalar g;
    };
} Vec2;

typedef struct Vec3
{
    union {
        Scalar x;
        Scalar r;
    };
    union {
        Scalar y;
        Scalar g;
    };
    union {
        Scalar z;
        Scalar b;
    };
    #ifndef VEC3_PACKED // define this to save space
    // Dummy component for better SIMD utilization (YMM registers).
    // Usually requires some compiler flag e.g. `-march=native` or `-mavx` to be
    // effective.
    // Using this component for math or logic is undefined.
    Scalar _;
    #endif
} Vec3;

typedef struct Vec4
{
    union {
        Scalar x;
        Scalar r;
    };
    union {
        Scalar y;
        Scalar g;
    };
    union {
        Scalar z;
        Scalar b;
    };
    union {
        Scalar w;
        Scalar a;
    };
} Vec4;

// ------------------------------------
// Matrices
//
// MatN is a reference type. You can explicitly create a copy on stack with
// `matN_copy()` or `matN()`. There is no heap allocations in this library.
//
// Unlike in GLSL, indexing once does not return a VecN. However, you can create
// a vector by indexing to a matrix using vector constructors.

// A NxN multidimensional array decays to these.
typedef Scalar(*MAT_RESTRICT Mat2)[2];
typedef Scalar(*MAT_RESTRICT Mat3)[3];
typedef Scalar(*MAT_RESTRICT Mat4)[4];

// Value matrices are mostly not used, but they allow functions to return Mat2,
// Mat3, and Mat4 by value. Macros usually return .m field as MatN.

typedef struct Mat2Value
{
    Scalar m[2][2];
} Mat2Value;

typedef struct Mat3Value
{
    Scalar m[3][3];
} Mat3Value;

typedef struct Mat4Value
{
    Scalar m[4][4];
} Mat4Value;

// ----------------------------------------------------------------------------
// Constructors

// Not all combination of vector and scalar arguments supported by GLSL are
// supported by us, only the most useful ones, namely:
//
// - constructing all components from scalars
// - constructing first components from a vector, rest from scalars
// - constructing all components from a single scalar
//
// and in case of matrices:
//
// - constructing all components from scalars
// - constructing all components from vectors
// - constructing diagonal values from a single scalar, rest zeroed
//
// We add a few constructors of our own:
//
// - VecN can be constructed from pointers pointing to N elements
// - Matrices can be copied on stack by passing other another matrix

#define vec2(...) VEC_OVERLOAD(__VA_ARGS__, _, _, \
    vec2_init, \
    _Generic(__VA_ARGS__, \
        Scalar*:       vec2_initp, \
        const Scalar*: vec2_initp, \
        default:       vec2_inits))(__VA_ARGS__)

#define vec3(...) VEC_OVERLOAD(__VA_ARGS__, _, \
    vec3_init,   \
    vec3_initv2, \
    _Generic(__VA_ARGS__, \
        Scalar*:       vec3_initp, \
        const Scalar*: vec3_initp, \
        default:       vec3_inits))(__VA_ARGS__)

#define vec4(...) VEC_OVERLOAD(__VA_ARGS__, \
    vec4_init,   \
    vec4_initv2, \
    vec4_initv3, \
    _Generic(__VA_ARGS__, \
        Scalar*:       vec4_initp, \
        const Scalar*: vec4_initp, \
        default: vec4_inits))(__VA_ARGS__)

#define mat2(...) MAT_INIT_OVERLOAD(__VA_ARGS__, _, _, _, _, _, _, _, _, _, _, _, _, \
    mat2_init, _, \
    mat2_initv2,  \
    _Generic(__VA_ARGS__, Mat2: mat2_copy, default: mat2_init_diagonal))(__VA_ARGS__).m

#define mat3(...) MAT_INIT_OVERLOAD(__VA_ARGS__, _, _, _, _, _, _, _, \
    mat3_init, _, _, _, _, _, \
    mat3_initv3, _, \
    _Generic(__VA_ARGS__, Mat3: mat3_copy, default: mat3_init_diagonal))(__VA_ARGS__).m

#define mat4(...) MAT_INIT_OVERLOAD(__VA_ARGS__, \
    mat4_init, _, _, _, _, _, _, _, _, _, _, _,  \
    mat4_initv4, _, _, \
    _Generic(__VA_ARGS__, Mat4: mat4_copy, default: mat4_init_diagonal))(__VA_ARGS__).m

// ----------------------------------------------------------------------------
// Swizzling

// You can get a new vector as r-value with swizzling macros, They work the same
// same as in GLSL, any combination of components can be used. You can also
// assign with dedicated macros.
//
// Here is how the macros would be defined in GLSL. `ijkl` are any of `xyzw` or
// `rgba`.
#if VEC_SWIZZLING_PSEUDOCODE
#define vec_i(V)              (V.i)
#define vec_ij(V)             (V.ij)
#define vec_ijk(V)            (V.ijk)
#define vec_ijkl(V)           (V.ijkl)

#define vec_i_assign(V, U)    (V.i    = U)
#define vec_ij_assign(V, U)   (V.ij   = U)
#define vec_ijk_assign(V, U)  (V.ijk  = U)
#define vec_ijkl_assign(V, U) (V.ijkl = U)
#endif

// ----------------------------------------------------------------------------
// Generic Macros

// TODO still missing a lot of GLSL functionality.
// TODO C++ support.
// TODO comparisons for branchless programming.

#define vec_add(V, U) _Generic(V, \
    Vec2: _Generic(U, Vec2: vec2_add, default: vec2_adds), \
    Vec3: _Generic(U, Vec3: vec3_add, default: vec3_adds), \
    Vec4: _Generic(U, Vec4: vec4_add, default: vec4_adds))(V, U)

#define vec_sub(V, U) _Generic(V, \
    Vec2: _Generic(U, Vec2: vec2_sub, default: vec2_subs), \
    Vec3: _Generic(U, Vec3: vec3_sub, default: vec3_subs), \
    Vec4: _Generic(U, Vec4: vec4_sub, default: vec4_subs))(V, U)

#define vec_mul(V, U) _Generic(V, \
    Vec2: _Generic(U, Vec2: vec2_mul, Mat2: vec2_mat2, default: vec2_muls), \
    Vec3: _Generic(U, Vec3: vec3_mul, Mat3: vec3_mat3, default: vec3_muls), \
    Vec4: _Generic(U, Vec4: vec4_mul, Mat4: vec4_mat4, default: vec4_muls))(V, U)

#define vec_div(V, U) _Generic(V, \
    Vec2: _Generic(U, Vec2: vec2_div, default: vec2_divs), \
    Vec3: _Generic(U, Vec3: vec3_div, default: vec3_divs), \
    Vec4: _Generic(U, Vec4: vec4_div, default: vec4_divs))(V, U)

#define vec_pow(V, U) _Generic(V, \
    Vec2: _Generic(U, Vec2: vec2_pow, default: vec2_pows), \
    Vec3: _Generic(U, Vec3: vec3_pow, default: vec3_pows), \
    Vec4: _Generic(U, Vec4: vec4_pow, default: vec4_pows))(V, U)

#define vec_dot(V, U) _Generic(V, \
    Vec2: vec2_dot, Vec3: vec3_dot, Vec4: vec4_dot)(V, U)

#define vec_cross(V, U) vec3_cross(V, U)

#define vec_reflect(V, N) _Generic(V, \
    Vec2: vec2_reflect, Vec3: vec3_reflect, Vec4: vec4_reflect)(V, N)

#define vec_abs(V) _Generic(V, \
    Vec2: vec2_abs, Vec3: vec3_abs, Vec4: vec4_abs)(V)

#define vec_min(V, U) _Generic(V, \
    Vec2: vec2_min, Vec3: vec3_min, Vec4: vec4_min)(V, U)

#define vec_max(V, U) _Generic(V, \
    Vec2: vec2_max, Vec3: vec3_max, Vec4: vec4_max)(V, U)

#define vec_clamp(V, MIN, MAX) _Generic(V, \
    Vec2: _Generic(MIN, Vec2: vec2_clamp, default: vec2_clamps), \
    Vec3: _Generic(MIN, Vec3: vec3_clamp, default: vec3_clamps), \
    Vec4: _Generic(MIN, Vec4: vec4_clamp, default: vec4_clamps))(V, MIN, MAX)

#define vec_mix(V, U, A) _Generic(V, \
    Vec2: _Generic(A, Vec2: vec2_mix, default: vec2_mixs), \
    Vec3: _Generic(A, Vec3: vec3_mix, default: vec3_mixs), \
    Vec4: _Generic(A, Vec4: vec4_mix, default: vec4_mixs))(V, U, A)

#define vec_length(V) _Generic(V, \
    Vec2: vec2_length, Vec3: vec3_length, Vec4: vec4_length)(V)

#define vec_normalize(V) _Generic(V, \
    Vec2: vec2_normalize, Vec3: vec3_normalize, Vec4: vec4_normalize)(V)

#define mat_add(A, B) _Generic(A, \
    Vec2: _Generic(B, Mat2: mat2_add, default: mat2_adds), \
    Vec3: _Generic(B, Mat3: mat3_add, default: mat3_adds), \
    Vec4: _Generic(B, Mat4: mat4_add, default: mat4_adds))(A, B)

#define mat_sub(A, B) _Generic(A, \
    Vec2: _Generic(B, Mat2: mat2_sub, default: mat2_subs), \
    Vec3: _Generic(B, Mat3: mat3_sub, default: mat3_subs), \
    Vec4: _Generic(B, Mat4: mat4_sub, default: mat4_subs))(A, B)

#define mat_mul(A, B) _Generic(A, \
    Vec2: _Generic(B, Vec2: mat2_vec2, Mat2: mat2_mul, default: mat2_muls), \
    Vec3: _Generic(B, Vec3: mat3_vec3, Mat3: mat3_mul, default: mat3_muls), \
    Vec4: _Generic(B, Vec4: mat4_vec4, Mat4: mat4_mul, default: mat4_muls))(A, B)

#define mat_comp_mul(A, B) _Generic(A, \
    Vec2: _Generic(B, Mat2: mat2_comp_mul, default: mat2_comp_muls), \
    Vec3: _Generic(B, Mat3: mat3_comp_mul, default: mat3_comp_muls), \
    Vec4: _Generic(B, Mat4: mat4_comp_mul, default: mat4_comp_muls))(A, B)

#define mat_div(A, B) _Generic(A, \
    Vec2: _Generic(B, Mat2: mat2_div, default: mat2_divs), \
    Vec3: _Generic(B, Mat3: mat3_div, default: mat3_divs), \
    Vec4: _Generic(B, Mat4: mat4_div, default: mat4_divs))(A, B)

// ----------------------------------------------------------------------------
// Scalar

VEC_FUNC Scalar s_clamp(Scalar s, Scalar min, Scalar max)
{
    return fmin(fmax(s, min), max);
}

VEC_FUNC Scalar s_mix(Scalar s1, Scalar s2, Scalar a)
{
    return s1*(1.f-a) + s2*a;
}

// ----------------------------------------------------------------------------
// Vec2

VEC_FUNC Vec2 vec2_init(Scalar x, Scalar y)
{
    Vec2 v = { {x}, {y} };
    return v;
}

VEC_FUNC Vec2 vec2_inits(Scalar s)
{
    return vec2_init(s, s);
}

VEC_FUNC Vec2 vec2_initp(const Scalar s[VEC_STATIC 2])
{
    return vec2_init(s[0], s[1]);
}

VEC_FUNC Vec2 vec2_add(Vec2 v1, Vec2 v2)
{
    return vec2_init(v1.x + v2.x, v1.y + v2.y);
}

VEC_FUNC Vec2 vec2_sub(Vec2 v1, Vec2 v2)
{
    return vec2_init(v1.x - v2.x, v1.y - v2.y);
}

VEC_FUNC Vec2 vec2_mul(Vec2 v1, Vec2 v2)
{
    return vec2_init(v1.x * v2.x, v1.y * v2.y);
}

VEC_FUNC Vec2 vec2_div(Vec2 v1, Vec2 v2)
{
    return vec2_init(v1.x / v2.x, v1.y / v2.y);
}

VEC_FUNC Vec2 vec2_pow(Vec2 v1, Vec2 v2)
{
    return vec2_init(pow(v1.x, v2.x), pow(v1.y, v2.y));
}

VEC_FUNC Vec2 vec2_adds(Vec2 v, Scalar s)
{
    return vec2_init(v.x + s, v.y + s);
}

VEC_FUNC Vec2 vec2_subs(Vec2 v, Scalar s)
{
    return vec2_init(v.x - s, v.y - s);
}

VEC_FUNC Vec2 vec2_muls(Vec2 v, Scalar s)
{
    return vec2_init(v.x * s, v.y * s);
}

VEC_FUNC Vec2 vec2_divs(Vec2 v, Scalar s)
{
    return vec2_init(v.x / s, v.y / s);
}

VEC_FUNC Vec2 vec2_pows(Vec2 v, Scalar s)
{
    return vec2_init(pow(v.x, s), pow(v.y, s));
}

VEC_FUNC Scalar vec2_dot(Vec2 v1, Vec2 v2)
{
    return v1.x*v2.x + v1.y*v2.y;
}

VEC_FUNC Vec2 vec2_reflect(Vec2 v, Vec2 n)
{
    return vec2_sub(v, vec2_muls(n, 2.*vec2_dot(n, v)));
}

VEC_FUNC Vec2 vec2_abs(Vec2 v)
{
    return vec2_init(fabs(v.x), fabs(v.y));
}

VEC_FUNC Vec2 vec2_min(Vec2 v1, Vec2 v2)
{
    return vec2_init(fmin(v1.x, v2.x), fmin(v1.y, v2.y));
}

VEC_FUNC Vec2 vec2_max(Vec2 v1, Vec2 v2)
{
    return vec2_init(fmax(v1.x, v2.x), fmax(v1.y, v2.y));
}

VEC_FUNC Vec2 vec2_clamp(Vec2 v, Vec2 min, Vec2 max)
{
    return vec2_init(fmin(fmax(v.x, min.x), max.x), fmin(fmax(v.y, min.y), max.y));
}

VEC_FUNC Vec2 vec2_clamps(Vec2 v, Scalar min, Scalar max)
{
    return vec2_init(fmin(fmax(v.x, min), max), fmin(fmax(v.y, min), max));
}

VEC_FUNC Vec2 vec2_mix(Vec2 v1, Vec2 v2, Vec2 a)
{
    return vec2_init(
        v1.x*(1.f-a.x) + v2.x*a.x,
        v1.y*(1.f-a.y) + v2.y*a.y);
}

VEC_FUNC Vec2 vec2_mixs(Vec2 v1, Vec2 v2, Scalar a)
{
    return vec2_init(
        v1.x*(1.f-a) + v2.x*a,
        v1.y*(1.f-a) + v2.y*a);
}

VEC_FUNC Scalar vec2_length(Vec2 v)
{
    return sqrt(v.x*v.x + v.y*v.y);
}

VEC_FUNC Vec2 vec2_normalize(Vec2 v)
{
    return vec2_divs(v, vec2_length(v));
}

VEC_FUNC Vec2 vec2_mat2(Vec2 v, const Scalar m[2][2])
{
    return vec2_init(
        v.x*m[0][0] + v.y*m[1][0],
        v.x*m[0][1] + v.y*m[1][1]);
}

VEC_FUNC Vec2 mat2_vec2(const Scalar m[2][2], Vec2 v)
{
    return vec2_init(
        m[0][0]*v.x + m[0][1]*v.y,
        m[1][0]*v.x + m[1][1]*v.y);
}

VEC_FUNC Vec2 vec2_rotate(Vec2 v, Scalar angle)
{
    Scalar m[2][2] = {
        { cos(angle), -sin(angle) },
        { sin(angle),  cos(angle) }
    };
    return mat2_vec2(m, v);
}

// ----------------------------------------------------------------------------
// Vec3

VEC_FUNC Vec3 vec3_init(Scalar x, Scalar y, Scalar z)
{
    #ifndef VEC3_PACKED
    Vec3 v = { {x}, {y}, {z}, 0 };
    #else
    Vec3 v = { {x}, {y}, {z} };
    #endif
    return v;
}

#define vec3_initv2(V, Z) vec3_init((V).x, (V).y, Z)

VEC_FUNC Vec3 vec3_inits(Scalar s)
{
    return vec3_init(s, s, s);
}

VEC_FUNC Vec3 vec3_initp(const Scalar s[VEC_STATIC 3])
{
    return vec3_init(s[0], s[1], s[2]);
}

// May help for utilizing YMM registers.
VEC_FUNC Vec3 vec3_ymm_init(Scalar x, Scalar y, Scalar z, Scalar dummy)
{
    #ifndef VEC3_PACKED
    Vec3 v = { {x}, {y}, {z}, dummy };
    #else
    Vec3 v = { {x}, {y}, {z} };
    #endif
    return v;
}

#ifdef VEC3_PACKED // ignore the last dummy parameter
#define vec3_ymm_init(X, Y, Z, ...) vec3_init(X, Y, Z)
#endif

VEC_FUNC Vec3 vec3_add(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1._ + v2._);
}

VEC_FUNC Vec3 vec3_sub(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1._ - v2._);
}

VEC_FUNC Vec3 vec3_mul(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1._ * v2._);
}

VEC_FUNC Vec3 vec3_div(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1._ / v2._);
}

VEC_FUNC Vec3 vec3_pow(Vec3 v1, Vec3 v2)
{
    return vec3_init(pow(v1.x, v2.x), pow(v1.y, v2.y), pow(v1.z, v2.z));
}

VEC_FUNC Vec3 vec3_adds(Vec3 v, Scalar s)
{
    return vec3_ymm_init(v.x + s, v.y + s, v.z + s, v._ + s);
}

VEC_FUNC Vec3 vec3_subs(Vec3 v, Scalar s)
{
    return vec3_ymm_init(v.x - s, v.y - s, v.z - s, v._ - s);
}

VEC_FUNC Vec3 vec3_muls(Vec3 v, Scalar s)
{
    return vec3_ymm_init(v.x * s, v.y * s, v.z * s, v._ * s);
}

VEC_FUNC Vec3 vec3_divs(Vec3 v, Scalar s)
{
    return vec3_ymm_init(v.x / s, v.y / s, v.z / s, v._ / s);
}

VEC_FUNC Vec3 vec3_pows(Vec3 v, Scalar s)
{
    return vec3_init(pow(v.x, s), pow(v.y, s), pow(v.z, s));
}

VEC_FUNC Scalar vec3_dot(Vec3 v1, Vec3 v2)
{
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

VEC_FUNC Vec3 vec3_reflect(Vec3 v, Vec3 n)
{
    return vec3_sub(v, vec3_muls(n, 2.*vec3_dot(n, v)));
}

VEC_FUNC Vec3 vec3_cross(Vec3 v1, Vec3 v2)
{
    return vec3_init(
        v1.y*v2.z - v1.z*v2.y,
        v1.z*v2.x - v1.x*v2.z,
        v1.x*v2.y - v1.y*v2.x);
}

VEC_FUNC Vec3 vec3_abs(Vec3 v)
{
    return vec3_ymm_init(fabs(v.x), fabs(v.y), fabs(v.z), fabs(v._));
}

VEC_FUNC Vec3 vec3_min(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(fmin(v1.x, v2.x), fmin(v1.y, v2.y), fmin(v1.z, v2.z), fmin(v1._, v2._));
}

VEC_FUNC Vec3 vec3_max(Vec3 v1, Vec3 v2)
{
    return vec3_ymm_init(fmax(v1.x, v2.x), fmax(v1.y, v2.y), fmax(v1.z, v2.z), fmax(v1._, v2._));
}

VEC_FUNC Vec3 vec3_clamp(Vec3 v, Vec3 min, Vec3 max)
{
    return vec3_ymm_init(
        fmin(fmax(v.x, min.x), max.x),
        fmin(fmax(v.y, min.y), max.y),
        fmin(fmax(v.z, min.z), max.z),
        fmin(fmax(v._, min._), max._));
}

VEC_FUNC Vec3 vec3_clamps(Vec3 v, Scalar min, Scalar max)
{
    return vec3_ymm_init(
        fmin(fmax(v.x, min), max),
        fmin(fmax(v.y, min), max),
        fmin(fmax(v.z, min), max),
        fmin(fmax(v._, min), max));
}

VEC_FUNC Vec3 vec3_mix(Vec3 v1, Vec3 v2, Vec3 a)
{
    return vec3_init(
        v1.x*(1.f-a.x) + v2.x*a.x,
        v1.y*(1.f-a.y) + v2.y*a.y,
        v1.z*(1.f-a.z) + v2.z*a.z);
}

VEC_FUNC Vec3 vec3_mixs(Vec3 v1, Vec3 v2, Scalar a)
{
    return vec3_init(
        v1.x*(1.f-a) + v2.x*a,
        v1.y*(1.f-a) + v2.y*a,
        v1.z*(1.f-a) + v2.z*a);
}

VEC_FUNC Scalar vec3_length(Vec3 v)
{
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

VEC_FUNC Vec3 vec3_normalize(Vec3 v)
{
    return vec3_divs(v, vec3_length(v));
}

VEC_FUNC Vec3 vec3_inverse_lookat(Vec3 v1, Vec3 v2)
{
    Vec3 r;

    Vec3 f = v2;
    if (f.y == 1 || f.y == -1)
        r = vec3_init(1, 0, 0);
    else
        r = vec3_normalize(vec3_cross(vec3_init(0, 1, 0), f));
    Vec3 u = vec3_cross(f, r);
    Vec3 t = v1;
    v1.x = t.x*r.x + t.y*u.x + t.z*f.x;
    v1.y = t.x*r.y + t.y*u.y + t.z*f.y;
    v1.z = t.x*r.z + t.y*u.z + t.z*f.z;
    return v1;
}

VEC_FUNC Vec3 vec3_lookat(Vec3 v1, Vec3 v2)
{
    Vec3 r;

    Vec3 f = v2;
    if (f.y == 1 || f.y == -1)
        r = vec3_init(1, 0, 0);
    else
        r = vec3_normalize(vec3_cross(vec3_init(0, 1, 0), f));
    Vec3 u = vec3_cross(f, r);
    Vec3 t = v1;
    v1.x = t.x*r.x + t.y*r.y + t.z*r.z;
    v1.y = t.x*u.x + t.y*u.y + t.z*u.z;
    v1.z = t.x*f.x + t.y*f.y + t.z*f.z;
    return v1;
}

VEC_FUNC Vec3 vec3_mat3(Vec3 v, Mat3 m)
{
    return vec3_init(
        v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0],
        v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1],
        v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2]);
}

VEC_FUNC Vec3 mat3_vec3(Mat3 m, Vec3 v)
{
    return vec3_init(
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
}

VEC_FUNC Vec3 vec3_rotatex(Vec3 v, Scalar r)
{
    Scalar m[3][3] =
        {{1, 0, 0}, {0, cos(r), -sin(r)}, {0, sin(r), cos(r)}};

    return vec3_mat3(v, m);
}

VEC_FUNC Vec3 vec3_rotatey(Vec3 v, Scalar r)
{
    Scalar m[3][3] =
        {{cos(r), 0, sin(r)}, {0, 1, 0}, {-sin(r), 0, cos(r)}};

    return vec3_mat3(v, m);
}

VEC_FUNC Vec3 vec3_rotatez(Vec3 v, Scalar r)
{
    Scalar m[3][3] =
        {{cos(r), -sin(r), 0}, {sin(r), cos(r), 0}, {0, 0, 1}};

    return vec3_mat3(v, m);
}

VEC_FUNC Vec3 vec3_axis_rotation(Vec3 v, Vec3 axis, Scalar r)
{
    Scalar m[3][3] = {{
        axis.x*axis.x*(1 - cos(r)) + cos(r),
        axis.x*axis.y*(1 - cos(r)) - axis.z*sin(r),
        axis.x*axis.z*(1 - cos(r)) + axis.y*sin(r)
    }, {
        axis.x*axis.y*(1 - cos(r)) + axis.z*sin(r),
        axis.y*axis.y*(1 - cos(r)) + cos(r),
        axis.y*axis.z*(1 - cos(r)) - axis.x*sin(r)
    }, {
        axis.x*axis.z*(1 - cos(r)) - axis.y*sin(r),
        axis.y*axis.z*(1 - cos(r)) + axis.x*sin(r),
        axis.z*axis.z*(1 - cos(r)) + cos(r)
    }};

    return vec3_mat3(v, m);
}

// ----------------------------------------------------------------------------
// Vec4

VEC_FUNC Vec4 vec4_init(Scalar r, Scalar g, Scalar b, Scalar a)
{
    Vec4 v = { {r}, {g}, {b}, {a} };
    return v;
}

#define vec4_initv2(V, Z, W) vec4_init((V).x, (V).y, Z, W)

#define vec4_initv3(V, A) vec4_init((V).r, (V).g, (V).b, A)

VEC_FUNC Vec4 vec4_inits(Scalar s)
{
    return vec4_init(s, s, s, s);
}

VEC_FUNC Vec4 vec4_initp(const Scalar s[VEC_STATIC 4])
{
    return vec4_init(s[0], s[1], s[2], s[3]);
}

VEC_FUNC Vec4 vec4_add(Vec4 v1, Vec4 v2)
{
    return vec4_init(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

VEC_FUNC Vec4 vec4_sub(Vec4 v1, Vec4 v2)
{
    return vec4_init(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

VEC_FUNC Vec4 vec4_mul(Vec4 v1, Vec4 v2)
{
    return vec4_init(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

VEC_FUNC Vec4 vec4_div(Vec4 v1, Vec4 v2)
{
    return vec4_init(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}

VEC_FUNC Vec4 vec4_pow(Vec4 v1, Vec4 v2)
{
    return vec4_init(pow(v1.x, v2.x), pow(v1.y, v2.y), pow(v1.z, v2.z), pow(v1.w, v2.w));
}

VEC_FUNC Vec4 vec4_adds(Vec4 v, Scalar s)
{
    return vec4_init(v.x + s, v.y + s, v.z + s, v.w + s);
}

VEC_FUNC Vec4 vec4_subs(Vec4 v, Scalar s)
{
    return vec4_init(v.x - s, v.y - s, v.z - s, v.w - s);
}

VEC_FUNC Vec4 vec4_muls(Vec4 v, Scalar s)
{
    return vec4_init(v.x * s, v.y * s, v.z * s, v.w * s);
}

VEC_FUNC Vec4 vec4_divs(Vec4 v, Scalar s)
{
    return vec4_init(v.x / s, v.y / s, v.z / s, v.w / s);
}

VEC_FUNC Vec4 vec4_pows(Vec4 v, Scalar s)
{
    return vec4_init(pow(v.x, s), pow(v.y, s), pow(v.z, s), pow(v.w, s));
}

VEC_FUNC Scalar vec4_dot(Vec4 v1, Vec4 v2)
{
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
}

VEC_FUNC Vec4 vec4_reflect(Vec4 v, Vec4 n)
{
    return vec4_sub(v, vec4_muls(n, 2.*vec4_dot(n, v)));
}

VEC_FUNC Vec4 vec4_abs(Vec4 v)
{
    return vec4_init(fabs(v.x), fabs(v.y), fabs(v.z), fabs(v.w));
}

VEC_FUNC Vec4 vec4_min(Vec4 v1, Vec4 v2)
{
    return vec4_init(fmin(v1.x, v2.x), fmin(v1.y, v2.y), fmin(v1.z, v2.z), fmin(v1.w, v2.w));
}

VEC_FUNC Vec4 vec4_max(Vec4 v1, Vec4 v2)
{
    return vec4_init(fmax(v1.x, v2.x), fmax(v1.y, v2.y), fmax(v1.z, v2.z), fmax(v1.w, v2.w));
}

VEC_FUNC Vec4 vec4_clamp(Vec4 v, Vec4 min, Vec4 max)
{
    return vec4_init(
        fmin(fmax(v.r, min.r), max.r),
        fmin(fmax(v.g, min.g), max.g),
        fmin(fmax(v.b, min.b), max.b),
        fmin(fmax(v.a, min.a), max.a));
}

VEC_FUNC Vec4 vec4_clamps(Vec4 v, Scalar min, Scalar max)
{
    return vec4_init(
        fmin(fmax(v.r, min), max),
        fmin(fmax(v.g, min), max),
        fmin(fmax(v.b, min), max),
        fmin(fmax(v.a, min), max));
}

VEC_FUNC Vec4 vec4_mix(Vec4 v1, Vec4 v2, Vec4 a)
{
    return vec4_init(
        v1.x*(1.f-a.x) + v2.x*a.x,
        v1.y*(1.f-a.y) + v2.y*a.y,
        v1.z*(1.f-a.z) + v2.z*a.z,
        v1.w*(1.f-a.w) + v2.w*a.w);
}

VEC_FUNC Vec4 vec4_mixs(Vec4 v1, Vec4 v2, Scalar a)
{
    return vec4_init(
        v1.x*(1.f-a) + v2.x*a,
        v1.y*(1.f-a) + v2.y*a,
        v1.z*(1.f-a) + v2.z*a,
        v1.w*(1.f-a) + v2.w*a);
}

VEC_FUNC Scalar vec4_length(Vec4 v)
{
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

VEC_FUNC Vec4 vec4_normalize(Vec4 v)
{
    return vec4_divs(v, vec4_length(v));
}

VEC_FUNC Vec4 vec4_mat4(Vec4 v, const Scalar m[4][4])
{
    return vec4_init(
        v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0] + v.w*m[3][0],
        v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1] + v.w*m[3][1],
        v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2] + v.w*m[3][2],
        v.x*m[0][3] + v.y*m[1][3] + v.z*m[2][3] + v.w*m[3][3]);
}

VEC_FUNC Vec4 mat4_vec4(const Scalar m[4][4], Vec4 v)
{
    return vec4_init(
        m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
        m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
        m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
        m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w);
}

// ----------------------------------------------------------------------------
// Mat2

VEC_FUNC Mat2Value mat2_init(
    Scalar m00, Scalar m01,
    Scalar m10, Scalar m11)
{
    Mat2Value m = {{
        {m00, m01},
        {m10, m11},
    }};
    return m;
}

#define mat2_initv2(V1, V2) mat2_init( \
    (V1).x, (V2).x, \
    (V1).y, (V2).y)

VEC_FUNC Mat2Value mat2_init_diagonal(Scalar s)
{
    return mat2_init(
        s, 0,
        0, s);
}

VEC_FUNC Mat2Value mat2_copy(Mat2 A)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j];
    return M;
}

VEC_FUNC Mat2Value mat2_add(Mat2 A, Mat2 B)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] + B[i][j];
    return M;
}

VEC_FUNC Mat2Value mat2_sub(Mat2 A, Mat2 B)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] - B[i][j];
    return M;
}

VEC_FUNC Mat2Value mat2_comp_mul(Mat2 A, Mat2 B)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] * B[i][j];
    return M;
}

VEC_FUNC Mat2Value mat2_mul(Mat2 A, Mat2 B)
{
    Mat2Value M;
    for (int i = 0; i < 2; i++) {
        M.m[i][0] = A[0][0] * B[i][0] + A[1][0] * B[i][1];
        M.m[i][1] = A[0][1] * B[i][0] + A[1][1] * B[i][1];
    }
    return M;
}

VEC_FUNC Mat2Value mat2_div(Mat2 A, Mat2 B)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] / B[i][j];
    return M;
}

VEC_FUNC Mat2Value mat2_adds(Mat2 A, Scalar s)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] + s;
    return M;
}

VEC_FUNC Mat2Value mat2_subs(Mat2 A, Scalar s)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] - s;
    return M;
}

VEC_FUNC Mat2Value mat2_comp_muls(Mat2 A, Scalar s)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] * s;
    return M;
}

VEC_FUNC Mat2Value mat2_divs(Mat2 A, Scalar s)
{
    Mat2Value M;
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2; ++j)
            M.m[i][j] = A[i][j] / s;
    return M;
}

// ----------------------------------------------------------------------------
// Mat3

VEC_FUNC Mat3Value mat3_init(
    Scalar m00, Scalar m01, Scalar m02,
    Scalar m10, Scalar m11, Scalar m12,
    Scalar m20, Scalar m21, Scalar m22)
{
    Mat3Value m = {{
        {m00, m01, m02},
        {m10, m11, m12},
        {m20, m21, m22},
    }};
    return m;
}

#define mat3_initv3(V1, V2, V3) mat3_init( \
    (V1).x, (V2).x, (V2).x, \
    (V1).y, (V2).y, (V2).y, \
    (V1).z, (V2).z, (V2).z)

VEC_FUNC Mat3Value mat3_init_diagonal(Scalar s)
{
    return mat3_init(
        s, 0, 0,
        0, s, 0,
        0, 0, s);
}

VEC_FUNC Mat3Value mat3_copy(Mat3 A)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j];
    return M;
}

VEC_FUNC Mat3Value mat3_add(Mat3 A, Mat3 B)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] + B[i][j];
    return M;
}

VEC_FUNC Mat3Value mat3_sub(Mat3 A, Mat3 B)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] - B[i][j];
    return M;
}

VEC_FUNC Mat3Value mat3_comp_mul(Mat3 A, Mat3 B)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] * B[i][j];
    return M;
}

VEC_FUNC Mat3Value mat3_mul(Mat3 A, Mat3 B)
{
    Mat3Value M;
    for (int i = 0; i < 3; i++) {
        M.m[i][0] = A[0][0] * B[i][0] + A[1][0] * B[i][1] + A[2][0] * B[i][2];
        M.m[i][1] = A[0][1] * B[i][0] + A[1][1] * B[i][1] + A[2][1] * B[i][2];
        M.m[i][2] = A[0][2] * B[i][0] + A[1][2] * B[i][1] + A[2][2] * B[i][2];
    }
    return M;
}

VEC_FUNC Mat3Value mat3_div(Mat3 A, Mat3 B)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] / B[i][j];
    return M;
}

VEC_FUNC Mat3Value mat3_adds(Mat3 A, Scalar s)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] + s;
    return M;
}

VEC_FUNC Mat3Value mat3_subs(Mat3 A, Scalar s)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] - s;
    return M;
}

VEC_FUNC Mat3Value mat3_comp_muls(Mat3 A, Scalar s)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] * s;
    return M;
}

VEC_FUNC Mat3Value mat3_divs(Mat3 A, Scalar s)
{
    Mat3Value M;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            M.m[i][j] = A[i][j] / s;
    return M;
}

// ----------------------------------------------------------------------------
// Mat4

VEC_FUNC Mat4Value mat4_init(
    Scalar m00, Scalar m01, Scalar m02, Scalar m03,
    Scalar m10, Scalar m11, Scalar m12, Scalar m13,
    Scalar m20, Scalar m21, Scalar m22, Scalar m23,
    Scalar m30, Scalar m31, Scalar m32, Scalar m33)
{
    Mat4Value m = {{
        {m00, m01, m02, m03},
        {m10, m11, m12, m13},
        {m20, m21, m22, m23},
        {m30, m31, m32, m33}
    }};
    return m;
}

#define mat4_initv4(V1, V2, V3, V4) mat4_init( \
    (V1).x, (V2).x, (V3).x, (V4).x, \
    (V1).y, (V2).y, (V3).y, (V4).y, \
    (V1).z, (V2).z, (V3).z, (V4).z, \
    (V1).w, (V2).w, (V3).w, (V4).w)

VEC_FUNC Mat4Value mat4_init_diagonal(Scalar s)
{
    return mat4_init(
        s, 0, 0, 0,
        0, s, 0, 0,
        0, 0, s, 0,
        0, 0, 0, s);
}

VEC_FUNC Mat4Value mat4_copy(Mat4 A)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j];
    return M;
}

VEC_FUNC Mat4Value mat4_add(Mat4 A, Mat4 B)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] + B[i][j];
    return M;
}

VEC_FUNC Mat4Value mat4_sub(Mat4 A, Mat4 B)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] - B[i][j];
    return M;
}

VEC_FUNC Mat4Value mat4_comp_mul(Mat4 A, Mat4 B)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] * B[i][j];
    return M;
}

VEC_FUNC Mat4Value mat4_mul(Mat4 A, Mat4 B)
{
    Mat4Value M;
    for (int i = 0; i < 4; i++) {
        M.m[i][0] = A[0][0] * B[i][0] + A[1][0] * B[i][1] + A[2][0] * B[i][2];
        M.m[i][1] = A[0][1] * B[i][0] + A[1][1] * B[i][1] + A[2][1] * B[i][2];
        M.m[i][2] = A[0][2] * B[i][0] + A[1][2] * B[i][1] + A[2][2] * B[i][2];
        M.m[i][3] = A[0][3] * B[i][0] + A[1][3] * B[i][1] + A[2][3] * B[i][2];
    }
    return M;
}

VEC_FUNC Mat4Value mat4_div(Mat4 A, Mat4 B)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] / B[i][j];
    return M;
}

VEC_FUNC Mat4Value mat4_adds(Mat4 A, Scalar s)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] + s;
    return M;
}

VEC_FUNC Mat4Value mat4_subs(Mat4 A, Scalar s)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] - s;
    return M;
}

VEC_FUNC Mat4Value mat4_comp_muls(Mat4 A, Scalar s)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] * s;
    return M;
}

VEC_FUNC Mat4Value mat4_divs(Mat4 A, Scalar s)
{
    Mat4Value M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.m[i][j] = A[i][j] / s;
    return M;
}

// ----------------------------------------------------------------------------
// Swizzling

#define vec_x(V) ((V).x)
#define vec_y(V) ((V).y)
#define vec_z(V) ((V).z)
#define vec_w(V) ((V).w)
#define vec_xx(V) vec2((V).x, (V).x)
#define vec_xy(V) vec2((V).x, (V).y)
#define vec_xz(V) vec2((V).x, (V).z)
#define vec_xw(V) vec2((V).x, (V).w)
#define vec_yx(V) vec2((V).y, (V).x)
#define vec_yy(V) vec2((V).y, (V).y)
#define vec_yz(V) vec2((V).y, (V).z)
#define vec_yw(V) vec2((V).y, (V).w)
#define vec_zx(V) vec2((V).z, (V).x)
#define vec_zy(V) vec2((V).z, (V).y)
#define vec_zz(V) vec2((V).z, (V).z)
#define vec_zw(V) vec2((V).z, (V).w)
#define vec_wx(V) vec2((V).w, (V).x)
#define vec_wy(V) vec2((V).w, (V).y)
#define vec_wz(V) vec2((V).w, (V).z)
#define vec_ww(V) vec2((V).w, (V).w)
#define vec_xxx(V) vec3((V).x, (V).x, (V).x)
#define vec_xxy(V) vec3((V).x, (V).x, (V).y)
#define vec_xxz(V) vec3((V).x, (V).x, (V).z)
#define vec_xxw(V) vec3((V).x, (V).x, (V).w)
#define vec_xyx(V) vec3((V).x, (V).y, (V).x)
#define vec_xyy(V) vec3((V).x, (V).y, (V).y)
#define vec_xyz(V) vec3((V).x, (V).y, (V).z)
#define vec_xyw(V) vec3((V).x, (V).y, (V).w)
#define vec_xzx(V) vec3((V).x, (V).z, (V).x)
#define vec_xzy(V) vec3((V).x, (V).z, (V).y)
#define vec_xzz(V) vec3((V).x, (V).z, (V).z)
#define vec_xzw(V) vec3((V).x, (V).z, (V).w)
#define vec_xwx(V) vec3((V).x, (V).w, (V).x)
#define vec_xwy(V) vec3((V).x, (V).w, (V).y)
#define vec_xwz(V) vec3((V).x, (V).w, (V).z)
#define vec_xww(V) vec3((V).x, (V).w, (V).w)
#define vec_yxx(V) vec3((V).y, (V).x, (V).x)
#define vec_yxy(V) vec3((V).y, (V).x, (V).y)
#define vec_yxz(V) vec3((V).y, (V).x, (V).z)
#define vec_yxw(V) vec3((V).y, (V).x, (V).w)
#define vec_yyx(V) vec3((V).y, (V).y, (V).x)
#define vec_yyy(V) vec3((V).y, (V).y, (V).y)
#define vec_yyz(V) vec3((V).y, (V).y, (V).z)
#define vec_yyw(V) vec3((V).y, (V).y, (V).w)
#define vec_yzx(V) vec3((V).y, (V).z, (V).x)
#define vec_yzy(V) vec3((V).y, (V).z, (V).y)
#define vec_yzz(V) vec3((V).y, (V).z, (V).z)
#define vec_yzw(V) vec3((V).y, (V).z, (V).w)
#define vec_ywx(V) vec3((V).y, (V).w, (V).x)
#define vec_ywy(V) vec3((V).y, (V).w, (V).y)
#define vec_ywz(V) vec3((V).y, (V).w, (V).z)
#define vec_yww(V) vec3((V).y, (V).w, (V).w)
#define vec_zxx(V) vec3((V).z, (V).x, (V).x)
#define vec_zxy(V) vec3((V).z, (V).x, (V).y)
#define vec_zxz(V) vec3((V).z, (V).x, (V).z)
#define vec_zxw(V) vec3((V).z, (V).x, (V).w)
#define vec_zyx(V) vec3((V).z, (V).y, (V).x)
#define vec_zyy(V) vec3((V).z, (V).y, (V).y)
#define vec_zyz(V) vec3((V).z, (V).y, (V).z)
#define vec_zyw(V) vec3((V).z, (V).y, (V).w)
#define vec_zzx(V) vec3((V).z, (V).z, (V).x)
#define vec_zzy(V) vec3((V).z, (V).z, (V).y)
#define vec_zzz(V) vec3((V).z, (V).z, (V).z)
#define vec_zzw(V) vec3((V).z, (V).z, (V).w)
#define vec_zwx(V) vec3((V).z, (V).w, (V).x)
#define vec_zwy(V) vec3((V).z, (V).w, (V).y)
#define vec_zwz(V) vec3((V).z, (V).w, (V).z)
#define vec_zww(V) vec3((V).z, (V).w, (V).w)
#define vec_wxx(V) vec3((V).w, (V).x, (V).x)
#define vec_wxy(V) vec3((V).w, (V).x, (V).y)
#define vec_wxz(V) vec3((V).w, (V).x, (V).z)
#define vec_wxw(V) vec3((V).w, (V).x, (V).w)
#define vec_wyx(V) vec3((V).w, (V).y, (V).x)
#define vec_wyy(V) vec3((V).w, (V).y, (V).y)
#define vec_wyz(V) vec3((V).w, (V).y, (V).z)
#define vec_wyw(V) vec3((V).w, (V).y, (V).w)
#define vec_wzx(V) vec3((V).w, (V).z, (V).x)
#define vec_wzy(V) vec3((V).w, (V).z, (V).y)
#define vec_wzz(V) vec3((V).w, (V).z, (V).z)
#define vec_wzw(V) vec3((V).w, (V).z, (V).w)
#define vec_wwx(V) vec3((V).w, (V).w, (V).x)
#define vec_wwy(V) vec3((V).w, (V).w, (V).y)
#define vec_wwz(V) vec3((V).w, (V).w, (V).z)
#define vec_www(V) vec3((V).w, (V).w, (V).w)
#define vec_xxxx(V) vec4((V).x, (V).x, (V).x, (V).x)
#define vec_xxxy(V) vec4((V).x, (V).x, (V).x, (V).y)
#define vec_xxxz(V) vec4((V).x, (V).x, (V).x, (V).z)
#define vec_xxxw(V) vec4((V).x, (V).x, (V).x, (V).w)
#define vec_xxyx(V) vec4((V).x, (V).x, (V).y, (V).x)
#define vec_xxyy(V) vec4((V).x, (V).x, (V).y, (V).y)
#define vec_xxyz(V) vec4((V).x, (V).x, (V).y, (V).z)
#define vec_xxyw(V) vec4((V).x, (V).x, (V).y, (V).w)
#define vec_xxzx(V) vec4((V).x, (V).x, (V).z, (V).x)
#define vec_xxzy(V) vec4((V).x, (V).x, (V).z, (V).y)
#define vec_xxzz(V) vec4((V).x, (V).x, (V).z, (V).z)
#define vec_xxzw(V) vec4((V).x, (V).x, (V).z, (V).w)
#define vec_xxwx(V) vec4((V).x, (V).x, (V).w, (V).x)
#define vec_xxwy(V) vec4((V).x, (V).x, (V).w, (V).y)
#define vec_xxwz(V) vec4((V).x, (V).x, (V).w, (V).z)
#define vec_xxww(V) vec4((V).x, (V).x, (V).w, (V).w)
#define vec_xyxx(V) vec4((V).x, (V).y, (V).x, (V).x)
#define vec_xyxy(V) vec4((V).x, (V).y, (V).x, (V).y)
#define vec_xyxz(V) vec4((V).x, (V).y, (V).x, (V).z)
#define vec_xyxw(V) vec4((V).x, (V).y, (V).x, (V).w)
#define vec_xyyx(V) vec4((V).x, (V).y, (V).y, (V).x)
#define vec_xyyy(V) vec4((V).x, (V).y, (V).y, (V).y)
#define vec_xyyz(V) vec4((V).x, (V).y, (V).y, (V).z)
#define vec_xyyw(V) vec4((V).x, (V).y, (V).y, (V).w)
#define vec_xyzx(V) vec4((V).x, (V).y, (V).z, (V).x)
#define vec_xyzy(V) vec4((V).x, (V).y, (V).z, (V).y)
#define vec_xyzz(V) vec4((V).x, (V).y, (V).z, (V).z)
#define vec_xyzw(V) vec4((V).x, (V).y, (V).z, (V).w)
#define vec_xywx(V) vec4((V).x, (V).y, (V).w, (V).x)
#define vec_xywy(V) vec4((V).x, (V).y, (V).w, (V).y)
#define vec_xywz(V) vec4((V).x, (V).y, (V).w, (V).z)
#define vec_xyww(V) vec4((V).x, (V).y, (V).w, (V).w)
#define vec_xzxx(V) vec4((V).x, (V).z, (V).x, (V).x)
#define vec_xzxy(V) vec4((V).x, (V).z, (V).x, (V).y)
#define vec_xzxz(V) vec4((V).x, (V).z, (V).x, (V).z)
#define vec_xzxw(V) vec4((V).x, (V).z, (V).x, (V).w)
#define vec_xzyx(V) vec4((V).x, (V).z, (V).y, (V).x)
#define vec_xzyy(V) vec4((V).x, (V).z, (V).y, (V).y)
#define vec_xzyz(V) vec4((V).x, (V).z, (V).y, (V).z)
#define vec_xzyw(V) vec4((V).x, (V).z, (V).y, (V).w)
#define vec_xzzx(V) vec4((V).x, (V).z, (V).z, (V).x)
#define vec_xzzy(V) vec4((V).x, (V).z, (V).z, (V).y)
#define vec_xzzz(V) vec4((V).x, (V).z, (V).z, (V).z)
#define vec_xzzw(V) vec4((V).x, (V).z, (V).z, (V).w)
#define vec_xzwx(V) vec4((V).x, (V).z, (V).w, (V).x)
#define vec_xzwy(V) vec4((V).x, (V).z, (V).w, (V).y)
#define vec_xzwz(V) vec4((V).x, (V).z, (V).w, (V).z)
#define vec_xzww(V) vec4((V).x, (V).z, (V).w, (V).w)
#define vec_xwxx(V) vec4((V).x, (V).w, (V).x, (V).x)
#define vec_xwxy(V) vec4((V).x, (V).w, (V).x, (V).y)
#define vec_xwxz(V) vec4((V).x, (V).w, (V).x, (V).z)
#define vec_xwxw(V) vec4((V).x, (V).w, (V).x, (V).w)
#define vec_xwyx(V) vec4((V).x, (V).w, (V).y, (V).x)
#define vec_xwyy(V) vec4((V).x, (V).w, (V).y, (V).y)
#define vec_xwyz(V) vec4((V).x, (V).w, (V).y, (V).z)
#define vec_xwyw(V) vec4((V).x, (V).w, (V).y, (V).w)
#define vec_xwzx(V) vec4((V).x, (V).w, (V).z, (V).x)
#define vec_xwzy(V) vec4((V).x, (V).w, (V).z, (V).y)
#define vec_xwzz(V) vec4((V).x, (V).w, (V).z, (V).z)
#define vec_xwzw(V) vec4((V).x, (V).w, (V).z, (V).w)
#define vec_xwwx(V) vec4((V).x, (V).w, (V).w, (V).x)
#define vec_xwwy(V) vec4((V).x, (V).w, (V).w, (V).y)
#define vec_xwwz(V) vec4((V).x, (V).w, (V).w, (V).z)
#define vec_xwww(V) vec4((V).x, (V).w, (V).w, (V).w)
#define vec_yxxx(V) vec4((V).y, (V).x, (V).x, (V).x)
#define vec_yxxy(V) vec4((V).y, (V).x, (V).x, (V).y)
#define vec_yxxz(V) vec4((V).y, (V).x, (V).x, (V).z)
#define vec_yxxw(V) vec4((V).y, (V).x, (V).x, (V).w)
#define vec_yxyx(V) vec4((V).y, (V).x, (V).y, (V).x)
#define vec_yxyy(V) vec4((V).y, (V).x, (V).y, (V).y)
#define vec_yxyz(V) vec4((V).y, (V).x, (V).y, (V).z)
#define vec_yxyw(V) vec4((V).y, (V).x, (V).y, (V).w)
#define vec_yxzx(V) vec4((V).y, (V).x, (V).z, (V).x)
#define vec_yxzy(V) vec4((V).y, (V).x, (V).z, (V).y)
#define vec_yxzz(V) vec4((V).y, (V).x, (V).z, (V).z)
#define vec_yxzw(V) vec4((V).y, (V).x, (V).z, (V).w)
#define vec_yxwx(V) vec4((V).y, (V).x, (V).w, (V).x)
#define vec_yxwy(V) vec4((V).y, (V).x, (V).w, (V).y)
#define vec_yxwz(V) vec4((V).y, (V).x, (V).w, (V).z)
#define vec_yxww(V) vec4((V).y, (V).x, (V).w, (V).w)
#define vec_yyxx(V) vec4((V).y, (V).y, (V).x, (V).x)
#define vec_yyxy(V) vec4((V).y, (V).y, (V).x, (V).y)
#define vec_yyxz(V) vec4((V).y, (V).y, (V).x, (V).z)
#define vec_yyxw(V) vec4((V).y, (V).y, (V).x, (V).w)
#define vec_yyyx(V) vec4((V).y, (V).y, (V).y, (V).x)
#define vec_yyyy(V) vec4((V).y, (V).y, (V).y, (V).y)
#define vec_yyyz(V) vec4((V).y, (V).y, (V).y, (V).z)
#define vec_yyyw(V) vec4((V).y, (V).y, (V).y, (V).w)
#define vec_yyzx(V) vec4((V).y, (V).y, (V).z, (V).x)
#define vec_yyzy(V) vec4((V).y, (V).y, (V).z, (V).y)
#define vec_yyzz(V) vec4((V).y, (V).y, (V).z, (V).z)
#define vec_yyzw(V) vec4((V).y, (V).y, (V).z, (V).w)
#define vec_yywx(V) vec4((V).y, (V).y, (V).w, (V).x)
#define vec_yywy(V) vec4((V).y, (V).y, (V).w, (V).y)
#define vec_yywz(V) vec4((V).y, (V).y, (V).w, (V).z)
#define vec_yyww(V) vec4((V).y, (V).y, (V).w, (V).w)
#define vec_yzxx(V) vec4((V).y, (V).z, (V).x, (V).x)
#define vec_yzxy(V) vec4((V).y, (V).z, (V).x, (V).y)
#define vec_yzxz(V) vec4((V).y, (V).z, (V).x, (V).z)
#define vec_yzxw(V) vec4((V).y, (V).z, (V).x, (V).w)
#define vec_yzyx(V) vec4((V).y, (V).z, (V).y, (V).x)
#define vec_yzyy(V) vec4((V).y, (V).z, (V).y, (V).y)
#define vec_yzyz(V) vec4((V).y, (V).z, (V).y, (V).z)
#define vec_yzyw(V) vec4((V).y, (V).z, (V).y, (V).w)
#define vec_yzzx(V) vec4((V).y, (V).z, (V).z, (V).x)
#define vec_yzzy(V) vec4((V).y, (V).z, (V).z, (V).y)
#define vec_yzzz(V) vec4((V).y, (V).z, (V).z, (V).z)
#define vec_yzzw(V) vec4((V).y, (V).z, (V).z, (V).w)
#define vec_yzwx(V) vec4((V).y, (V).z, (V).w, (V).x)
#define vec_yzwy(V) vec4((V).y, (V).z, (V).w, (V).y)
#define vec_yzwz(V) vec4((V).y, (V).z, (V).w, (V).z)
#define vec_yzww(V) vec4((V).y, (V).z, (V).w, (V).w)
#define vec_ywxx(V) vec4((V).y, (V).w, (V).x, (V).x)
#define vec_ywxy(V) vec4((V).y, (V).w, (V).x, (V).y)
#define vec_ywxz(V) vec4((V).y, (V).w, (V).x, (V).z)
#define vec_ywxw(V) vec4((V).y, (V).w, (V).x, (V).w)
#define vec_ywyx(V) vec4((V).y, (V).w, (V).y, (V).x)
#define vec_ywyy(V) vec4((V).y, (V).w, (V).y, (V).y)
#define vec_ywyz(V) vec4((V).y, (V).w, (V).y, (V).z)
#define vec_ywyw(V) vec4((V).y, (V).w, (V).y, (V).w)
#define vec_ywzx(V) vec4((V).y, (V).w, (V).z, (V).x)
#define vec_ywzy(V) vec4((V).y, (V).w, (V).z, (V).y)
#define vec_ywzz(V) vec4((V).y, (V).w, (V).z, (V).z)
#define vec_ywzw(V) vec4((V).y, (V).w, (V).z, (V).w)
#define vec_ywwx(V) vec4((V).y, (V).w, (V).w, (V).x)
#define vec_ywwy(V) vec4((V).y, (V).w, (V).w, (V).y)
#define vec_ywwz(V) vec4((V).y, (V).w, (V).w, (V).z)
#define vec_ywww(V) vec4((V).y, (V).w, (V).w, (V).w)
#define vec_zxxx(V) vec4((V).z, (V).x, (V).x, (V).x)
#define vec_zxxy(V) vec4((V).z, (V).x, (V).x, (V).y)
#define vec_zxxz(V) vec4((V).z, (V).x, (V).x, (V).z)
#define vec_zxxw(V) vec4((V).z, (V).x, (V).x, (V).w)
#define vec_zxyx(V) vec4((V).z, (V).x, (V).y, (V).x)
#define vec_zxyy(V) vec4((V).z, (V).x, (V).y, (V).y)
#define vec_zxyz(V) vec4((V).z, (V).x, (V).y, (V).z)
#define vec_zxyw(V) vec4((V).z, (V).x, (V).y, (V).w)
#define vec_zxzx(V) vec4((V).z, (V).x, (V).z, (V).x)
#define vec_zxzy(V) vec4((V).z, (V).x, (V).z, (V).y)
#define vec_zxzz(V) vec4((V).z, (V).x, (V).z, (V).z)
#define vec_zxzw(V) vec4((V).z, (V).x, (V).z, (V).w)
#define vec_zxwx(V) vec4((V).z, (V).x, (V).w, (V).x)
#define vec_zxwy(V) vec4((V).z, (V).x, (V).w, (V).y)
#define vec_zxwz(V) vec4((V).z, (V).x, (V).w, (V).z)
#define vec_zxww(V) vec4((V).z, (V).x, (V).w, (V).w)
#define vec_zyxx(V) vec4((V).z, (V).y, (V).x, (V).x)
#define vec_zyxy(V) vec4((V).z, (V).y, (V).x, (V).y)
#define vec_zyxz(V) vec4((V).z, (V).y, (V).x, (V).z)
#define vec_zyxw(V) vec4((V).z, (V).y, (V).x, (V).w)
#define vec_zyyx(V) vec4((V).z, (V).y, (V).y, (V).x)
#define vec_zyyy(V) vec4((V).z, (V).y, (V).y, (V).y)
#define vec_zyyz(V) vec4((V).z, (V).y, (V).y, (V).z)
#define vec_zyyw(V) vec4((V).z, (V).y, (V).y, (V).w)
#define vec_zyzx(V) vec4((V).z, (V).y, (V).z, (V).x)
#define vec_zyzy(V) vec4((V).z, (V).y, (V).z, (V).y)
#define vec_zyzz(V) vec4((V).z, (V).y, (V).z, (V).z)
#define vec_zyzw(V) vec4((V).z, (V).y, (V).z, (V).w)
#define vec_zywx(V) vec4((V).z, (V).y, (V).w, (V).x)
#define vec_zywy(V) vec4((V).z, (V).y, (V).w, (V).y)
#define vec_zywz(V) vec4((V).z, (V).y, (V).w, (V).z)
#define vec_zyww(V) vec4((V).z, (V).y, (V).w, (V).w)
#define vec_zzxx(V) vec4((V).z, (V).z, (V).x, (V).x)
#define vec_zzxy(V) vec4((V).z, (V).z, (V).x, (V).y)
#define vec_zzxz(V) vec4((V).z, (V).z, (V).x, (V).z)
#define vec_zzxw(V) vec4((V).z, (V).z, (V).x, (V).w)
#define vec_zzyx(V) vec4((V).z, (V).z, (V).y, (V).x)
#define vec_zzyy(V) vec4((V).z, (V).z, (V).y, (V).y)
#define vec_zzyz(V) vec4((V).z, (V).z, (V).y, (V).z)
#define vec_zzyw(V) vec4((V).z, (V).z, (V).y, (V).w)
#define vec_zzzx(V) vec4((V).z, (V).z, (V).z, (V).x)
#define vec_zzzy(V) vec4((V).z, (V).z, (V).z, (V).y)
#define vec_zzzz(V) vec4((V).z, (V).z, (V).z, (V).z)
#define vec_zzzw(V) vec4((V).z, (V).z, (V).z, (V).w)
#define vec_zzwx(V) vec4((V).z, (V).z, (V).w, (V).x)
#define vec_zzwy(V) vec4((V).z, (V).z, (V).w, (V).y)
#define vec_zzwz(V) vec4((V).z, (V).z, (V).w, (V).z)
#define vec_zzww(V) vec4((V).z, (V).z, (V).w, (V).w)
#define vec_zwxx(V) vec4((V).z, (V).w, (V).x, (V).x)
#define vec_zwxy(V) vec4((V).z, (V).w, (V).x, (V).y)
#define vec_zwxz(V) vec4((V).z, (V).w, (V).x, (V).z)
#define vec_zwxw(V) vec4((V).z, (V).w, (V).x, (V).w)
#define vec_zwyx(V) vec4((V).z, (V).w, (V).y, (V).x)
#define vec_zwyy(V) vec4((V).z, (V).w, (V).y, (V).y)
#define vec_zwyz(V) vec4((V).z, (V).w, (V).y, (V).z)
#define vec_zwyw(V) vec4((V).z, (V).w, (V).y, (V).w)
#define vec_zwzx(V) vec4((V).z, (V).w, (V).z, (V).x)
#define vec_zwzy(V) vec4((V).z, (V).w, (V).z, (V).y)
#define vec_zwzz(V) vec4((V).z, (V).w, (V).z, (V).z)
#define vec_zwzw(V) vec4((V).z, (V).w, (V).z, (V).w)
#define vec_zwwx(V) vec4((V).z, (V).w, (V).w, (V).x)
#define vec_zwwy(V) vec4((V).z, (V).w, (V).w, (V).y)
#define vec_zwwz(V) vec4((V).z, (V).w, (V).w, (V).z)
#define vec_zwww(V) vec4((V).z, (V).w, (V).w, (V).w)
#define vec_wxxx(V) vec4((V).w, (V).x, (V).x, (V).x)
#define vec_wxxy(V) vec4((V).w, (V).x, (V).x, (V).y)
#define vec_wxxz(V) vec4((V).w, (V).x, (V).x, (V).z)
#define vec_wxxw(V) vec4((V).w, (V).x, (V).x, (V).w)
#define vec_wxyx(V) vec4((V).w, (V).x, (V).y, (V).x)
#define vec_wxyy(V) vec4((V).w, (V).x, (V).y, (V).y)
#define vec_wxyz(V) vec4((V).w, (V).x, (V).y, (V).z)
#define vec_wxyw(V) vec4((V).w, (V).x, (V).y, (V).w)
#define vec_wxzx(V) vec4((V).w, (V).x, (V).z, (V).x)
#define vec_wxzy(V) vec4((V).w, (V).x, (V).z, (V).y)
#define vec_wxzz(V) vec4((V).w, (V).x, (V).z, (V).z)
#define vec_wxzw(V) vec4((V).w, (V).x, (V).z, (V).w)
#define vec_wxwx(V) vec4((V).w, (V).x, (V).w, (V).x)
#define vec_wxwy(V) vec4((V).w, (V).x, (V).w, (V).y)
#define vec_wxwz(V) vec4((V).w, (V).x, (V).w, (V).z)
#define vec_wxww(V) vec4((V).w, (V).x, (V).w, (V).w)
#define vec_wyxx(V) vec4((V).w, (V).y, (V).x, (V).x)
#define vec_wyxy(V) vec4((V).w, (V).y, (V).x, (V).y)
#define vec_wyxz(V) vec4((V).w, (V).y, (V).x, (V).z)
#define vec_wyxw(V) vec4((V).w, (V).y, (V).x, (V).w)
#define vec_wyyx(V) vec4((V).w, (V).y, (V).y, (V).x)
#define vec_wyyy(V) vec4((V).w, (V).y, (V).y, (V).y)
#define vec_wyyz(V) vec4((V).w, (V).y, (V).y, (V).z)
#define vec_wyyw(V) vec4((V).w, (V).y, (V).y, (V).w)
#define vec_wyzx(V) vec4((V).w, (V).y, (V).z, (V).x)
#define vec_wyzy(V) vec4((V).w, (V).y, (V).z, (V).y)
#define vec_wyzz(V) vec4((V).w, (V).y, (V).z, (V).z)
#define vec_wyzw(V) vec4((V).w, (V).y, (V).z, (V).w)
#define vec_wywx(V) vec4((V).w, (V).y, (V).w, (V).x)
#define vec_wywy(V) vec4((V).w, (V).y, (V).w, (V).y)
#define vec_wywz(V) vec4((V).w, (V).y, (V).w, (V).z)
#define vec_wyww(V) vec4((V).w, (V).y, (V).w, (V).w)
#define vec_wzxx(V) vec4((V).w, (V).z, (V).x, (V).x)
#define vec_wzxy(V) vec4((V).w, (V).z, (V).x, (V).y)
#define vec_wzxz(V) vec4((V).w, (V).z, (V).x, (V).z)
#define vec_wzxw(V) vec4((V).w, (V).z, (V).x, (V).w)
#define vec_wzyx(V) vec4((V).w, (V).z, (V).y, (V).x)
#define vec_wzyy(V) vec4((V).w, (V).z, (V).y, (V).y)
#define vec_wzyz(V) vec4((V).w, (V).z, (V).y, (V).z)
#define vec_wzyw(V) vec4((V).w, (V).z, (V).y, (V).w)
#define vec_wzzx(V) vec4((V).w, (V).z, (V).z, (V).x)
#define vec_wzzy(V) vec4((V).w, (V).z, (V).z, (V).y)
#define vec_wzzz(V) vec4((V).w, (V).z, (V).z, (V).z)
#define vec_wzzw(V) vec4((V).w, (V).z, (V).z, (V).w)
#define vec_wzwx(V) vec4((V).w, (V).z, (V).w, (V).x)
#define vec_wzwy(V) vec4((V).w, (V).z, (V).w, (V).y)
#define vec_wzwz(V) vec4((V).w, (V).z, (V).w, (V).z)
#define vec_wzww(V) vec4((V).w, (V).z, (V).w, (V).w)
#define vec_wwxx(V) vec4((V).w, (V).w, (V).x, (V).x)
#define vec_wwxy(V) vec4((V).w, (V).w, (V).x, (V).y)
#define vec_wwxz(V) vec4((V).w, (V).w, (V).x, (V).z)
#define vec_wwxw(V) vec4((V).w, (V).w, (V).x, (V).w)
#define vec_wwyx(V) vec4((V).w, (V).w, (V).y, (V).x)
#define vec_wwyy(V) vec4((V).w, (V).w, (V).y, (V).y)
#define vec_wwyz(V) vec4((V).w, (V).w, (V).y, (V).z)
#define vec_wwyw(V) vec4((V).w, (V).w, (V).y, (V).w)
#define vec_wwzx(V) vec4((V).w, (V).w, (V).z, (V).x)
#define vec_wwzy(V) vec4((V).w, (V).w, (V).z, (V).y)
#define vec_wwzz(V) vec4((V).w, (V).w, (V).z, (V).z)
#define vec_wwzw(V) vec4((V).w, (V).w, (V).z, (V).w)
#define vec_wwwx(V) vec4((V).w, (V).w, (V).w, (V).x)
#define vec_wwwy(V) vec4((V).w, (V).w, (V).w, (V).y)
#define vec_wwwz(V) vec4((V).w, (V).w, (V).w, (V).z)
#define vec_wwww(V) vec4((V).w, (V).w, (V).w, (V).w)

#define vec_r(V) ((V).r)
#define vec_g(V) ((V).g)
#define vec_b(V) ((V).b)
#define vec_a(V) ((V).a)
#define vec_rr(V) vec2((V).r, (V).r)
#define vec_rg(V) vec2((V).r, (V).g)
#define vec_rb(V) vec2((V).r, (V).b)
#define vec_ra(V) vec2((V).r, (V).a)
#define vec_gr(V) vec2((V).g, (V).r)
#define vec_gg(V) vec2((V).g, (V).g)
#define vec_gb(V) vec2((V).g, (V).b)
#define vec_ga(V) vec2((V).g, (V).a)
#define vec_br(V) vec2((V).b, (V).r)
#define vec_bg(V) vec2((V).b, (V).g)
#define vec_bb(V) vec2((V).b, (V).b)
#define vec_ba(V) vec2((V).b, (V).a)
#define vec_ar(V) vec2((V).a, (V).r)
#define vec_ag(V) vec2((V).a, (V).g)
#define vec_ab(V) vec2((V).a, (V).b)
#define vec_aa(V) vec2((V).a, (V).a)
#define vec_rrr(V) vec3((V).r, (V).r, (V).r)
#define vec_rrg(V) vec3((V).r, (V).r, (V).g)
#define vec_rrb(V) vec3((V).r, (V).r, (V).b)
#define vec_rra(V) vec3((V).r, (V).r, (V).a)
#define vec_rgr(V) vec3((V).r, (V).g, (V).r)
#define vec_rgg(V) vec3((V).r, (V).g, (V).g)
#define vec_rgb(V) vec3((V).r, (V).g, (V).b)
#define vec_rga(V) vec3((V).r, (V).g, (V).a)
#define vec_rbr(V) vec3((V).r, (V).b, (V).r)
#define vec_rbg(V) vec3((V).r, (V).b, (V).g)
#define vec_rbb(V) vec3((V).r, (V).b, (V).b)
#define vec_rba(V) vec3((V).r, (V).b, (V).a)
#define vec_rar(V) vec3((V).r, (V).a, (V).r)
#define vec_rag(V) vec3((V).r, (V).a, (V).g)
#define vec_rab(V) vec3((V).r, (V).a, (V).b)
#define vec_raa(V) vec3((V).r, (V).a, (V).a)
#define vec_grr(V) vec3((V).g, (V).r, (V).r)
#define vec_grg(V) vec3((V).g, (V).r, (V).g)
#define vec_grb(V) vec3((V).g, (V).r, (V).b)
#define vec_gra(V) vec3((V).g, (V).r, (V).a)
#define vec_ggr(V) vec3((V).g, (V).g, (V).r)
#define vec_ggg(V) vec3((V).g, (V).g, (V).g)
#define vec_ggb(V) vec3((V).g, (V).g, (V).b)
#define vec_gga(V) vec3((V).g, (V).g, (V).a)
#define vec_gbr(V) vec3((V).g, (V).b, (V).r)
#define vec_gbg(V) vec3((V).g, (V).b, (V).g)
#define vec_gbb(V) vec3((V).g, (V).b, (V).b)
#define vec_gba(V) vec3((V).g, (V).b, (V).a)
#define vec_gar(V) vec3((V).g, (V).a, (V).r)
#define vec_gag(V) vec3((V).g, (V).a, (V).g)
#define vec_gab(V) vec3((V).g, (V).a, (V).b)
#define vec_gaa(V) vec3((V).g, (V).a, (V).a)
#define vec_brr(V) vec3((V).b, (V).r, (V).r)
#define vec_brg(V) vec3((V).b, (V).r, (V).g)
#define vec_brb(V) vec3((V).b, (V).r, (V).b)
#define vec_bra(V) vec3((V).b, (V).r, (V).a)
#define vec_bgr(V) vec3((V).b, (V).g, (V).r)
#define vec_bgg(V) vec3((V).b, (V).g, (V).g)
#define vec_bgb(V) vec3((V).b, (V).g, (V).b)
#define vec_bga(V) vec3((V).b, (V).g, (V).a)
#define vec_bbr(V) vec3((V).b, (V).b, (V).r)
#define vec_bbg(V) vec3((V).b, (V).b, (V).g)
#define vec_bbb(V) vec3((V).b, (V).b, (V).b)
#define vec_bba(V) vec3((V).b, (V).b, (V).a)
#define vec_bar(V) vec3((V).b, (V).a, (V).r)
#define vec_bag(V) vec3((V).b, (V).a, (V).g)
#define vec_bab(V) vec3((V).b, (V).a, (V).b)
#define vec_baa(V) vec3((V).b, (V).a, (V).a)
#define vec_arr(V) vec3((V).a, (V).r, (V).r)
#define vec_arg(V) vec3((V).a, (V).r, (V).g)
#define vec_arb(V) vec3((V).a, (V).r, (V).b)
#define vec_ara(V) vec3((V).a, (V).r, (V).a)
#define vec_agr(V) vec3((V).a, (V).g, (V).r)
#define vec_agg(V) vec3((V).a, (V).g, (V).g)
#define vec_agb(V) vec3((V).a, (V).g, (V).b)
#define vec_aga(V) vec3((V).a, (V).g, (V).a)
#define vec_abr(V) vec3((V).a, (V).b, (V).r)
#define vec_abg(V) vec3((V).a, (V).b, (V).g)
#define vec_abb(V) vec3((V).a, (V).b, (V).b)
#define vec_aba(V) vec3((V).a, (V).b, (V).a)
#define vec_aar(V) vec3((V).a, (V).a, (V).r)
#define vec_aag(V) vec3((V).a, (V).a, (V).g)
#define vec_aab(V) vec3((V).a, (V).a, (V).b)
#define vec_aaa(V) vec3((V).a, (V).a, (V).a)
#define vec_rrrr(V) vec4((V).r, (V).r, (V).r, (V).r)
#define vec_rrrg(V) vec4((V).r, (V).r, (V).r, (V).g)
#define vec_rrrb(V) vec4((V).r, (V).r, (V).r, (V).b)
#define vec_rrra(V) vec4((V).r, (V).r, (V).r, (V).a)
#define vec_rrgr(V) vec4((V).r, (V).r, (V).g, (V).r)
#define vec_rrgg(V) vec4((V).r, (V).r, (V).g, (V).g)
#define vec_rrgb(V) vec4((V).r, (V).r, (V).g, (V).b)
#define vec_rrga(V) vec4((V).r, (V).r, (V).g, (V).a)
#define vec_rrbr(V) vec4((V).r, (V).r, (V).b, (V).r)
#define vec_rrbg(V) vec4((V).r, (V).r, (V).b, (V).g)
#define vec_rrbb(V) vec4((V).r, (V).r, (V).b, (V).b)
#define vec_rrba(V) vec4((V).r, (V).r, (V).b, (V).a)
#define vec_rrar(V) vec4((V).r, (V).r, (V).a, (V).r)
#define vec_rrag(V) vec4((V).r, (V).r, (V).a, (V).g)
#define vec_rrab(V) vec4((V).r, (V).r, (V).a, (V).b)
#define vec_rraa(V) vec4((V).r, (V).r, (V).a, (V).a)
#define vec_rgrr(V) vec4((V).r, (V).g, (V).r, (V).r)
#define vec_rgrg(V) vec4((V).r, (V).g, (V).r, (V).g)
#define vec_rgrb(V) vec4((V).r, (V).g, (V).r, (V).b)
#define vec_rgra(V) vec4((V).r, (V).g, (V).r, (V).a)
#define vec_rggr(V) vec4((V).r, (V).g, (V).g, (V).r)
#define vec_rggg(V) vec4((V).r, (V).g, (V).g, (V).g)
#define vec_rggb(V) vec4((V).r, (V).g, (V).g, (V).b)
#define vec_rgga(V) vec4((V).r, (V).g, (V).g, (V).a)
#define vec_rgbr(V) vec4((V).r, (V).g, (V).b, (V).r)
#define vec_rgbg(V) vec4((V).r, (V).g, (V).b, (V).g)
#define vec_rgbb(V) vec4((V).r, (V).g, (V).b, (V).b)
#define vec_rgba(V) vec4((V).r, (V).g, (V).b, (V).a)
#define vec_rgar(V) vec4((V).r, (V).g, (V).a, (V).r)
#define vec_rgag(V) vec4((V).r, (V).g, (V).a, (V).g)
#define vec_rgab(V) vec4((V).r, (V).g, (V).a, (V).b)
#define vec_rgaa(V) vec4((V).r, (V).g, (V).a, (V).a)
#define vec_rbrr(V) vec4((V).r, (V).b, (V).r, (V).r)
#define vec_rbrg(V) vec4((V).r, (V).b, (V).r, (V).g)
#define vec_rbrb(V) vec4((V).r, (V).b, (V).r, (V).b)
#define vec_rbra(V) vec4((V).r, (V).b, (V).r, (V).a)
#define vec_rbgr(V) vec4((V).r, (V).b, (V).g, (V).r)
#define vec_rbgg(V) vec4((V).r, (V).b, (V).g, (V).g)
#define vec_rbgb(V) vec4((V).r, (V).b, (V).g, (V).b)
#define vec_rbga(V) vec4((V).r, (V).b, (V).g, (V).a)
#define vec_rbbr(V) vec4((V).r, (V).b, (V).b, (V).r)
#define vec_rbbg(V) vec4((V).r, (V).b, (V).b, (V).g)
#define vec_rbbb(V) vec4((V).r, (V).b, (V).b, (V).b)
#define vec_rbba(V) vec4((V).r, (V).b, (V).b, (V).a)
#define vec_rbar(V) vec4((V).r, (V).b, (V).a, (V).r)
#define vec_rbag(V) vec4((V).r, (V).b, (V).a, (V).g)
#define vec_rbab(V) vec4((V).r, (V).b, (V).a, (V).b)
#define vec_rbaa(V) vec4((V).r, (V).b, (V).a, (V).a)
#define vec_rarr(V) vec4((V).r, (V).a, (V).r, (V).r)
#define vec_rarg(V) vec4((V).r, (V).a, (V).r, (V).g)
#define vec_rarb(V) vec4((V).r, (V).a, (V).r, (V).b)
#define vec_rara(V) vec4((V).r, (V).a, (V).r, (V).a)
#define vec_ragr(V) vec4((V).r, (V).a, (V).g, (V).r)
#define vec_ragg(V) vec4((V).r, (V).a, (V).g, (V).g)
#define vec_ragb(V) vec4((V).r, (V).a, (V).g, (V).b)
#define vec_raga(V) vec4((V).r, (V).a, (V).g, (V).a)
#define vec_rabr(V) vec4((V).r, (V).a, (V).b, (V).r)
#define vec_rabg(V) vec4((V).r, (V).a, (V).b, (V).g)
#define vec_rabb(V) vec4((V).r, (V).a, (V).b, (V).b)
#define vec_raba(V) vec4((V).r, (V).a, (V).b, (V).a)
#define vec_raar(V) vec4((V).r, (V).a, (V).a, (V).r)
#define vec_raag(V) vec4((V).r, (V).a, (V).a, (V).g)
#define vec_raab(V) vec4((V).r, (V).a, (V).a, (V).b)
#define vec_raaa(V) vec4((V).r, (V).a, (V).a, (V).a)
#define vec_grrr(V) vec4((V).g, (V).r, (V).r, (V).r)
#define vec_grrg(V) vec4((V).g, (V).r, (V).r, (V).g)
#define vec_grrb(V) vec4((V).g, (V).r, (V).r, (V).b)
#define vec_grra(V) vec4((V).g, (V).r, (V).r, (V).a)
#define vec_grgr(V) vec4((V).g, (V).r, (V).g, (V).r)
#define vec_grgg(V) vec4((V).g, (V).r, (V).g, (V).g)
#define vec_grgb(V) vec4((V).g, (V).r, (V).g, (V).b)
#define vec_grga(V) vec4((V).g, (V).r, (V).g, (V).a)
#define vec_grbr(V) vec4((V).g, (V).r, (V).b, (V).r)
#define vec_grbg(V) vec4((V).g, (V).r, (V).b, (V).g)
#define vec_grbb(V) vec4((V).g, (V).r, (V).b, (V).b)
#define vec_grba(V) vec4((V).g, (V).r, (V).b, (V).a)
#define vec_grar(V) vec4((V).g, (V).r, (V).a, (V).r)
#define vec_grag(V) vec4((V).g, (V).r, (V).a, (V).g)
#define vec_grab(V) vec4((V).g, (V).r, (V).a, (V).b)
#define vec_graa(V) vec4((V).g, (V).r, (V).a, (V).a)
#define vec_ggrr(V) vec4((V).g, (V).g, (V).r, (V).r)
#define vec_ggrg(V) vec4((V).g, (V).g, (V).r, (V).g)
#define vec_ggrb(V) vec4((V).g, (V).g, (V).r, (V).b)
#define vec_ggra(V) vec4((V).g, (V).g, (V).r, (V).a)
#define vec_gggr(V) vec4((V).g, (V).g, (V).g, (V).r)
#define vec_gggg(V) vec4((V).g, (V).g, (V).g, (V).g)
#define vec_gggb(V) vec4((V).g, (V).g, (V).g, (V).b)
#define vec_ggga(V) vec4((V).g, (V).g, (V).g, (V).a)
#define vec_ggbr(V) vec4((V).g, (V).g, (V).b, (V).r)
#define vec_ggbg(V) vec4((V).g, (V).g, (V).b, (V).g)
#define vec_ggbb(V) vec4((V).g, (V).g, (V).b, (V).b)
#define vec_ggba(V) vec4((V).g, (V).g, (V).b, (V).a)
#define vec_ggar(V) vec4((V).g, (V).g, (V).a, (V).r)
#define vec_ggag(V) vec4((V).g, (V).g, (V).a, (V).g)
#define vec_ggab(V) vec4((V).g, (V).g, (V).a, (V).b)
#define vec_ggaa(V) vec4((V).g, (V).g, (V).a, (V).a)
#define vec_gbrr(V) vec4((V).g, (V).b, (V).r, (V).r)
#define vec_gbrg(V) vec4((V).g, (V).b, (V).r, (V).g)
#define vec_gbrb(V) vec4((V).g, (V).b, (V).r, (V).b)
#define vec_gbra(V) vec4((V).g, (V).b, (V).r, (V).a)
#define vec_gbgr(V) vec4((V).g, (V).b, (V).g, (V).r)
#define vec_gbgg(V) vec4((V).g, (V).b, (V).g, (V).g)
#define vec_gbgb(V) vec4((V).g, (V).b, (V).g, (V).b)
#define vec_gbga(V) vec4((V).g, (V).b, (V).g, (V).a)
#define vec_gbbr(V) vec4((V).g, (V).b, (V).b, (V).r)
#define vec_gbbg(V) vec4((V).g, (V).b, (V).b, (V).g)
#define vec_gbbb(V) vec4((V).g, (V).b, (V).b, (V).b)
#define vec_gbba(V) vec4((V).g, (V).b, (V).b, (V).a)
#define vec_gbar(V) vec4((V).g, (V).b, (V).a, (V).r)
#define vec_gbag(V) vec4((V).g, (V).b, (V).a, (V).g)
#define vec_gbab(V) vec4((V).g, (V).b, (V).a, (V).b)
#define vec_gbaa(V) vec4((V).g, (V).b, (V).a, (V).a)
#define vec_garr(V) vec4((V).g, (V).a, (V).r, (V).r)
#define vec_garg(V) vec4((V).g, (V).a, (V).r, (V).g)
#define vec_garb(V) vec4((V).g, (V).a, (V).r, (V).b)
#define vec_gara(V) vec4((V).g, (V).a, (V).r, (V).a)
#define vec_gagr(V) vec4((V).g, (V).a, (V).g, (V).r)
#define vec_gagg(V) vec4((V).g, (V).a, (V).g, (V).g)
#define vec_gagb(V) vec4((V).g, (V).a, (V).g, (V).b)
#define vec_gaga(V) vec4((V).g, (V).a, (V).g, (V).a)
#define vec_gabr(V) vec4((V).g, (V).a, (V).b, (V).r)
#define vec_gabg(V) vec4((V).g, (V).a, (V).b, (V).g)
#define vec_gabb(V) vec4((V).g, (V).a, (V).b, (V).b)
#define vec_gaba(V) vec4((V).g, (V).a, (V).b, (V).a)
#define vec_gaar(V) vec4((V).g, (V).a, (V).a, (V).r)
#define vec_gaag(V) vec4((V).g, (V).a, (V).a, (V).g)
#define vec_gaab(V) vec4((V).g, (V).a, (V).a, (V).b)
#define vec_gaaa(V) vec4((V).g, (V).a, (V).a, (V).a)
#define vec_brrr(V) vec4((V).b, (V).r, (V).r, (V).r)
#define vec_brrg(V) vec4((V).b, (V).r, (V).r, (V).g)
#define vec_brrb(V) vec4((V).b, (V).r, (V).r, (V).b)
#define vec_brra(V) vec4((V).b, (V).r, (V).r, (V).a)
#define vec_brgr(V) vec4((V).b, (V).r, (V).g, (V).r)
#define vec_brgg(V) vec4((V).b, (V).r, (V).g, (V).g)
#define vec_brgb(V) vec4((V).b, (V).r, (V).g, (V).b)
#define vec_brga(V) vec4((V).b, (V).r, (V).g, (V).a)
#define vec_brbr(V) vec4((V).b, (V).r, (V).b, (V).r)
#define vec_brbg(V) vec4((V).b, (V).r, (V).b, (V).g)
#define vec_brbb(V) vec4((V).b, (V).r, (V).b, (V).b)
#define vec_brba(V) vec4((V).b, (V).r, (V).b, (V).a)
#define vec_brar(V) vec4((V).b, (V).r, (V).a, (V).r)
#define vec_brag(V) vec4((V).b, (V).r, (V).a, (V).g)
#define vec_brab(V) vec4((V).b, (V).r, (V).a, (V).b)
#define vec_braa(V) vec4((V).b, (V).r, (V).a, (V).a)
#define vec_bgrr(V) vec4((V).b, (V).g, (V).r, (V).r)
#define vec_bgrg(V) vec4((V).b, (V).g, (V).r, (V).g)
#define vec_bgrb(V) vec4((V).b, (V).g, (V).r, (V).b)
#define vec_bgra(V) vec4((V).b, (V).g, (V).r, (V).a)
#define vec_bggr(V) vec4((V).b, (V).g, (V).g, (V).r)
#define vec_bggg(V) vec4((V).b, (V).g, (V).g, (V).g)
#define vec_bggb(V) vec4((V).b, (V).g, (V).g, (V).b)
#define vec_bgga(V) vec4((V).b, (V).g, (V).g, (V).a)
#define vec_bgbr(V) vec4((V).b, (V).g, (V).b, (V).r)
#define vec_bgbg(V) vec4((V).b, (V).g, (V).b, (V).g)
#define vec_bgbb(V) vec4((V).b, (V).g, (V).b, (V).b)
#define vec_bgba(V) vec4((V).b, (V).g, (V).b, (V).a)
#define vec_bgar(V) vec4((V).b, (V).g, (V).a, (V).r)
#define vec_bgag(V) vec4((V).b, (V).g, (V).a, (V).g)
#define vec_bgab(V) vec4((V).b, (V).g, (V).a, (V).b)
#define vec_bgaa(V) vec4((V).b, (V).g, (V).a, (V).a)
#define vec_bbrr(V) vec4((V).b, (V).b, (V).r, (V).r)
#define vec_bbrg(V) vec4((V).b, (V).b, (V).r, (V).g)
#define vec_bbrb(V) vec4((V).b, (V).b, (V).r, (V).b)
#define vec_bbra(V) vec4((V).b, (V).b, (V).r, (V).a)
#define vec_bbgr(V) vec4((V).b, (V).b, (V).g, (V).r)
#define vec_bbgg(V) vec4((V).b, (V).b, (V).g, (V).g)
#define vec_bbgb(V) vec4((V).b, (V).b, (V).g, (V).b)
#define vec_bbga(V) vec4((V).b, (V).b, (V).g, (V).a)
#define vec_bbbr(V) vec4((V).b, (V).b, (V).b, (V).r)
#define vec_bbbg(V) vec4((V).b, (V).b, (V).b, (V).g)
#define vec_bbbb(V) vec4((V).b, (V).b, (V).b, (V).b)
#define vec_bbba(V) vec4((V).b, (V).b, (V).b, (V).a)
#define vec_bbar(V) vec4((V).b, (V).b, (V).a, (V).r)
#define vec_bbag(V) vec4((V).b, (V).b, (V).a, (V).g)
#define vec_bbab(V) vec4((V).b, (V).b, (V).a, (V).b)
#define vec_bbaa(V) vec4((V).b, (V).b, (V).a, (V).a)
#define vec_barr(V) vec4((V).b, (V).a, (V).r, (V).r)
#define vec_barg(V) vec4((V).b, (V).a, (V).r, (V).g)
#define vec_barb(V) vec4((V).b, (V).a, (V).r, (V).b)
#define vec_bara(V) vec4((V).b, (V).a, (V).r, (V).a)
#define vec_bagr(V) vec4((V).b, (V).a, (V).g, (V).r)
#define vec_bagg(V) vec4((V).b, (V).a, (V).g, (V).g)
#define vec_bagb(V) vec4((V).b, (V).a, (V).g, (V).b)
#define vec_baga(V) vec4((V).b, (V).a, (V).g, (V).a)
#define vec_babr(V) vec4((V).b, (V).a, (V).b, (V).r)
#define vec_babg(V) vec4((V).b, (V).a, (V).b, (V).g)
#define vec_babb(V) vec4((V).b, (V).a, (V).b, (V).b)
#define vec_baba(V) vec4((V).b, (V).a, (V).b, (V).a)
#define vec_baar(V) vec4((V).b, (V).a, (V).a, (V).r)
#define vec_baag(V) vec4((V).b, (V).a, (V).a, (V).g)
#define vec_baab(V) vec4((V).b, (V).a, (V).a, (V).b)
#define vec_baaa(V) vec4((V).b, (V).a, (V).a, (V).a)
#define vec_arrr(V) vec4((V).a, (V).r, (V).r, (V).r)
#define vec_arrg(V) vec4((V).a, (V).r, (V).r, (V).g)
#define vec_arrb(V) vec4((V).a, (V).r, (V).r, (V).b)
#define vec_arra(V) vec4((V).a, (V).r, (V).r, (V).a)
#define vec_argr(V) vec4((V).a, (V).r, (V).g, (V).r)
#define vec_argg(V) vec4((V).a, (V).r, (V).g, (V).g)
#define vec_argb(V) vec4((V).a, (V).r, (V).g, (V).b)
#define vec_arga(V) vec4((V).a, (V).r, (V).g, (V).a)
#define vec_arbr(V) vec4((V).a, (V).r, (V).b, (V).r)
#define vec_arbg(V) vec4((V).a, (V).r, (V).b, (V).g)
#define vec_arbb(V) vec4((V).a, (V).r, (V).b, (V).b)
#define vec_arba(V) vec4((V).a, (V).r, (V).b, (V).a)
#define vec_arar(V) vec4((V).a, (V).r, (V).a, (V).r)
#define vec_arag(V) vec4((V).a, (V).r, (V).a, (V).g)
#define vec_arab(V) vec4((V).a, (V).r, (V).a, (V).b)
#define vec_araa(V) vec4((V).a, (V).r, (V).a, (V).a)
#define vec_agrr(V) vec4((V).a, (V).g, (V).r, (V).r)
#define vec_agrg(V) vec4((V).a, (V).g, (V).r, (V).g)
#define vec_agrb(V) vec4((V).a, (V).g, (V).r, (V).b)
#define vec_agra(V) vec4((V).a, (V).g, (V).r, (V).a)
#define vec_aggr(V) vec4((V).a, (V).g, (V).g, (V).r)
#define vec_aggg(V) vec4((V).a, (V).g, (V).g, (V).g)
#define vec_aggb(V) vec4((V).a, (V).g, (V).g, (V).b)
#define vec_agga(V) vec4((V).a, (V).g, (V).g, (V).a)
#define vec_agbr(V) vec4((V).a, (V).g, (V).b, (V).r)
#define vec_agbg(V) vec4((V).a, (V).g, (V).b, (V).g)
#define vec_agbb(V) vec4((V).a, (V).g, (V).b, (V).b)
#define vec_agba(V) vec4((V).a, (V).g, (V).b, (V).a)
#define vec_agar(V) vec4((V).a, (V).g, (V).a, (V).r)
#define vec_agag(V) vec4((V).a, (V).g, (V).a, (V).g)
#define vec_agab(V) vec4((V).a, (V).g, (V).a, (V).b)
#define vec_agaa(V) vec4((V).a, (V).g, (V).a, (V).a)
#define vec_abrr(V) vec4((V).a, (V).b, (V).r, (V).r)
#define vec_abrg(V) vec4((V).a, (V).b, (V).r, (V).g)
#define vec_abrb(V) vec4((V).a, (V).b, (V).r, (V).b)
#define vec_abra(V) vec4((V).a, (V).b, (V).r, (V).a)
#define vec_abgr(V) vec4((V).a, (V).b, (V).g, (V).r)
#define vec_abgg(V) vec4((V).a, (V).b, (V).g, (V).g)
#define vec_abgb(V) vec4((V).a, (V).b, (V).g, (V).b)
#define vec_abga(V) vec4((V).a, (V).b, (V).g, (V).a)
#define vec_abbr(V) vec4((V).a, (V).b, (V).b, (V).r)
#define vec_abbg(V) vec4((V).a, (V).b, (V).b, (V).g)
#define vec_abbb(V) vec4((V).a, (V).b, (V).b, (V).b)
#define vec_abba(V) vec4((V).a, (V).b, (V).b, (V).a)
#define vec_abar(V) vec4((V).a, (V).b, (V).a, (V).r)
#define vec_abag(V) vec4((V).a, (V).b, (V).a, (V).g)
#define vec_abab(V) vec4((V).a, (V).b, (V).a, (V).b)
#define vec_abaa(V) vec4((V).a, (V).b, (V).a, (V).a)
#define vec_aarr(V) vec4((V).a, (V).a, (V).r, (V).r)
#define vec_aarg(V) vec4((V).a, (V).a, (V).r, (V).g)
#define vec_aarb(V) vec4((V).a, (V).a, (V).r, (V).b)
#define vec_aara(V) vec4((V).a, (V).a, (V).r, (V).a)
#define vec_aagr(V) vec4((V).a, (V).a, (V).g, (V).r)
#define vec_aagg(V) vec4((V).a, (V).a, (V).g, (V).g)
#define vec_aagb(V) vec4((V).a, (V).a, (V).g, (V).b)
#define vec_aaga(V) vec4((V).a, (V).a, (V).g, (V).a)
#define vec_aabr(V) vec4((V).a, (V).a, (V).b, (V).r)
#define vec_aabg(V) vec4((V).a, (V).a, (V).b, (V).g)
#define vec_aabb(V) vec4((V).a, (V).a, (V).b, (V).b)
#define vec_aaba(V) vec4((V).a, (V).a, (V).b, (V).a)
#define vec_aaar(V) vec4((V).a, (V).a, (V).a, (V).r)
#define vec_aaag(V) vec4((V).a, (V).a, (V).a, (V).g)
#define vec_aaab(V) vec4((V).a, (V).a, (V).a, (V).b)
#define vec_aaaa(V) vec4((V).a, (V).a, (V).a, (V).a)

#define vec_x_assign(V, U) ((V).x = (U).x)
#define vec_y_assign(V, U) ((V).y = (U).x)
#define vec_z_assign(V, U) ((V).z = (U).x)
#define vec_w_assign(V, U) ((V).w = (U).x)
#define vec_xy_assign(V, U) ((V).x = (U).x, (V).y = (U).y)
#define vec_xz_assign(V, U) ((V).x = (U).x, (V).z = (U).y)
#define vec_xw_assign(V, U) ((V).x = (U).x, (V).w = (U).y)
#define vec_yx_assign(V, U) ((V).y = (U).x, (V).x = (U).y)
#define vec_yz_assign(V, U) ((V).y = (U).x, (V).z = (U).y)
#define vec_yw_assign(V, U) ((V).y = (U).x, (V).w = (U).y)
#define vec_zx_assign(V, U) ((V).z = (U).x, (V).x = (U).y)
#define vec_zy_assign(V, U) ((V).z = (U).x, (V).y = (U).y)
#define vec_zw_assign(V, U) ((V).z = (U).x, (V).w = (U).y)
#define vec_wx_assign(V, U) ((V).w = (U).x, (V).x = (U).y)
#define vec_wy_assign(V, U) ((V).w = (U).x, (V).y = (U).y)
#define vec_wz_assign(V, U) ((V).w = (U).x, (V).z = (U).y)
#define vec_xyz_assign(V, U) ((V).x = (U).x, (V).y = (U).y, (V).z = (U).z)
#define vec_xyw_assign(V, U) ((V).x = (U).x, (V).y = (U).y, (V).w = (U).z)
#define vec_xzy_assign(V, U) ((V).x = (U).x, (V).z = (U).y, (V).y = (U).z)
#define vec_xzw_assign(V, U) ((V).x = (U).x, (V).z = (U).y, (V).w = (U).z)
#define vec_xwy_assign(V, U) ((V).x = (U).x, (V).w = (U).y, (V).y = (U).z)
#define vec_xwz_assign(V, U) ((V).x = (U).x, (V).w = (U).y, (V).z = (U).z)
#define vec_yxz_assign(V, U) ((V).y = (U).x, (V).x = (U).y, (V).z = (U).z)
#define vec_yxw_assign(V, U) ((V).y = (U).x, (V).x = (U).y, (V).w = (U).z)
#define vec_yzx_assign(V, U) ((V).y = (U).x, (V).z = (U).y, (V).x = (U).z)
#define vec_yzw_assign(V, U) ((V).y = (U).x, (V).z = (U).y, (V).w = (U).z)
#define vec_ywx_assign(V, U) ((V).y = (U).x, (V).w = (U).y, (V).x = (U).z)
#define vec_ywz_assign(V, U) ((V).y = (U).x, (V).w = (U).y, (V).z = (U).z)
#define vec_zxy_assign(V, U) ((V).z = (U).x, (V).x = (U).y, (V).y = (U).z)
#define vec_zxw_assign(V, U) ((V).z = (U).x, (V).x = (U).y, (V).w = (U).z)
#define vec_zyx_assign(V, U) ((V).z = (U).x, (V).y = (U).y, (V).x = (U).z)
#define vec_zyw_assign(V, U) ((V).z = (U).x, (V).y = (U).y, (V).w = (U).z)
#define vec_zwx_assign(V, U) ((V).z = (U).x, (V).w = (U).y, (V).x = (U).z)
#define vec_zwy_assign(V, U) ((V).z = (U).x, (V).w = (U).y, (V).y = (U).z)
#define vec_wxy_assign(V, U) ((V).w = (U).x, (V).x = (U).y, (V).y = (U).z)
#define vec_wxz_assign(V, U) ((V).w = (U).x, (V).x = (U).y, (V).z = (U).z)
#define vec_wyx_assign(V, U) ((V).w = (U).x, (V).y = (U).y, (V).x = (U).z)
#define vec_wyz_assign(V, U) ((V).w = (U).x, (V).y = (U).y, (V).z = (U).z)
#define vec_wzx_assign(V, U) ((V).w = (U).x, (V).z = (U).y, (V).x = (U).z)
#define vec_wzy_assign(V, U) ((V).w = (U).x, (V).z = (U).y, (V).y = (U).z)
#define vec_xyzw_assign(V, U) ((V).x = (U).x, (V).y = (U).y, (V).z = (U).z, (V).w = (U).w)
#define vec_xywz_assign(V, U) ((V).x = (U).x, (V).y = (U).y, (V).w = (U).z, (V).z = (U).w)
#define vec_xzyw_assign(V, U) ((V).x = (U).x, (V).z = (U).y, (V).y = (U).z, (V).w = (U).w)
#define vec_xzwy_assign(V, U) ((V).x = (U).x, (V).z = (U).y, (V).w = (U).z, (V).y = (U).w)
#define vec_xwyz_assign(V, U) ((V).x = (U).x, (V).w = (U).y, (V).y = (U).z, (V).z = (U).w)
#define vec_xwzy_assign(V, U) ((V).x = (U).x, (V).w = (U).y, (V).z = (U).z, (V).y = (U).w)
#define vec_yxzw_assign(V, U) ((V).y = (U).x, (V).x = (U).y, (V).z = (U).z, (V).w = (U).w)
#define vec_yxwz_assign(V, U) ((V).y = (U).x, (V).x = (U).y, (V).w = (U).z, (V).z = (U).w)
#define vec_yzxw_assign(V, U) ((V).y = (U).x, (V).z = (U).y, (V).x = (U).z, (V).w = (U).w)
#define vec_yzwx_assign(V, U) ((V).y = (U).x, (V).z = (U).y, (V).w = (U).z, (V).x = (U).w)
#define vec_ywxz_assign(V, U) ((V).y = (U).x, (V).w = (U).y, (V).x = (U).z, (V).z = (U).w)
#define vec_ywzx_assign(V, U) ((V).y = (U).x, (V).w = (U).y, (V).z = (U).z, (V).x = (U).w)
#define vec_zxyw_assign(V, U) ((V).z = (U).x, (V).x = (U).y, (V).y = (U).z, (V).w = (U).w)
#define vec_zxwy_assign(V, U) ((V).z = (U).x, (V).x = (U).y, (V).w = (U).z, (V).y = (U).w)
#define vec_zyxw_assign(V, U) ((V).z = (U).x, (V).y = (U).y, (V).x = (U).z, (V).w = (U).w)
#define vec_zywx_assign(V, U) ((V).z = (U).x, (V).y = (U).y, (V).w = (U).z, (V).x = (U).w)
#define vec_zwxy_assign(V, U) ((V).z = (U).x, (V).w = (U).y, (V).x = (U).z, (V).y = (U).w)
#define vec_zwyx_assign(V, U) ((V).z = (U).x, (V).w = (U).y, (V).y = (U).z, (V).x = (U).w)
#define vec_wxyz_assign(V, U) ((V).w = (U).x, (V).x = (U).y, (V).y = (U).z, (V).z = (U).w)
#define vec_wxzy_assign(V, U) ((V).w = (U).x, (V).x = (U).y, (V).z = (U).z, (V).y = (U).w)
#define vec_wyxz_assign(V, U) ((V).w = (U).x, (V).y = (U).y, (V).x = (U).z, (V).z = (U).w)
#define vec_wyzx_assign(V, U) ((V).w = (U).x, (V).y = (U).y, (V).z = (U).z, (V).x = (U).w)
#define vec_wzxy_assign(V, U) ((V).w = (U).x, (V).z = (U).y, (V).x = (U).z, (V).y = (U).w)
#define vec_wzyx_assign(V, U) ((V).w = (U).x, (V).z = (U).y, (V).y = (U).z, (V).x = (U).w)

#define vec_r_assign(V, U) ((V).r = (U).x)
#define vec_g_assign(V, U) ((V).g = (U).x)
#define vec_b_assign(V, U) ((V).b = (U).x)
#define vec_a_assign(V, U) ((V).a = (U).x)
#define vec_rg_assign(V, U) ((V).r = (U).x, (V).g = (U).y)
#define vec_rb_assign(V, U) ((V).r = (U).x, (V).b = (U).y)
#define vec_ra_assign(V, U) ((V).r = (U).x, (V).a = (U).y)
#define vec_gr_assign(V, U) ((V).g = (U).x, (V).r = (U).y)
#define vec_gb_assign(V, U) ((V).g = (U).x, (V).b = (U).y)
#define vec_ga_assign(V, U) ((V).g = (U).x, (V).a = (U).y)
#define vec_br_assign(V, U) ((V).b = (U).x, (V).r = (U).y)
#define vec_bg_assign(V, U) ((V).b = (U).x, (V).g = (U).y)
#define vec_ba_assign(V, U) ((V).b = (U).x, (V).a = (U).y)
#define vec_ar_assign(V, U) ((V).a = (U).x, (V).r = (U).y)
#define vec_ag_assign(V, U) ((V).a = (U).x, (V).g = (U).y)
#define vec_ab_assign(V, U) ((V).a = (U).x, (V).b = (U).y)
#define vec_rgb_assign(V, U) ((V).r = (U).x, (V).g = (U).y, (V).b = (U).z)
#define vec_rga_assign(V, U) ((V).r = (U).x, (V).g = (U).y, (V).a = (U).z)
#define vec_rbg_assign(V, U) ((V).r = (U).x, (V).b = (U).y, (V).g = (U).z)
#define vec_rba_assign(V, U) ((V).r = (U).x, (V).b = (U).y, (V).a = (U).z)
#define vec_rag_assign(V, U) ((V).r = (U).x, (V).a = (U).y, (V).g = (U).z)
#define vec_rab_assign(V, U) ((V).r = (U).x, (V).a = (U).y, (V).b = (U).z)
#define vec_grb_assign(V, U) ((V).g = (U).x, (V).r = (U).y, (V).b = (U).z)
#define vec_gra_assign(V, U) ((V).g = (U).x, (V).r = (U).y, (V).a = (U).z)
#define vec_gbr_assign(V, U) ((V).g = (U).x, (V).b = (U).y, (V).r = (U).z)
#define vec_gba_assign(V, U) ((V).g = (U).x, (V).b = (U).y, (V).a = (U).z)
#define vec_gar_assign(V, U) ((V).g = (U).x, (V).a = (U).y, (V).r = (U).z)
#define vec_gab_assign(V, U) ((V).g = (U).x, (V).a = (U).y, (V).b = (U).z)
#define vec_brg_assign(V, U) ((V).b = (U).x, (V).r = (U).y, (V).g = (U).z)
#define vec_bra_assign(V, U) ((V).b = (U).x, (V).r = (U).y, (V).a = (U).z)
#define vec_bgr_assign(V, U) ((V).b = (U).x, (V).g = (U).y, (V).r = (U).z)
#define vec_bga_assign(V, U) ((V).b = (U).x, (V).g = (U).y, (V).a = (U).z)
#define vec_bar_assign(V, U) ((V).b = (U).x, (V).a = (U).y, (V).r = (U).z)
#define vec_bag_assign(V, U) ((V).b = (U).x, (V).a = (U).y, (V).g = (U).z)
#define vec_arg_assign(V, U) ((V).a = (U).x, (V).r = (U).y, (V).g = (U).z)
#define vec_arb_assign(V, U) ((V).a = (U).x, (V).r = (U).y, (V).b = (U).z)
#define vec_agr_assign(V, U) ((V).a = (U).x, (V).g = (U).y, (V).r = (U).z)
#define vec_agb_assign(V, U) ((V).a = (U).x, (V).g = (U).y, (V).b = (U).z)
#define vec_abr_assign(V, U) ((V).a = (U).x, (V).b = (U).y, (V).r = (U).z)
#define vec_abg_assign(V, U) ((V).a = (U).x, (V).b = (U).y, (V).g = (U).z)
#define vec_rgba_assign(V, U) ((V).r = (U).x, (V).g = (U).y, (V).b = (U).z, (V).a = (U).w)
#define vec_rgab_assign(V, U) ((V).r = (U).x, (V).g = (U).y, (V).a = (U).z, (V).b = (U).w)
#define vec_rbga_assign(V, U) ((V).r = (U).x, (V).b = (U).y, (V).g = (U).z, (V).a = (U).w)
#define vec_rbag_assign(V, U) ((V).r = (U).x, (V).b = (U).y, (V).a = (U).z, (V).g = (U).w)
#define vec_ragb_assign(V, U) ((V).r = (U).x, (V).a = (U).y, (V).g = (U).z, (V).b = (U).w)
#define vec_rabg_assign(V, U) ((V).r = (U).x, (V).a = (U).y, (V).b = (U).z, (V).g = (U).w)
#define vec_grba_assign(V, U) ((V).g = (U).x, (V).r = (U).y, (V).b = (U).z, (V).a = (U).w)
#define vec_grab_assign(V, U) ((V).g = (U).x, (V).r = (U).y, (V).a = (U).z, (V).b = (U).w)
#define vec_gbra_assign(V, U) ((V).g = (U).x, (V).b = (U).y, (V).r = (U).z, (V).a = (U).w)
#define vec_gbar_assign(V, U) ((V).g = (U).x, (V).b = (U).y, (V).a = (U).z, (V).r = (U).w)
#define vec_garb_assign(V, U) ((V).g = (U).x, (V).a = (U).y, (V).r = (U).z, (V).b = (U).w)
#define vec_gabr_assign(V, U) ((V).g = (U).x, (V).a = (U).y, (V).b = (U).z, (V).r = (U).w)
#define vec_brga_assign(V, U) ((V).b = (U).x, (V).r = (U).y, (V).g = (U).z, (V).a = (U).w)
#define vec_brag_assign(V, U) ((V).b = (U).x, (V).r = (U).y, (V).a = (U).z, (V).g = (U).w)
#define vec_bgra_assign(V, U) ((V).b = (U).x, (V).g = (U).y, (V).r = (U).z, (V).a = (U).w)
#define vec_bgar_assign(V, U) ((V).b = (U).x, (V).g = (U).y, (V).a = (U).z, (V).r = (U).w)
#define vec_barg_assign(V, U) ((V).b = (U).x, (V).a = (U).y, (V).r = (U).z, (V).g = (U).w)
#define vec_bagr_assign(V, U) ((V).b = (U).x, (V).a = (U).y, (V).g = (U).z, (V).r = (U).w)
#define vec_argb_assign(V, U) ((V).a = (U).x, (V).r = (U).y, (V).g = (U).z, (V).b = (U).w)
#define vec_arbg_assign(V, U) ((V).a = (U).x, (V).r = (U).y, (V).b = (U).z, (V).g = (U).w)
#define vec_agrb_assign(V, U) ((V).a = (U).x, (V).g = (U).y, (V).r = (U).z, (V).b = (U).w)
#define vec_agbr_assign(V, U) ((V).a = (U).x, (V).g = (U).y, (V).b = (U).z, (V).r = (U).w)
#define vec_abrg_assign(V, U) ((V).a = (U).x, (V).b = (U).y, (V).r = (U).z, (V).g = (U).w)
#define vec_abgr_assign(V, U) ((V).a = (U).x, (V).b = (U).y, (V).g = (U).z, (V).r = (U).w)

#endif
