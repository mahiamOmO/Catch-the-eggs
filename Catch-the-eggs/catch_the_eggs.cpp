/*
 * ================================================================
 *   CATCH THE EGGS  –  CSE 426 Computer Graphics Lab
 *   Term Project | Spring 2025   *** ENHANCED VERSION ***
 * ================================================================
 *
 *  MARKS (from PDF):
 *  [20] Base game: chicken, 4 item types, keyboard+mouse basket,
 *       timer, score, 3 power-up blocks
 *  [ 5] Menu page: Start / Resume / High Score / Exit, page nav
 *  [ 5] Pause (P/ESC) and Exit any time
 *
 *  BONUS implemented:
 *   (+) 2 sticks with 2 chickens
 *   (+) Wind / airflow that drifts eggs sideways
 *   (+) Shield power-up (next poop negated)
 *   (+) Help / Controls page
 *   (+) Day-Night sky toggle (N key)
 *
 *  NEW ENHANCEMENTS:
 *   [FIX] Pause no longer steals 50 s from timer
 *   [FIX] High-Score BACK button now works correctly
 *   [NEW] Game time = 60 seconds (1 minute)
 *   [NEW] Beautiful menu background: house, trees, fence, birds
 *   [NEW] Sound effects  (Windows: Beep API; Linux: silent stub)
 *   [NEW] Star collectible  +15 pts  (spinning, glowing)
 *   [NEW] Diamond collectible  +20 pts
 *   [NEW] Rare Rainbow egg  +25 pts  (colour-cycling)
 *   [NEW] Score pop-up text when you catch something
 *   [NEW] Basket glow flash on power-up catch
 *   [NEW] Colour timer-bar that turns red when time is low
 *   [NEW] Fence along ground, animated grass blades, flowers
 *   [NEW] Animated sun rays & flying birds in game background
 *
 * ----------------------------------------------------------------
 *  COMPILE
 *    Linux  :  g++ catch_the_eggs_enhanced.cpp -o game -lGL -lGLU -lglut -lm
 *    Windows:  g++ catch_the_eggs_enhanced.cpp -o game \
 *              -lfreeglut -lopengl32 -lglu32 -lm -lwinmm
 *
 *  CONTROLS
 *    Left/Right Arrow or A/D  – move basket
 *    Mouse move               – move basket (during play)
 *    P  or  ESC               – pause / resume  (no time penalty)
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
   SOUND  (Windows uses Beep(); Linux stub is silent.
           For real audio add SDL_mixer or OpenAL.)
---------------------------------------------------------------- */
static void sndBeep(int freq, int ms)
{
#ifdef _WIN32
    Beep(freq, ms);
#else
    (void)freq; (void)ms;
#endif
}

/* ----------------------------------------------------------------
   WINDOW & MATH
---------------------------------------------------------------- */
static const int   WIN_W = 800;
static const int   WIN_H = 600;
static const float PI    = 3.14159265f;

/* ----------------------------------------------------------------
   GAME STATE
---------------------------------------------------------------- */
enum GameState { ST_MENU, ST_PLAY, ST_PAUSE, ST_OVER, ST_HISCORE, ST_HELP };
static GameState gState = ST_MENU;

/* ----------------------------------------------------------------
   ITEM TYPES
---------------------------------------------------------------- */
enum ItemType {
    EGG_NORMAL,   /* white   +1  pt */
    EGG_BLUE,     /* blue    +5  pt */
    EGG_GOLDEN,   /* gold   +10  pt */
    EGG_RAINBOW,  /* rainbow +25  pt  (rare) */
    POOP,         /* brown  -10  pt */
    PWR_BASKET,   /* orange  bigger basket 10 s */
    PWR_SLOW,     /* cyan    slow eggs     8 s  */
    PWR_TIME,     /* green   +20 seconds        */
    PWR_SHIELD,   /* purple  negate 1 poop       */
    BONUS_STAR,   /* yellow  +15 pt  (spinning star) */
    BONUS_DIAMOND /* cyan    +20 pt  (gem)            */
};

/* ----------------------------------------------------------------
   STRUCTS
---------------------------------------------------------------- */
struct Item {
    float x, y;
    float vy;          /* fall speed px/s            */
    float vx;          /* horizontal drift (wind)     */
    float angle;       /* spin angle for star/diamond */
    ItemType type;
    bool  active;
};

/* floating score text that drifts upward */
struct ScorePopup {
    float x, y, life;
    int   pts;
    float r, g, b;
};

struct Chicken {
    float x, y;
    float speed;
    int   dir;         /* +1=right  -1=left           */
    float layTimer;
    float layInterval;
};

struct Basket {
    float cx;          /* centre x                    */
    float y;           /* bottom y                    */
    float w;           /* current width               */
};

struct Cloud {
    float x, y, spd;
};

/* ----------------------------------------------------------------
   GLOBALS
---------------------------------------------------------------- */
/* 2 sticks / 2 chickens (BONUS: multiple sticks) */
static const int N_STICKS = 2;
static const float STICK_Y[N_STICKS] = { WIN_H - 90.f, WIN_H - 230.f };

static std::vector<Chicken>    gHens;
static Basket                  gBasket;
static std::vector<Item>       gItems;
static std::vector<Cloud>      gClouds;
static std::vector<ScorePopup> gPopups;  /* NEW: floating score text */

static int   gScore     = 0;
static int   gHiScore   = 0;
static float gTime      = 60.f;   /* 1 minute  [CHANGED from 120] */

/* power-up timers */
static bool  gSlowOn    = false;   static float gSlowT  = 0.f;
static bool  gBigOn     = false;   static float gBigT   = 0.f;
static bool  gShield    = false;
static float gBasketGlow= 0.f;    /* NEW: glow flash after power-up */

/* wind (BONUS) */
static bool  gWindOn    = false;
static float gWindT     = 0.f;
static float gWindF     = 0.f;    /* drift px/s */

/* night mode (BONUS) */
static bool  gNight     = false;

/* global animation clock (used for sun rays, grass, menu anim) */
static float gAnim      = 0.f;   /* NEW */

static const float BW_NORMAL = 110.f;
static const float BW_BIG    = 210.f;
static const float BH        = 44.f;
static const float BSPEED    = 380.f;

static bool  gKeys[256]   = {};
static bool  gSpec[256]   = {};
static int   gHover       = -1;
static float gLastTime    = 0.f;

/* fixed stars for menu/night */
static float gStarX[100], gStarY[100], gStarBr[100];

/* ================================================================
   UTILITY
================================================================ */
static float randF(float lo, float hi){
    return lo + (float)rand()/RAND_MAX*(hi-lo);
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

/* ---- NEW helpers -------------------------------------------- */

/* 5-point star centred at (cx,cy), outer radius R */
static void drawStar5(float cx,float cy,float R,
                      float r,float g,float b)
{
    float ri=R*0.42f;
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float a = -PI/2 + i*PI/5;
        float rad=(i%2==0)?R:ri;
        glVertex2f(cx+rad*cosf(a), cy+rad*sinf(a));
    }
    glEnd();
    /* inner highlight */
    glColor3f(fminf(r+0.35f,1.f),fminf(g+0.35f,1.f),fminf(b+0.35f,1.f));
    glBegin(GL_POLYGON);
    for(int i=0;i<10;i++){
        float a = -PI/2 + i*PI/5;
        float rad=(i%2==0)?R*0.30f:ri*0.30f;
        glVertex2f(cx+rad*cosf(a), cy+rad*sinf(a));
    }
    glEnd();
}

