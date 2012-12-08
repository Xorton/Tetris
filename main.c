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

#define STATE_FALLING 1
#define STATE_LANDING 2
#define STATE_PACKING 3
#define STATE_GAMEOVER 4


int fieldX = (screenWidth-fieldW)/2;
int fieldY = (screenWidth-fieldH)/2;
SDL_Surface* screen;
SDL_Surface* graphics;
Uint32 started;
SDL_Rect blocks[6];
char field[12][25];
char title[100];

unsigned int figures [7][5]=
{
    {0b0000001100100010, 0b0000000001110001, 0b0000001000100110, 0b0000010001110000, 0},
    {0b0000011000100010, 0b0000000101110000, 0b0000001000100011, 0b0000000001110100, 0},
    {0b0000000001110010, 0b0000001001100010, 0b0000001001110000, 0b0000001000110010, 0},
    {0b0000001001100100, 0b0000011000110000, 0, 0, 0},
    {0b0000010001100010, 0b0000001101100000, 0, 0, 0},
    {0b0000000011110000, 0b0010001000100010, 0, 0, 0},
    {0b0000011001100000, 0, 0, 0, 0}
};

//Данные по фигурам [нижняя граница, центрХ, центрY] центр относительно ориджина
float figInfo[7][4][3]=
{
    { {2.0, 0.0, 0.5}, {1.0, 0.5, 0.0}, {2.0, 1.0, 0.5}, {2.0, 0.5, 1.0} },
    { {2.0, 1.0, 0.5}, {2.0, 0.5, 1.0}, {2.0, 0.0, 0.5}, {1.0, 0.5, 0.0} },
    { {1.0, 0.5, 0.0}, {2.0, 1.0, 0.5}, {2.0, 0.5, 1.0}, {2.0, 0.0, 0.5} },
    { {2.0, 1.0, 0.5}, {2.0, 0.5, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
    { {2.0, 1.0, 0.5}, {2.0, 0.5, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
    { {1.0, 1.0, 0.5}, {3.0, 0.5, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
    { {2.0, 1.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
};

char state;
char curFigure, nextFigure;
char curRotation, nextRotation;
char curColor, nextColor;
int figureX, figureY;

Uint32 background;
Uint32 fieldBackground;
Uint32 border;

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

    srand(time(NULL));
}

void DrawBlock(float x, float y, char color)
{
    if(color<1||color>6||y<4) return;
    SDL_Rect dst;
    dst.x = fieldX-2+(x-1)*30;
    dst.y = fieldY-2+(y-4)*30;
    dst.w = 32;
    dst.h = 32;
    SDL_BlitSurface(graphics, &blocks[color-1], screen, &dst);
}

void DrawFigure(char figure, char rotation, char color, float x, float y)
{
    //int k;
    for(int i = 0; i<4; i++)
    {
        for(int j = 0; j<4; j++)
        {
            if(figures[figure][rotation]>>(i+j*4)&0b1)
            {
                DrawBlock(x+i-1, y+j-1, color);
                /*k=y+j;
                while(field[(int)(x+i-1)][k]==0)
                {
                    DrawBlock(x+i-1, k, 1);
                    k++;
                }*/
            }
        }
    }
    //char ncolor = 7-color;
    //DrawBlock((float)(figInfo[figure][rotation][1]+x)-0.5, (float)(figInfo[figure][rotation][2]+y)-0.5, ncolor);
    //DrawBlock((float)(figInfo[figure][rotation][1]+x)-0.5, figInfo[figure][rotation][0]+y, ncolor);
}

void DrawStatic()
{
    for(int i = 1; i<11; i++)  //1 11
    {
        for(int j = 4; j<24; j++)   //4 24
        {
            DrawBlock(i, j, field[i][j]);
        }
    }
}

void NewFigure()
{
    curFigure = nextFigure;
    curRotation = nextRotation;
    curColor = nextColor;
    figureX = 5;
    //figureY = 1;

    do
    {
        nextFigure = rand()%7;
    }
    while(nextFigure==curFigure);

    do
    {
        nextColor = rand()%6+1;
    }
    while(nextColor==curColor);

    switch(nextFigure)
    {
    case 0:
    case 1:
    case 2:
        nextRotation = rand()%4;
        break;

    case 3:
    case 4:
    case 5:
        nextRotation = rand()%2;
        break;

    case 6:
        nextRotation = 0;
    }

    figureY = 4-figInfo[curFigure][curRotation][0];

    //SDL_FillRect(screen, NULL, fieldBackground);
    //DrawFigure(nextFigure, nextRotation, nextColor, 14.5-figInfo[nextFigure][nextRotation][1], 6.5-figInfo[nextFigure][nextRotation][2]);
    //SDL_UpdateRect(screen, fieldX+30*11, fieldY, 30*5, 30*5);
    //sprintf(title, "Figure %d, Rotation %d, Color %d", curFigure, curRotation, curColor);
    //SDL_WM_SetCaption(title,NULL);
}

bool IsCollide(int x, int y, int rotation)
{
    for(int i = 0; i<4; i++)
    {
        for(int j = 0; j<4; j++)
        {
            if( (figures[curFigure][rotation]>>(i+j*4)&0b1) && (field[x+i-1][y+j-1]!=0))
                return true;
        }
    }
    return false;
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

    background = getpixel(graphics, 0, 0);
    fieldBackground = getpixel(graphics, 110, 15);
    border = getpixel(graphics, 100, 2);

    SDL_SetColorKey(graphics, SDL_SRCCOLORKEY, background);

    InitGame();

    //SDL_FillRect(screen, NULL, background);
    //BorderedRect(screen, fieldX-1, fieldY-1, fieldW+2, fieldH+2, border, fieldBackground);
    //BorderedRect(screen, fieldX-1+30*11, fieldY-1, 30*5+2, 30*5+2, border, fieldBackground);
    curFigure=0;
    curRotation=0;
    curColor=2;
    figureX=5;
    figureY=10;
    //DrawFigure(fig,rot,col,fx,fy);
    //DrawStatic();


    SDL_Flip(screen);

    SDL_Event event;

    NewFigure();
    NewFigure();
    state = STATE_FALLING;

    started = SDL_GetTicks();

    char tRotation;


    //Главный цикл
    while(true)
    {
        //Обработка событий от SDL
        while (SDL_PollEvent(&event))
        {
            if(state==STATE_FALLING)
            {
                switch (event.type)
                {
                case SDL_MOUSEBUTTONDOWN:

                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                    case SDLK_LEFT:
                        if(!IsCollide(figureX-1, figureY, curRotation)) figureX--;
                        break;
                    case SDLK_RIGHT:
                        if(!IsCollide(figureX+1, figureY, curRotation)) figureX++;
                        break;
                    case SDLK_DOWN:
                        if(!IsCollide(figureX, figureY+1, curRotation)) figureY++;
                        break;
                    case SDLK_UP:
                        tRotation = figures[curFigure][curRotation+1]!=0?curRotation+1:0;
                        if(!IsCollide(figureX, figureY, tRotation)) curRotation = tRotation;
                        break;
                    case SDLK_END:
                        if(!IsCollide(figureX, figureY-1, curRotation)) figureY--;
                        break;
                    case SDLK_c:
                        curColor = curColor<6?curColor+1:1;
                        break;
                    case SDLK_f:
                        curFigure = curFigure<6?curFigure+1:0;
                        break;
                    case SDLK_n:
                        NewFigure();
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
        }

        if(SDL_GetTicks()-started>period_ms)
        {
            int moves;

            switch(state)
            {
            case STATE_FALLING:
                if(IsCollide(figureX, figureY+1, curRotation))
                {
                    for(int i = 0; i<4; i++)
                    {
                        for(int j = 0; j<4; j++)
                        {
                            if(figures[curFigure][curRotation]>>(i+j*4)&0b1)
                                field[figureX+i-1][figureY+j-1] = curColor;
                        }
                    }

                    bool isFull;
                    bool fullExist = false;

                    for(int j = 23; j>3; j--)
                    {
                        isFull = true;
                        for(int i = 1; i<11; i++)
                        {
                            if(field[i][j]==0)
                            {
                                isFull = false;
                                break;
                            }
                        }

                        if(isFull)
                        {
                            for(int i = 2; i<11; i++)
                            {
                                field[i][j]=0;
                            }
                            field[1][j] = 100;
                            fullExist = true;
                        }
                    }

                    NewFigure();

                    if(fullExist) state = STATE_PACKING;
                }
                else
                {
                    figureY++;
                }
                break;
                case STATE_PACKING:
                    moves = 0;
                    for(int j = 23; j>3; j--)
                    {
                        if(field[1][j]==100)
                        {
                            moves++;
                        }
                        else
                        {
                            if(moves>0)
                            {
                                for(int i = 1; i<11; i++)
                                {
                                    field[i][j+moves] = field[i][j];
                                    field[i][j] = 0;
                                }
                            }
                        }
                    }
                    state = STATE_FALLING;
                break;
            }
            started = SDL_GetTicks();
        }

        //SDL_FillRect(screen, NULL, fieldBackground);
        SDL_FillRect(screen, NULL, background);
        BorderedRect(screen, fieldX-1, fieldY-1, fieldW+2, fieldH+2, border, fieldBackground);
        BorderedRect(screen, fieldX-1+30*11, fieldY-1, 30*5+2, 30*5+2, border, fieldBackground);
        DrawStatic();
        DrawFigure(curFigure, curRotation, curColor, figureX, figureY);
        DrawFigure(nextFigure, nextRotation, nextColor, 14.5-figInfo[nextFigure][nextRotation][1], 6.5-figInfo[nextFigure][nextRotation][2]);
        //DrawFigure(fig, rot, col, fx, fy);
        //SDL_UpdateRect(screen, fieldX, fieldY, fieldW, fieldH);
        SDL_Flip(screen);

        //Перерыв. Чтоб программа не жрала все ядро
        SDL_Delay(TimeLeft());
    }

    return 0;
}
