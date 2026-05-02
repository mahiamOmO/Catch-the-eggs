/*
 * ================================================================
 *   CATCH THE EGGS  –  CSE 426 Computer Graphics Lab
 *   Term Project | Spring 2025
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
 * ----------------------------------------------------------------
 *  COMPILE
 *    Linux  :  g++ catch_the_eggs.cpp -o game -lGL -lGLU -lglut -lm
 *    Windows:  g++ catch_the_eggs.cpp -o game -lfreeglut -lopengl32 -lglu32 -lm
 *
 *  CONTROLS
 *    Left/Right Arrow or A/D  – move basket
 *    Mouse move               – move basket (during play)
 *    P  or  ESC               – pause / resume
 *    N                        – toggle night mode (bonus)
 *    Mouse click              – menu navigation
 * ================================================================
 */

#ifdef _WIN32
#  include <windows.h>
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
   GAME STATE
---------------------------------------------------------------- */
enum GameState { ST_MENU, ST_PLAY, ST_PAUSE, ST_OVER, ST_HISCORE, ST_HELP };
static GameState gState = ST_MENU;

/* ----------------------------------------------------------------
   ITEM TYPES
---------------------------------------------------------------- */
enum ItemType {
    EGG_NORMAL,   /* white  +1  pt */
    EGG_BLUE,     /* blue   +5  pt */
    EGG_GOLDEN,   /* gold  +10  pt */
    POOP,         /* brown -10  pt */
    PWR_BASKET,   /* orange  bigger basket 10 s */
    PWR_SLOW,     /* cyan    slow eggs     8 s  */
    PWR_TIME,     /* green   +20 seconds        */
    PWR_SHIELD    /* purple  negate 1 poop       */
};

/* ----------------------------------------------------------------
   STRUCTS
---------------------------------------------------------------- */
struct Item {
    float x, y;
    float vy;          /* fall speed px/s            */
    float vx;          /* horizontal drift (wind)     */
    ItemType type;
    bool  active;
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

static std::vector<Chicken> gHens;
static Basket  gBasket;
static std::vector<Item>   gItems;
static std::vector<Cloud>  gClouds;

static int   gScore     = 0;
static int   gHiScore   = 0;
static float gTime      = 120.f;   /* 2 minutes */

/* power-up timers */
static bool  gSlowOn    = false;   static float gSlowT  = 0.f;
static bool  gBigOn     = false;   static float gBigT   = 0.f;
static bool  gShield    = false;

/* wind (BONUS) */
static bool  gWindOn    = false;
static float gWindT     = 0.f;
static float gWindF     = 0.f;    /* drift px/s */

/* night mode (BONUS) */
static bool  gNight     = false;

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
    quad(x+4,y-4,w,h, 0,0,0);
    /* fill */
    if(hover) quad(x,y,w,h, 0.95f,0.70f,0.10f);
    else       quad(x,y,w,h, 0.15f,0.15f,0.42f);
    quadLine(x,y,w,h, 1,1,1,2);
    float tx=x+w/2-(float)strlen(lbl)*5.f;
    txt(tx,y+h/2-6,lbl);
}

