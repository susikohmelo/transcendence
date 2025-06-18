// ----------------------------------------------------------------------------
// Constants

#define PLAYER1      1
#define PLAYER2      2
#define SPLIT_SCREEN 3

#define BALL_SIZE       .125f
#define COURT_LENGTH    3.f
#define PADDLE_SIZE     (float3)(.25f, .25f, .025f)

#define MIN_DISTANCE    .025f
#define MAX_DISTANCE    2.f
#define MAX_ITERATIONS  64
#define NORMAL_EPSILON  .001f
#define FIELD_OF_VIEW   70.f

// ----------------------------------------------------------------------------
// SDFs

float sdf_sphere(float3 coords, float radius)
{
    return length(coords) - radius;
}

float sdf_plane(float3 coords, float3 normal)
{
    return -dot(coords, normal);
}

float sdf_box(float3 coords, float3 size)
{
    coords         = fabs(coords);
    float3 surface = coords - size;
    float  outside = length(max((float3)(0), surface));
    float  inside  = min(0.f, max(max(surface.x, surface.y), surface.z));
    return outside + inside;
}

float sdf_box_rounded(float3 coords, float3 size, float rounding)
{
    rounding *= min(min(size.x, size.y), size.z);
    return sdf_box(coords, size - rounding) - rounding;
}

float sdf_walls(float3 origin)
{
    float separation    = 1.f + BALL_SIZE - .05f;
    float walls_outside = sdf_box(origin, (float3)(COURT_LENGTH - .1f));
    float walls_inside  = sdf_box_rounded(origin, (float3)(separation, separation, COURT_LENGTH), .15f);
    float walls = max(walls_outside, -walls_inside);

    origin     = -fabs(origin);
    float ends = sdf_plane((float3)(0,0,-COURT_LENGTH) - origin, (float3)(0,0,1));
    return max(walls, -ends);
}

float sdf_ball(float3 origin, float3 ball)
{
    return sdf_sphere(ball - origin, BALL_SIZE);
}

float sdf_paddle_insides(float3 origin, float3 player1, float3 player2)
{
    float3 size = PADDLE_SIZE;
    size.xy *= .9f;
    size.z  += .01f;
    return min(
        sdf_box(player1 - origin, size),
        sdf_box(player2 - origin, size));
}

float sdf_paddles(float3 origin, float3 player1, float3 player2)
{
    float paddle_rounding = .75f;
    float outsides = min(
        sdf_box_rounded(player1 - origin, PADDLE_SIZE, paddle_rounding),
        sdf_box_rounded(player2 - origin, PADDLE_SIZE, paddle_rounding));

    return max(outsides, -sdf_paddle_insides(origin, player1, player2));
}

float sdf_scene(float3 origin, float3 ball, float3 player1, float3 player2)
{
    float sdf = 999999999.f;
    sdf = min(sdf, sdf_walls(origin));
    sdf = min(sdf, sdf_ball(origin, ball));
    sdf = min(sdf, sdf_paddles(origin, player1, player2));
    return sdf;
}

float3 scene_normal(float3 origin, float3 ball, float3 player1, float3 player2)
{
    float2 e = (float2)(NORMAL_EPSILON, 0);
    return normalize((float3)(
        sdf_scene(origin + e.xyy, ball, player1, player2) - sdf_scene(origin - e.xyy, ball, player1, player2),
        sdf_scene(origin + e.yxy, ball, player1, player2) - sdf_scene(origin - e.yxy, ball, player1, player2),
        sdf_scene(origin + e.yyx, ball, player1, player2) - sdf_scene(origin - e.yyx, ball, player1, player2)));
}

// ----------------------------------------------------------------------------
// Lighting

typedef struct Light
{
    float3 pos;
    float3 color;
} Light;

float3 phong(
    float3 ray_pos, float3 ray_dir, float3 normal, float3 ball, float3 player1, float3 player2)
{
    float3 material_color  = (float3)(.55f, .1f, .85f);
    material_color.rb     += .2f*(float2)(.5f*ray_pos.z, -ray_pos.z);

    #define LIGHTS_COUNT 3
    Light lights[LIGHTS_COUNT] = {
        { ball, (float3)(1) },
        { player1 + (float3)(0,0,-.5f), (float3)(1) },
        { player2 - (float3)(0,0,-.5f), (float3)(1) },
    };

    float3 surface_color = (float3)(0);

    for (size_t i = 0; i < LIGHTS_COUNT; ++i)
    {
        float3 light_dir      = normalize(lights[i].pos - ray_pos);
        float3 reflection_dir = light_dir - 2.f*dot(light_dir, normal)*normal;

        float diffuse = max(0.f, dot(light_dir, normal));
        if (i == 0)
            diffuse *= 3.f*diffuse*diffuse;
        float specular = max(0.f, dot(ray_dir, reflection_dir));
        float specular_shine = 512.f;
        specular = pow(specular, specular_shine);

        surface_color += .5f*diffuse*material_color + .1f*specular*lights[i].color;
    }
    return surface_color;
}

