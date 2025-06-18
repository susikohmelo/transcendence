#include <gamestate.h>
#include <vec.h>

static t_gamestate GAMESTATE;
#define u_ball    vec3(GAMESTATE.ball.x, GAMESTATE.ball.y, GAMESTATE.ball.z)
#define u_player1 vec3(GAMESTATE.player1.x, GAMESTATE.player1.y, -court_length)
#define u_player2 vec3(GAMESTATE.player2.x, GAMESTATE.player2.y, +court_length)
#define u_player  GAMESTATE.player

const Scalar ball_size    = .125f;
const Scalar court_length = 3.f;
const Vec3 paddle_size = {{.25f}, {.25f}, {.025f}, 0};

#define MIN_DISTANCE    .025f
#define MAX_DISTANCE    2.f
#define MAX_ITERATIONS  64
#define NORMAL_EPSILON  .001f
#define FIELD_OF_VIEW   70.f

// ----------------------------------------------------------------------------
// SDFs

Scalar sdf_sphere(Vec3 coords, Scalar radius)
{
    return vec_length(coords) - radius;
}

Scalar sdf_plane(Vec3 coords, Vec3 normal)
{
    return -vec_dot(coords, normal);
}

Scalar sdf_box(Vec3 coords, Vec3 size)
{
    coords         = vec_abs(coords);
    Vec3   surface = vec3_sub(coords, size);
    Scalar outside = vec_length(vec_max(vec3(0), surface));
    Scalar inside  = fmin(0.f, fmax(fmax(surface.x, surface.y), surface.z));
    return outside + inside;
}

Scalar sdf_box_rounded(Vec3 coords, Vec3 size, Scalar rounding)
{
    rounding *= fmin(fmin(size.x, size.y), size.z);
    return sdf_box(coords, vec_sub(size, rounding)) - rounding;
}

Scalar sdf_walls(Vec3 origin)
{
    Scalar separation = 1.f + ball_size - .05f;
    Scalar walls_outside = sdf_box(origin, vec3(court_length - .1f));
    Scalar walls_inside  = sdf_box_rounded(origin, vec3(separation, separation, court_length), .15f);
    Scalar walls = fmax(walls_outside, -walls_inside);

    origin      = vec_mul(vec_abs(origin), -1.f);
    Scalar ends = sdf_plane(vec_sub(vec3(0,0,-court_length), origin), vec3(0,0,1));
    return fmax(walls, -ends);
}

Scalar sdf_ball(Vec3 origin)
{
    return sdf_sphere(vec_sub(u_ball, origin), ball_size);
}

Scalar sdf_paddle_insides(Vec3 origin)
{
    Vec3 size   = paddle_size;
    Vec2 scaled = vec_mul(vec_xy(size), .9f);
    vec_xy_assign(size, scaled);
    size.z += .01f;
    return fmin(
        sdf_box(vec_sub(u_player1, origin), size),
        sdf_box(vec_sub(u_player2, origin), size));
}

Scalar sdf_paddles(Vec3 origin)
{
    Scalar paddle_rounding = .75f;
    Scalar outsides = fmin(
        sdf_box_rounded(vec_sub(u_player1, origin), paddle_size, paddle_rounding),
        sdf_box_rounded(vec_sub(u_player2, origin), paddle_size, paddle_rounding));

    return fmax(outsides, -sdf_paddle_insides(origin));
}

Scalar sdf_scene(Vec3 origin)
{
    Scalar sdf = 999999999.f;
    sdf = fmin(sdf, sdf_walls(origin));
    sdf = fmin(sdf, sdf_ball(origin));
    sdf = fmin(sdf, sdf_paddles(origin));
    return sdf;
}

Vec3 scene_normal(Vec3 origin)
{
    Vec2 e = vec2(NORMAL_EPSILON, 0);
    return vec_normalize(vec3(
        sdf_scene(vec_add(origin, vec_xyy(e))) - sdf_scene(vec_sub(origin, vec_xyy(e))),
        sdf_scene(vec_add(origin, vec_yxy(e))) - sdf_scene(vec_sub(origin, vec_yxy(e))),
        sdf_scene(vec_add(origin, vec_yyx(e))) - sdf_scene(vec_sub(origin, vec_yyx(e)))));
}

// ----------------------------------------------------------------------------
// Lighting

typedef struct Light
{
    Vec3 pos;
    Vec3 color;
} Light;

