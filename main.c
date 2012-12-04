#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <windows.h>
#include <dirent.h>
#include <math.h>
#include <SDL\SDL.h>

#define videoflags SDL_HWSURFACE | SDL_DOUBLEBUF
#define sleep_ms 5
#define period_ms 500

int screen_width = 640;     //Размеры окна при старте
int screen_height = 480;    //Потом будут браться при настройке
SDL_Surface *screen;
Uint32 started;

Uint32 TimeLeft(void)
{
    static Uint32 next_time = 0;
    Uint32 now;

    now = SDL_GetTicks();
    if ( next_time <= now )
    {
        next_time = now+sleep_ms;
        return(0);
    }
    return(next_time-now);
}

void quit()
{
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    //Стандартные действия по инициализации SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0 )
    {
        fprintf(stderr, "Can't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    atexit(quit);

    //SDL_Surface *screen;
    screen = SDL_SetVideoMode(screen_width, screen_height, 32, videoflags);
    if ( screen == NULL )
    {
        fprintf(stderr, "Can't set Video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("Tetris",NULL); //Заголовок окна (будет браться из настроек)

    SDL_Event event;

    started = SDL_GetTicks();

    //Главный цикл
    while(true)
    {
        //Обработка событий от SDL
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_MOUSEBUTTONDOWN:

                break;
            case SDL_KEYDOWN:

                break;
            case SDL_KEYUP:
                if(event.key.keysym.sym==SDLK_ESCAPE)
                    exit(0);
                break;
            case SDL_QUIT:
                exit(0);
            }
        }

        if(SDL_GetTicks()-started>period_ms)
        {
            started = SDL_GetTicks();
        }
        //Перерыв. Чтоб программа не жрала все ядро
        SDL_Delay(TimeLeft());
    }

    return 0;
}
