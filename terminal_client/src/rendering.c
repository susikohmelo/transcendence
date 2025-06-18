#define CL_TARGET_OPENCL_VERSION 300
#include <CL/opencl.h>

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <tengi.h>
#include <gamestate.h>
#include <vec.h>

#define C_END     "\e[0m"

#define FG_BLACK  "\e[0;30m"
#define FG_RED    "\e[0;31m"
#define FG_GREEN  "\e[0;32m"
#define FG_YELLOW "\e[0;33m"
#define FG_BLUE   "\e[0;34m"
#define FG_PURPLE "\e[0;35m"
#define FG_CYAN   "\e[0;36m"
#define FG_WHITE  "\e[0;37m"

#define BG_BLACK  "\e[40m"
#define BG_RED    "\e[41m"
#define BG_GREEN  "\e[42m"
#define BG_YELLOW "\e[43m"
#define BG_BLUE   "\e[44m"
#define BG_PURPLE "\e[45m"
#define BG_CYAN   "\e[46m"
#define BG_WHITE  "\e[47m"

t_gamestate *g_gamestate_ptr = NULL;

#if EXAMPLE_RENDERING // ------------------------------------------------------

#define MAX_ITERATIONS 64
#define MIN_DISTANCE  .01f

static Scalar sdf_sphere(Vec3 origin, Vec3 coords, Scalar radius)
{
    return vec3_length(vec3_sub(coords, origin)) - radius;
}

static Vec4 main_image(Vec2 i_coords, Vec2 i_resolution, Scalar aspect_ratio, Scalar time)
{
    Vec2 uv = vec_sub(vec_mul(vec_div(i_coords, i_resolution), 2.f), 1.f);
    uv.x *= aspect_ratio;

    Vec3 pixel_color = {0};
    Vec3 ray_origin = {0};
    Vec3 ray_dir = vec_normalize(vec3(uv, 1.f));

    Scalar animate = sin(.2f*M_PI*time);

    size_t i;
    for (i = 0; i < MAX_ITERATIONS; ++i)
    {
        Scalar length = sdf_sphere(ray_origin, vec3(-1.f, animate, 3.f), 1.f);
        if (length >= MIN_DISTANCE) {
            ray_origin = vec_add(ray_origin, vec_mul(ray_dir, length));
            continue;
        }
        pixel_color = vec3(3.2f - vec3_length(ray_origin), 0, 0);
        break;
    }
    if (i == MAX_ITERATIONS) {
        if (uv.x - animate*uv.y <= .75)
            pixel_color = vec3(0, vec_length(uv), vec_dot(uv, uv) - .5f);
        else
            pixel_color = vec3(1,0,fmax(0.f, uv.x - animate*uv.y - .75));
    }
    return vec4(vec_pow(pixel_color, 1.f/2.2f), 1);
}
#else
Vec4 main_image(Vec2 i_coords, Vec2 i_resolution, Scalar aspect_ratio, Scalar char_height, Scalar time, t_gamestate);
#endif

// ----------------------------------------------------------------------------
// Library usage

static void fill_render_buffer(Vec4 colors[], IVec2 resolution, t_gamestate gamestate)
{
    Vec2 char_size = tengi_estimate_cell_size();
    if (char_size.x == 0.f) {
        fprintf(stderr, "Character cell size is 0!\n");
        return;
    }
    Scalar char_height = char_size.y/char_size.x;
    Vec2 reso = vec2(resolution.x, resolution.y);
	double time = tengi_time();

    #pragma omp parallel for
	for (int y = 0; y < resolution.y; ++y)
	{
		for (int x = 0; x < resolution.x; ++x)
		{
            Vec2 coords = vec2(x, resolution.y - y);
            coords.x /= char_height;
            coords.x -= .5f * (reso.x/char_height - reso.x);

			colors[y*resolution.x + x]
                = main_image(
                    coords,
                    reso,
                    (Scalar)resolution.x/resolution.y,
                    char_height,
                    time,
                    gamestate);
		}
	}
}

typedef struct opencl_context
{
    cl_mem           out_buffer;
    cl_kernel        kernel;
    cl_program       program;
    cl_command_queue command_queue;
    cl_context       context;
} OpenCLContext;

static void release_context(OpenCLContext* cl)
{
    if (cl == NULL)
        return;
    if (cl->out_buffer != NULL)
        clReleaseMemObject(cl->out_buffer);
    if (cl->kernel != NULL)
        clReleaseKernel(cl->kernel);
    if (cl->program != NULL)
        clReleaseProgram(cl->program);
    if (cl->command_queue != NULL)
        clReleaseCommandQueue(cl->command_queue);
    if (cl->context != NULL)
        clReleaseContext(cl->context);
    memset(cl, 0, sizeof*cl);
}