// ----------------------------------------------------------------------------
// Ray Marching

float3 ray_march(
    float3 ray_pos, float3 ray_dir, float3 ball, float3 player1, float3 player2)
{
    float3 surface_color = (float3)(0);
    float  window_fog = 0.f;
    for (size_t i = 0; i < MAX_ITERATIONS; ++i)
    {
        float scene_sdf = sdf_scene(ray_pos, ball, player1, player2);
        float glass_sdf = sdf_paddle_insides(ray_pos, player1, player2);
        float sdf = min(scene_sdf, glass_sdf);
        if (sdf >= MAX_DISTANCE)
            break;
        if (sdf >= MIN_DISTANCE) {
            ray_pos += sdf*ray_dir;
            continue;
        }
        if (glass_sdf <= MIN_DISTANCE) {
            window_fog += .1f;
            ray_pos += 2.f*PADDLE_SIZE.z*ray_dir;
            continue;
        }
        float3 normal       = scene_normal(ray_pos, ball, player1, player2);
        float3 contribution = phong(ray_pos, ray_dir, normal, ball, player1, player2);
        // ball rotation and normal coloring would go here, skip that for now
        surface_color = contribution;
    }
    return surface_color + window_fog;
}

// ----------------------------------------------------------------------------
// Main Rendering

#define PI		3.14159265358979323846f

float3 pixel_color(
    float2 uv, bool split_screen_player2, float3 ball, float3 player1, float3 player2, int player)
{
    const float camera_distance_from_screen = 1.f/tan((PI/360.f)*FIELD_OF_VIEW);

    float3 ray_pos = (float3)(0,0, -(COURT_LENGTH + camera_distance_from_screen + .4f));
    float3 ray_dir = normalize((float3)(uv, camera_distance_from_screen));

    if (player == PLAYER2 || split_screen_player2) {
        ray_pos.z *= -1.f;
        ray_dir.z *= -1.f;
    }
    return ray_march(ray_pos, ray_dir, ball, player1, player2);
}

kernel void main_image(
    global float4* out,
    float3         ball,
    float2         player1xy,
    float2         player2xy,
    float          char_height,
    int            player)
{
    uint2 i_coords     = (uint2)(get_global_id(0),   get_global_id(1));
    uint2 i_resolution = (uint2)(get_global_size(0), get_global_size(1));
    float2 coords     = convert_float2(i_coords);
    float2 resolution = convert_float2(i_resolution);
    coords.x /= char_height;
    coords.x -= .5f * (resolution.x/char_height - resolution.x);

    bool split_screen_player2 = false;
    float2 uv = 2.f*coords/resolution - 1.f;
    uv.y *= -1.f;
    float aspect_ratio = resolution.x/resolution.y;
    if (player != SPLIT_SCREEN)
    {
        if (resolution.x/char_height >= resolution.y)
            uv.x *= aspect_ratio;
        else {
            uv.y /= aspect_ratio;
            uv *= char_height;
        }
    }
    else
    {
        uv.x *= char_height;
        uv *= 2.f;
        uv.x += 1.f;
        if (uv.x >= 1.f) {
            uv.x -= 2.f;
            uv *= -1.f;
            split_screen_player2 = true;
        }
        uv.x *= .5f*aspect_ratio;
        uv /= char_height;
        if (.5f*resolution.x/char_height <= resolution.y)
            uv /= .5f*aspect_ratio/char_height;
    }

    float3 player1 = (float3)(player1xy, -COURT_LENGTH);
    float3 player2 = (float3)(player2xy, +COURT_LENGTH);

    float3 frag_color = pixel_color(uv, split_screen_player2, ball, player1, player2, player);

    float3 gamma_corrected = pow(frag_color, (float3)(1.f/2.2f));
    out[i_coords.y*i_resolution.x + i_coords.x] = (float4)(gamma_corrected, 1.f);
}
