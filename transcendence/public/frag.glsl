precision highp float;

uniform float u_time;
uniform vec2 u_resolution;
uniform vec3 u_ball;
uniform vec3 u_player1;
uniform vec3 u_player2;
const float ball_size = .125;
const float court_length = 3.;
const vec3 paddle_size = vec3(.25, .25, .025);

// Macros possibly defined in app.js:
// #define QUALITY
// #define SPLIT_SCREEN
// #define PLAYER2

#define QUALITY_LOW  0
#define QUALITY_MID  1
#define QUALITY_HIGH 2

#define MIN_DISTANCE    .005
#define MAX_DISTANCE    2.
#define MAX_ITERATIONS  128
#define NORMAL_EPSILON  .001
#define FIELD_OF_VIEW   70.

// ----------------------------------------------------------------------------
// Utilities

#define PI 3.14159265359

#define smoothmin(a, b, smoothing) ( mix(a,b,( clamp( ((a)-(b))/(smoothing) + .5,0.,1.)))-\
                                     .5*( clamp( ((a)-(b))/(smoothing) + .5 , 0., 1.) )  *\
                                     (1. - ( clamp( ((a)-(b))/(smoothing) + .5 ,0.,1.))) *\
                                     (smoothing)                                          )

mat2 rotate(float angle)
{
    return mat2(
        cos(angle), -sin(angle),
        sin(angle),  cos(angle));
}

float rand()
{
    return fract(54325.*sin(dot(gl_FragCoord.xy, vec2(1.432, 1.43))));
}

float rand(float x)
{
    //return fract(473.4539*x*sin(97.3855*x + 27.896));
    return fract(46245.*sin(dot(vec2(-52.434, -5.221*x), vec2(34.*x, 36.16))));
}

float rand(vec2 st)
{
    return fract(87336.*sin(dot(st, vec2(3.562, 6.865))));
}

float rand(vec3 p)
{
    return fract(69245.*sin(dot(p, vec3(42.655, 6.424, -13.574))));
}

float value_noise(float x)
{
    float x0 = rand(floor(x));
    float x1 = rand(floor(x + 1.));

    return mix(x0, x1, fract(x));
}

float value_noise(vec2 st)
{
    float x0y0 = rand(floor(st + vec2(0., 0.)));
    float x0y1 = rand(floor(st + vec2(0., 1.)));
    float x1y0 = rand(floor(st + vec2(1., 0.)));
    float x1y1 = rand(floor(st + vec2(1., 1.)));

    float l = mix(x0y0, x0y1, fract(st.y));
    float r = mix(x1y0, x1y1, fract(st.y));
    return mix(l, r, fract(st.x));
}

float value_noise(vec2 st, int octaves)
{
    float k = 1.;
    float f = 1.;
    float ret = 0.;
    for(int i = 0; i < octaves; i++)
    {
        ret += k*value_noise(f*st);
        k *= 1./sqrt(2.);
        f *= 2.;
    }
    return ret/(sqrt(float(octaves)));
}

float value_noise(vec3 p)
{
    float x0y0z0 = rand(floor(p + vec3(0., 0., 0.)));
    float x0y1z0 = rand(floor(p + vec3(0., 1., 0.)));
    float x1y0z0 = rand(floor(p + vec3(1., 0., 0.)));
    float x1y1z0 = rand(floor(p + vec3(1., 1., 0.)));

    float x0y0z1 = rand(floor(p + vec3(0., 0., 1.)));
    float x0y1z1 = rand(floor(p + vec3(0., 1., 1.)));
    float x1y0z1 = rand(floor(p + vec3(1., 0., 1.)));
    float x1y1z1 = rand(floor(p + vec3(1., 1., 1.)));

    float l0 = mix(x0y0z0, x0y1z0, fract(p.y));
    float r0 = mix(x1y0z0, x1y1z0, fract(p.y));

    float l1 = mix(x0y0z1, x0y1z1, fract(p.y));
    float r1 = mix(x1y0z1, x1y1z1, fract(p.y));

    return mix(mix(l0, r0, fract(p.x)),
               mix(l1, r1, fract(p.x)), fract(p.z));
}

float value_noise(vec3 p, int octaves)
{
    float k = 1.;
    float f = 1.;
    float ret = 0.;
    for(int i = 0; i < octaves; i++)
    {
        //ret += k*value_noise(f*p + value_noise(f*p));
        ret += k*value_noise(f*p);
        k *= 1./sqrt(2.);
        f *= 2.;
    }
    return ret/(sqrt(float(octaves)));
}

float value_noise(vec3 p, int octaves, int stacks)
{
    float angle = .5*PI/float(stacks);
    float ret   = value_noise(p, octaves);
    for(int i = 1; i < stacks; i++)
    {
        p.xy = rotate(-angle) * p.xy + 5.768*float(i);
        p.xz = rotate( angle) * p.xz + 3.535*float(i);
        p.yz = rotate( angle) * p.yz - 7.217*float(i);
        ret += value_noise(p, octaves);
    }
    return smoothstep(0., 1., ret/float(stacks));
}