static OpenCLContext* fill_render_buffer_accelerated(
    Vec4 colors[], IVec2 iresolution, cl_device_id device, t_gamestate gamestate)
{
    static cl_device_id last_device;
    static IVec2 last_resolution;
    static OpenCLContext cl;
    static char program_source[8192];
    static size_t global_work_size[2];

    if (device != last_device || iresolution.x != last_resolution.x || iresolution.y != last_resolution.y)
    { // (re)init
        release_context(&cl);

        cl.context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
        if (cl.context == NULL)
            return release_context(&cl), NULL;

        cl.command_queue = clCreateCommandQueueWithProperties(cl.context, device, NULL, NULL);
        if (cl.command_queue == NULL)
            return release_context(&cl), NULL;

        if (program_source[0] == '\0') {
            FILE* program_source_file = fopen("./src/frag.cl", "r");
            assert(program_source_file != NULL);
            size_t program_source_length = fread(program_source, 1, sizeof program_source, program_source_file);
            assert(program_source_length != 0);
            assert(program_source_length < sizeof program_source - sizeof"");
            program_source[program_source_length] = '\0';
            fclose(program_source_file);
        }
        cl.program = clCreateProgramWithSource(cl.context, 1, &(const char*){program_source}, NULL, NULL);
        if (cl.program == NULL)
            return release_context(&cl), NULL;
        if (clBuildProgram(cl.program, 1, &device, "", NULL, NULL) != CL_SUCCESS) {
            char diagnostics[256] = "";
            size_t diagnostics_length = 0;
            clGetProgramBuildInfo(
                cl.program, device, CL_PROGRAM_BUILD_LOG, sizeof diagnostics, diagnostics, &diagnostics_length);
            fprintf(stderr, "OpenCL program build failed: %s\n", diagnostics);
            exit(EXIT_FAILURE);
        }

        cl.out_buffer = clCreateBuffer(
            cl.context, CL_MEM_WRITE_ONLY, iresolution.x*iresolution.y*sizeof colors[0], NULL, NULL);
        if (cl.out_buffer == NULL)
            return release_context(&cl), NULL;

        cl.kernel = clCreateKernel(cl.program, "main_image", NULL);
        if (cl.kernel == NULL)
            return release_context(&cl), NULL;

        if (clSetKernelArg(cl.kernel, 0, sizeof(cl_mem), &cl.out_buffer) != CL_SUCCESS)
            return release_context(&cl), NULL;

        global_work_size[0] = iresolution.x;
        global_work_size[1] = iresolution.y;

        last_resolution = iresolution;
        last_device     = device;
    } // end init

    cl_float3 ball       = {.x = gamestate.ball.x,    .y = gamestate.ball.y, .z = gamestate.ball.z };
    cl_float2 player1    = {.x = gamestate.player1.x, .y = gamestate.player1.y };
    cl_float2 player2    = {.x = gamestate.player2.x, .y = gamestate.player2.y };
    Vec2 char_size       = tengi_estimate_cell_size();
    cl_float char_height = char_size.y/char_size.x;
    cl_int player        = gamestate.player;

    if (clSetKernelArg(cl.kernel, 1, sizeof ball,        &ball)        != CL_SUCCESS ||
        clSetKernelArg(cl.kernel, 2, sizeof player1,     &player1)     != CL_SUCCESS ||
        clSetKernelArg(cl.kernel, 3, sizeof player2,     &player2)     != CL_SUCCESS ||
        clSetKernelArg(cl.kernel, 4, sizeof char_height, &char_height) != CL_SUCCESS ||
        clSetKernelArg(cl.kernel, 5, sizeof player,      &player)      != CL_SUCCESS  )
        return release_context(&cl), NULL;

    if (clEnqueueNDRangeKernel(
            cl.command_queue, cl.kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL)
        != CL_SUCCESS)
        return release_context(&cl), NULL;

    if (clEnqueueReadBuffer(
            cl.command_queue,
            cl.out_buffer,
            CL_TRUE, 0,
            iresolution.x*iresolution.y*sizeof colors[0],
            colors, 0, NULL, NULL)
        != CL_SUCCESS)
        return release_context(&cl), NULL;

    if (clFinish(cl.command_queue) != CL_SUCCESS)
        return release_context(&cl), NULL;

    return &cl;
}