/* diamond gem */
static void drawDiamond(float cx,float cy,float sz,
                        float r,float g,float b)
{
    /* top half */
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    glVertex2f(cx,      cy+sz);
    glVertex2f(cx-sz*0.7f, cy+sz*0.2f);
    glVertex2f(cx,      cy-sz*0.5f);
    glVertex2f(cx+sz*0.7f, cy+sz*0.2f);
    glEnd();
    /* facet shading */
    glColor3f(fminf(r+0.25f,1.f),fminf(g+0.25f,1.f),fminf(b+0.25f,1.f));
    glBegin(GL_TRIANGLES);
    glVertex2f(cx, cy+sz);
    glVertex2f(cx-sz*0.7f, cy+sz*0.2f);
    glVertex2f(cx, cy+sz*0.25f);
    glEnd();
    /* outline */
    glColor3f(1,1,1); glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx,      cy+sz);
    glVertex2f(cx-sz*0.7f, cy+sz*0.2f);
    glVertex2f(cx,      cy-sz*0.5f);
    glVertex2f(cx+sz*0.7f, cy+sz*0.2f);
    glEnd();
}

/* rainbow egg – hue shift driven by gAnim */
static void drawRainbowEgg(float cx,float cy,float rx,float ry)
{
    int seg=48;
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){
        float t=(float)i/seg + gAnim*0.5f;
        /* simple hue wheel */
        float h=fmodf(t,1.f)*6.f;
        int   hi=(int)h; float f=h-hi;
        float ev=1.f, ep=0.f, eq=1.f-f, et=f;
        float er,eg2,eb;
        switch(hi){
            case 0: er=ev;eg2=et;eb=ep; break;
            case 1: er=eq;eg2=ev;eb=ep; break;
            case 2: er=ep;eg2=ev;eb=et; break;
            case 3: er=ep;eg2=eq;eb=ev; break;
            case 4: er=et;eg2=ep;eb=ev; break;
            default:er=ev;eg2=ep;eb=eq; break;
        }
        glColor3f(er,eg2,eb);
        float a=2*PI*i/seg;
        glVertex2f(cx+rx*cosf(a), cy+ry*sinf(a)*(a<PI?1.0f:0.80f));
    }
    glEnd();
    /* shine */
    ellipse(cx-rx*0.22f,cy+ry*0.22f, rx*0.20f,ry*0.18f, 1,1,1);
}

/* ---- Scenic elements for menu & game backgrounds ------------ */

/* single tree at (bx,groundY) */
static void drawTree(float bx,float groundY)
{
    /* trunk */
    glBegin(GL_QUADS);
    glColor3f(0.45f,0.28f,0.08f);
    glVertex2f(bx-8,groundY);    glVertex2f(bx+8,groundY);
    glVertex2f(bx+6,groundY+55); glVertex2f(bx-6,groundY+55);
    glEnd();
    /* three foliage tiers */
    float cols[3][3]={{0.18f,0.62f,0.18f},{0.14f,0.52f,0.14f},{0.22f,0.70f,0.22f}};
    for(int t=0;t<3;t++){
        float ty=groundY+38+t*26;
        float tw=52-t*10;
        glColor3f(cols[t][0],cols[t][1],cols[t][2]);
        glBegin(GL_TRIANGLES);
        glVertex2f(bx-tw,ty); glVertex2f(bx+tw,ty); glVertex2f(bx,ty+42-t*8);
        glEnd();
        /* highlight */
        glColor3f(cols[t][0]+0.08f,cols[t][1]+0.10f,cols[t][2]+0.08f);
        glBegin(GL_TRIANGLES);
        glVertex2f(bx-tw*0.4f,ty+6); glVertex2f(bx,ty+42-t*8); glVertex2f(bx-tw*0.1f,ty);
        glEnd();
    }
}