// ----------------------------------------------------------------------------
// SDFs

float sdf_sphere(vec3 coords, float radius)
{
    return length(coords) - radius;
}

float sdf_plane(vec3 coords, vec3 normal)
{
    return -dot(coords, normal);
}

float sdf_box(vec3 coords, vec3 size)
{
    coords         = abs(coords);
    vec3   surface = coords - size;
    float  outside = length(max(vec3(0), surface));
    float  inside  = min(0., max(max(surface.x, surface.y), surface.z));
    return outside + inside;
}

float sdf_box_rounded(vec3 coords, vec3 size, float rounding)
{
    rounding *= min(min(size.x,size.y), size.z);
    return sdf_box(coords, size - rounding) - rounding;
}

float sdf_walls(vec3 origin)
{
    float separation = 1. + ball_size - .05;

    #ifndef TUBE
    #ifdef BOX_WALLS
    float walls_outside = sdf_box(origin, vec3(2.9));
    float walls_inside  = sdf_box_rounded(origin, vec3(separation, separation, 3), .15);
    float walls = max(walls_outside, -walls_inside);
    #endif

    origin = -abs(origin);

    #ifdef PLANE_WALLS // almost the same as box walls, corners are rounded differently
    float walls = smoothmin(
        sdf_plane(vec3(0,-separation,0) - origin, vec3(0,1,0)),
        sdf_plane(vec3(-separation,0,0) - origin, vec3(1,0,0)),
        .5);
    #endif

    float ends = sdf_plane(vec3(0,0,-court_length) - origin, vec3(0,0,1));
    return max(walls, -ends);

    #else // TUBE

    float rounding = .5;
    float walls_outside = sdf_box(origin, vec3(separation, separation, 2.9 - rounding)) - rounding;
    float walls_inside  = sdf_box_rounded(origin, vec3(separation, separation, 3), .15);
    return max(walls_outside, -walls_inside);

    #endif
}

float sdf_ball(vec3 origin)
{
    return sdf_sphere(u_ball - origin, ball_size);
}

float sdf_paddle_insides(vec3 origin)
{
    vec3 size = paddle_size;
    size.xy *= .9;
    size.z  += .01;
    return min(
        sdf_box(u_player1 - origin, size),
        sdf_box(u_player2 - origin, size));
}

float sdf_paddles(vec3 origin)
{
    const float paddle_rounding = .75;
    float outsides = min(
        sdf_box_rounded(u_player1 - origin, paddle_size, paddle_rounding),
        sdf_box_rounded(u_player2 - origin, paddle_size, paddle_rounding));

    return max(outsides, -sdf_paddle_insides(origin));
}

float sdf_scene(vec3 origin)
{
    float sdf = 99999999.;
    sdf = min(sdf, sdf_walls(origin));
    sdf = min(sdf, sdf_ball(origin));
    sdf = min(sdf, sdf_paddles(origin));
    return sdf;
}

vec3 scene_normal(vec3 origin)
{
    vec2 e = vec2(NORMAL_EPSILON, 0);
    return normalize(vec3(
        sdf_scene(origin + e.xyy) - sdf_scene(origin - e.xyy),
        sdf_scene(origin + e.yxy) - sdf_scene(origin - e.yxy),
        sdf_scene(origin + e.yyx) - sdf_scene(origin - e.yyx)));
}

// ----------------------------------------------------------------------------
// Lighting

struct Light
{
    vec3 pos;
    vec3 color;
};

vec3 phong(vec3 ray_pos, vec3 ray_dir, vec3 normal)
{
    vec3 ambient_light  = .001*vec3(.85, .65, 1.);
    vec3 material_color = vec3(.55, .1, .85);
    material_color.rb  += .2*vec2(.5*ray_pos.z, -ray_pos.z);

    #define LIGHTS_COUNT 3
    Light lights[LIGHTS_COUNT] = Light[LIGHTS_COUNT](
        Light(u_ball, vec3(1)),
        #ifdef TUBE // push paddle lights further back so we can see corners
        Light(u_player1 + vec3(0,0,-2.), vec3(1)),
        Light(u_player2 - vec3(0,0,-2.), vec3(1))
        #else // lights just behind the paddles for nicer plane speculars
        Light(u_player1 + vec3(0,0,-.5), vec3(1)),
        Light(u_player2 - vec3(0,0,-.5), vec3(1))
        #endif
    );

    vec3 surface_color = vec3(0);

    for (int i = 0; i < LIGHTS_COUNT; ++i)
    {
        vec3 light_dir      = normalize(lights[i].pos - ray_pos);
        vec3 reflection_dir = reflect(light_dir, normal);

        float diffuse  = max(0., dot(light_dir, normal));
        if (i == 0)
            diffuse *= 3.*diffuse*diffuse;
        float specular = max(0., dot(ray_dir, reflection_dir));
        float specular_shine = 512.;
        specular = pow(specular, specular_shine);

        surface_color +=
            .5*diffuse*material_color
          + .1*specular*lights[i].color;
    }
    return surface_color + ambient_light;
}