/* ================================================================
   BACKGROUND
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
        for(int i=0;i<100;i++){
            glColor3f(gStarBr[i],gStarBr[i],gStarBr[i]);
            glVertex2f(gStarX[i],gStarY[i]);
        }
        glEnd();
        /* moon */
        ellipse(680,WIN_H-60,30,30, 0.95f,0.95f,0.75f);
        ellipse(692,WIN_H-55,26,26, 0.04f,0.04f,0.15f);
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.45f,0.75f,0.98f); glVertex2f(0,0);
        glColor3f(0.45f,0.75f,0.98f); glVertex2f(WIN_W,0);
        glColor3f(0.18f,0.45f,0.90f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.18f,0.45f,0.90f); glVertex2f(0,WIN_H);
        glEnd();
        /* sun */
        ellipse(100,WIN_H-70,32,32, 1.0f,0.90f,0.10f);
        ellipse(100,WIN_H-70,26,26, 1.0f,0.95f,0.30f);
        /* rays */
        glColor3f(1,0.92f,0.2f); glLineWidth(2);
        for(int i=0;i<8;i++){
            float a=PI/4*i;
            glBegin(GL_LINES);
            glVertex2f(100+34*cosf(a),WIN_H-70+34*sinf(a));
            glVertex2f(100+48*cosf(a),WIN_H-70+48*sinf(a));
            glEnd();
        }
    }

    /* clouds (skip at night for look) */
    if(!gNight){
        for(auto& c:gClouds){
            ellipse(c.x,    c.y,    34,23, 1,1,1);
            ellipse(c.x+30, c.y+9,  28,20, 1,1,1);
            ellipse(c.x-30, c.y+6,  25,18, 1,1,1);
            ellipse(c.x+8,  c.y+23, 22,17, 1,1,1);
        }
    }

    /* grass */
    glBegin(GL_QUADS);
    glColor3f(0.18f,0.68f,0.18f); glVertex2f(0,0);
    glColor3f(0.18f,0.68f,0.18f); glVertex2f(WIN_W,0);
    glColor3f(0.08f,0.45f,0.08f); glVertex2f(WIN_W,68);
    glColor3f(0.08f,0.45f,0.08f); glVertex2f(0,68);
    glEnd();
    /* grass blades */
    glColor3f(0.12f,0.58f,0.12f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=11){
        glBegin(GL_LINES);
        glVertex2f(i,68); glVertex2f(i+5,82);
        glEnd();
    }

    /* ---- bamboo sticks (BONUS: 2 sticks) ---- */
    for(int s=0;s<N_STICKS;s++){
        float sy=STICK_Y[s];
        /* stick body */
        glBegin(GL_QUADS);
        glColor3f(0.50f,0.32f,0.06f); glVertex2f(40,sy);
        glColor3f(0.50f,0.32f,0.06f); glVertex2f(WIN_W-40,sy);
        glColor3f(0.38f,0.22f,0.02f); glVertex2f(WIN_W-40,sy+12);
        glColor3f(0.38f,0.22f,0.02f); glVertex2f(40,sy+12);
        glEnd();
        /* joints */
        glColor3f(0.30f,0.18f,0.01f); glLineWidth(2.f);
        for(int j=80;j<WIN_W-80;j+=55){
            glBegin(GL_LINES);
            glVertex2f(j,sy); glVertex2f(j,sy+12);
            glEnd();
        }
        /* support poles */
        glColor3f(0.35f,0.22f,0.04f); glLineWidth(4.f);
        glBegin(GL_LINES);
        glVertex2f(40, sy+12); glVertex2f(40, 68);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(WIN_W-40, sy+12); glVertex2f(WIN_W-40, 68);
        glEnd();
    }

    /* wind indicator (BONUS) */
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
    /* bar */
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(0,WIN_H-40); glVertex2f(WIN_W,WIN_H-40);
    glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();

    char buf[64];

    sprintf(buf,"Score: %d",gScore);
    txt(12, WIN_H-25, buf, 1,1,0);

    int m=(int)gTime/60, s=(int)gTime%60;
    sprintf(buf,"Time: %d:%02d",m,s);
    float tg=(gTime<20)?0.2f:1.f;
    txt(WIN_W/2-42, WIN_H-25, buf, 1,tg,0);

    sprintf(buf,"Best: %d",gHiScore);
    txt(WIN_W-115, WIN_H-25, buf, 0.75f,0.75f,0.75f);

    /* active power-up tags */
    float px=12;
    if(gSlowOn){
        quad(px,WIN_H-58, 60,16, 0,0.6f,0.6f);
        txt(px+4,WIN_H-49,"SLOW", 0,0,0, GLUT_BITMAP_HELVETICA_12);
        px+=66;
    }
    if(gBigOn){
        quad(px,WIN_H-58, 76,16, 0.8f,0.4f,0);
        txt(px+4,WIN_H-49,"BIG BASKET", 0,0,0, GLUT_BITMAP_HELVETICA_12);
        px+=82;
    }
    if(gShield){
        quad(px,WIN_H-58, 62,16, 0.55f,0.0f,0.75f);
        txt(px+4,WIN_H-49,"SHIELD", 1,1,1, GLUT_BITMAP_HELVETICA_12);
    }

    /* egg legend bottom-right */
    float lx=WIN_W-180, ly=10;
    drawEgg(lx,    ly+6, 7,9,  1.f,0.97f,0.88f); txt(lx+10,ly+2,"=1",  0.9f,0.9f,0.9f,GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+40, ly+6, 7,9,  0.4f,0.6f,1.f);   txt(lx+50,ly+2,"=5",  0.4f,0.7f,1.f, GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+80, ly+6, 8,10, 1.f,0.85f,0.f);   txt(lx+90,ly+2,"=10", 1.f,0.85f,0.f, GLUT_BITMAP_HELVETICA_12);
    drawPoop(lx+132,ly+2);
    txt(lx+142,ly+2,"-10", 1,0.3f,0.3f, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   GAME INIT
================================================================ */
static void spawnItem(float chickenX, float chickenY);

static void initGame()
{
    gScore=0; gTime=120.f; gItems.clear();
    gSlowOn=false; gSlowT=0;
    gBigOn=false;  gBigT=0;
    gShield=false;
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

    float r=(float)rand()/RAND_MAX;
    /* probability table */
    if     (r<0.05f) it.type=EGG_GOLDEN;
    else if(r<0.15f) it.type=EGG_BLUE;
    else if(r<0.52f) it.type=EGG_NORMAL;
    else if(r<0.66f) it.type=POOP;
    else if(r<0.73f) it.type=PWR_BASKET;
    else if(r<0.80f) it.type=PWR_SLOW;
    else if(r<0.90f) it.type=PWR_TIME;
    else             it.type=PWR_SHIELD;

    it.vy = randF(80,165);
    gItems.push_back(it);
}

/* ================================================================
   UPDATE
================================================================ */
static void update(float dt)
{
    if(gState!=ST_PLAY) return;

    /* countdown */
    gTime-=dt;
    if(gTime<=0){ gTime=0; if(gScore>gHiScore)gHiScore=gScore; gState=ST_OVER; return; }

    /* power timers */
    if(gSlowOn){ gSlowT-=dt; if(gSlowT<=0)gSlowOn=false; }
    if(gBigOn) { gBigT -=dt; if(gBigT <=0){ gBigOn=false; gBasket.w=BW_NORMAL; } }

    /* wind timer (BONUS) */
    if(gWindOn){ gWindT-=dt; if(gWindT<=0)gWindOn=false; }

    /* random wind spawn */
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
        it.y -=it.vy*sm*dt;
        it.x +=it.vx*sm*dt;   /* wind drift (BONUS) */

        /* basket catch zone */
        float bL=gBasket.cx-gBasket.w/2+14;
        float bR=gBasket.cx+gBasket.w/2-14;
        float bT=gBasket.y+BH;

        if(it.y<=bT && it.y>=gBasket.y-10 && it.x>=bL && it.x<=bR){
            it.active=false;
            switch(it.type){
                case EGG_NORMAL: gScore+=1;  break;
                case EGG_BLUE:   gScore+=5;  break;
                case EGG_GOLDEN: gScore+=10; break;
                case POOP:
                    if(gShield){ gShield=false; }
                    else        { gScore-=10; if(gScore<0)gScore=0; }
                    break;
                case PWR_BASKET:
                    gBigOn=true; gBigT=10.f; gBasket.w=BW_BIG; break;
                case PWR_SLOW:
                    gSlowOn=true; gSlowT=8.f; break;
                case PWR_TIME:
                    gTime+=20.f; break;
                case PWR_SHIELD:
                    gShield=true; break;
            }
        }

        /* hit ground or leave window */
        if(it.y<60 || it.x<-30 || it.x>WIN_W+30) it.active=false;
    }

    /* remove dead items */
    gItems.erase(
        std::remove_if(gItems.begin(),gItems.end(),
                       [](const Item& i){return !i.active;}),
        gItems.end());
}

