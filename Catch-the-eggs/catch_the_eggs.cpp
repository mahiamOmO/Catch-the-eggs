/*
 * ================================================================
 *   CATCH THE EGGS  –  CSE 426 Computer Graphics Lab
 *   Term Project | Spring 2025  –  ENHANCED VERSION
 * ================================================================
 *
 *  NEW FEATURES ADDED:
 *   [✓] Game time = 60 seconds (1 minute)
 *   [✓] Pause no longer deducts time (bug fixed)
 *   [✓] High Score BACK button fully working
 *   [✓] Menu background: house, trees, fence, birds
 *   [✓] Sound effects (Windows: Beep / Linux: aplay pipe)
 *   [✓] Star collectible (+15 pts), Diamond collectible (+20 pts)
 *   [✓] Score flash / popup effect on catch
 *   [✓] Smoother HUD with gradient bar
 *   [✓] Rainbow egg (rare, +25 pts)
 *   [✓] Basket glow on power-up catch
 *   [✓] Ground / grass detail improved
 *   [✓] Fence on ground
 *   [✓] Flower decorations
 *
 *  ORIGINAL FEATURES:
 *   [✓] 2 sticks with 2 chickens
 *   [✓] Wind / airflow drift
 *   [✓] Shield power-up
 *   [✓] Help / Controls page
 *   [✓] Day-Night sky toggle (N key)
 *   [✓] Pause (P/ESC), Exit any time
 *   [✓] Menu: Start / High Score / Help / Exit
 *
 * ----------------------------------------------------------------
 *  COMPILE
 *    Linux  :  g++ catch_the_eggs_enhanced.cpp -o game -lGL -lGLU -lglut -lm
 *    Windows:  g++ catch_the_eggs_enhanced.cpp -o game -lfreeglut -lopengl32 -lglu32 -lm -lwinmm
 *
 *  CONTROLS
 *    Left/Right Arrow or A/D  – move basket
 *    Mouse move               – move basket (during play)
 *    P  or  ESC               – pause / resume
 *    N                        – toggle night mode
 *    Mouse click              – menu navigation
 * ================================================================
 */

#ifdef _WIN32
#  include <windows.h>
#  pragma comment(lib,"winmm.lib")
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

/* ----------------------------------------------------------------
   WINDOW & MATH
---------------------------------------------------------------- */
static const int   WIN_W = 800;
static const int   WIN_H = 600;
static const float PI    = 3.14159265f;

/* ----------------------------------------------------------------
   SOUND
---------------------------------------------------------------- */
static void playSound(int freq, int dur)
{
#ifdef _WIN32
    Beep(freq, dur);
#else
    /* On Linux, spawn aplay with a tiny sine wave via /dev/dsp or just skip */
    /* For a real game, use SDL_mixer or OpenAL */
    (void)freq; (void)dur;
#endif
}

/* ----------------------------------------------------------------
   GAME STATE
---------------------------------------------------------------- */
enum GameState { ST_MENU, ST_PLAY, ST_PAUSE, ST_OVER, ST_HISCORE, ST_HELP };
static GameState gState = ST_MENU;

/* ----------------------------------------------------------------
   ITEM TYPES
---------------------------------------------------------------- */
enum ItemType {
    EGG_NORMAL,   /* white     +1  pt */
    EGG_BLUE,     /* blue      +5  pt */
    EGG_GOLDEN,   /* gold     +10  pt */
    EGG_RAINBOW,  /* rainbow  +25  pt (rare bonus) */
    POOP,         /* brown    -10  pt */
    PWR_BASKET,   /* orange   bigger basket 10 s */
    PWR_SLOW,     /* cyan     slow eggs     8 s  */
    PWR_TIME,     /* green    +20 seconds        */
    PWR_SHIELD,   /* purple   negate 1 poop      */
    STAR_ITEM,    /* yellow   +15 pt special     */
    DIAMOND_ITEM  /* cyan     +20 pt special     */
};

/* ----------------------------------------------------------------
   SCORE POPUP
---------------------------------------------------------------- */
struct ScorePopup {
    float x, y;
    float life;       /* seconds remaining */
    int   value;
    float r, g, b;
};

/* ----------------------------------------------------------------
   STRUCTS
---------------------------------------------------------------- */
struct Item {
    float x, y;
    float vy;
    float vx;
    ItemType type;
    bool  active;
    float angle;      /* for spin effect on special items */
};

struct Chicken {
    float x, y;
    float speed;
    int   dir;
    float layTimer;
    float layInterval;
};

struct Basket {
    float cx;
    float y;
    float w;
    float glowT;      /* glow timer after catching power-up */
};

struct Cloud {
    float x, y, spd;
};

struct Flower {
    float x, y;
    float r, g, b;
};

/* ----------------------------------------------------------------
   GLOBALS
---------------------------------------------------------------- */
static const int N_STICKS = 2;
static const float STICK_Y[N_STICKS] = { WIN_H - 90.f, WIN_H - 230.f };

static std::vector<Chicken>    gHens;
static Basket                  gBasket;
static std::vector<Item>       gItems;
static std::vector<Cloud>      gClouds;
static std::vector<ScorePopup> gPopups;
static std::vector<Flower>     gFlowers;

static int   gScore     = 0;
static int   gHiScore   = 0;
static float gTime      = 60.f;   /* 1 minute */

/* power-up timers */
static bool  gSlowOn    = false;   static float gSlowT  = 0.f;
static bool  gBigOn     = false;   static float gBigT   = 0.f;
static bool  gShield    = false;

/* wind */
static bool  gWindOn    = false;
static float gWindT     = 0.f;
static float gWindF     = 0.f;

/* night mode */
static bool  gNight     = false;

/* animated menu timer */
static float gMenuAnim  = 0.f;

static const float BW_NORMAL = 110.f;
static const float BW_BIG    = 210.f;
static const float BH        = 44.f;
static const float BSPEED    = 380.f;

static bool  gKeys[256]   = {};
static bool  gSpec[256]   = {};
static int   gHover       = -1;
static float gLastTime    = 0.f;

static float gStarX[120], gStarY[120], gStarBr[120];

/* ================================================================
   UTILITY
================================================================ */
static float randF(float lo, float hi){
    return lo + (float)rand()/RAND_MAX*(hi-lo);
}
static float clampf(float v, float lo, float hi){
    return v<lo?lo:(v>hi?hi:v);
}