// ----------------------------------------------------------------------------
// Ray marching

const float reflectivity = .125; // 0 for no reflection, 1 for full reflectivity.
const int   REFLECTIONS  = QUALITY;

vec3 stars(vec2 uv)
{
    #if QUALITY == 0
    return vec3(0);
    #endif
    vec3 color = .5 + .5*cos(u_time + 32.*uv.xyx + vec3(0., 2., 4.));
    float noise = value_noise(vec3(16.*uv, .125*u_time), 4);
    float star_mask = pow(noise, 128.);
    return star_mask*color + .03*pow(noise, 8.);
}

vec3 ray_march(vec3 ray_pos, vec3 ray_dir, vec2 uv)
{
    vec3 surface_color = vec3(0);
    float reflection_contribution = 1.;
    float window_fog = 0.;
    for (int i = 0, i_refl = 0; i < MAX_ITERATIONS; ++i)
    {
        float scene_sdf = sdf_scene(ray_pos);
        float glass_sdf = sdf_paddle_insides(ray_pos);
        float sdf = min(scene_sdf, glass_sdf);
        if (sdf >= MAX_DISTANCE) {
            surface_color += step(surface_color, vec3(0)) * stars(uv);
            break;
        }
        if (sdf >= MIN_DISTANCE) {
            ray_pos += sdf*ray_dir;
            continue;
        }
        if (glass_sdf <= MIN_DISTANCE) {
            vec3 noise_coords = 2.*vec3(uv*length(ray_pos), u_time);
            float noise = value_noise(noise_coords, 1 + int(0 < QUALITY));
            window_fog += 1.5*MIN_DISTANCE*noise;
            ray_pos    += 1.0*MIN_DISTANCE*ray_dir;
            ray_dir    += .001 * (2.*noise - 1.);
            continue;
        }
        vec3 normal = scene_normal(ray_pos);
        vec3 contribution = reflection_contribution*phong(ray_pos, ray_dir, normal);
        if (sdf_ball(ray_pos) <= MIN_DISTANCE) {
            contribution *= 3.0;
            vec3 rotated = normal;
            rotated.xy *= rotate(1.*u_time);
            rotated.zx *= rotate(1.5*u_time);
            contribution += .5*reflection_contribution*rotated*rotated;
        }
        surface_color += contribution;
        surface_color = clamp(surface_color, 0., 1.);

        if (i_refl++ >= REFLECTIONS)
            break;
        reflection_contribution *= reflectivity;
        ray_dir  = reflect(ray_dir, normal);
        ray_pos += 2.*MIN_DISTANCE*ray_dir;
    }
    return surface_color + window_fog;
}

// ----------------------------------------------------------------------------
// Main rendering

// WARNING: CAN BE VERY HEAVY! Kernel size = (ANTI_ALIASING + 1)^2
const int ANTI_ALIASING = int(QUALITY == QUALITY_HIGH);

vec3 pixel_color(vec2 uv, float aspect_ratio)
{
    const float camera_distance_from_screen = 1./tan((PI/360.)*FIELD_OF_VIEW);

    vec3 ray_pos = vec3(0,0, -(court_length + camera_distance_from_screen + .4));
    vec3 ray_dir = normalize(vec3(uv, camera_distance_from_screen));

    #ifdef PLAYER2
    ray_pos.z *= -1.;
    ray_dir.z *= -1.;
    #endif

    #ifdef SPLIT_SCREEN
    uv *= 1.3;
    if (uv.x <= 0.) {
        uv.x   += .65*aspect_ratio;
        ray_pos = vec3(0,0, -(court_length + camera_distance_from_screen + .4));
        ray_dir = normalize(vec3(uv, camera_distance_from_screen));
    } else {
        uv.x   -= .65*aspect_ratio;
        ray_pos = -vec3(0,0, -(court_length + camera_distance_from_screen + .4));
        ray_dir = -normalize(vec3(uv, camera_distance_from_screen));
    }
    #endif
    return ray_march(ray_pos, ray_dir, uv);
}

void main()
{
    vec2 uv = 2.*gl_FragCoord.xy/u_resolution.xy - 1.;
    float aspect_ratio = u_resolution.x/u_resolution.y;
    uv.x *= aspect_ratio;
    float e = fwidth(uv.y);

    float aa = .5*float(ANTI_ALIASING);
    vec3 frag_color = vec3(0);
    for (float i = -aa; i <= aa; ++i)
        for (float j = -aa; j <= aa; ++j)
            frag_color += pixel_color(uv + aa*e*vec2(i, j), aspect_ratio);
    aa = float(ANTI_ALIASING + 1);
    frag_color /= aa*aa;

    vec3 gamma_corrected = pow(frag_color, vec3(1./2.2));
    gl_FragColor = vec4(gamma_corrected, 1.0);
}
