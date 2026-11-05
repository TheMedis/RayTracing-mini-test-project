#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <math.h>

#define WIDTH 1000
#define HEIGHT 800
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GRAY 0xefefefef
#define COLOR_YELLOW 0xffffd700
#define RAYS_NUMBER 500  // 360 rays = 1 ray per degree, smooth with triangle filling
#define SHADOW_CIRCLE_SPEED 3
#define MAX_CIRCLES 10


struct Circle
{
    double x;
    double y;
    double r;
    double vx;  // velocity in x direction
    double vy;  // velocity in y direction
};

struct Ray
{
    double x_start, y_start;
    double angle;
    double cos_a;  // precomputed cosine
    double sin_a;  // precomputed sine
};

// Draw a filled triangle using scanline algorithm
void fill_triangle(SDL_Surface *surface, int x0, int y0, int x1, int y1, int x2, int y2, Uint32 color)
{
    // Sort vertices by y coordinate (y0 <= y1 <= y2)
    if (y0 > y1) { int tmp; tmp = y0; y0 = y1; y1 = tmp; tmp = x0; x0 = x1; x1 = tmp; }
    if (y0 > y2) { int tmp; tmp = y0; y0 = y2; y2 = tmp; tmp = x0; x0 = x2; x2 = tmp; }
    if (y1 > y2) { int tmp; tmp = y1; y1 = y2; y2 = tmp; tmp = x1; x1 = x2; x2 = tmp; }
    
    if (y0 == y2) return; // Degenerate triangle
    
    Uint32 *pixels = (Uint32 *)surface->pixels;
    int pitch = surface->pitch / 4;
    int w = surface->w;
    int h = surface->h;
    
    // Scan from top to bottom
    for (int y = y0; y <= y2; y++)
    {
        if (y < 0 || y >= h) continue;
        
        int xa, xb;
        
        // Calculate x coordinates for this scanline
        if (y < y1)
        {
            // Top half of triangle
            xa = y0 == y1 ? x0 : x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            xb = y0 == y2 ? x0 : x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        }
        else
        {
            // Bottom half of triangle
            xa = y1 == y2 ? x1 : x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            xb = y0 == y2 ? x0 : x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        }
        
        // Ensure xa <= xb
        if (xa > xb) { int tmp = xa; xa = xb; xb = tmp; }
        
        // Clamp to screen bounds
        if (xa < 0) xa = 0;
        if (xb >= w) xb = w - 1;
        
        // Draw horizontal line
        int row_offset = y * pitch;
        for (int x = xa; x <= xb; x++)
        {
            pixels[row_offset + x] = color;
        }
    }
}

// fast line drawing function
// found bresenham algorithm online (i have a math degree but still im lazy to understand it fk it it works)
void draw_line_fast(SDL_Surface *surface, int x0, int y0, int x1, int y1, int thickness, Uint32 color)
{
    // Calculate Standard Bresenham Variables i dont understand the math but is ok it draws line
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    // check if line is more vertical or horizontal
    // so we can thicken it in right direction
    int is_horizontal = (dx > abs(dy));

    // get pixel array for surface
    Uint32 *pixels = (Uint32 *)surface->pixels;
    int pitch = surface->pitch / 4; // Pitch in integers
    int w = surface->w;
    int h = surface->h;

    int r = thickness / 2;

    while (1)
    {
        for (int i = -r; i <= r; i++)
        {
            int draw_x = x0;
            int draw_y = y0;

            if (is_horizontal)
            {
                draw_y += i; // thicker on Y
            }
            else
            {
                draw_x += i; // thicker on X
            }

            // Dont break memory safety check no writing to random memory
            if (draw_x >= 0 && draw_x < w && draw_y >= 0 && draw_y < h)
            {
                pixels[draw_y * pitch + draw_x] = color;
            }
        }

        // check if reached end of line magic
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// draw single pixel faster than drawing rectangle 1 pixel size
void put_pixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
    // Safety check dont cook memory
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h)
        return;

    // Get  array of pixels
    Uint32 *pixels = (Uint32 *)surface->pixels;

    // Calculate the index
    // pitch is the width of a row in bytes divide by 4 because Uint32 is 4 bytes
    int index = y * (surface->pitch / 4) + x;

    pixels[index] = color;
}

// moves circles in random directions with random speeds and bounces off walls
void move_circles_random(struct Circle circles[], int count)
{
    for (int i = 0; i < count; i++)
    {
        // Update position
        circles[i].x += circles[i].vx;
        circles[i].y += circles[i].vy;
        
        // Bounce off left or right walls
        if (circles[i].x - circles[i].r <= 0)
        {
            circles[i].x = circles[i].r;
            circles[i].vx = -circles[i].vx;
        }
        else if (circles[i].x + circles[i].r >= WIDTH)
        {
            circles[i].x = WIDTH - circles[i].r;
            circles[i].vx = -circles[i].vx;
        }
        
        // Bounce off top or bottom walls
        if (circles[i].y - circles[i].r <= 0)
        {
            circles[i].y = circles[i].r;
            circles[i].vy = -circles[i].vy;
        }
        else if (circles[i].y + circles[i].r >= HEIGHT)
        {
            circles[i].y = HEIGHT - circles[i].r;
            circles[i].vy = -circles[i].vy;
        }
    }
}

