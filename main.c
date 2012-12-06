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

#define fieldW 298
#define fieldH 598

#define screenWidth 800
#define screenHeight 800


int fieldX = (screenWidth-fieldW)/2;
int fieldY = (screenWidth-fieldH)/2;
SDL_Surface* screen;
SDL_Surface* graphics;
Uint32 started;
SDL_Rect blocks[6];
char field[12][25];

unsigned int figures [7][5]={{0b0000001100100010, 0b0000000001110001, 0b0000001000100110, 0b0000010001110000, 0},
                             {0b0000011000100010, 0b0000000101110000, 0b0000001000100011, 0b0000000001110100, 0},
                             {0b0000000001110010, 0b0000001001100010, 0b0000001001110000, 0b0000001000110010, 0},
                             {0b0000001001100100, 0b0000011000110000, 0, 0, 0},
                             {0b0000010001100010, 0b0000001101100000, 0, 0, 0},
                             {0b0000000011110000, 0b0010001000100010, 0, 0, 0},
                             {0b0000011001100000, 0, 0, 0, 0}};

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

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

void quit()
{
    SDL_Quit();
}

void BorderedRect(SDL_Surface* target, int x, int y, int width, int height, Uint32 borderColor, Uint32 backgroundColor)
{
    SDL_Rect drawingRect;
    drawingRect.x = x-1;
    drawingRect.y = y-1;
    drawingRect.w = width+2;
    drawingRect.h = height+2;
    SDL_FillRect(target, &drawingRect, borderColor);

    drawingRect.x = x+1;
    drawingRect.y = y+1;
    drawingRect.w = width-2;
    drawingRect.h = height-2;
    SDL_FillRect(target, &drawingRect, backgroundColor);
}

void InitGame()
{
    int i,j;

    for(i=0; i<3; i++)
    {
        for(j=0; j<2; j++)
        {
            blocks[i+j*3].x = 33*i+1;
            blocks[i+j*3].y = 33*j+1;
            blocks[i+j*3].w = 32;
            blocks[i+j*3].h = 32;
        }
    }

    for(i=1; i<11; i++)
    {
        for(j=0; j<24; j++)
        {
            field[i][j] = 0;
        }
    }

    for(j=0; j<24; j++)
    {
        field[0][j] = 1;
        field[11][j] = 1;
    }

    for(i=0; i<12; i++)
    {
        field[i][24] = 1;
    }
}

void DrawBlock(int x, int y, char color)
{
    if(color<1||color>6) return;
    SDL_Rect dst;
    dst.x = x;
    dst.y = y;
    dst.w = 32;
    dst.h = 32;
    SDL_BlitSurface(graphics, &blocks[color-1], screen, &dst);
}

void DrawFigure(char figure, char rotation, char color, int x, int y)
{
    for(int i = 0;i<4;i++)
    {
        for(int j = 0;j<4;j++)
        {
            if(figures[figure][rotation]>>(i+j*4)&0b1)
                DrawBlock(fieldX-2+(x-1)*30+i*30, fieldY-2+(y-4)*30+j*30, color);
        }
    }
}

void DrawStatic()
{
    for(int i = 1; i<11; i++)
    {
        for(int j = 4; j<24; j++)
        {
            DrawBlock(fieldX-2+(i-1)*30, fieldY-2+(j-4)*30, field[i][j]);
        }
    }
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

    screen = SDL_SetVideoMode(screenWidth, screenHeight, 32, videoflags);
    if ( screen == NULL )
    {
        fprintf(stderr, "Can't set Video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_WM_SetCaption("Tetris",NULL);

    graphics = SDL_LoadBMP("graphics.bmp");

    Uint32 background = getpixel(graphics, 0, 0);
    Uint32 fieldBackground = getpixel(graphics, 110, 15);
    Uint32 border = getpixel(graphics, 100, 2);

    SDL_SetColorKey(graphics, SDL_SRCCOLORKEY, background);

    InitGame();

    SDL_FillRect(screen, NULL, background);
    BorderedRect(screen, fieldX-1, fieldY-1, fieldW+2, fieldH+2, border, fieldBackground);
    char fig=0;
    char rot=0;
    char col=1;
    char fx=5;
    char fy=10;
    //DrawFigure(fig,rot,col,fx,fy);
    //DrawStatic();


    SDL_Flip(screen);

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
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_LEFT:
                            fx--;
                        break;
                        case SDLK_RIGHT:
                            fx++;
                        break;
                        case SDLK_UP:
                            fy--;
                        break;
                        case SDLK_DOWN:
                            fy++;
                        break;
                        case SDLK_r:
                            rot = figures[fig][rot+1]!=0?rot+1:0;
                        break;
                        case SDLK_c:
                            col = col<6?col+1:1;
                        break;
                        case SDLK_f:
                            fig = fig<6?fig+1:0;
                        break;
                    }
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

        SDL_FillRect(screen, NULL, fieldBackground);
        DrawFigure(fig,rot,col,fx,fy);
        SDL_UpdateRect(screen, fieldX, fieldY, fieldW, fieldH);
        //Перерыв. Чтоб программа не жрала все ядро
        SDL_Delay(TimeLeft());
    }

    return 0;
}