/* house: left-bottom corner at (bx,by) */
static void drawHouse(float bx,float by,float sc=1.f)
{
    float W=100*sc, H=72*sc, RH=52*sc;
    /* wall */
    glBegin(GL_QUADS);
    glColor3f(0.85f,0.62f,0.38f); glVertex2f(bx,by);
    glColor3f(0.85f,0.62f,0.38f); glVertex2f(bx+W,by);
    glColor3f(0.72f,0.48f,0.28f); glVertex2f(bx+W,by+H);
    glColor3f(0.72f,0.48f,0.28f); glVertex2f(bx,by+H);
    glEnd();
    /* roof */
    glColor3f(0.70f,0.15f,0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(bx-4,by+H); glVertex2f(bx+W+4,by+H); glVertex2f(bx+W/2,by+H+RH);
    glEnd();
    /* roof shadow side */
    glColor3f(0.55f,0.10f,0.07f);
    glBegin(GL_TRIANGLES);
    glVertex2f(bx+W/2,by+H+RH); glVertex2f(bx+W+4,by+H); glVertex2f(bx+W*0.55f,by+H);
    glEnd();
    /* chimney */
    float cx2=bx+W*0.72f;
    glColor3f(0.55f,0.30f,0.20f);
    glBegin(GL_QUADS);
    glVertex2f(cx2,by+H+RH*0.50f); glVertex2f(cx2+12*sc,by+H+RH*0.50f);
    glVertex2f(cx2+12*sc,by+H+RH+8*sc); glVertex2f(cx2,by+H+RH+8*sc);
    glEnd();
    /* chimney cap */
    glColor3f(0.38f,0.20f,0.12f);
    glBegin(GL_QUADS);
    glVertex2f(cx2-3,by+H+RH+8*sc); glVertex2f(cx2+15*sc,by+H+RH+8*sc);
    glVertex2f(cx2+15*sc,by+H+RH+12*sc); glVertex2f(cx2-3,by+H+RH+12*sc);
    glEnd();
    /* door */
    float dx=bx+W*0.38f, dw=24*sc, dh=34*sc;
    glColor3f(0.40f,0.22f,0.08f);
    glBegin(GL_QUADS);
    glVertex2f(dx,by); glVertex2f(dx+dw,by);
    glVertex2f(dx+dw,by+dh); glVertex2f(dx,by+dh);
    glEnd();
    /* door arch */
    glColor3f(0.35f,0.18f,0.06f);
    glBegin(GL_POLYGON);
    for(int i=0;i<=12;i++){
        float a=PI*i/12;
        glVertex2f(dx+dw/2+dw/2*cosf(a), by+dh+dh*0.18f*sinf(a));
    }
    glEnd();
    /* door knob */
    ellipse(dx+dw*0.75f, by+dh*0.5f, 2.5f*sc,2.5f*sc, 0.9f,0.75f,0.1f);
    /* windows */
    float wsize=18*sc;
    float wxA=bx+W*0.12f, wxB=bx+W*0.65f, wy=by+H*0.52f;
    for(int wi=0;wi<2;wi++){
        float wx=(wi==0)?wxA:wxB;
        glColor3f(0.68f,0.48f,0.22f);
        glBegin(GL_QUADS);
        glVertex2f(wx-2,wy-2); glVertex2f(wx+wsize+2,wy-2);
        glVertex2f(wx+wsize+2,wy+wsize+2); glVertex2f(wx-2,wy+wsize+2);
        glEnd();
        glColor3f(0.62f,0.82f,1.f);
        glBegin(GL_QUADS);
        glVertex2f(wx,wy); glVertex2f(wx+wsize,wy);
        glVertex2f(wx+wsize,wy+wsize); glVertex2f(wx,wy+wsize);
        glEnd();
        /* cross */
        glColor3f(0.68f,0.48f,0.22f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(wx+wsize/2,wy); glVertex2f(wx+wsize/2,wy+wsize);
        glVertex2f(wx,wy+wsize/2); glVertex2f(wx+wsize,wy+wsize/2);
        glEnd();
    }
}

/* wooden fence strip from x0 to x1 at height y */
static void drawFence(float x0,float x1,float y)
{
    /* rails */
    glColor3f(0.82f,0.70f,0.50f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(x0,y+16); glVertex2f(x1,y+16);
    glVertex2f(x0,y+7);  glVertex2f(x1,y+7);
    glEnd();
    /* posts */
    glColor3f(0.76f,0.62f,0.42f); glLineWidth(3.5f);
    for(float px=x0; px<=x1; px+=26){
        glBegin(GL_LINES);
        glVertex2f(px,y); glVertex2f(px,y+24);
        glEnd();
        /* pointed top */
        glColor3f(0.70f,0.55f,0.35f);
        glBegin(GL_TRIANGLES);
        glVertex2f(px-4,y+24); glVertex2f(px+4,y+24); glVertex2f(px,y+32);
        glEnd();
        glColor3f(0.76f,0.62f,0.42f); glLineWidth(3.5f);
    }
}

/* simple flower at (cx,cy) */
static void drawFlower(float cx,float cy,float pr,float pg,float pb)
{
    /* petals */
    for(int i=0;i<6;i++){
        float a=PI/3*i;
        ellipse(cx+8*cosf(a), cy+8*sinf(a), 5,4, pr,pg,pb);
    }
    /* centre */
    ellipse(cx,cy,5,5, 1.f,0.92f,0.10f);
}

/* bird (two bezier-like arcs) */
static void drawBird(float cx,float cy)
{
    glColor3f(0.1f,0.08f,0.08f); glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(cx-10,cy+4); glVertex2f(cx-5,cy); glVertex2f(cx,cy+2);
    glVertex2f(cx+5,cy);    glVertex2f(cx+10,cy+4);
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
    /* drop shadow */
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(x+5,y-5); glVertex2f(x+w+5,y-5);
    glVertex2f(x+w+5,y+h-5); glVertex2f(x+5,y+h-5);
    glEnd();
    /* gradient fill */
    glBegin(GL_QUADS);
    if(hover){
        glColor3f(1.0f,0.82f,0.08f); glVertex2f(x,y);
        glColor3f(1.0f,0.82f,0.08f); glVertex2f(x+w,y);
        glColor3f(0.88f,0.60f,0.02f); glVertex2f(x+w,y+h);
        glColor3f(0.88f,0.60f,0.02f); glVertex2f(x,y+h);
    } else {
        glColor3f(0.18f,0.18f,0.50f); glVertex2f(x,y);
        glColor3f(0.18f,0.18f,0.50f); glVertex2f(x+w,y);
        glColor3f(0.10f,0.10f,0.35f); glVertex2f(x+w,y+h);
        glColor3f(0.10f,0.10f,0.35f); glVertex2f(x,y+h);
    }
    glEnd();
    /* border */
    float br=hover?1.f:0.55f, bg2=hover?0.9f:0.55f, bb=hover?0.1f:0.9f;
    glColor3f(br,bg2,bb); glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
    /* label */
    float tw=strlen(lbl)*6.2f;
    float lr=hover?0.f:1.f, lg2=hover?0.f:1.f, lb=hover?0.f:1.f;
    txt(x+w/2-tw/2, y+h/2-7, lbl, lr,lg2,lb);
}

/* ================================================================
   BACKGROUND  (game world)
================================================================ */
static void drawBG()
{
    /* ---- sky ---- */
    if(gNight){
        glBegin(GL_QUADS);
        glColor3f(0.02f,0.02f,0.08f); glVertex2f(0,0);
        glColor3f(0.02f,0.02f,0.08f); glVertex2f(WIN_W,0);
        glColor3f(0.04f,0.04f,0.15f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.04f,0.04f,0.15f); glVertex2f(0,WIN_H);
        glEnd();
        /* twinkling stars */
        glPointSize(2.f);
        glBegin(GL_POINTS);
        for(int i=0;i<100;i++){
            float br=gStarBr[i]*(0.7f+0.3f*sinf(gAnim*2.f+i));
            glColor3f(br,br,br);
            glVertex2f(gStarX[i],gStarY[i]);
        }
        glEnd();
        /* moon */
        ellipse(680,WIN_H-60,30,30, 0.95f,0.95f,0.75f);
        ellipse(694,WIN_H-55,26,26, 0.04f,0.04f,0.15f);
    } else {
        /* day sky gradient */
        glBegin(GL_QUADS);
        glColor3f(0.42f,0.72f,0.98f); glVertex2f(0,0);
        glColor3f(0.42f,0.72f,0.98f); glVertex2f(WIN_W,0);
        glColor3f(0.16f,0.42f,0.92f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.16f,0.42f,0.92f); glVertex2f(0,WIN_H);
        glEnd();
        /* animated sun */
        ellipse(100,WIN_H-70,34,34, 1.0f,0.92f,0.10f);
        ellipse(100,WIN_H-70,28,28, 1.0f,0.96f,0.30f);
        glColor3f(1,0.92f,0.2f); glLineWidth(2.2f);
        for(int i=0;i<8;i++){
            float a=PI/4*i + gAnim*0.6f;
            glBegin(GL_LINES);
            glVertex2f(100+35*cosf(a),WIN_H-70+35*sinf(a));
            glVertex2f(100+50*cosf(a),WIN_H-70+50*sinf(a));
            glEnd();
        }
        /* flying birds */
        float boff=fmodf(gAnim*28.f, (float)(WIN_W+60))-30;
        drawBird(boff,    WIN_H-125);
        drawBird(boff+42, WIN_H-138);
        drawBird(boff+220,WIN_H-115);
    }

    /* clouds */
    if(!gNight){
        for(auto& c:gClouds){
            ellipse(c.x,    c.y,    36,24, 1,1,1);
            ellipse(c.x+32, c.y+10, 28,20, 1,1,1);
            ellipse(c.x-30, c.y+7,  26,19, 1,1,1);
            ellipse(c.x+9,  c.y+24, 23,17, 1,1,1);
        }
    }

    /* distant hill silhouette */
    glColor3f(0.22f,0.55f,0.22f);
    glBegin(GL_POLYGON);
    glVertex2f(0,68);
    for(int i=0;i<=20;i++){
        float px=(float)WIN_W*i/20.f;
        float py=68+28*sinf(px*0.013f)+18*sinf(px*0.021f+1.2f);
        glVertex2f(px,py);
    }
    glVertex2f(WIN_W,68);
    glEnd();

    /* grass strip */
    glBegin(GL_QUADS);
    glColor3f(0.20f,0.70f,0.20f); glVertex2f(0,0);
    glColor3f(0.20f,0.70f,0.20f); glVertex2f(WIN_W,0);
    glColor3f(0.10f,0.50f,0.10f); glVertex2f(WIN_W,70);
    glColor3f(0.10f,0.50f,0.10f); glVertex2f(0,70);
    glEnd();
    /* animated grass blades */
    glColor3f(0.14f,0.60f,0.14f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=10){
        float sway=4.f*sinf(gAnim*1.8f+i*0.25f);
        glBegin(GL_LINES);
        glVertex2f((float)i,70); glVertex2f(i+sway+4,84);
        glEnd();
    }

    /* flowers along ground */
    static const float fxs[]={30,85,150,230,340,460,570,650,720,780};
    static const float frs[]={1.f,0.9f,1.f,0.4f,1.f,0.8f,0.6f,1.f,0.9f,0.4f};
    static const float fgs[]={0.2f,0.8f,0.5f,0.6f,0.3f,0.2f,0.2f,0.8f,0.2f,0.9f};
    static const float fbs[]={0.5f,0.2f,0.2f,1.f,0.8f,0.9f,1.f,0.3f,0.7f,0.4f};
    for(int i=0;i<10;i++){
        glColor3f(0.12f,0.52f,0.12f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(fxs[i],70); glVertex2f(fxs[i],82);
        glEnd();
        drawFlower(fxs[i],86, frs[i],fgs[i],fbs[i]);
    }

    /* wooden fence */
    drawFence(0,(float)WIN_W, 60);

    /* house on the right side (background scenery) */
    drawHouse(560, 70, 0.85f);

    /* trees flanking house */
    drawTree(520, 70);
    drawTree(700, 70);
    drawTree(750, 70);

    /* bamboo sticks */
    for(int s=0;s<N_STICKS;s++){
        float sy=STICK_Y[s];
        glBegin(GL_QUADS);
        glColor3f(0.50f,0.32f,0.06f); glVertex2f(40,sy);
        glColor3f(0.50f,0.32f,0.06f); glVertex2f(WIN_W-40,sy);
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
        glVertex2f(40,sy+12); glVertex2f(40,70);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(WIN_W-40,sy+12); glVertex2f(WIN_W-40,70);
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

    /* body */
    ellipse(cx,cy,24,20, 0.78f,0.48f,0.10f);

    /* head */
    float hx=cx+f*20, hy=cy+16;
    ellipse(hx,hy,13,11, 0.82f,0.52f,0.12f);

    /* beak */
    glColor3f(1.f,0.78f,0.f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx+f*12,hy+1);
    glVertex2f(hx+f*27,hy-1);
    glVertex2f(hx+f*12,hy-5);
    glEnd();

    /* eye */
    ellipse(hx+f*5,hy+3,  3,3, 0,0,0);
    ellipse(hx+f*6,hy+3.8f,1,1, 1,1,1);

    /* comb */
    glColor3f(0.9f,0.1f,0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx-4,hy+10); glVertex2f(hx-1,hy+22); glVertex2f(hx+4,hy+10);
    glVertex2f(hx+4,hy+10); glVertex2f(hx+7,hy+20); glVertex2f(hx+10,hy+10);
    glEnd();

    /* wattle */
    ellipse(hx+f*8,hy-7, 5,6, 0.9f,0.1f,0.1f);

    /* wing */
    glColor3f(0.62f,0.36f,0.08f);
    glBegin(GL_POLYGON);
    glVertex2f(cx-f*5, cy+8);
    glVertex2f(cx-f*22,cy+2);
    glVertex2f(cx-f*18,cy-12);
    glVertex2f(cx-f*2, cy-6);
    glEnd();

    /* tail feathers */
    glColor3f(0.66f,0.42f,0.09f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-f*22,cy+5);  glVertex2f(cx-f*40,cy+18); glVertex2f(cx-f*24,cy-2);
    glVertex2f(cx-f*20,cy-2);  glVertex2f(cx-f*38,cy-10); glVertex2f(cx-f*20,cy-14);
    glEnd();

    /* legs */
    glColor3f(1.f,0.78f,0.f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(cx-7,cy-18); glVertex2f(cx-10,cy-32);
    glVertex2f(cx+7,cy-18); glVertex2f(cx+10,cy-32);
    glEnd();
    /* feet */
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
    /* outline */
    glColor3f(r*0.60f,g*0.60f,b*0.60f); glLineWidth(1.f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<40;i++){
        float a=2*PI*i/40;
        glVertex2f(cx+rx*cosf(a), cy+ry*sinf(a)*(a<PI?1.0f:0.80f));
    }
    glEnd();
    /* shine */
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
    /* eyes on poop (cute/funny) */
    ellipse(cx-4,cy+22, 2,2, 1,1,1);
    ellipse(cx+4,cy+22, 2,2, 1,1,1);
    ellipse(cx-4,cy+22, 1,1, 0,0,0);
    ellipse(cx+4,cy+22, 1,1, 0,0,0);
    /* stink lines */
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
    /* block */
    quad(cx-21,cy-21,42,42, r,g,b);
    /* shine top-left panel */
    quad(cx-18,cy+2, 20,14,
         fminf(r+0.22f,1.f),fminf(g+0.22f,1.f),fminf(b+0.22f,1.f));
    /* border */
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
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(tl+4,by+BH-4); glVertex2f(tr+4,by+BH-4);
    glVertex2f(br+4,by-4);    glVertex2f(bl+4,by-4);
    glEnd();

    /* body fill */
    glBegin(GL_QUADS);
    glColor3f(0.82f,0.58f,0.22f); glVertex2f(tl,by+BH);
    glColor3f(0.82f,0.58f,0.22f); glVertex2f(tr,by+BH);
    glColor3f(0.65f,0.42f,0.12f); glVertex2f(br,by);
    glColor3f(0.65f,0.42f,0.12f); glVertex2f(bl,by);
    glEnd();

    /* weave horizontals */
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
    /* weave verticals */
    for(int i=0;i<=5;i++){
        float frac=(float)i/5;
        float xt=tl+(tr-tl)*frac;
        float xb=bl+(br-bl)*frac;
        glBegin(GL_LINES);
        glVertex2f(xt,by+BH); glVertex2f(xb,by);
        glEnd();
    }

    /* outline */
    glColor3f(0.42f,0.26f,0.05f); glLineWidth(2.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(tl,by+BH); glVertex2f(tr,by+BH);
    glVertex2f(br,by);    glVertex2f(bl,by);
    glEnd();

    /* handle */
    glColor3f(0.72f,0.50f,0.18f); glLineWidth(3.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx-bw*0.22f, by+BH);
    glVertex2f(bx-bw*0.22f, by+BH+20);
    glVertex2f(bx+bw*0.22f, by+BH+20);
    glVertex2f(bx+bw*0.22f, by+BH);
    glEnd();

    /* shield glow (BONUS) */
    if(gShield){
        glColor3f(0.72f,0.12f,0.96f); glLineWidth(3.f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(tl-6,by+BH+6); glVertex2f(tr+6,by+BH+6);
        glVertex2f(br+6,by-6);    glVertex2f(bl-6,by-6);
        glEnd();
    }
}

/* ================================================================
   HUD
================================================================ */
static void drawHUD()
{
    /* gradient top bar */
    glBegin(GL_QUADS);
    glColor3f(0.04f,0.04f,0.16f); glVertex2f(0,WIN_H-42);
    glColor3f(0.04f,0.04f,0.16f); glVertex2f(WIN_W,WIN_H-42);
    glColor3f(0.08f,0.08f,0.28f); glVertex2f(WIN_W,WIN_H);
    glColor3f(0.08f,0.08f,0.28f); glVertex2f(0,WIN_H);
    glEnd();
    /* separator line */
    glColor3f(0.4f,0.4f,0.7f); glLineWidth(1.f);
    glBegin(GL_LINES);
    glVertex2f(0,WIN_H-42); glVertex2f(WIN_W,WIN_H-42);
    glEnd();

    char buf[64];

    sprintf(buf,"Score: %d",gScore);
    txt(12, WIN_H-26, buf, 1,1,0);

    int m=(int)gTime/60, s=(int)gTime%60;
    sprintf(buf,"Time: %d:%02d",m,s);
    float tg=(gTime<15)?0.1f:1.f;
    float tr2=(gTime<15)?1.f:0.9f;
    txt(WIN_W/2-42, WIN_H-26, buf, tr2,tg,0);

    sprintf(buf,"Best: %d",gHiScore);
    txt(WIN_W-115, WIN_H-26, buf, 0.75f,0.75f,0.85f);

    /* colour time bar */
    float frac=gTime/60.f; if(frac>1)frac=1; if(frac<0)frac=0;
    float barW=(WIN_W-20)*frac;
    float barR=1.f-frac; float barG=frac;
    quad(10, WIN_H-40, WIN_W-20, 6, 0.15f,0.15f,0.15f);   /* bg */
    glBegin(GL_QUADS);
    glColor3f(barR,barG,0); glVertex2f(10,WIN_H-40);
    glColor3f(barR,barG,0); glVertex2f(10+barW,WIN_H-40);
    glColor3f(fminf(barR+0.3f,1.f),fminf(barG+0.3f,1.f),0); glVertex2f(10+barW,WIN_H-34);
    glColor3f(fminf(barR+0.3f,1.f),fminf(barG+0.3f,1.f),0); glVertex2f(10,WIN_H-34);
    glEnd();

    /* active power-up tags */
    float px=12;
    if(gSlowOn){
        quad(px,WIN_H-60, 60,16, 0,0.6f,0.6f);
        txt(px+4,WIN_H-51,"SLOW", 0,0,0, GLUT_BITMAP_HELVETICA_12); px+=66;
    }
    if(gBigOn){
        quad(px,WIN_H-60, 76,16, 0.8f,0.4f,0);
        txt(px+4,WIN_H-51,"BIG BASKET", 0,0,0, GLUT_BITMAP_HELVETICA_12); px+=82;
    }
    if(gShield){
        quad(px,WIN_H-60, 62,16, 0.55f,0.0f,0.75f);
        txt(px+4,WIN_H-51,"SHIELD", 1,1,1, GLUT_BITMAP_HELVETICA_12);
    }

    /* basket glow flash */
    if(gBasketGlow>0){
        float g=gBasketGlow/0.5f;
        float bx2=gBasket.cx, by2=gBasket.y, bw2=gBasket.w;
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,1,0.3f,g*0.55f);
        glBegin(GL_QUADS);
        glVertex2f(bx2-bw2/2-12,by2-10); glVertex2f(bx2+bw2/2+12,by2-10);
        glVertex2f(bx2+bw2/2+12,by2+BH+14); glVertex2f(bx2-bw2/2-12,by2+BH+14);
        glEnd();
        glDisable(GL_BLEND);
    }

    /* legend (bottom right) – now includes star & diamond */
    float lx=WIN_W-230, ly=8;
    drawEgg(lx,   ly+6, 7,9, 1.f,0.97f,0.88f); txt(lx+10,ly+2,"+1",  0.9f,0.9f,0.9f,GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+38,ly+6, 7,9, 0.4f,0.6f,1.f);   txt(lx+48,ly+2,"+5",  0.4f,0.7f,1.f, GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+76,ly+6, 8,10,1.f,0.85f,0.f);   txt(lx+86,ly+2,"+10", 1.f,0.85f,0.f, GLUT_BITMAP_HELVETICA_12);
    drawRainbowEgg(lx+120,ly+8,7,9);            txt(lx+130,ly+2,"+25",1.f,0.4f,1.f,  GLUT_BITMAP_HELVETICA_12);
    drawStar5(lx+170,ly+8, 8, 1,0.9f,0.05f);   txt(lx+180,ly+2,"+15",1.f,0.85f,0.f, GLUT_BITMAP_HELVETICA_12);
    drawDiamond(lx+210,ly+8,7,0.3f,0.9f,1.f);  txt(lx+220,ly+2,"+20",0.3f,0.9f,1.f, GLUT_BITMAP_HELVETICA_12);
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
    gShield=false; gBasketGlow=0.f;
    gWindOn=false; gWindT=0; gWindF=0;

    /* chickens on each stick */
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

    /* clouds */
    gClouds.clear();
    for(int i=0;i<5;i++){
        Cloud c;
        c.x=randF(60,WIN_W-60);
        c.y=randF(WIN_H*0.55f,WIN_H*0.88f);
        c.spd=randF(18,38);
        gClouds.push_back(c);
    }
}

static void spawnItem(float chickenX,float chickenY)
{
    Item it;
    it.x      = chickenX + randF(-8,8);
    it.y      = chickenY - 22;
    it.active = true;
    it.vx     = gWindOn ? gWindF : 0.f;
    it.angle  = randF(0,360);

    float r=(float)rand()/RAND_MAX;
    /* probability table */
    if     (r<0.02f) it.type=EGG_RAINBOW;
    else if(r<0.07f) it.type=EGG_GOLDEN;
    else if(r<0.17f) it.type=EGG_BLUE;
    else if(r<0.48f) it.type=EGG_NORMAL;
    else if(r<0.60f) it.type=POOP;
    else if(r<0.66f) it.type=PWR_BASKET;
    else if(r<0.72f) it.type=PWR_SLOW;
    else if(r<0.80f) it.type=PWR_TIME;
    else if(r<0.86f) it.type=PWR_SHIELD;
    else if(r<0.93f) it.type=BONUS_STAR;
    else             it.type=BONUS_DIAMOND;

    it.vy = randF(80,165);
    gItems.push_back(it);
}

/* ================================================================
   UPDATE
================================================================ */
static void update(float dt)
{
    gAnim += dt;   /* global animation clock always ticks */

    if(gState!=ST_PLAY) return;

    /* countdown */
    gTime-=dt;
    if(gTime<=0){ gTime=0; if(gScore>gHiScore)gHiScore=gScore; gState=ST_OVER; return; }

    /* power timers */
    if(gSlowOn){ gSlowT-=dt; if(gSlowT<=0)gSlowOn=false; }
    if(gBigOn) { gBigT -=dt; if(gBigT <=0){ gBigOn=false; gBasket.w=BW_NORMAL; } }
    if(gBasketGlow>0) gBasketGlow-=dt;

    /* wind timer */
    if(gWindOn){ gWindT-=dt; if(gWindT<=0)gWindOn=false; }
    static float windSpawn=20.f;
    windSpawn-=dt;
    if(windSpawn<0){
        windSpawn=randF(15,35);
        gWindOn=true; gWindT=randF(4,8);
        gWindF=randF(30,70)*(rand()%2?1:-1);
    }

    /* clouds */
    for(auto& c:gClouds){
        c.x+=c.spd*dt;
        if(c.x>WIN_W+80) c.x=-80;
    }

    /* chickens */
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

    /* basket keyboard */
    if(gSpec[GLUT_KEY_LEFT] ||gKeys['a']||gKeys['A'])
        gBasket.cx-=BSPEED*dt;
    if(gSpec[GLUT_KEY_RIGHT]||gKeys['d']||gKeys['D'])
        gBasket.cx+=BSPEED*dt;
    float half=gBasket.w/2;
    if(gBasket.cx-half<0)     gBasket.cx=half;
    if(gBasket.cx+half>WIN_W) gBasket.cx=WIN_W-half;

    /* items */
    float sm=gSlowOn?0.38f:1.f;
    for(auto& it:gItems){
        if(!it.active)continue;
        it.y    -= it.vy*sm*dt;
        it.x    += it.vx*sm*dt;
        it.angle += 120.f*dt;  /* spin for star/diamond */

        /* basket catch zone */
        float bL=gBasket.cx-gBasket.w/2+14;
        float bR=gBasket.cx+gBasket.w/2-14;
        float bT=gBasket.y+BH;

        if(it.y<=bT && it.y>=gBasket.y-10 && it.x>=bL && it.x<=bR){
            it.active=false;
            ScorePopup pop; pop.x=it.x; pop.y=it.y+BH/2; pop.life=1.2f;
            switch(it.type){
                case EGG_NORMAL:
                    gScore+=1; pop.pts=1; pop.r=1;pop.g=1;pop.b=0.8f;
                    sndBeep(600,40); break;
                case EGG_BLUE:
                    gScore+=5; pop.pts=5; pop.r=0.4f;pop.g=0.8f;pop.b=1;
                    sndBeep(800,50); break;
                case EGG_GOLDEN:
                    gScore+=10; pop.pts=10; pop.r=1;pop.g=0.85f;pop.b=0;
                    sndBeep(1000,60); break;
                case EGG_RAINBOW:
                    gScore+=25; pop.pts=25; pop.r=1;pop.g=0.3f;pop.b=1;
                    sndBeep(1200,80); break;
                case BONUS_STAR:
                    gScore+=15; pop.pts=15; pop.r=1;pop.g=0.9f;pop.b=0.1f;
                    sndBeep(900,60); break;
                case BONUS_DIAMOND:
                    gScore+=20; pop.pts=20; pop.r=0.3f;pop.g=0.9f;pop.b=1;
                    sndBeep(1050,60); break;
                case POOP:
                    if(gShield){ gShield=false; pop.pts=0; pop.r=0.8f;pop.g=0.2f;pop.b=1; sndBeep(400,60); }
                    else        { gScore-=10;   pop.pts=-10; pop.r=1;pop.g=0.2f;pop.b=0.2f; sndBeep(150,180); }
                    break;
                case PWR_BASKET:
                    gBigOn=true; gBigT=10; gBasket.w=BW_BIG;
                    gBasketGlow=0.5f;
                    pop.pts=0; pop.r=1;pop.g=0.6f;pop.b=0;
                    sndBeep(700,80); break;
                case PWR_SLOW:
                    gSlowOn=true; gSlowT=8;
                    gBasketGlow=0.5f;
                    pop.pts=0; pop.r=0;pop.g=1;pop.b=1;
                    sndBeep(700,80); break;
                case PWR_TIME:
                    gTime+=20; if(gTime>120)gTime=120;
                    gBasketGlow=0.5f;
                    pop.pts=0; pop.r=0.2f;pop.g=1;pop.b=0.2f;
                    sndBeep(750,80); break;
                case PWR_SHIELD:
                    gShield=true;
                    gBasketGlow=0.5f;
                    pop.pts=0; pop.r=0.75f;pop.g=0.2f;pop.b=1;
                    sndBeep(850,80); break;
                default: pop.pts=0; pop.r=1;pop.g=1;pop.b=1; break;
            }
            gPopups.push_back(pop);
        }

        /* hit ground or leave window */
        if(it.y<60 || it.x<-30 || it.x>WIN_W+30) it.active=false;
    }

    /* remove dead items */
    gItems.erase(
        std::remove_if(gItems.begin(),gItems.end(),
                       [](const Item& i){return !i.active;}),
        gItems.end());

    /* age popups */
    for(auto& p:gPopups) p.life-=dt;
    gPopups.erase(
        std::remove_if(gPopups.begin(),gPopups.end(),
                       [](const ScorePopup& p){return p.life<=0;}),
        gPopups.end());
}

/* ================================================================
   SCORE POPUPS
================================================================ */
static void drawPopups()
{
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(auto& p:gPopups){
        float alpha=p.life/1.2f;
        float rise =(1.f-p.life/1.2f)*38;
        glColor4f(p.r,p.g,p.b,alpha);
        char buf[16];
        if(p.pts>0)  sprintf(buf,"+%d",p.pts);
        else if(p.pts<0) sprintf(buf,"%d",p.pts);
        else         sprintf(buf,"OK!");
        glRasterPos2f(p.x-10, p.y+rise);
        for(const char* c=buf;*c;c++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
    }
    glDisable(GL_BLEND);
}

/* ================================================================
   MENU  (scenic background: house, trees, fence, birds)
================================================================ */
static void drawMenu()
{
    /* daytime sky gradient */
    glBegin(GL_QUADS);
    glColor3f(0.35f,0.62f,0.98f); glVertex2f(0,0);
    glColor3f(0.35f,0.62f,0.98f); glVertex2f(WIN_W,0);
    glColor3f(0.55f,0.78f,1.0f);  glVertex2f(WIN_W,WIN_H);
    glColor3f(0.55f,0.78f,1.0f);  glVertex2f(0,WIN_H);
    glEnd();

    /* stars twinkling gently */
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for(int i=0;i<60;i++){
        float br=gStarBr[i]*(0.4f+0.3f*sinf(gAnim*1.5f+i));
        glColor3f(br,br,br);
        glVertex2f(gStarX[i],gStarY[i]);
    }
    glEnd();

    /* sun */
    ellipse(90,WIN_H-80,36,36, 1.f,0.92f,0.10f);
    ellipse(90,WIN_H-80,28,28, 1.f,0.96f,0.30f);
    glColor3f(1,0.92f,0.2f); glLineWidth(2.2f);
    for(int i=0;i<8;i++){
        float a=PI/4*i + gAnim*0.5f;
        glBegin(GL_LINES);
        glVertex2f(90+37*cosf(a),WIN_H-80+37*sinf(a));
        glVertex2f(90+52*cosf(a),WIN_H-80+52*sinf(a));
        glEnd();
    }

    /* menu clouds */
    static const float MCX[]={130,340,580,720};
    static const float MCY[]={WIN_H-150,WIN_H-175,WIN_H-145,WIN_H-168};
    for(int i=0;i<4;i++){
        float cx2=MCX[i]; float cy2=MCY[i];
        ellipse(cx2,    cy2,    40,25, 1,1,1);
        ellipse(cx2+35, cy2+10, 30,21, 1,1,1);
        ellipse(cx2-32, cy2+8,  28,20, 1,1,1);
        ellipse(cx2+10, cy2+26, 26,18, 1,1,1);
    }

    /* birds */
    float boff=fmodf(gAnim*22.f,(float)(WIN_W+60))-30;
    drawBird(boff,      WIN_H-210);
    drawBird(boff+45,   WIN_H-225);
    drawBird(boff+240,  WIN_H-200);

    /* distant hill */
    glColor3f(0.20f,0.52f,0.20f);
    glBegin(GL_POLYGON);
    glVertex2f(0,95);
    for(int i=0;i<=24;i++){
        float px=(float)WIN_W*i/24.f;
        float py=95+32*sinf(px*0.011f)+20*sinf(px*0.022f+0.8f);
        glVertex2f(px,py);
    }
    glVertex2f(WIN_W,95);
    glEnd();

    /* grass */
    glBegin(GL_QUADS);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(0,0);
    glColor3f(0.22f,0.72f,0.22f); glVertex2f(WIN_W,0);
    glColor3f(0.12f,0.55f,0.12f); glVertex2f(WIN_W,98);
    glColor3f(0.12f,0.55f,0.12f); glVertex2f(0,98);
    glEnd();
    /* animated blades */
    glColor3f(0.16f,0.62f,0.16f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=10){
        float sway=4.f*sinf(gAnim*2.f+i*0.22f);
        glBegin(GL_LINES);
        glVertex2f((float)i,98); glVertex2f(i+sway+4,110);
        glEnd();
    }

    /* flowers */
    static const float MFX[]={20,70,130,200,310,480,600,670,730,785};
    static const float MFR[]={1,0.9f,1,0.4f,1,0.8f,0.5f,1,0.9f,0.4f};
    static const float MFG[]={0.2f,0.8f,0.4f,0.6f,0.3f,0.2f,0.2f,0.8f,0.2f,0.9f};
    static const float MFB[]={0.5f,0.2f,0.2f,1,0.8f,0.9f,1,0.3f,0.7f,0.4f};
    for(int i=0;i<10;i++){
        glColor3f(0.12f,0.52f,0.12f); glLineWidth(1.5f);
        glBegin(GL_LINES);
        glVertex2f(MFX[i],98); glVertex2f(MFX[i],110);
        glEnd();
        drawFlower(MFX[i],114, MFR[i],MFG[i],MFB[i]);
    }

    /* fence */
    drawFence(0,(float)WIN_W, 88);

    /* left group of trees */
    drawTree(55,  98);
    drawTree(108, 98);
    drawTree(155, 98);

    /* main house (centre-left) */
    drawHouse(280, 98, 1.05f);

    /* smaller house (right) */
    drawHouse(560, 98, 0.82f);

    /* right trees */
    drawTree(668, 98);
    drawTree(718, 98);
    drawTree(765, 98);

    /* ---- dark panel behind title + buttons ---- */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f,0.02f,0.12f,0.72f);
    glBegin(GL_QUADS);
    glVertex2f(WIN_W/2-215, WIN_H/2-128);
    glVertex2f(WIN_W/2+215, WIN_H/2-128);
    glVertex2f(WIN_W/2+215, WIN_H-60);
    glVertex2f(WIN_W/2-215, WIN_H-60);
    glEnd();
    glDisable(GL_BLEND);
    glColor3f(0.5f,0.5f,0.85f); glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(WIN_W/2-215, WIN_H/2-128);
    glVertex2f(WIN_W/2+215, WIN_H/2-128);
    glVertex2f(WIN_W/2+215, WIN_H-60);
    glVertex2f(WIN_W/2-215, WIN_H-60);
    glEnd();

    /* title shadow + title */
    txtBig(WIN_W/2-155+2, WIN_H-110-2, "CATCH  THE  EGGS", 0,0,0);
    txtBig(WIN_W/2-155,   WIN_H-110,   "CATCH  THE  EGGS", 1.f,0.92f,0.05f);

    txt(WIN_W/2-115, WIN_H-148,
        "CSE 426  |  Computer Graphics Lab", 0.72f,0.72f,1.f,
        GLUT_BITMAP_HELVETICA_18);

    /* decorative items row */
    float iy=WIN_H-192;
    drawEgg(WIN_W/2-160,iy,16,22, 1.f,0.97f,0.88f);
    drawEgg(WIN_W/2-110,iy,18,24, 1.f,0.85f,0.f);
    drawEgg(WIN_W/2-58, iy,16,22, 0.4f,0.6f,1.f);
    drawRainbowEgg(WIN_W/2-5,iy,15,21);
    drawStar5(WIN_W/2+48, iy, 14, 1,0.9f,0.05f);
    drawDiamond(WIN_W/2+100,iy,13, 0.3f,0.9f,1.f);
    drawPoop(WIN_W/2+142, iy-8);

    /* buttons */
    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx, WIN_H/2+55,  bw,bh, "START GAME", gHover==0);
    button(bx, WIN_H/2,     bw,bh, "HIGH SCORE",  gHover==1);
    button(bx, WIN_H/2-55,  bw,bh, "HELP",        gHover==2);
    button(bx, WIN_H/2-110, bw,bh, "EXIT",        gHover==3);

    txt(WIN_W/2-185,18,
        "Arrow/A/D or Mouse to move  |  P/ESC to pause  |  N = Night mode",
        0.55f,0.55f,0.72f, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   OVERLAY helpers
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
    txtBig(WIN_W/2-52, WIN_H/2+100, "PAUSED", 1,1,0);

    float bx=WIN_W/2-75, bw=150, bh=40;
    button(bx, WIN_H/2+38, bw,bh, "RESUME",  gHover==0);
    button(bx, WIN_H/2-10, bw,bh, "RESTART", gHover==1);
    button(bx, WIN_H/2-58, bw,bh, "MENU",    gHover==2);
    button(bx, WIN_H/2-106,bw,bh, "EXIT",    gHover==3);
}

/* ================================================================
   GAME OVER SCREEN
================================================================ */
static void drawGameOver()
{
    darkOverlay(0.68f);
    txtBig(WIN_W/2-85, WIN_H/2+120, "GAME  OVER", 1,0.25f,0.25f);

    char buf[64];
    sprintf(buf,"Your Score : %d",gScore);
    txt(WIN_W/2-80, WIN_H/2+72, buf, 1,1,0);
    sprintf(buf,"Best Score : %d",gHiScore);
    txt(WIN_W/2-80, WIN_H/2+44, buf, 0.8f,0.8f,0.8f);

    float bx=WIN_W/2-75, bw=150, bh=40;
    button(bx, WIN_H/2-10, bw,bh, "PLAY AGAIN", gHover==0);
    button(bx, WIN_H/2-58, bw,bh, "MENU",       gHover==1);
    button(bx, WIN_H/2-106,bw,bh, "EXIT",       gHover==2);
}

/* ================================================================
   HIGH SCORE SCREEN
================================================================ */
static void drawHiScore()
{
    drawMenu();
    darkOverlay(0.72f);

    /* panel */
    quad(180,160, 440,270, 0.06f,0.06f,0.20f);
    quadLine(180,160,440,270, 1,1,0,2);

    txtBig(WIN_W/2-75, 395, "HIGH SCORE", 1,1,0);

    char buf[32];
    sprintf(buf,"%d  pts",gHiScore);
    txtBig(WIN_W/2-50, 300, buf, 1,0.85f,0);
    if(gHiScore==0)
        txt(WIN_W/2-85,260,"Play a round first!", 0.7f,0.7f,0.7f);

    /* BACK button: y=175, matches getBtn below */
    float bx=WIN_W/2-60, bw=120, bh=36;
    button(bx,175,bw,bh,"BACK", gHover==0);
}

/* ================================================================
   HELP SCREEN  (BONUS)
================================================================ */
static void drawHelp()
{
    drawMenu();
    darkOverlay(0.78f);

    quad(80,100, 640,400, 0.06f,0.06f,0.20f);
    quadLine(80,100,640,400, 1,0.9f,0,2);

    txtBig(WIN_W/2-55,475,"CONTROLS & HELP", 1,0.9f,0);

    const char* lines[]={
        "CONTROLS:",
        "  Arrow Left/Right  or  A/D     :  Move basket",
        "  Mouse move (during play)       :  Move basket",
        "  P  or  ESC                         :  Pause / Resume  (no time penalty)",
        "  N                                        :  Toggle Night mode",
        "",
        "EGGS (catch for points):",
        "  White Egg  = +1 pt     Blue Egg    = +5 pt",
        "  Gold Egg   = +10 pt    Rainbow Egg = +25 pt  (RARE!)",
        "  Poop       = -10 pt    (AVOID!)",
        "",
        "BONUS COLLECTIBLES:",
        "  Star (yellow spinning) = +15 pt",
        "  Diamond (cyan gem)     = +20 pt",
        "",
        "POWER-UP BLOCKS (falling coloured cubes):",
        "  BIG    (orange) : basket doubles in size for 10 s",
        "  SLOW   (cyan)   : eggs fall slower for 8 s",
        "  +TIME  (green)  : adds 20 seconds to the clock",
        "  SHIELD (purple) : next poop caught is negated",
        "",
        "  Wind can drift eggs sideways!    Time limit: 60 seconds."
    };
    int n=sizeof(lines)/sizeof(lines[0]);
    for(int i=0;i<n;i++){
        float lr=0.88f,lg=0.88f,lb=0.88f;
        if(lines[i][0]!='\0' && lines[i][0]!=' '){ lr=1;lg=0.85f;lb=0.3f; }
        txt(98, 440-i*20, lines[i], lr,lg,lb, GLUT_BITMAP_HELVETICA_12);
    }

    float bx=WIN_W/2-60, bw=120, bh=36;
    button(bx,108,bw,bh,"BACK", gHover==0);
}

/* ================================================================
   BUTTON HIT TESTING
================================================================ */
static int getBtn(int mx,int my)
{
    /* my is already in OpenGL coords (y flipped in callbacks) */
    float bx,bw=160,bh=42;
    if(gState==ST_MENU){
        bx=WIN_W/2-80;
        if(inRect(mx,my,bx,WIN_H/2+55, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2,    bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-55, bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2-110,bw,bh)) return 3;
    } else if(gState==ST_PAUSE){
        bx=WIN_W/2-75; bw=150; bh=40;
        if(inRect(mx,my,bx,WIN_H/2+38, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2-10, bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-58, bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2-106,bw,bh)) return 3;
    } else if(gState==ST_OVER){
        bx=WIN_W/2-75; bw=150; bh=40;
        if(inRect(mx,my,bx,WIN_H/2-10, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2-58, bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-106,bw,bh)) return 2;
    } else if(gState==ST_HISCORE){
        if(inRect(mx,my,WIN_W/2-60,175,120,36)) return 0;
    } else if(gState==ST_HELP){
        if(inRect(mx,my,WIN_W/2-60,108,120,36)) return 0;
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
        /* game world */
        drawBG();

        /* chickens */
        for(auto& h:gHens) drawChicken(h.x,h.y,h.dir);

        /* falling items */
        for(auto& it:gItems){
            if(!it.active)continue;
            switch(it.type){
                case EGG_NORMAL:    drawEgg(it.x,it.y,10,14, 1.f,0.97f,0.88f);  break;
                case EGG_BLUE:      drawEgg(it.x,it.y,10,14, 0.4f,0.6f,1.f);    break;
                case EGG_GOLDEN:    drawEgg(it.x,it.y,12,16, 1.f,0.85f,0.f);    break;
                case EGG_RAINBOW:   drawRainbowEgg(it.x,it.y,12,16);             break;
                case POOP:          drawPoop(it.x,it.y);                          break;
                case BONUS_STAR:    drawStar5(it.x,it.y,13, 1.f,0.9f,0.05f);    break;
                case BONUS_DIAMOND: drawDiamond(it.x,it.y,12, 0.3f,0.9f,1.f);   break;
                default:            drawPower(it.x,it.y,it.type);                 break;
            }
        }

        /* basket */
        drawBasket(gBasket.cx,gBasket.y,gBasket.w);

        /* score popups */
        drawPopups();

        /* HUD */
        drawHUD();

        /* overlays */
        if(gState==ST_PAUSE) drawPause();
        if(gState==ST_OVER)  drawGameOver();
    }

    glutSwapBuffers();
}

/* ================================================================
   TIMER LOOP
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
    if(k==27||k=='p'||k=='P'){        /* ESC or P  – no time penalty */
        if(gState==ST_PLAY)       gState=ST_PAUSE;
        else if(gState==ST_PAUSE) gState=ST_PLAY;
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
    int gy=WIN_H-my;     /* flip y */
    if(gState==ST_PLAY){
        setBasketX((float)mx);
    }
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
        if(clicked==0) gState=ST_MENU;
    }
    glutPostRedisplay();
}

static void mouseFunction(int btn,int state,int mx,int my)
{
    mouseClick(btn,state,mx,my);
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

    /* pre-generate stars */
    for(int i=0;i<100;i++){
        gStarX[i]=randF(0,(float)WIN_W);
        gStarY[i]=randF(0,(float)WIN_H);
        gStarBr[i]=randF(0.5f,1.0f);
    }

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(100,80);
    glutCreateWindow("Catch The Eggs  |  CSE 426  |  Enhanced Edition");

    glClearColor(0,0,0,1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specDown);
    glutSpecialUpFunc(specUp);
    glutPassiveMotionFunc(mouseMove);
    glutMotionFunc(mouseMove);
    glutMouseFunc(mouseFunction);

    gLastTime=glutGet(GLUT_ELAPSED_TIME)/1000.f;
    glutTimerFunc(16,timerCB,0);

    glutMainLoop();
    return 0;
}