void generate_rays(struct Circle circle, struct Ray rays[RAYS_NUMBER])
{
    for (int i = 0; i < RAYS_NUMBER; i++)
    {
        double angle = ((double)i / RAYS_NUMBER) * 2 * M_PI;
        struct Ray ray;
        ray.x_start = circle.x;
        ray.y_start = circle.y;
        ray.angle = angle;
        ray.cos_a = cos(angle);  // precompute
        ray.sin_a = sin(angle);  // precompute
        rays[i] = ray;
    }
}

// Check if light source is inside any of the shadow circles
int is_light_inside_circle(struct Circle light, struct Circle objects[], int object_count)
{
    for (int i = 0; i < object_count; i++)
    {
        double dx = light.x - objects[i].x;
        double dy = light.y - objects[i].y;
        double dist_squared = dx * dx + dy * dy;
        
        // If light center is inside the circle (no margin or light leaks through near the edge)
        if (dist_squared <= objects[i].r * objects[i].r)
        {
            return 1;  // Light is inside
        }
    }
    return 0;  // Light is not inside any circle
}

// Draw all rays and check collisions with objects and walls
void FillRays(SDL_Surface *surface, struct Ray rays[RAYS_NUMBER], Uint32 color, struct Circle light_source, struct Circle objects[], int object_count)
{
    // If light source is inside a circle, don't draw any rays
    if (is_light_inside_circle(light_source, objects, object_count))
    {
        return;  // Exit early, no rays drawn
    }
    
    // freeze surface memory to manually write os wont kill us i hope im scared to test without
    SDL_LockSurface(surface);

    // Store ray endpoints to draw triangles between them
    int endpoints_x[RAYS_NUMBER];
    int endpoints_y[RAYS_NUMBER];

    for (int i = 0; i < RAYS_NUMBER; i++)
    {
        double start_x = rays[i].x_start;
        double start_y = rays[i].y_start;

        // use precomputed trig values
        double cos_a = rays[i].cos_a;
        double sin_a = rays[i].sin_a;

        // distance to wall initially bigger than screen
        double dist_wall = 1000000.0;

        // Vertical Wall collision (left or right)
        if (cos_a != 0)
        {
            double target_x = (cos_a > 0) ? WIDTH : 0;
            double d = (target_x - start_x) / cos_a;
            if (d > 0 && d < dist_wall)
                dist_wall = d;
        }

        // Horizontal Wall collision (top or bottom)
        if (sin_a != 0)
        {
            double target_y = (sin_a > 0) ? HEIGHT : 0;
            double d = (target_y - start_y) / sin_a;
            if (d > 0 && d < dist_wall)
                dist_wall = d;
        }

        // check collision with all circle objects
        double dist_closest_circle = 1000000.0;

        // loop through all objects and find closest hit
        for (int j = 0; j < object_count; j++)
        {
            struct Circle object = objects[j];
            
            // vector from Ray Start to Circle Center
            double ox = start_x - object.x;
            double oy = start_y - object.y;

            // i have no idea how this formula works but hey it works
            // quadratic coefficients: t^2 + 2(L*D)t + (|L|^2 - r^2) = 0
            // fancy math
            double b = 2.0 * (ox * cos_a + oy * sin_a);
            double c = ox * ox + oy * oy - object.r * object.r;  // faster than pow()
            double discriminant = b * b - 4.0 * c;

            if (discriminant >= 0)
            {
                // ray hits the circle line (enter or exit)
                // we only care about the entry point (smaller t)
                double t1 = (-b - sqrt(discriminant)) / 2.0;

                // t1 must be > 0 (in front of us)
                if (t1 > 0 && t1 < dist_closest_circle)
                    dist_closest_circle = t1;
            }
        }

        // pick the closest intersection and overshoot slightly
        double final_dist = (dist_closest_circle < dist_wall ? dist_closest_circle : dist_wall) + 4.0;

        // Store endpoint for triangle filling
        endpoints_x[i] = (int)(start_x + final_dist * cos_a);
        endpoints_y[i] = (int)(start_y + final_dist * sin_a);
    }
    
    // Draw filled triangles between consecutive rays to eliminate gaps
    int center_x = (int)rays[0].x_start;
    int center_y = (int)rays[0].y_start;
    
    for (int i = 0; i < RAYS_NUMBER; i++)
    {
        int next = (i + 1) % RAYS_NUMBER;  // Wrap around to close the circle
        fill_triangle(surface, center_x, center_y, 
                     endpoints_x[i], endpoints_y[i],
                     endpoints_x[next], endpoints_y[next], color);
    }

    SDL_UnlockSurface(surface);
}