void *rendering_loop(void *void_gamestate_ptr)
{
    // --------------------------------
    // OpenCL

    cl_uint platforms_length = 0;
    cl_uint devices_length   = 0;
    cl_platform_id platforms[16];
    cl_device_id   devices[64];

    printf(
        "OpenCL platform 0: none\n"
        "\tDevice 0: default (CPU)\n\n");

    clGetPlatformIDs(sizeof platforms/sizeof platforms[0], platforms, &platforms_length);
    for (size_t i = 0; i < platforms_length; ++i) {
        char name[64] = "";
        size_t name_length = 0;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof name, name, &name_length);
        name_length -= name_length > 0;
        printf("OpenCL platform %zu: %s\n", i+1, name);

        cl_uint devices_for_platform;
        clGetDeviceIDs(
            platforms[i],
            CL_DEVICE_TYPE_ALL,
            sizeof devices/sizeof devices[0] - devices_length,
            devices + devices_length,
            &devices_for_platform);

        for (size_t j = 0; j < devices_for_platform; ++j) {
            clGetDeviceInfo(
                devices[devices_length + j],
                CL_DEVICE_NAME,
                sizeof name,
                name,
                &name_length);
            name_length -= name_length > 0;
            printf("\tDevice %zu: %s\n", devices_length + j + 1, name);
        }
        devices_length += devices_for_platform;
        puts("");
    }
    char text[1024] = "";

    OpenCLContext* opencl = NULL;
	g_gamestate_ptr = (t_gamestate*)void_gamestate_ptr;
    g_gamestate_ptr->resize_timer = 100; // don't wait initially

    while (true)
    {
        IVec2 resolution;

        pthread_mutex_lock(&g_gamestate_ptr->lock);
        t_gamestate gamestate = *g_gamestate_ptr;
        g_gamestate_ptr->resize_timer++;;
        pthread_mutex_unlock(&g_gamestate_ptr->lock);

        if (gamestate.exit_thread == true)
            break;

        size_t device_index = gamestate.selected_accelerator;
        if (device_index >= devices_length)
            device_index = -1;
        cl_device_id device = NULL;
        if (device_index != (size_t)-1)
            device = devices[device_index];

        if ( ! gamestate.game_running || gamestate.resize_timer <= 30)
        {
            usleep(1000*1000/60);
            continue;
        }
        else
        {
            resolution = tengi_get_terminal_resolution();

            Vec4* colors = malloc(sizeof(Vec4) * resolution.x*resolution.y);

            if (device_index == (size_t)-1)
    			fill_render_buffer(colors, resolution, gamestate);
            else
                assert(
                    (opencl = fill_render_buffer_accelerated(
                        colors, resolution, device, gamestate)
                    ) != NULL);

            tengi_draw(resolution, colors, text);

			free(colors);
		}

        usleep(1000*1000/60);

        if (gamestate.menu_state == MENU_GAME_ROOT) {
            if (gamestate.hovered == -1)
                snprintf(text, sizeof text,
                    FG_CYAN BG_BLACK "Select hardware accelerator" C_END "\n"
                    FG_RED  BG_BLACK "Exit                       " C_END "\n");
            else if (gamestate.hovered == 0)
                snprintf(text, sizeof text,
                    FG_BLACK BG_RED  "Select hardware accelerator" C_END "\n"
                    FG_RED BG_BLACK  "Exit                       " C_END "\n");
            else
                snprintf(text, sizeof text,
                    FG_CYAN BG_BLACK "Select hardware accelerator" C_END "\n"
                    FG_BLACK BG_RED  "Exit                       " C_END "\n");
        } else {
            text[0] = '\0';
            if (gamestate.hovered == 0)
                strcat(text, FG_BLACK BG_RED);
            else if (device_index == (size_t)-1)
                strcat(text, "\e[48;2;122;122;122m");
            else
                strcat(text, FG_CYAN BG_BLACK);
            sprintf(
                text + strlen(text), "%-*s" C_END "\n",
                MENU_SELECT_ACCELERATOR_LENGTH, "Default (CPU)");

            for (size_t i = 0; i < devices_length; ++i)
            {
                char name[MENU_SELECT_ACCELERATOR_LENGTH];
                size_t name_length = 0;
                clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof name, name, &name_length);
                if (gamestate.hovered - 1 == (int)i)
                    strcat(text, FG_BLACK BG_RED);
                else if (i == device_index)
                    strcat(text, "\e[48;2;122;122;122m");
                else
                    strcat(text, FG_CYAN BG_BLACK);
                sprintf(
                    text + strlen(text), "%-*s" C_END "\n",
                    MENU_SELECT_ACCELERATOR_LENGTH, name);
            }
        }
    }
    release_context(opencl);
    for (size_t i = 0; i < devices_length; ++i)
        clReleaseDevice(devices[i]);
    return NULL;
}