Vec3 phong(Vec3 ray_pos, Vec3 ray_dir, Vec3 normal)
{
    Vec3 material_color    = vec3(.55f, .1f, .85f);
    Vec2 material_gradient = vec_add(vec_rb(material_color), vec_mul(vec2(.5f*ray_pos.z, -ray_pos.z), .2f));
    vec_rb_assign(material_color, material_gradient);

    #define LIGHTS_COUNT 3
    Light lights[LIGHTS_COUNT] = {
        { u_ball, vec3(1) },
        { vec_add(u_player1, vec3(0,0,-.5f)), vec3(1) },
        { vec_sub(u_player2, vec3(0,0,-.5f)), vec3(1) }
    };

    Vec3 surface_color = vec3(0);

    for (size_t i = 0; i < LIGHTS_COUNT; ++i)
    {
        Vec3 light_dir      = vec_normalize(vec_sub(lights[i].pos, ray_pos));
        Vec3 reflection_dir = vec_reflect(light_dir, normal);

        Scalar diffuse = fmax(0.f, vec_dot(light_dir, normal));
        if (i == 0)
            diffuse *= 3.f*diffuse*diffuse;
        Scalar specular = fmax(0.f, vec_dot(ray_dir, reflection_dir));
        Scalar specular_shine = 512.f;
        specular = pow(specular, specular_shine);

        surface_color = vec_add(
            surface_color,
            vec_add(
                vec_mul(material_color, .5f*diffuse),
                vec_mul(lights[i].color, .1f*specular)));
    }
    return surface_color;
}

// ----------------------------------------------------------------------------
// Ray Marching

Scalar u_time = 0.f;

Vec3 ray_march(Vec3 ray_pos, Vec3 ray_dir)
{
    Vec3 surface_color = vec3(0);
    Scalar window_fog = 0.f;
    for (size_t i = 0; i < MAX_ITERATIONS; ++i)
    {
        Scalar scene_sdf = sdf_scene(ray_pos);
        Scalar glass_sdf = sdf_paddle_insides(ray_pos);
        Scalar sdf = fmin(scene_sdf, glass_sdf);
        if (sdf >= MAX_DISTANCE)
            break;
        if (sdf >= MIN_DISTANCE) {
            ray_pos = vec_add(ray_pos, vec_mul(ray_dir, sdf));
            continue;
        }
        if (glass_sdf <= MIN_DISTANCE) {
            window_fog += .1f;
            ray_pos = vec_add(ray_pos, vec_mul(ray_dir, 2.f*paddle_size.z));
            continue;
        }
        Vec3 normal = scene_normal(ray_pos);
        Vec3 contribution = phong(ray_pos, ray_dir, normal);
        // if (sdf_ball(ray_pos) <= MIN_DISTANCE) { // uncomment for more coloured rotating ball
        //     Vec3 rotated    = normal;
        //     Vec2 rotated_xy = vec2_rotate(vec_xy(rotated), 1.f*u_time);
        //     Vec2 rotated_zx = vec2_rotate(vec_zx(rotated), 1.f5*u_time);
        //     vec_xy_assign(rotated, rotated_xy);
        //     vec_zx_assign(rotated, rotated_zx);
        //     contribution = vec_add(contribution, vec_mul(vec_mul(rotated, rotated), .5f));
        // }
        surface_color = contribution;
        break;
    }
    return vec_add(surface_color, window_fog);
}

// ----------------------------------------------------------------------------
// Main Rendering

Vec3 pixel_color(Vec2 uv, bool split_screen_player2)
{
    const Scalar camera_distance_from_screen = 1.f/tan((M_PI/360.f)*FIELD_OF_VIEW);

    Vec3 ray_pos = vec3(0,0, -(court_length + camera_distance_from_screen + .4f));
    Vec3 ray_dir = vec_normalize(vec3(uv, camera_distance_from_screen));

    if (u_player == PLAYER2 || split_screen_player2) {
        ray_pos.z *= -1.f;
        ray_dir.z *= -1.f;
    }
    return ray_march(ray_pos, ray_dir);
}

Vec4 main_image(
    Vec2 i_coords,
    Vec2 i_resolution,
    Scalar aspect_ratio,
    Scalar char_height,
    Scalar time,
    t_gamestate gamestate)
{
    GAMESTATE = gamestate;
    u_time = time;
    bool split_screen_player2 = false;

    Vec2 uv = vec_sub(vec_mul(vec_div(i_coords, i_resolution), 2.f), 1.f);
    if (u_player != SPLIT_SCREEN)
    {
        if (i_resolution.x/char_height > i_resolution.y)
            uv.x *= aspect_ratio;
        else {
            uv.y /= aspect_ratio;
            uv = vec_mul(uv, char_height);
        }
    }
    else
    {
        uv.x *= char_height;
        uv = vec_mul(uv, 2.f);
        uv.x += 1.f;
        if (uv.x >= 1.f) {
            uv.x -= 2.f;
            uv = vec_mul(uv, -1.f);
            split_screen_player2 = true;
        }
        uv.x *= .5f*aspect_ratio;
        uv = vec_div(uv, char_height);
        if (.5f*i_resolution.x/char_height <= i_resolution.y)
            uv = vec_div(uv, .5f*aspect_ratio/char_height);
    }

    Vec3 frag_color = pixel_color(uv, split_screen_player2);

    Vec3 gamma_corrected = vec_pow(frag_color, vec3(1.f/2.2f));
    return vec4(gamma_corrected, 1.f);
}