/* ================================================================
   MENU
================================================================ */
static void drawMenu()
{
    /* dark-blue gradient background */
    glBegin(GL_QUADS);
    glColor3f(0.04f,0.04f,0.18f); glVertex2f(0,0);
    glColor3f(0.04f,0.04f,0.18f); glVertex2f(WIN_W,0);
    glColor3f(0.10f,0.02f,0.28f); glVertex2f(WIN_W,WIN_H);
    glColor3f(0.10f,0.02f,0.28f); glVertex2f(0,WIN_H);
    glEnd();

    /* stars */
    glPointSize(2.f);
    glBegin(GL_POINTS);
    for(int i=0;i<100;i++){
        glColor3f(gStarBr[i],gStarBr[i],gStarBr[i]);
        glVertex2f(gStarX[i],gStarY[i]);
    }
    glEnd();

    /* title */
    txtBig(WIN_W/2-155, WIN_H-110, "CATCH  THE  EGGS", 1.f,0.92f,0.05f);

    /* sub */
    txt(WIN_W/2-115, WIN_H-148,
        "CSE 426  |  Computer Graphics Lab", 0.7f,0.7f,1.f,
        GLUT_BITMAP_HELVETICA_18);

    /* decorative eggs */
    drawEgg(WIN_W/2-120, WIN_H-195, 18,24, 1.f,0.97f,0.88f);
    drawEgg(WIN_W/2-50,  WIN_H-205, 20,26, 1.f,0.85f,0.f);
    drawEgg(WIN_W/2+20,  WIN_H-200, 18,24, 0.4f,0.6f,1.f);
    drawPoop(WIN_W/2+80, WIN_H-210);

    /* buttons */
    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx, WIN_H/2+55,  bw,bh, "START GAME", gHover==0);
    button(bx, WIN_H/2,     bw,bh, "HIGH SCORE",  gHover==1);
    button(bx, WIN_H/2-55,  bw,bh, "HELP",        gHover==2);
    button(bx, WIN_H/2-110, bw,bh, "EXIT",        gHover==3);

    txt(WIN_W/2-165,20,
        "Arrow/A/D or Mouse to move basket  |  P / ESC to pause",
        0.55f,0.55f,0.55f, GLUT_BITMAP_HELVETICA_12);
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
    /* reuse menu bg */
    drawMenu();
    darkOverlay(0.75f);

    /* panel */
    quad(200,180, 400,240, 0.08f,0.08f,0.22f);
    quadLine(200,180,400,240, 1,1,0,2);

    txtBig(WIN_W/2-75, 385, "HIGH SCORE", 1,1,0);

    char buf[32];
    sprintf(buf,"%d",gHiScore);
    txtBig(WIN_W/2-30, 290, buf, 1,0.85f,0);

    txt(WIN_W/2-30,240,"points", 0.8f,0.8f,0.8f);

    float bx=WIN_W/2-60, bw=120, bh=36;
    button(bx,200,bw,bh,"BACK", gHover==0);
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
        "Arrow Left / Right  or  A / D  :  Move basket",
        "Mouse Move (during play)         :  Move basket",
        "P  or  ESC                              :  Pause / Resume",
        "N                                             :  Toggle Night mode (bonus)",
        "",
        "  White Egg   =  +1 pt     Blue Egg   =  +5 pt",
        "  Gold  Egg   =  +10 pt    Poop          =  -10 pt",
        "",
        "  POWER-UPS (falling blocks):",
        "  BIG (orange)   : basket size x2 for 10 s",
        "  SLOW (cyan)    : eggs fall slower for 8 s",
        "  +TIME (green)  : adds 20 seconds",
        "  SHIELD (purple): next poop has no effect  (BONUS)",
        "",
        "  Wind can drift eggs sideways!  (BONUS)"
    };
    int n=sizeof(lines)/sizeof(lines[0]);
    for(int i=0;i<n;i++){
        txt(100, 440-i*24, lines[i], 0.88f,0.88f,0.88f, GLUT_BITMAP_HELVETICA_12);
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
    } else if(gState==ST_HISCORE||gState==ST_HELP){
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
                case EGG_NORMAL: drawEgg(it.x,it.y,10,14, 1.f,0.97f,0.88f); break;
                case EGG_BLUE:   drawEgg(it.x,it.y,10,14, 0.4f,0.6f,1.f);   break;
                case EGG_GOLDEN: drawEgg(it.x,it.y,12,16, 1.f,0.85f,0.f);   break;
                case POOP:       drawPoop(it.x,it.y);                         break;
                default:         drawPower(it.x,it.y,it.type);                break;
            }
        }

        /* basket */
        drawBasket(gBasket.cx,gBasket.y,gBasket.w);

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
    if(k==27||k=='p'||k=='P'){        /* ESC or P */
        if(gState==ST_PLAY)  gState=ST_PAUSE;
        else if(gState==ST_PAUSE) gState=ST_PLAY;
    }
    if((k=='n'||k=='N')&&gState==ST_PLAY) gNight=!gNight;
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
    glutCreateWindow("Catch The Eggs  |  CSE 426");

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
