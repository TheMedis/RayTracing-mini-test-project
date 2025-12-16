#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

#define WIDTH 900
#define HEIGHT 600
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GRAY 0xefefefef
#define COLOR_YELLOW 0xffffd700
#define RAYS_NUMBER 4000
#define SHADOW_CIRCLE_SPEED 3
 
struct Circle
{
    double x;
    double y;
    double r;
};

struct Ray
{
    double x_start,y_start;
    double angle;
};
 

// fast line drawing function
// found bresenham algorithm online (i have a math degree but still im lazy to understand it fk it it works)
void draw_line_fast(SDL_Surface* surface, int x0, int y0, int x1, int y1, int thickness, Uint32 color)
{
    // Calculate Standard Bresenham Variables i dont understand the math but is ok it draws line
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    // check if line is more vertical or horizontal
    //so we can thicken it in right direction
    int is_horizontal = (dx > abs(dy));

    // get pixel array for surface
    Uint32* pixels = (Uint32*)surface->pixels;
    int pitch = surface->pitch / 4; // Pitch in integers
    int w = surface->w;
    int h = surface->h;
    
    int r = thickness / 2; 

    while (1) {
        for (int i = -r; i <= r; i++) {
            int draw_x = x0;
            int draw_y = y0;

            if (is_horizontal) {
                draw_y += i; // thicker on Y
            } else {
                draw_x += i; // thicker on X
            }

            // Dont break memory safety check no writing to random memory
            if (draw_x >= 0 && draw_x < w && draw_y >= 0 && draw_y < h) {
                pixels[draw_y * pitch + draw_x] = color;
            }
        }
        
        //check if reached end of line magic
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

//draw single pixel faster than drawing rectangle 1 pixel size
void put_pixel(SDL_Surface* surface, int x, int y, Uint32 color)
{
    // Safety check dont cook memory
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;

    // Get  array of pixels
    Uint32* pixels = (Uint32*)surface->pixels;

    // Calculate the index
    // pitch is the width of a row in bytes divide by 4 because Uint32 is 4 bytes
    int index = y * (surface->pitch / 4) + x;

    pixels[index] = color;
}


//moves obstacle up and down
void move_shadow_circle(struct Circle* circle,int* up)
{
    
    if (((circle->y - circle->r) <= 0) || ((circle->y + circle->r) >= HEIGHT)) 
    {
        *up = !(*up);
    }

	if (*up==0)
	{
	    circle->y -= SHADOW_CIRCLE_SPEED;
	}
	else
	{
	    circle->y += SHADOW_CIRCLE_SPEED;
	}
}




// Generate all rays from circle center
void generate_rays(struct Circle circle, struct Ray rays[RAYS_NUMBER])
{
    for (int i=0; i<RAYS_NUMBER;i++)
    {
        double angle = ((double) i / RAYS_NUMBER) * 2 * M_PI;
        struct Ray ray = {circle.x, circle.y, angle};
        rays[i] = ray;
    }   
}

// Draw all rays and check collisions with object and walls
void FillRays(SDL_Surface* surface, struct Ray rays[RAYS_NUMBER], Uint32 color, struct Circle object)
{
    // freeze surface memory to manually write os wont kill us i hope im scared to test without
    SDL_LockSurface(surface);

    for(int i = 0; i < RAYS_NUMBER; i++)
    {
        double angle = rays[i].angle;
        double start_x = rays[i].x_start;
        double start_y = rays[i].y_start;
        
        // precalculate trig
        double cos_a = cos(angle);
        double sin_a = sin(angle);

        // distance to wall initially bigger than screen
        double dist_wall = 1000000.0;
        
        // Vertical Wall collision (top or bot)
        if (cos_a != 0) {
            double target_x;
            if (cos_a > 0)
            {
                target_x = WIDTH;
            }
            else
            {
                target_x = 0;
            }
            double d = (target_x - start_x) / cos_a;
            if (d > 0 && d < dist_wall) dist_wall = d;
        }
        
        // Horizontal Wall collision (left or right)
        if (sin_a != 0) {
            double target_y;
            if (sin_a > 0 )
            {
                target_y = HEIGHT;
            }
            else
            {
                target_y = 0;
            }
            double d = (target_y - start_y) / sin_a;
            if (d > 0 && d < dist_wall) dist_wall = d;
        }
        
        // check collision with circle object
        double dist_circle = 1000000.0;
        
        // vector from Ray Start to Circle Center
        double ox = start_x - object.x;
        double oy = start_y - object.y;

        // i have no idea how this formula works but hey it works
        // quadratic coefficients: t^2 + 2(L*D)t + (|L|^2 - r^2) = 0
        //fancy math
        double b = 2.0 * (ox * cos_a + oy * sin_a);
        double c = pow(ox, 2) + pow(oy, 2) - pow(object.r, 2);
        double discriminant = b * b - 4 * c;

        if (discriminant >= 0) {
            // ray hits the circle line (enter or exit)
            // we only care about the entry point (smaller t)
            double t1 = (-b - sqrt(discriminant)) / 2.0;
            
            // t1 must be > 0 (in front of us)
            if (t1 > 0) dist_circle = t1;
        }

        // pick the closest intersection
        double final_dist;
        if (dist_circle < dist_wall)
        {
            final_dist = dist_circle;
        }
        else
        {
            final_dist = dist_wall;
        }
        // overshoot the ray into circle to lessen double to int rounding errors
        final_dist += 4.0;

        // draw line
        int end_x = (int)(start_x + final_dist * cos_a);
        int end_y = (int)(start_y + final_dist * sin_a);
        
        draw_line_fast(surface, (int)start_x, (int)start_y, end_x, end_y,3, color);
    }
    
    SDL_UnlockSurface(surface);
}


 
// Draws a filled circle onto the given surface.
// using SDL_FillRect for every single pixel
// we calculate distance from circle center to all pixels inside
// smallest bounding square that contains circle to decide if we draw it or not
//(i should improve this but sleep first)
//TODO : Do better circles 
void FillCircle(SDL_Surface* surface , struct Circle circle, Uint32 color)
{
    double radius_squared = pow(circle.r,2);

    // Iterate over the bounding Box (square area surrounding the circle)
    // rather than the entire screen
    for (double x = circle.x - circle.r; x <= circle.x + circle.r;x++)
    {

        for (double y = circle.y - circle.r; y <= circle.y + circle.r;y++)
        {
            
            double distance_squared = pow(x-circle.x,2) + pow(y-circle.y,2);
            // If the point is inside the circle draw it
            if (distance_squared < radius_squared)
            {
                put_pixel(surface,x,y,color);
            }
        }
    }

}





int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Raytracing", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    
    struct Circle circle = {200, 200, 80};
    struct Circle shadow_circle = {450, 300, 140};
    SDL_Rect erase_rect = {0, 0, WIDTH, HEIGHT}; 

    struct Ray rays[RAYS_NUMBER];
    generate_rays(circle, rays);

    int shadow_up = 0;
    int simulation_running = 1;
    
    int following_mouse = 1;

    SDL_Event event;
    while (simulation_running)
    {
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                simulation_running = 0;
            }

            //freeze light source on mouse click
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                // freeze unfreeze
                following_mouse = !following_mouse;
                
                // ff we just unfroze snap to current cursor
                if (following_mouse) {
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

        SDL_FillRect(surface, &erase_rect, COLOR_BLACK); //draw black rectangle to clear screen
        FillCircle(surface, circle, COLOR_WHITE);
        
        move_shadow_circle(&shadow_circle, &shadow_up);
        FillRays(surface, rays, COLOR_YELLOW, shadow_circle);
        FillCircle(surface, shadow_circle, COLOR_WHITE);
       
        SDL_Delay(10); //fps cap
        SDL_UpdateWindowSurface(window);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
