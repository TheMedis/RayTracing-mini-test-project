#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

#define WIDTH 900
#define HEIGHT 600
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GRAY 0xefefefef
#define RAYS_NUMBER 1000
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



/**
 * generate all rays from circle center
 * 
 *
 *@param circle             struct circle.
 *@param rays[RAYS_NUMBER]  array that will contain all rays generated.
 */
void generate_rays(struct Circle circle, struct Ray rays[RAYS_NUMBER])
{
    for (int i=0; i<RAYS_NUMBER;i++)
    {
        double angle = ((double) i / RAYS_NUMBER) * 2 * M_PI;
        struct Ray ray = {circle.x, circle.y, angle};
        rays[i] = ray;
    }   
}

/**
* draw all rays and check collisions with object and border
*
*
*@param surface             the SDL surface to draw on.
*@param rays[RAYS_NUMBER]   the array containing all rays to draw.
*@param color               32-bit color value.
*@param object              struct Circle.
*/
void FillRays(SDL_Surface* surface, struct Ray rays[RAYS_NUMBER], Uint32 color,struct Circle object)
{
    double radius_squared = pow(object.r,2);
    for(int i = 0; i < RAYS_NUMBER; i++)
    {
        struct Ray ray = rays[i];

        int end_of_screen = 0;
        int object_hit = 0;

        double step = 1;
        double x_draw = ray.x_start;
        double y_draw = ray.y_start;
        while(!end_of_screen && !object_hit)
        {
            x_draw += step*cos(ray.angle);
            y_draw += step*sin(ray.angle);

            SDL_Rect pixel = (SDL_Rect){x_draw,y_draw,1,1};
            SDL_FillRect(surface,&pixel,color);
        
            if (x_draw < 0 || x_draw > WIDTH) end_of_screen = 1;
            if (y_draw < 0 || y_draw > HEIGHT) end_of_screen = 1;

            //Does the ray hit object
            double distance_squared = pow(x_draw-object.x,2) + pow(y_draw-object.y,2);
            if (distance_squared < radius_squared)
            {
                break;
            }
            
        }
        
    }
}


/** 
 * Draws a filled circle onto the given surface.
 * using SDL_FillRect for every single pixel
 * we calculate distance from circle center to all pixels inside
 * smallest bounding square that contains circle to decide if we draw it or not
 * 
 * @param surface  The SDL surface to draw on.
 * @param circle   Struct Circle
 * @param color    The 32-bit color value.
 */
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
                //draw rectangle 1 pixel size inside circle
                //could try manipulating pixel directly but lazy me wins again
                SDL_Rect pixel = (SDL_Rect){x,y,1,1}; 
                SDL_FillRect(surface,&pixel,color);
            }
        }
    }

}





int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Raytracing",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIDTH,HEIGHT,0);
    
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    
    struct Circle circle = {200,200,80};
    struct Circle shadow_circle = {650,300,140};
    SDL_Rect erase_rect = {0,0,WIDTH,HEIGHT}; //draw this to clear screen to black

    struct Ray rays[RAYS_NUMBER];
    generate_rays(circle,rays);

    int shadow_up = 0;
    int simulation_running = 1;
    SDL_Event event;
    while (simulation_running)
    {
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                simulation_running = 0;
            }
            //move circle to mouse click pos
            if (event.type == SDL_MOUSEMOTION && event.motion.state != 0)
            {
                
                circle.x = event.motion.x;
                circle.y = event.motion.y;
                generate_rays(circle,rays);
            }
        }
        SDL_FillRect(surface,&erase_rect,COLOR_BLACK);
        FillCircle(surface, circle, COLOR_WHITE);
        
        move_shadow_circle(&shadow_circle,&shadow_up);
        FillRays(surface,rays,COLOR_GRAY,shadow_circle);
        FillCircle(surface, shadow_circle, COLOR_WHITE);
       
        
        //cap framerate
        //// 1000 ms / 10 ms delay  = 100 max fps 
        SDL_Delay(10); 
        SDL_UpdateWindowSurface(window);
    }

    

}