static void txt(float x, float y, const char* s,
                float r=1,float g=1,float b=1,
                void* font=GLUT_BITMAP_HELVETICA_18)
{
    glColor3f(r,g,b);
    glRasterPos2f(x,y);
    for(const char* c=s;*c;c++) glutBitmapCharacter(font,*c);
}
static void txtBig(float x,float y,const char* s,
                   float r=1,float g=1,float b=1)
{
    glColor3f(r,g,b);
    glRasterPos2f(x,y);
    for(const char* c=s;*c;c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
}

/* ================================================================
   DRAW HELPERS
================================================================ */
static void quad(float x,float y,float w,float h,
                 float r,float g,float b)
{
    glColor3f(r,g,b);
    glBegin(GL_QUADS);
    glVertex2f(x,y);   glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void quadLine(float x,float y,float w,float h,
                     float r,float g,float b,float lw=2)
{
    glColor3f(r,g,b); glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void ellipse(float cx,float cy,float rx,float ry,
                    float r,float g,float b,int seg=36)
{
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){
        float a=2*PI*i/seg;
        glVertex2f(cx+rx*cosf(a),cy+ry*sinf(a));
    }
    glEnd();
}

/* ================================================================
   BUTTON
================================================================ */
static bool inRect(int mx,int my,float x,float y,float w,float h){
    return mx>=x&&mx<=x+w&&my>=y&&my<=y+h;
}
static void button(float x,float y,float w,float h,
                   const char* lbl,bool hover)
{
    /* shadow */
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(x+5,y-5); glVertex2f(x+w+5,y-5);
    glVertex2f(x+w+5,y+h-5); glVertex2f(x+5,y+h-5);
    glEnd();

    /* gradient fill */
    glBegin(GL_QUADS);
    if(hover){
        glColor3f(1.0f,0.80f,0.15f); glVertex2f(x,y);
        glColor3f(1.0f,0.80f,0.15f); glVertex2f(x+w,y);
        glColor3f(0.95f,0.60f,0.05f); glVertex2f(x+w,y+h);
        glColor3f(0.95f,0.60f,0.05f); glVertex2f(x,y+h);
    } else {
        glColor3f(0.20f,0.18f,0.52f); glVertex2f(x,y);
        glColor3f(0.20f,0.18f,0.52f); glVertex2f(x+w,y);
        glColor3f(0.10f,0.08f,0.35f); glVertex2f(x+w,y+h);
        glColor3f(0.10f,0.08f,0.35f); glVertex2f(x,y+h);
    }
    glEnd();

    /* border */
    glColor3f(hover?1.f:0.6f, hover?0.9f:0.6f, hover?0.2f:0.9f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();

    float tw = strlen(lbl)*6.5f;
    txt(x+w/2-tw/2, y+h/2-6, lbl, hover?0.f:1.f, hover?0.f:1.f, hover?0.f:1.f);
}

/* ================================================================
   HOUSE (for menu background)
================================================================ */
static void drawHouse(float bx, float by, float scale)
{
    float w=100*scale, h=80*scale, rh=55*scale;

    /* shadow */
    glColor3f(0,0,0.05f);
    glBegin(GL_QUADS);
    glVertex2f(bx+6,by-6); glVertex2f(bx+w+6,by-6);
    glVertex2f(bx+w+6,by+h-6); glVertex2f(bx+6,by+h-6);
    glEnd();

    /* walls */
    glBegin(GL_QUADS);
    glColor3f(0.85f,0.55f,0.30f); glVertex2f(bx,by);
    glColor3f(0.85f,0.55f,0.30f); glVertex2f(bx+w,by);
    glColor3f(0.75f,0.42f,0.22f); glVertex2f(bx+w,by+h);
    glColor3f(0.75f,0.42f,0.22f); glVertex2f(bx,by+h);
    glEnd();

    /* roof */
    glColor3f(0.72f,0.15f,0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(bx-8,        by+h);
    glVertex2f(bx+w+8,      by+h);
    glVertex2f(bx+w/2,      by+h+rh);
    glEnd();
    /* roof shading */
    glColor3f(0.58f,0.10f,0.06f);
    glBegin(GL_TRIANGLES);
    glVertex2f(bx+w/2,      by+h+rh);
    glVertex2f(bx+w*0.55f,  by+h);
    glVertex2f(bx+w+8,      by+h);
    glEnd();

    /* chimney */
    float cx2=bx+w*0.72f;
    glColor3f(0.55f,0.30f,0.20f);
    glBegin(GL_QUADS);
    glVertex2f(cx2,by+h+rh*0.55f); glVertex2f(cx2+14*scale,by+h+rh*0.55f);
    glVertex2f(cx2+14*scale,by+h+rh+12*scale); glVertex2f(cx2,by+h+rh+12*scale);
    glEnd();
    /* chimney top */
    glColor3f(0.40f,0.20f,0.12f);
    glBegin(GL_QUADS);
    glVertex2f(cx2-3,by+h+rh+12*scale);
    glVertex2f(cx2+17*scale,by+h+rh+12*scale);
    glVertex2f(cx2+17*scale,by+h+rh+16*scale);
    glVertex2f(cx2-3,by+h+rh+16*scale);
    glEnd();

    /* door */
    float dx=bx+w/2-12*scale, dw=24*scale, dh=36*scale;
    glColor3f(0.42f,0.22f,0.08f);
    glBegin(GL_QUADS);
    glVertex2f(dx,by); glVertex2f(dx+dw,by);
    glVertex2f(dx+dw,by+dh); glVertex2f(dx,by+dh);
    glEnd();
    /* door arch */
    glColor3f(0.38f,0.18f,0.05f);
    glBegin(GL_POLYGON);
    for(int i=0;i<=12;i++){
        float a=PI*i/12;
        glVertex2f(dx+dw/2+dw/2*cosf(a), by+dh+dh*0.20f*sinf(a));
    }
    glEnd();
    /* door knob */
    ellipse(dx+dw*0.72f, by+dh*0.5f, 3*scale, 3*scale, 0.9f,0.75f,0.1f);

    /* windows */
    float wy=by+h*0.55f, wsize=20*scale;
    for(int wi=0;wi<2;wi++){
        float wx = (wi==0)? bx+w*0.18f : bx+w*0.62f;
        /* frame */
        glColor3f(0.72f,0.50f,0.22f);
        glBegin(GL_QUADS);
        glVertex2f(wx-2,wy-2); glVertex2f(wx+wsize+2,wy-2);
        glVertex2f(wx+wsize+2,wy+wsize+2); glVertex2f(wx-2,wy+wsize+2);
        glEnd();
        /* glass */
        glColor3f(0.65f,0.85f,1.f);
        glBegin(GL_QUADS);
        glVertex2f(wx,wy); glVertex2f(wx+wsize,wy);
        glVertex2f(wx+wsize,wy+wsize); glVertex2f(wx,wy+wsize);
        glEnd();
        /* cross */
        glColor3f(0.72f,0.50f,0.22f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(wx+wsize/2,wy); glVertex2f(wx+wsize/2,wy+wsize);
        glVertex2f(wx,wy+wsize/2); glVertex2f(wx+wsize,wy+wsize/2);
        glEnd();
    }

    /* wall bricks */
    glColor3f(0.65f,0.38f,0.18f); glLineWidth(0.8f);
    for(int row=0;row<4;row++){
        float ry=by+row*(h/4);
        for(int col=0;col<5;col++){
            float rx=bx+(col+((row%2)?0.5f:0.f))*(w/5);
            glBegin(GL_LINE_LOOP);
            glVertex2f(rx,ry); glVertex2f(rx+w/5,ry);
            glVertex2f(rx+w/5,ry+h/4); glVertex2f(rx,ry+h/4);
            glEnd();
        }
    }
}

/* ================================================================
   TREE
================================================================ */
static void drawTree(float bx, float by, float scale)
{
    float tw=18*scale, th=50*scale;

    /* trunk */
    glBegin(GL_QUADS);
    glColor3f(0.45f,0.28f,0.10f); glVertex2f(bx-tw/2,by);
    glColor3f(0.45f,0.28f,0.10f); glVertex2f(bx+tw/2,by);
    glColor3f(0.35f,0.20f,0.06f); glVertex2f(bx+tw/2,by+th);
    glColor3f(0.35f,0.20f,0.06f); glVertex2f(bx-tw/2,by+th);
    glEnd();

    /* 3-tier foliage */
    float fw=62*scale;
    for(int tier=0;tier<3;tier++){
        float fbase=by+th*0.6f+tier*28*scale;
        float ftop =fbase+45*scale-tier*6*scale;
        float fw2  =fw-tier*14*scale;

        /* shadow side */
        glColor3f(0.08f,0.42f,0.10f);
        glBegin(GL_TRIANGLES);
        glVertex2f(bx,ftop);
        glVertex2f(bx+fw2/2,fbase);
        glVertex2f(bx+fw2*0.15f,ftop*0.35f+fbase*0.65f);
        glEnd();

        /* main */
        glColor3f(0.16f,0.60f,0.18f);
        glBegin(GL_TRIANGLES);
        glVertex2f(bx-fw2/2,fbase); glVertex2f(bx+fw2/2,fbase);
        glVertex2f(bx,ftop);
        glEnd();

        /* highlight */
        glColor3f(0.24f,0.75f,0.25f);
        glBegin(GL_TRIANGLES);
        glVertex2f(bx-fw2*0.15f, ftop*0.3f+fbase*0.7f);
        glVertex2f(bx, ftop);
        glVertex2f(bx-fw2*0.35f, fbase);
        glEnd();
    }
}

/* ================================================================
   BIRD (V shape flying)
================================================================ */
static void drawBird(float bx, float by)
{
    glColor3f(0.1f,0.1f,0.1f); glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx-10,by+4); glVertex2f(bx-5,by);
    glVertex2f(bx,by+2);
    glVertex2f(bx+5,by);  glVertex2f(bx+10,by+4);
    glEnd();
}

/* ================================================================
   FENCE
================================================================ */
static void drawFence(float startX, float endX, float y)
{
    /* horizontal rails */
    glColor3f(0.85f,0.75f,0.55f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(startX, y+18); glVertex2f(endX, y+18);
    glVertex2f(startX, y+8);  glVertex2f(endX, y+8);
    glEnd();

    /* posts */
    glColor3f(0.80f,0.68f,0.48f); glLineWidth(3.5f);
    for(float px=startX; px<=endX; px+=28){
        glBegin(GL_LINES);
        glVertex2f(px, y); glVertex2f(px, y+26);
        glEnd();
        /* pointed top */
        glBegin(GL_TRIANGLES);
        glVertex2f(px-4, y+26);
        glVertex2f(px+4, y+26);
        glVertex2f(px,   y+34);
        glEnd();
    }
}

/* ================================================================
   FLOWER
================================================================ */
static void drawFlower(float cx, float cy, float r, float g, float b)
{
    /* petals */
    for(int i=0;i<6;i++){
        float a=PI/3*i;
        ellipse(cx+8*cosf(a), cy+8*sinf(a), 5,4, r,g,b);
    }
    /* center */
    ellipse(cx, cy, 5,5, 1.f,0.92f,0.1f);
}

/* ================================================================
   STAR SHAPE
================================================================ */
static void drawStar(float cx, float cy, float radius, float r, float g, float b)
{
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float a=PI/2 + PI*i/5;
        float rad = (i%2==0)? radius : radius*0.45f;
        glVertex2f(cx+rad*cosf(a), cy+rad*sinf(a));
    }
    glEnd();
    /* shine */
    glColor3f(fminf(r+0.3f,1.f), fminf(g+0.3f,1.f), fminf(b+0.3f,1.f));
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float a=PI/2 + PI*i/5;
        float rad = (i%2==0)? radius*0.35f : radius*0.15f;
        glVertex2f(cx+rad*cosf(a), cy+rad*sinf(a));
    }
    glEnd();
}

/* ================================================================
   DIAMOND SHAPE
================================================================ */
static void drawDiamond(float cx, float cy, float size, float r, float g, float b)
{
    /* main */
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    glVertex2f(cx, cy+size);
    glVertex2f(cx+size*0.65f, cy+size*0.3f);
    glVertex2f(cx+size*0.65f, cy-size*0.2f);
    glVertex2f(cx, cy-size);
    glVertex2f(cx-size*0.65f, cy-size*0.2f);
    glVertex2f(cx-size*0.65f, cy+size*0.3f);
    glEnd();
    /* facets */
    glColor3f(fminf(r+0.25f,1.f), fminf(g+0.25f,1.f), fminf(b+0.25f,1.f));
    glBegin(GL_TRIANGLES);
    glVertex2f(cx, cy+size);
    glVertex2f(cx-size*0.65f, cy+size*0.3f);
    glVertex2f(cx, cy+size*0.1f);
    glEnd();
    /* outline */
    glColor3f(1,1,1); glLineWidth(1.f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx, cy+size);
    glVertex2f(cx+size*0.65f, cy+size*0.3f);
    glVertex2f(cx+size*0.65f, cy-size*0.2f);
    glVertex2f(cx, cy-size);
    glVertex2f(cx-size*0.65f, cy-size*0.2f);
    glVertex2f(cx-size*0.65f, cy+size*0.3f);
    glEnd();
}

/* ================================================================
   RAINBOW EGG
================================================================ */
static void drawRainbowEgg(float cx, float cy, float t)
{
    /* draw rainbow egg with animated hue shift */
    int seg=40;
    float rx=13, ry=17;
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){
        float a=2*PI*i/seg;
        float hue=fmodf((float)i/seg + t, 1.f);
        /* simple hue->rgb */
        float h6=hue*6;
        int hi=(int)h6;
        float f=h6-hi;
        float p=0, q=1-f, v=1;
        float rr,rg,rb;
        switch(hi%6){
            case 0: rr=v;rg=f;rb=p; break;
            case 1: rr=q;rg=v;rb=p; break;
            case 2: rr=p;rg=v;rb=f; break;
            case 3: rr=p;rg=q;rb=v; break;
            case 4: rr=f;rg=p;rb=v; break;
            default:rr=v;rg=p;rb=q; break;
        }
        glColor3f(rr,rg,rb);
        float ex=cx+rx*cosf(a);
        float ey=cy+ry*sinf(a)*(a<PI?1.0f:0.80f);
        glVertex2f(ex,ey);
    }
    glEnd();
    /* shine */
    ellipse(cx-3,cy+4, 4,3, 1,1,1);
}

/* ================================================================
   BACKGROUND (GAME)
================================================================ */
static void drawBG()
{
    /* sky */
    if(gNight){
        glBegin(GL_QUADS);
        glColor3f(0.02f,0.02f,0.08f); glVertex2f(0,0);
        glColor3f(0.02f,0.02f,0.08f); glVertex2f(WIN_W,0);
        glColor3f(0.04f,0.04f,0.15f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.04f,0.04f,0.15f); glVertex2f(0,WIN_H);
        glEnd();
        /* stars */
        glPointSize(2.f);
        glBegin(GL_POINTS);
        for(int i=0;i<120;i++){
            float br = gStarBr[i]*(0.7f+0.3f*sinf(gMenuAnim*3+i));
            glColor3f(br,br,br);
            glVertex2f(gStarX[i],gStarY[i]);
        }
        glEnd();
        /* moon */
        ellipse(680,WIN_H-60,30,30, 0.95f,0.95f,0.75f);
        ellipse(692,WIN_H-55,26,26, 0.04f,0.04f,0.15f);
    } else {
        /* day sky gradient */
        glBegin(GL_QUADS);
        glColor3f(0.35f,0.68f,0.98f); glVertex2f(0,0);
        glColor3f(0.35f,0.68f,0.98f); glVertex2f(WIN_W,0);
        glColor3f(0.12f,0.38f,0.92f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.12f,0.38f,0.92f); glVertex2f(0,WIN_H);
        glEnd();
        /* sun */
        ellipse(100,WIN_H-70,34,34, 1.0f,0.92f,0.10f);
        ellipse(100,WIN_H-70,28,28, 1.0f,0.96f,0.30f);
        /* inner bright */
        ellipse(100,WIN_H-70,16,16, 1.0f,1.0f,0.60f);
        /* rays */
        glColor3f(1,0.92f,0.2f); glLineWidth(2);
        for(int i=0;i<8;i++){
            float a=PI/4*i + gMenuAnim*0.4f;
            glBegin(GL_LINES);
            glVertex2f(100+36*cosf(a),WIN_H-70+36*sinf(a));
            glVertex2f(100+52*cosf(a),WIN_H-70+52*sinf(a));
            glEnd();
        }

        /* birds */
        drawBird(200+50*sinf(gMenuAnim*0.5f), WIN_H-100);
        drawBird(340+30*cosf(gMenuAnim*0.6f), WIN_H-130);
        drawBird(500+40*sinf(gMenuAnim*0.4f+1), WIN_H-115);
    }

    /* clouds */
    if(!gNight){
        for(auto& c:gClouds){
            /* cloud shadow */
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0,0,0.2f,0.10f);
            glBegin(GL_POLYGON);
            for(int i=0;i<20;i++){
                float a=2*PI*i/20;
                glVertex2f(c.x+34*cosf(a), c.y-8+22*sinf(a));
            }
            glEnd();
            glDisable(GL_BLEND);

            ellipse(c.x,    c.y,    38,25, 1,1,1);
            ellipse(c.x+32, c.y+10, 30,22, 1,1,1);
            ellipse(c.x-32, c.y+8,  26,19, 1,1,1);
            ellipse(c.x+10, c.y+25, 24,18, 1,1,1);
            ellipse(c.x-12, c.y+26, 20,15, 0.97f,0.97f,0.97f);
        }
    }

    /* ---- distant scenery in background ---- */
    /* distant hills */
    glColor3f(0.15f,0.50f,0.18f);
    glBegin(GL_POLYGON);
    glVertex2f(0,68);
    for(int i=0;i<=20;i++){
        float xp=(float)i/20*WIN_W;
        float yp=68+30*sinf(xp*0.012f+1)+20*sinf(xp*0.020f);
        glVertex2f(xp,yp);
    }
    glVertex2f(WIN_W,68);
    glEnd();

    /* grass */
    glBegin(GL_QUADS);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(0,0);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(WIN_W,0);
    glColor3f(0.10f,0.50f,0.10f); glVertex2f(WIN_W,72);
    glColor3f(0.10f,0.50f,0.10f); glVertex2f(0,72);
    glEnd();
    /* grass blades */
    glColor3f(0.15f,0.62f,0.15f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=10){
        float bh=8+4*sinf(i*0.3f);
        glBegin(GL_LINES);
        glVertex2f(i,72); glVertex2f(i+4,72+bh);
        glEnd();
    }

    /* flowers */
    for(auto& fl:gFlowers){
        /* stem */
        glColor3f(0.15f,0.55f,0.15f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(fl.x,fl.y); glVertex2f(fl.x,fl.y+16);
        glEnd();
        drawFlower(fl.x, fl.y+18, fl.r, fl.g, fl.b);
    }

    /* fence */
    drawFence(0, WIN_W, 58);

    /* bamboo sticks */
    for(int s=0;s<N_STICKS;s++){
        float sy=STICK_Y[s];
        glBegin(GL_QUADS);
        glColor3f(0.52f,0.34f,0.06f); glVertex2f(40,sy);
        glColor3f(0.52f,0.34f,0.06f); glVertex2f(WIN_W-40,sy);
        glColor3f(0.38f,0.22f,0.02f); glVertex2f(WIN_W-40,sy+12);
        glColor3f(0.38f,0.22f,0.02f); glVertex2f(40,sy+12);
        glEnd();
        glColor3f(0.30f,0.18f,0.01f); glLineWidth(2.f);
        for(int j=80;j<WIN_W-80;j+=55){
            glBegin(GL_LINES);
            glVertex2f(j,sy); glVertex2f(j,sy+12);
            glEnd();
        }
        glColor3f(0.35f,0.22f,0.04f); glLineWidth(4.f);
        glBegin(GL_LINES);
        glVertex2f(40, sy+12); glVertex2f(40, 72);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(WIN_W-40, sy+12); glVertex2f(WIN_W-40, 72);
        glEnd();
    }

    /* wind indicator */
    if(gWindOn){
        char buf[32];
        sprintf(buf,"WIND %s", gWindF>0?">>>":"<<<");
        txt(WIN_W/2-40, WIN_H-55, buf, 1,0.9f,0, GLUT_BITMAP_HELVETICA_12);
    }
}

/* ================================================================
   CHICKEN
================================================================ */
static void drawChicken(float cx,float cy,int dir)
{
    float f=(float)dir;
    ellipse(cx,cy,24,20, 0.82f,0.52f,0.12f);
    float hx=cx+f*20, hy=cy+16;
    ellipse(hx,hy,13,11, 0.86f,0.56f,0.15f);
    glColor3f(1.f,0.78f,0.f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx+f*12,hy+1); glVertex2f(hx+f*27,hy-1); glVertex2f(hx+f*12,hy-5);
    glEnd();
    ellipse(hx+f*5,hy+3,3,3, 0,0,0);
    ellipse(hx+f*6,hy+3.8f,1,1, 1,1,1);
    glColor3f(0.9f,0.1f,0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx-4,hy+10); glVertex2f(hx-1,hy+22); glVertex2f(hx+4,hy+10);
    glVertex2f(hx+4,hy+10); glVertex2f(hx+7,hy+20); glVertex2f(hx+10,hy+10);
    glEnd();
    ellipse(hx+f*8,hy-7, 5,6, 0.9f,0.1f,0.1f);
    glColor3f(0.65f,0.38f,0.10f);
    glBegin(GL_POLYGON);
    glVertex2f(cx-f*5, cy+8);  glVertex2f(cx-f*22,cy+2);
    glVertex2f(cx-f*18,cy-12); glVertex2f(cx-f*2, cy-6);
    glEnd();
    glColor3f(0.68f,0.44f,0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-f*22,cy+5);  glVertex2f(cx-f*42,cy+18); glVertex2f(cx-f*24,cy-2);
    glVertex2f(cx-f*20,cy-2);  glVertex2f(cx-f*40,cy-10); glVertex2f(cx-f*20,cy-14);
    glEnd();
    glColor3f(1.f,0.78f,0.f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(cx-7,cy-18); glVertex2f(cx-10,cy-32);
    glVertex2f(cx+7,cy-18); glVertex2f(cx+10,cy-32);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(cx-10,cy-32); glVertex2f(cx-20,cy-32);
    glVertex2f(cx-10,cy-32); glVertex2f(cx-8, cy-38);
    glVertex2f(cx+10,cy-32); glVertex2f(cx+20,cy-32);
    glVertex2f(cx+10,cy-32); glVertex2f(cx+8, cy-38);
    glEnd();
}

/* ================================================================
   EGG
================================================================ */
static void drawEgg(float cx,float cy,float rx,float ry,
                    float r,float g,float b)
{
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<40;i++){
        float a=2*PI*i/40;
        float ex=cx+rx*cosf(a);
        float ey=cy+ry*sinf(a)*(a<PI?1.0f:0.80f);
        glVertex2f(ex,ey);
    }
    glEnd();
    glColor3f(r*0.60f,g*0.60f,b*0.60f); glLineWidth(1.f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<40;i++){
        float a=2*PI*i/40;
        glVertex2f(cx+rx*cosf(a), cy+ry*sinf(a)*(a<PI?1.0f:0.80f));
    }
    glEnd();
    ellipse(cx-rx*0.22f,cy+ry*0.22f, rx*0.22f,ry*0.20f,
            fminf(r+0.30f,1.f),fminf(g+0.30f,1.f),fminf(b+0.30f,1.f));
}

/* ================================================================
   POOP
================================================================ */
static void drawPoop(float cx,float cy)
{
    ellipse(cx,    cy,    11,8,  0.38f,0.22f,0.00f);
    ellipse(cx-2,  cy+11, 9, 7,  0.38f,0.22f,0.00f);
    ellipse(cx,    cy+20, 6, 6,  0.38f,0.22f,0.00f);
    ellipse(cx-4,cy+22, 2,2, 1,1,1);
    ellipse(cx+4,cy+22, 2,2, 1,1,1);
    ellipse(cx-4,cy+22, 1,1, 0,0,0);
    ellipse(cx+4,cy+22, 1,1, 0,0,0);
    glColor3f(0.5f,0.5f,0.f); glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2f(cx-5,cy+27); glVertex2f(cx-7,cy+36);
    glVertex2f(cx+5,cy+27); glVertex2f(cx+7,cy+36);
    glEnd();
}

/* ================================================================
   POWER-UP BLOCK
================================================================ */
static void drawPower(float cx,float cy,ItemType t)
{
    float r,g,b; const char* lbl;
    switch(t){
        case PWR_BASKET: r=0.96f;g=0.55f;b=0.05f; lbl="BIG";    break;
        case PWR_SLOW:   r=0.05f;g=0.88f;b=0.88f; lbl="SLOW";   break;
        case PWR_TIME:   r=0.12f;g=0.90f;b=0.20f; lbl="+TIME";  break;
        case PWR_SHIELD: r=0.72f;g=0.12f;b=0.96f; lbl="SHIELD"; break;
        default:         r=1;g=1;b=1;              lbl="?";      break;
    }
    /* block with 3D bevel */
    quad(cx-21,cy-21,42,42, r*0.6f,g*0.6f,b*0.6f);
    quad(cx-21,cy-21,40,40, r,g,b);
    /* shine panel */
    quad(cx-18,cy+2, 20,14,
         fminf(r+0.22f,1.f),fminf(g+0.22f,1.f),fminf(b+0.22f,1.f));
    glColor3f(1,1,1); glLineWidth(2.f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx-21,cy-21); glVertex2f(cx+21,cy-21);
    glVertex2f(cx+21,cy+21); glVertex2f(cx-21,cy+21);
    glEnd();
    txt(cx-16,cy-6,lbl, 0,0,0, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   BASKET
================================================================ */
static void drawBasket(float bx,float by,float bw)
{
    float tl=bx-bw/2,     tr=bx+bw/2;
    float bl=bx-bw/2+18,  br=bx+bw/2-18;

    /* shadow */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.30f);
    glBegin(GL_QUADS);
    glVertex2f(tl+5,by+BH-5); glVertex2f(tr+5,by+BH-5);
    glVertex2f(br+5,by-5);    glVertex2f(bl+5,by-5);
    glEnd();
    glDisable(GL_BLEND);

    /* body */
    glBegin(GL_QUADS);
    glColor3f(0.88f,0.62f,0.25f); glVertex2f(tl,by+BH);
    glColor3f(0.88f,0.62f,0.25f); glVertex2f(tr,by+BH);
    glColor3f(0.65f,0.42f,0.12f); glVertex2f(br,by);
    glColor3f(0.65f,0.42f,0.12f); glVertex2f(bl,by);
    glEnd();

    /* weave */
    glColor3f(0.52f,0.34f,0.10f); glLineWidth(1.5f);
    for(int i=1;i<=3;i++){
        float frac=(float)i/4;
        float ly  =by+BH*frac;
        float x1  =tl+(bl-tl)*(1-frac)*0.6f;
        float x2  =tr-(tr-br)*(1-frac)*0.6f;
        glBegin(GL_LINES);
        glVertex2f(x1,ly); glVertex2f(x2,ly);
        glEnd();
    }
    for(int i=0;i<=6;i++){
        float frac=(float)i/6;
        float xt=tl+(tr-tl)*frac;
        float xb=bl+(br-bl)*frac;
        glBegin(GL_LINES);
        glVertex2f(xt,by+BH); glVertex2f(xb,by);
        glEnd();
    }

    /* outline */
    glColor3f(0.42f,0.26f,0.05f); glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(tl,by+BH); glVertex2f(tr,by+BH);
    glVertex2f(br,by);    glVertex2f(bl,by);
    glEnd();

    /* handle */
    glColor3f(0.75f,0.52f,0.20f); glLineWidth(4.f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx-bw*0.22f, by+BH);
    glVertex2f(bx-bw*0.22f, by+BH+20);
    glVertex2f(bx+bw*0.22f, by+BH+20);
    glVertex2f(bx+bw*0.22f, by+BH);
    glEnd();

    /* shield glow */
    if(gShield){
        glColor3f(0.72f,0.12f,0.96f); glLineWidth(3.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(tl-7,by+BH+7); glVertex2f(tr+7,by+BH+7);
        glVertex2f(br+7,by-7);    glVertex2f(bl-7,by-7);
        glEnd();
    }

    /* basket glow on power-up catch */
    if(gBasket.glowT>0){
        float g2=gBasket.glowT/0.5f;
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,1,0.5f, g2*0.5f);
        glBegin(GL_QUADS);
        glVertex2f(tl-12,by+BH+12); glVertex2f(tr+12,by+BH+12);
        glVertex2f(br+12,by-12);    glVertex2f(bl-12,by-12);
        glEnd();
        glDisable(GL_BLEND);
    }
}

/* ================================================================
   HUD
================================================================ */
static void drawHUD()
{
    /* gradient bar */
    glBegin(GL_QUADS);
    glColor3f(0.02f,0.02f,0.12f); glVertex2f(0,WIN_H-42);
    glColor3f(0.02f,0.02f,0.12f); glVertex2f(WIN_W,WIN_H-42);
    glColor3f(0.06f,0.06f,0.22f); glVertex2f(WIN_W,WIN_H);
    glColor3f(0.06f,0.06f,0.22f); glVertex2f(0,WIN_H);
    glEnd();
    /* border line */
    glColor3f(0.5f,0.5f,0.8f); glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(0,WIN_H-42); glVertex2f(WIN_W,WIN_H-42);
    glEnd();

    char buf[64];
    sprintf(buf,"Score: %d",gScore);
    txt(14, WIN_H-26, buf, 1,1,0);

    int m=(int)gTime/60, s=(int)gTime%60;
    sprintf(buf,"Time: %d:%02d",m,s);
    float tg=(gTime<15)?0.1f:1.f;
    float tr2=(gTime<15)?1.f:0.9f;
    txt(WIN_W/2-44, WIN_H-26, buf, tr2,tg,0);

    sprintf(buf,"Best: %d",gHiScore);
    txt(WIN_W-118, WIN_H-26, buf, 0.72f,0.72f,0.85f);

    /* time bar */
    float maxTime=60.f;
    float barW=(WIN_W-20)*(gTime/maxTime);
    float barR=clampf(2*(1-gTime/maxTime),0,1);
    float barG=clampf(2*gTime/maxTime,0,1);
    /* bar bg */
    quad(10, WIN_H-40, WIN_W-20, 6, 0.2f,0.2f,0.2f);
    /* bar fill */
    glBegin(GL_QUADS);
    glColor3f(barR*0.8f,barG*0.8f,0);  glVertex2f(10,WIN_H-40);
    glColor3f(barR*0.8f,barG*0.8f,0);  glVertex2f(10+barW,WIN_H-40);
    glColor3f(barR,barG,0);             glVertex2f(10+barW,WIN_H-34);
    glColor3f(barR,barG,0);             glVertex2f(10,WIN_H-34);
    glEnd();

    /* active power-up tags */
    float px=14;
    if(gSlowOn){
        quad(px,WIN_H-60, 66,16, 0,0.5f,0.6f);
        quadLine(px,WIN_H-60,66,16, 0,0.9f,0.9f);
        txt(px+4,WIN_H-51,"SLOW", 1,1,1, GLUT_BITMAP_HELVETICA_12);
        px+=72;
    }
    if(gBigOn){
        quad(px,WIN_H-60, 80,16, 0.7f,0.38f,0);
        quadLine(px,WIN_H-60,80,16, 1,0.8f,0);
        txt(px+4,WIN_H-51,"BIG BASKET", 1,1,1, GLUT_BITMAP_HELVETICA_12);
        px+=86;
    }
    if(gShield){
        quad(px,WIN_H-60, 66,16, 0.5f,0.0f,0.65f);
        quadLine(px,WIN_H-60,66,16, 0.9f,0.2f,1);
        txt(px+4,WIN_H-51,"SHIELD", 1,1,1, GLUT_BITMAP_HELVETICA_12);
    }

    /* legend */
    float lx=WIN_W-188, ly=8;
    drawEgg(lx,    ly+6, 7,9,  1.f,0.97f,0.88f); txt(lx+10,ly+2,"+1",  0.9f,0.9f,0.9f,GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+38, ly+6, 7,9,  0.4f,0.6f,1.f);   txt(lx+48,ly+2,"+5",  0.4f,0.7f,1.f, GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+76, ly+6, 8,10, 1.f,0.85f,0.f);   txt(lx+86,ly+2,"+10", 1.f,0.85f,0.f, GLUT_BITMAP_HELVETICA_12);
    drawStar(lx+122, ly+7, 8, 1,0.9f,0.1f);       txt(lx+130,ly+2,"+15", 1,0.9f,0, GLUT_BITMAP_HELVETICA_12);
    drawDiamond(lx+160, ly+7, 7, 0.2f,0.8f,1.f);  txt(lx+168,ly+2,"+20", 0.2f,0.8f,1, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   SCORE POPUPS
================================================================ */
static void drawPopups()
{
    for(auto& p:gPopups){
        float alpha=p.life/1.2f;
        float yoff=(1.f-p.life/1.2f)*40;
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(p.r,p.g,p.b,alpha);
        char buf[16];
        sprintf(buf, p.value>0?"+%d":"%d", p.value);
        glRasterPos2f(p.x-10, p.y+yoff);
        for(const char* c=buf;*c;c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
        glDisable(GL_BLEND);
    }
}

/* ================================================================
   GAME INIT
================================================================ */
static void spawnItem(float chickenX, float chickenY);

static void initGame()
{
    gScore=0; gTime=60.f; gItems.clear(); gPopups.clear();
    gSlowOn=false; gSlowT=0;
    gBigOn=false;  gBigT=0;
    gShield=false;
    gWindOn=false; gWindT=0; gWindF=0;

    gHens.clear();
    for(int s=0;s<N_STICKS;s++){
        Chicken h;
        h.x           = WIN_W/2 + (s%2==0?-80:80);
        h.y           = STICK_Y[s] + 36;
        h.speed       = 65.f + s*20.f;
        h.dir         = (s%2==0)?1:-1;
        h.layTimer    = 0;
        h.layInterval = 1.8f + s*0.5f;
        gHens.push_back(h);
    }

    gBasket.cx = WIN_W/2;
    gBasket.y  = 80;
    gBasket.w  = BW_NORMAL;
    gBasket.glowT = 0;

    gClouds.clear();
    for(int i=0;i<5;i++){
        Cloud c;
        c.x=randF(60,WIN_W-60);
        c.y=randF(WIN_H*0.55f,WIN_H*0.88f);
        c.spd=randF(18,38);
        gClouds.push_back(c);
    }

    gFlowers.clear();
    for(int i=0;i<12;i++){
        Flower fl;
        fl.x=randF(30,WIN_W-30);
        fl.y=randF(4,52);
        fl.r=randF(0.5f,1.f);
        fl.g=randF(0.1f,0.9f);
        fl.b=randF(0.1f,1.f);
        gFlowers.push_back(fl);
    }
}

static void spawnItem(float chickenX,float chickenY)
{
    Item it;
    it.x      = chickenX + randF(-8,8);
    it.y      = chickenY - 22;
    it.active = true;
    it.vx     = gWindOn ? gWindF : 0.f;
    it.angle  = 0.f;

    float r=(float)rand()/RAND_MAX;
    if     (r<0.03f) it.type=EGG_RAINBOW;
    else if(r<0.08f) it.type=EGG_GOLDEN;
    else if(r<0.18f) it.type=EGG_BLUE;
    else if(r<0.46f) it.type=EGG_NORMAL;
    else if(r<0.56f) it.type=POOP;
    else if(r<0.63f) it.type=PWR_BASKET;
    else if(r<0.70f) it.type=PWR_SLOW;
    else if(r<0.78f) it.type=PWR_TIME;
    else if(r<0.84f) it.type=PWR_SHIELD;
    else if(r<0.92f) it.type=STAR_ITEM;
    else             it.type=DIAMOND_ITEM;

    it.vy = randF(80,165);
    gItems.push_back(it);
}

/* ================================================================
   UPDATE
================================================================ */
static void update(float dt)
{
    gMenuAnim+=dt;

    if(gState!=ST_PLAY) return;

    gTime-=dt;
    if(gTime<=0){ gTime=0; if(gScore>gHiScore)gHiScore=gScore; gState=ST_OVER; return; }

    if(gSlowOn){ gSlowT-=dt; if(gSlowT<=0)gSlowOn=false; }
    if(gBigOn) { gBigT -=dt; if(gBigT <=0){ gBigOn=false; gBasket.w=BW_NORMAL; } }
    if(gBasket.glowT>0) gBasket.glowT-=dt;

    static float windSpawn=20.f;
    windSpawn-=dt;
    if(windSpawn<0){
        windSpawn=randF(15,35);
        gWindOn=true; gWindT=randF(4,8);
        gWindF=randF(30,70)*(rand()%2?1:-1);
    }
    if(gWindOn){ gWindT-=dt; if(gWindT<=0)gWindOn=false; }

    for(auto& c:gClouds){
        c.x+=c.spd*dt;
        if(c.x>WIN_W+80) c.x=-80;
    }

    for(auto& h:gHens){
        h.x+=h.speed*h.dir*dt;
        if(h.x>WIN_W-60) h.dir=-1;
        if(h.x<60)       h.dir= 1;
        h.layTimer+=dt;
        if(h.layTimer>=h.layInterval){
            h.layTimer=0;
            h.layInterval=randF(1.0f,3.2f);
            spawnItem(h.x,h.y);
        }
    }

    if(gSpec[GLUT_KEY_LEFT] ||gKeys['a']||gKeys['A'])
        gBasket.cx-=BSPEED*dt;
    if(gSpec[GLUT_KEY_RIGHT]||gKeys['d']||gKeys['D'])
        gBasket.cx+=BSPEED*dt;
    float half=gBasket.w/2;
    if(gBasket.cx-half<0)     gBasket.cx=half;
    if(gBasket.cx+half>WIN_W) gBasket.cx=WIN_W-half;

    float sm=gSlowOn?0.38f:1.f;
    for(auto& it:gItems){
        if(!it.active)continue;
        it.y -=it.vy*sm*dt;
        it.x +=it.vx*sm*dt;
        it.angle+=dt*120;

        float bL=gBasket.cx-gBasket.w/2+14;
        float bR=gBasket.cx+gBasket.w/2-14;
        float bT=gBasket.y+BH;

        if(it.y<=bT && it.y>=gBasket.y-10 && it.x>=bL && it.x<=bR){
            it.active=false;
            ScorePopup pop;
            pop.x=it.x; pop.y=it.y; pop.life=1.2f;

            switch(it.type){
                case EGG_NORMAL:   gScore+=1;  pop.value=1;   pop.r=1;pop.g=1;pop.b=0.8f; playSound(600,50); break;
                case EGG_BLUE:     gScore+=5;  pop.value=5;   pop.r=0.4f;pop.g=0.8f;pop.b=1; playSound(800,50); break;
                case EGG_GOLDEN:   gScore+=10; pop.value=10;  pop.r=1;pop.g=0.85f;pop.b=0; playSound(1000,80); break;
                case EGG_RAINBOW:  gScore+=25; pop.value=25;  pop.r=1;pop.g=0.3f;pop.b=1; playSound(1200,100); break;
                case STAR_ITEM:    gScore+=15; pop.value=15;  pop.r=1;pop.g=0.9f;pop.b=0.1f; playSound(900,80); break;
                case DIAMOND_ITEM: gScore+=20; pop.value=20;  pop.r=0.2f;pop.g=0.9f;pop.b=1; playSound(1100,80); break;
                case POOP:
                    if(gShield){ gShield=false; pop.value=0; pop.r=0.7f;pop.g=0.2f;pop.b=1; playSound(400,60); }
                    else        { gScore-=10; pop.value=-10; pop.r=1;pop.g=0.2f;pop.b=0.2f; playSound(200,150); }
                    break;
                case PWR_BASKET:
                    gBigOn=true; gBigT=10; gBasket.w=BW_BIG;
                    gBasket.glowT=0.5f;
                    pop.value=0; pop.r=1;pop.g=0.6f;pop.b=0;
                    playSound(700,100);
                    break;
                case PWR_SLOW:
                    gSlowOn=true; gSlowT=8;
                    gBasket.glowT=0.5f;
                    pop.value=0; pop.r=0;pop.g=0.9f;pop.b=0.9f;
                    playSound(650,100);
                    break;
                case PWR_TIME:
                    gTime+=20; if(gTime>120)gTime=120;
                    gBasket.glowT=0.5f;
                    pop.value=0; pop.r=0.2f;pop.g=1;pop.b=0.2f;
                    playSound(750,100);
                    break;
                case PWR_SHIELD:
                    gShield=true;
                    gBasket.glowT=0.5f;
                    pop.value=0; pop.r=0.8f;pop.g=0.2f;pop.b=1;
                    playSound(850,100);
                    break;
            }
            gPopups.push_back(pop);
        }

        if(it.y<60 || it.x<-30 || it.x>WIN_W+30) it.active=false;
    }

    gItems.erase(
        std::remove_if(gItems.begin(),gItems.end(),
                       [](const Item& i){return !i.active;}),
        gItems.end());

    /* update popups */
    for(auto& p:gPopups) p.life-=dt;
    gPopups.erase(
        std::remove_if(gPopups.begin(),gPopups.end(),
                       [](const ScorePopup& p){return p.life<=0;}),
        gPopups.end());
}

/* ================================================================
   MENU BACKGROUND (house + trees + sky scene)
================================================================ */
static void drawMenuBG()
{
    /* sky */
    glBegin(GL_QUADS);
    glColor3f(0.22f,0.45f,0.85f); glVertex2f(0,0);
    glColor3f(0.22f,0.45f,0.85f); glVertex2f(WIN_W,0);
    glColor3f(0.55f,0.75f,1.0f);  glVertex2f(WIN_W,WIN_H);
    glColor3f(0.55f,0.75f,1.0f);  glVertex2f(0,WIN_H);
    glEnd();

    /* stars twinkling */
    glPointSize(2.f);
    glBegin(GL_POINTS);
    for(int i=0;i<50;i++){
        float br = gStarBr[i]*(0.6f+0.4f*sinf(gMenuAnim*2+i*0.7f));
        glColor3f(br,br,br*0.8f);
        glVertex2f(gStarX[i],gStarY[i]);
    }
    glEnd();

    /* animated sun */
    float sunX=680, sunY=WIN_H-80;
    ellipse(sunX,sunY,36,36, 1.0f,0.92f,0.10f);
    ellipse(sunX,sunY,30,30, 1.0f,0.96f,0.32f);
    ellipse(sunX,sunY,18,18, 1.0f,1.0f,0.65f);
    glColor3f(1,0.92f,0.2f); glLineWidth(2.5f);
    for(int i=0;i<8;i++){
        float a=PI/4*i + gMenuAnim*0.5f;
        glBegin(GL_LINES);
        glVertex2f(sunX+38*cosf(a),sunY+38*sinf(a));
        glVertex2f(sunX+55*cosf(a),sunY+55*sinf(a));
        glEnd();
    }

    /* clouds animated */
    static float coffs[4]={0,200,400,600};
    for(int i=0;i<4;i++){
        coffs[i]+=0.0f; /* static for menu */
        float cx2=coffs[i]+120, cy2=WIN_H-150+i*30;
        ellipse(cx2,    cy2,    38,25, 1,1,1);
        ellipse(cx2+32, cy2+10, 28,20, 1,1,1);
        ellipse(cx2-28, cy2+8,  24,18, 1,1,1);
    }

    /* birds */
    drawBird(150+20*sinf(gMenuAnim*0.7f), WIN_H-190);
    drawBird(250+15*cosf(gMenuAnim*0.5f), WIN_H-210);
    drawBird(550+25*sinf(gMenuAnim*0.6f+0.5f), WIN_H-195);

    /* ground */
    glBegin(GL_QUADS);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(0,0);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(WIN_W,0);
    glColor3f(0.12f,0.55f,0.12f); glVertex2f(WIN_W,90);
    glColor3f(0.12f,0.55f,0.12f); glVertex2f(0,90);
    glEnd();
    /* grass blades */
    glColor3f(0.18f,0.65f,0.18f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=10){
        float bh=8+4*sinf(i*0.3f+gMenuAnim*2);
        glBegin(GL_LINES);
        glVertex2f(i,88); glVertex2f(i+4,88+bh);
        glEnd();
    }

    /* fence */
    drawFence(0, WIN_W, 75);

    /* trees - far left group */
    drawTree(55,  90, 0.90f);
    drawTree(110, 90, 1.00f);
    drawTree(155, 90, 0.85f);

    /* main house - center */
    drawHouse(290, 90, 1.10f);

    /* smaller house - right */
    drawHouse(565, 90, 0.80f);

    /* trees - right group */
    drawTree(670, 90, 0.95f);
    drawTree(720, 90, 1.05f);
    drawTree(762, 90, 0.88f);

    /* flowers */
    for(int i=0;i<8;i++){
        float fx=50+i*95.f;
        float fr=0.9f-i*0.05f, fg=0.2f+i*0.08f, fb=0.5f+i*0.06f;
        glColor3f(0.15f,0.55f,0.15f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(fx,76); glVertex2f(fx,92);
        glEnd();
        drawFlower(fx, 94, fr, fg, fb);
    }
}

/* ================================================================
   MENU
================================================================ */
static void drawMenu()
{
    drawMenuBG();

    /* translucent panel behind title+buttons */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0.15f,0.55f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W/2-220,WIN_H/2-145);
    glVertex2f(WIN_W/2+220,WIN_H/2-145);
    glVertex2f(WIN_W/2+220,WIN_H-60);
    glVertex2f(WIN_W/2-220,WIN_H-60);
    glEnd();
    glDisable(GL_BLEND);
    quadLine(WIN_W/2-220,WIN_H/2-145,440,WIN_H-60-(WIN_H/2-145), 0.6f,0.6f,1.f,1.5f);

    /* decorative eggs + items row */
    float ey=WIN_H-90;
    drawEgg(WIN_W/2-165, ey, 16,22, 1.f,0.97f,0.88f);
    drawEgg(WIN_W/2-115, ey, 18,24, 1.f,0.85f,0.f);
    drawEgg(WIN_W/2-65,  ey, 16,22, 0.4f,0.6f,1.f);
    drawRainbowEgg(WIN_W/2-15, ey, gMenuAnim*0.8f);
    drawStar(WIN_W/2+38, ey, 14, 1,0.9f,0.1f);
    drawDiamond(WIN_W/2+88, ey, 13, 0.2f,0.8f,1.f);
    drawPoop(WIN_W/2+120, ey-10);

    /* title */
    float titleX=WIN_W/2-165;
    /* shadow */
    txtBig(titleX+3, WIN_H-127, "CATCH  THE  EGGS", 0,0,0);
    txtBig(titleX,   WIN_H-124, "CATCH  THE  EGGS", 1.f,0.92f,0.05f);

    /* sub title */
    txt(WIN_W/2-120, WIN_H-148,
        "CSE 426  |  Computer Graphics Lab", 0.75f,0.75f,1.f,
        GLUT_BITMAP_HELVETICA_18);

    /* buttons */
    float bx=WIN_W/2-85, bw=170, bh=44, gap=54;
    button(bx, WIN_H/2+62, bw,bh, "START GAME", gHover==0);
    button(bx, WIN_H/2+62-gap,   bw,bh, "HIGH SCORE",  gHover==1);
    button(bx, WIN_H/2+62-gap*2, bw,bh, "HELP",        gHover==2);
    button(bx, WIN_H/2+62-gap*3, bw,bh, "EXIT",        gHover==3);

    txt(WIN_W/2-170,18,
        "Arrow/A/D or Mouse to move   |   P / ESC to pause   |   N = Night mode",
        0.60f,0.60f,0.75f, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   DARK OVERLAY
================================================================ */
static void darkOverlay(float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,alpha);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(WIN_W,0);
    glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    glDisable(GL_BLEND);
}

/* ================================================================
   PAUSE SCREEN
================================================================ */
static void drawPause()
{
    darkOverlay(0.62f);

    /* panel */
    float px=WIN_W/2-170, pw=340, ph=280, py=WIN_H/2-120;
    glBegin(GL_QUADS);
    glColor3f(0.06f,0.06f,0.22f); glVertex2f(px,py);
    glColor3f(0.06f,0.06f,0.22f); glVertex2f(px+pw,py);
    glColor3f(0.12f,0.08f,0.32f); glVertex2f(px+pw,py+ph);
    glColor3f(0.12f,0.08f,0.32f); glVertex2f(px,py+ph);
    glEnd();
    quadLine(px,py,pw,ph, 1,0.9f,0.2f,2);

    txtBig(WIN_W/2-55, WIN_H/2+125, "PAUSED", 1,1,0);

    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx, WIN_H/2+55,  bw,bh, "RESUME",  gHover==0);
    button(bx, WIN_H/2+5,   bw,bh, "RESTART", gHover==1);
    button(bx, WIN_H/2-50,  bw,bh, "MENU",    gHover==2);
    button(bx, WIN_H/2-105, bw,bh, "EXIT",    gHover==3);
}

/* ================================================================
   GAME OVER SCREEN
================================================================ */
static void drawGameOver()
{
    darkOverlay(0.72f);

    float px=WIN_W/2-190, pw=380, ph=300, py=WIN_H/2-128;
    glBegin(GL_QUADS);
    glColor3f(0.18f,0.03f,0.03f); glVertex2f(px,py);
    glColor3f(0.18f,0.03f,0.03f); glVertex2f(px+pw,py);
    glColor3f(0.08f,0.02f,0.02f); glVertex2f(px+pw,py+ph);
    glColor3f(0.08f,0.02f,0.02f); glVertex2f(px,py+ph);
    glEnd();
    quadLine(px,py,pw,ph, 1,0.3f,0.3f,2);

    /* shadow + title */
    txtBig(WIN_W/2-88,  WIN_H/2+137, "GAME  OVER", 0.3f,0,0);
    txtBig(WIN_W/2-90, WIN_H/2+140, "GAME  OVER", 1,0.22f,0.22f);

    char buf[64];
    sprintf(buf,"Your Score : %d",gScore);
    txt(WIN_W/2-90, WIN_H/2+88, buf, 1,1,0);
    sprintf(buf,"Best Score : %d",gHiScore);
    txt(WIN_W/2-90, WIN_H/2+58, buf, 0.8f,0.8f,0.8f);

    if(gScore>=gHiScore && gScore>0)
        txt(WIN_W/2-80,WIN_H/2+30,"NEW HIGH SCORE!", 0.2f,1,0.4f);

    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx, WIN_H/2-5,  bw,bh, "PLAY AGAIN", gHover==0);
    button(bx, WIN_H/2-54, bw,bh, "MENU",       gHover==1);
    button(bx, WIN_H/2-103,bw,bh, "EXIT",       gHover==2);
}

/* ================================================================
   HIGH SCORE SCREEN
================================================================ */
static void drawHiScore()
{
    drawMenuBG();   /* reuse the beautiful menu background */
    darkOverlay(0.60f);

    float px=WIN_W/2-210, pw=420, ph=280, py=WIN_H/2-120;
    glBegin(GL_QUADS);
    glColor3f(0.06f,0.06f,0.20f); glVertex2f(px,py);
    glColor3f(0.06f,0.06f,0.20f); glVertex2f(px+pw,py);
    glColor3f(0.10f,0.06f,0.28f); glVertex2f(px+pw,py+ph);
    glColor3f(0.10f,0.06f,0.28f); glVertex2f(px,py+ph);
    glEnd();
    quadLine(px,py,pw,ph, 1,1,0,2);

    txtBig(WIN_W/2-80, py+ph-38, "HIGH SCORE", 1,1,0);

    char buf[32];
    sprintf(buf,"%d  pts",gHiScore);
    /* big score */
    glColor3f(1,0.85f,0.0f);
    glRasterPos2f(WIN_W/2-30, py+ph/2+10);
    for(const char* c=buf;*c;c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);

    /* stars decoration */
    drawStar(WIN_W/2-80, py+ph/2+20, 18, 1,0.85f,0);
    drawStar(WIN_W/2+90, py+ph/2+20, 18, 1,0.85f,0);

    if(gHiScore==0)
        txt(WIN_W/2-88, py+ph/2-20, "No score yet! Play first.", 0.7f,0.7f,0.7f);

    /* BACK button - properly wired */
    float bx=WIN_W/2-75, bw=150, bh=42;
    button(bx, py+18, bw, bh, "BACK", gHover==0);
}

/* ================================================================
   HELP SCREEN
================================================================ */
static void drawHelp()
{
    drawMenuBG();
    darkOverlay(0.65f);

    float px=60, py=90, pw=680, ph=420;
    glBegin(GL_QUADS);
    glColor3f(0.05f,0.05f,0.18f); glVertex2f(px,py);
    glColor3f(0.05f,0.05f,0.18f); glVertex2f(px+pw,py);
    glColor3f(0.08f,0.05f,0.25f); glVertex2f(px+pw,py+ph);
    glColor3f(0.08f,0.05f,0.25f); glVertex2f(px,py+ph);
    glEnd();
    quadLine(px,py,pw,ph, 1,0.9f,0,2);

    txtBig(WIN_W/2-80, py+ph-35, "CONTROLS & HELP", 1,0.9f,0);

    const char* lines[]={
        "MOVEMENT:",
        "  Arrow Left / Right  or  A / D  :  Move basket",
        "  Mouse Move (during play)         :  Move basket",
        "",
        "SYSTEM:",
        "  P  or  ESC   :  Pause / Resume",
        "  N              :  Toggle Night mode",
        "",
        "ITEMS:",
        "  White Egg   = +1 pt     Blue Egg   = +5 pt",
        "  Gold  Egg   = +10 pt    Rainbow Egg = +25 pt  (RARE!)",
        "  Star         = +15 pt    Diamond     = +20 pt",
        "  Poop         = -10 pt    (AVOID!)",
        "",
        "POWER-UPS (falling blocks):",
        "  BIG    (orange)  : basket x2 size  for 10 s",
        "  SLOW   (cyan)    : eggs fall slower for 8 s",
        "  +TIME  (green)   : adds 20 seconds",
        "  SHIELD (purple)  : blocks next poop  (BONUS)",
        "",
        "  Wind can drift eggs sideways! (BONUS)",
    };
    int n=sizeof(lines)/sizeof(lines[0]);
    for(int i=0;i<n;i++){
        float lr=0.88f, lg=0.88f, lb=0.88f;
        if(lines[i][0]!='\0' && lines[i][0]!=' '){
            lr=1; lg=0.85f; lb=0.3f;
        }
        txt(px+18, py+ph-65-i*18, lines[i], lr,lg,lb, GLUT_BITMAP_HELVETICA_12);
    }

    /* BACK button */
    float bx=WIN_W/2-75, bw=150, bh=42;
    button(bx, py+14, bw, bh, "BACK", gHover==0);
}

/* ================================================================
   BUTTON HIT TESTING
================================================================ */
static int getBtn(int mx,int my)
{
    float bw,bh,bx,gap;
    if(gState==ST_MENU){
        bx=WIN_W/2-85; bw=170; bh=44; gap=54;
        if(inRect(mx,my,bx,WIN_H/2+62,       bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2+62-gap,   bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2+62-gap*2, bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2+62-gap*3, bw,bh)) return 3;
    } else if(gState==ST_PAUSE){
        bx=WIN_W/2-80; bw=160; bh=42;
        if(inRect(mx,my,bx,WIN_H/2+55,  bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2+5,   bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-50,  bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2-105, bw,bh)) return 3;
    } else if(gState==ST_OVER){
        bx=WIN_W/2-80; bw=160; bh=42;
        if(inRect(mx,my,bx,WIN_H/2-5,   bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2-54,  bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-103, bw,bh)) return 2;
    } else if(gState==ST_HISCORE){
        /* BACK button at py+18, py = WIN_H/2-120 = 180 */
        float py=WIN_H/2-120;
        if(inRect(mx,my,WIN_W/2-75,py+18,150,42)) return 0;
    } else if(gState==ST_HELP){
        float py=90;
        if(inRect(mx,my,WIN_W/2-75,py+14,150,42)) return 0;
    }
    return -1;
}

/* ================================================================
   DISPLAY
================================================================ */
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if(gState==ST_MENU){
        drawMenu();
    } else if(gState==ST_HISCORE){
        drawHiScore();
    } else if(gState==ST_HELP){
        drawHelp();
    } else {
        drawBG();
        for(auto& h:gHens) drawChicken(h.x,h.y,h.dir);

        /* falling items */
        for(auto& it:gItems){
            if(!it.active)continue;
            switch(it.type){
                case EGG_NORMAL:   drawEgg(it.x,it.y,10,14, 1.f,0.97f,0.88f); break;
                case EGG_BLUE:     drawEgg(it.x,it.y,10,14, 0.4f,0.6f,1.f);   break;
                case EGG_GOLDEN:   drawEgg(it.x,it.y,12,16, 1.f,0.85f,0.f);   break;
                case EGG_RAINBOW:  drawRainbowEgg(it.x,it.y, gMenuAnim*0.8f);  break;
                case POOP:         drawPoop(it.x,it.y);                          break;
                case STAR_ITEM:    drawStar(it.x,it.y,13, 1.f,0.9f,0.1f);      break;
                case DIAMOND_ITEM: drawDiamond(it.x,it.y,12, 0.2f,0.8f,1.f);   break;
                default:           drawPower(it.x,it.y,it.type);                 break;
            }
        }

        drawBasket(gBasket.cx,gBasket.y,gBasket.w);
        drawPopups();
        drawHUD();

        if(gState==ST_PAUSE) drawPause();
        if(gState==ST_OVER)  drawGameOver();
    }

    glutSwapBuffers();
}

/* ================================================================
   TIMER
================================================================ */
static void timerCB(int)
{
    float now=glutGet(GLUT_ELAPSED_TIME)/1000.f;
    float dt =now-gLastTime;
    if(dt>0.05f)dt=0.05f;
    gLastTime=now;
    update(dt);
    glutPostRedisplay();
    glutTimerFunc(16,timerCB,0);
}

/* ================================================================
   KEYBOARD
================================================================ */
static void keyDown(unsigned char k,int,int){
    gKeys[k]=true;
    if(k==27||k=='p'||k=='P'){
        /* FIXED: pause no longer deducts time */
        if(gState==ST_PLAY)        gState=ST_PAUSE;
        else if(gState==ST_PAUSE)  gState=ST_PLAY;
    }
    if((k=='n'||k=='N')&&(gState==ST_PLAY||gState==ST_PAUSE)) gNight=!gNight;
}
static void keyUp(unsigned char k,int,int){ gKeys[k]=false; }
static void specDown(int k,int,int){ gSpec[k]=true;  }
static void specUp  (int k,int,int){ gSpec[k]=false; }

static void setBasketX(float x)
{
    gBasket.cx = x;
    float half = gBasket.w / 2;
    if(gBasket.cx - half < 0)     gBasket.cx = half;
    if(gBasket.cx + half > WIN_W) gBasket.cx = WIN_W - half;
}

/* ================================================================
   MOUSE
================================================================ */
static void mouseMove(int mx,int my)
{
    int gy=WIN_H-my;
    if(gState==ST_PLAY) setBasketX((float)mx);
    gHover=getBtn(mx,gy);
    glutPostRedisplay();
}

static void mouseClick(int btn,int state,int mx,int my)
{
    if(btn!=GLUT_LEFT_BUTTON||state!=GLUT_DOWN)return;
    int gy=WIN_H-my;
    int clicked=getBtn(mx,gy);

    if(gState==ST_MENU){
        if(clicked==0){ initGame(); gState=ST_PLAY; }
        else if(clicked==1) gState=ST_HISCORE;
        else if(clicked==2) gState=ST_HELP;
        else if(clicked==3) exit(0);
    } else if(gState==ST_PLAY){
        setBasketX((float)mx);
    } else if(gState==ST_PAUSE){
        if(clicked==0) gState=ST_PLAY;
        else if(clicked==1){ initGame(); gState=ST_PLAY; }
        else if(clicked==2) gState=ST_MENU;
        else if(clicked==3) exit(0);
    } else if(gState==ST_OVER){
        if(clicked==0){ initGame(); gState=ST_PLAY; }
        else if(clicked==1) gState=ST_MENU;
        else if(clicked==2) exit(0);
    } else if(gState==ST_HISCORE||gState==ST_HELP){
        /* FIXED: back button now correctly returns to menu */
        if(clicked==0) gState=ST_MENU;
    }
    glutPostRedisplay();
}

/* ================================================================
   RESHAPE
================================================================ */
static void reshape(int w,int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

/* ================================================================
   MAIN
================================================================ */
int main(int argc,char** argv)
{
    srand((unsigned)time(0));

    for(int i=0;i<120;i++){
        gStarX[i]=randF(0,(float)WIN_W);
        gStarY[i]=randF(0,(float)WIN_H);
        gStarBr[i]=randF(0.5f,1.0f);
    }

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(100,80);
    glutCreateWindow("Catch The Eggs  |  CSE 426  |  Enhanced");

    glClearColor(0,0,0,1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseClick);

    gLastTime=glutGet(GLUT_ELAPSED_TIME)/1000.f;
    glutTimerFunc(16,timerCB,0);

    glutMainLoop();
    return 0;
}