// Draws a filled circle onto the given surface.
// using SDL_FillRect for every single pixel
// we calculate distance from circle center to all pixels inside
// smallest bounding square that contains circle to decide if we draw it or not
//(i should improve this but sleep first)
// TODO : Do better circles
void FillCircle(SDL_Surface *surface, struct Circle circle, Uint32 color)
{
    double radius_squared = circle.r * circle.r;
    int min_x = (int)(circle.x - circle.r);
    int max_x = (int)(circle.x + circle.r);
    int min_y = (int)(circle.y - circle.r);
    int max_y = (int)(circle.y + circle.r);
    
    // Clamp to screen bounds
    if (min_x < 0) min_x = 0;
    if (max_x >= WIDTH) max_x = WIDTH - 1;
    if (min_y < 0) min_y = 0;
    if (max_y >= HEIGHT) max_y = HEIGHT - 1;

    // Direct pixel access for speed
    Uint32 *pixels = (Uint32 *)surface->pixels;
    int pitch = surface->pitch / 4;
    double cx = circle.x;
    double cy = circle.y;

    // Scanline algorithm - only check pixels that might be in circle
    for (int y = min_y; y <= max_y; y++)
    {
        double dy = y - cy;
        double dy2 = dy * dy;
        
        // Early exit if row is outside circle
        if (dy2 > radius_squared) continue;
        
        // Calculate horizontal extent for this scanline
        double half_width = sqrt(radius_squared - dy2);
        int x_start = (int)(cx - half_width);
        int x_end = (int)(cx + half_width);
        
        // Clamp to screen
        if (x_start < 0) x_start = 0;
        if (x_end >= WIDTH) x_end = WIDTH - 1;
        
        // Draw horizontal line (much faster than individual pixels)
        int row_offset = y * pitch;
        for (int x = x_start; x <= x_end; x++)
        {
            pixels[row_offset + x] = color;
        }
    }
}

int main()
{
    // Seed random number generator
    srand(time(NULL));
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Shadow Casting",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create GPU renderer with hardware acceleration
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
                                                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // no gpu (vm, remote desktop...) so use software renderer instead
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create texture for software rendering (we still need surface for pixel manipulation)
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             WIDTH, HEIGHT);

    // Create surface for pixel manipulation
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!texture || !surface)
    {
        printf("could not create texture/surface: %s\n", SDL_GetError());
        return 1;
    }

    struct Circle circle = {200, 200, 10, 0, 0};
    
    // Array of obstacle circles that cast shadows
    struct Circle shadow_circles[MAX_CIRCLES];
    int shadow_circle_count = 5;  // Change this number to create more or fewer circles
    
    // Generate circles with random positions, sizes, and velocities
    for (int i = 0; i < shadow_circle_count; i++)
    {
        double radius = 30 + rand() % 120;  // Random radius between 30 and 150
        double x = radius + rand() % (int)(WIDTH - 2 * radius);   // Random x ensuring circle fits
        double y = radius + rand() % (int)(HEIGHT - 2 * radius);  // Random y ensuring circle fits
        double vx = (rand() % 9 - 4);  // Random x velocity between -4 and 4
        double vy = (rand() % 9 - 4);  // Random y velocity between -4 and 4
        
        shadow_circles[i] = (struct Circle){x, y, radius, vx, vy};
    }
    
    SDL_Rect erase_rect = {0, 0, WIDTH, HEIGHT};

    struct Ray rays[RAYS_NUMBER];
    generate_rays(circle, rays);

    int simulation_running = 1;

    int following_mouse = 1;

    SDL_Event event;
    while (simulation_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                simulation_running = 0;
            }

            // freeze light source on mouse click
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                // freeze unfreeze
                following_mouse = !following_mouse;

                // ff we just unfroze snap to current cursor
                if (following_mouse)
                {
                    int mx, my;
                    SDL_GetMouseState(&mx, &my);
                    circle.x = mx;
                    circle.y = my;
                    generate_rays(circle, rays);
                }
            }

            // light follows mouse
            if (event.type == SDL_MOUSEMOTION && following_mouse)
            {
                circle.x = event.motion.x;
                circle.y = event.motion.y;
                generate_rays(circle, rays);
            }

        }

        SDL_FillRect(surface, &erase_rect, COLOR_BLACK); // draw black rectangle to clear screen
        FillCircle(surface, circle, COLOR_WHITE);

        // Move and draw all shadow circles
        move_circles_random(shadow_circles, shadow_circle_count);
        
        FillRays(surface, rays, COLOR_YELLOW, circle, shadow_circles, shadow_circle_count);
        
        for (int i = 0; i < shadow_circle_count; i++)
        {
            FillCircle(surface, shadow_circles[i], COLOR_WHITE);
        }

        // Upload surface to GPU texture and render
        SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(10); // fps cap
    }

    // Cleanup
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
