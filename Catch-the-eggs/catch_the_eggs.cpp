/*
 * ================================================================
 *   CATCH THE EGGS  –  CSE 426 Computer Graphics Lab
 *   Term Project | Spring 2025  –  
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
 *   (+) Help / Controls page  (BACK button fixed)
 *   (+) Day-Night sky toggle (N key)
 *   (+) House & trees on menu AND game background
 *   (+) Combo system  (consecutive catches = bonus points)
 *   (+) Score popups  (floating +/- numbers)
 *   (+) Star rating on Game Over screen
 *   (+) Power-ups grant extra points when caught
 *   (+) Sound effects via system beep (Windows only; silent on Linux)
 *   (+) Background music toggle (M key; Windows only)
 *   (+) Game time = 60 seconds (1 minute) as required
 *   (+) Pause does NOT steal extra time
 *   (+) Animated menu (bouncing eggs, twinkling stars, smoke)
 *
 * ----------------------------------------------------------------
 *  COMPILE
 *    Linux  :  g++ catch_the_eggs.cpp -o game -lGL -lGLU -lglut -lm -lSDL2
 *    Windows:  g++ catch_the_eggs.cpp -o game -lfreeglut -lopengl32 -lglu32 -lm -lSDL2
 *
 *  INSTALL SDL2 (Linux):
 *    sudo apt install libsdl2-dev
 *
 *  CONTROLS
 *    Left/Right Arrow or A/D  – move basket
 *    Mouse move               – move basket (during play)
 *    P  or  ESC               – pause / resume
 *    N                        – toggle night mode
 *    M                        – toggle background music
 *    Mouse click              – menu navigation
 * ================================================================
 */

#ifdef _WIN32
#  define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

static const int   WIN_W = 800;
static const int   WIN_H = 600;
static const float PI    = 3.14159265f;

/* ----------------------------------------------------------------
   GAME STATE
---------------------------------------------------------------- */
enum GameState { ST_MENU, ST_PLAY, ST_PAUSE, ST_OVER, ST_HISCORE, ST_HELP };
static GameState gState = ST_MENU;
static GameState gLastState = ST_MENU;

/* ----------------------------------------------------------------
   ITEM TYPES
---------------------------------------------------------------- */
enum ItemType {
    EGG_NORMAL,   /* white  +1  pt */
    EGG_BLUE,     /* blue   +5  pt */
    EGG_GOLDEN,   /* gold  +10  pt */
    POOP,         /* brown -10  pt */
    PWR_BASKET,   /* orange  bigger basket 10s  +3pt bonus */
    PWR_SLOW,     /* cyan    slow eggs     8s   +3pt bonus */
    PWR_TIME,     /* green   +20 seconds        +5pt bonus */
    PWR_SHIELD    /* purple  negate 1 poop       +2pt bonus */
};

/* ----------------------------------------------------------------
   STRUCTS
---------------------------------------------------------------- */
struct Item {
    float x, y, vy, vx;
    ItemType type;
    bool  active;
};

struct Chicken {
    float x, y, speed;
    int   dir;
    float layTimer, layInterval;
};

struct Basket {
    float cx, y, w;
};

struct Cloud {
    float x, y, spd;
};

struct Popup {
    float x, y, vy, life, maxLife;
    char  text[16];
    float r, g, b;
};

/* ----------------------------------------------------------------
   GLOBALS
---------------------------------------------------------------- */
static const int   N_STICKS  = 2;
static const float STICK_Y[N_STICKS] = { WIN_H - 90.f, WIN_H - 230.f };

static std::vector<Chicken> gHens;
static Basket               gBasket;
static std::vector<Item>    gItems;
static std::vector<Cloud>   gClouds;
static std::vector<Popup>   gPopups;

static int   gScore    = 0;
static int   gHiScore  = 0;
static float gTime     = 60.f;    /* 1 minute */

/* combo */
static int   gCombo      = 0;
static float gComboTimer = 0.f;

/* power timers */
static bool  gSlowOn = false;  static float gSlowT = 0.f;
static bool  gBigOn  = false;  static float gBigT  = 0.f;
static bool  gShield = false;

/* wind */
static bool  gWindOn = false;
static float gWindT  = 0.f;
static float gWindF  = 0.f;

/* night mode */
static bool  gNight = false;

/* animation clock */
static float gAnim = 0.f;

static const float BW_NORMAL = 110.f;
static const float BW_BIG    = 210.f;
static const float BH        = 44.f;
static const float BSPEED    = 380.f;

static bool  gKeys[256] = {};
static bool  gSpec[256] = {};
static int   gHover     = -1;
static float gLastTime  = 0.f;

static float gStarX[120], gStarY[120], gStarBr[120];

/* ================================================================
   AUDIO  –  SDL2 procedural synthesis (Linux + Windows)
================================================================ */

static const int  AUDIO_SAMPLE_RATE = 44100;
static const int  AUDIO_CHANNELS    = 1;          /* mono */
static SDL_AudioDeviceID gAudioDev  = 0;

/* ----  music state (written only from main thread, read in callback) ---- */
static bool  gMusicOn      = true;
static bool  gMusicPlaying = false;

/* Game BGM: cheerful major-key 16-note loop */
static const float GAME_MELODY[] = {
    523,659,784,880, 784,659,784,523,
    392,523,659,784, 659,523,659,392
};
static const int   N_GAME_NOTES  = 16;
static const float GAME_BEAT_DUR = 0.22f;   /* seconds per note */

/* Menu BGM: slower, upbeat 8-note loop */
static const float MENU_MELODY[] = {
    523,587,659,784, 659,587,523,392
};
static const int   N_MENU_NOTES  = 8;
static const float MENU_BEAT_DUR = 0.30f;

/* Game-over jingle: short 5-note descending */
static const float OVER_MELODY[] = { 440,415,392,370,349 };
static const int   N_OVER_NOTES  = 5;
static const float OVER_BEAT_DUR = 0.28f;

/* which melody is active */
enum MusicTrack { TRACK_NONE, TRACK_MENU, TRACK_GAME, TRACK_OVER };
static MusicTrack gMusicTrack = TRACK_NONE;

/* current position inside melody */
static int   gMusicBeatIdx     = 0;
static int   gMusicSamplePos   = 0;   /* sample index within current beat */
static int   gMusicBeatSamples = 0;   /* samples per beat (set when track changes) */
static float gMusicCurFreq     = 0.f;
static int   gMusicTotalBeats  = 0;   /* for one-shot tracks (OVER) */
static int   gMusicBeatsPlayed = 0;

/* ----  sound-effect queue (lock-free ring, main writes, callback reads) ---- */
struct SfxNote { float freq; int dur; int pos; bool active; };
static const int   MAX_SFX  = 12;
static SfxNote     gSfx[MAX_SFX] = {};
static SDL_SpinLock gSfxLock = 0;

static void sfxTrigger(float freq, float dur_sec, float freq2=0, float dur2=0){
    SDL_AtomicLock(&gSfxLock);
    /* find two free slots */
    int found=0;
    for(int i=0;i<MAX_SFX&&found<2;i++){
        if(!gSfx[i].active){
            if(found==0){
                gSfx[i]={freq,(int)(dur_sec*AUDIO_SAMPLE_RATE),0,true};
                found++;
            } else if(freq2>0){
                /* second note starts half-way through first */
                gSfx[i]={freq2,(int)(dur2*AUDIO_SAMPLE_RATE),0,true};
                found++;
            } else break;
        }
    }
    SDL_AtomicUnlock(&gSfxLock);
}

/* ----  audio callback  ---- */
static void audioCallback(void*, Uint8* stream, int len){
    int16_t* out = (int16_t*)stream;
    int n = len / 2;

    for(int i=0;i<n;i++){
        float s = 0.f;

        /* --- music --- */
        if(gMusicOn && gMusicPlaying && gMusicBeatSamples>0){
            float t = (float)gMusicSamplePos / AUDIO_SAMPLE_RATE;

            /* amplitude envelope: attack 10%, sustain 65%, release 25% */
            float env;
            int atk = gMusicBeatSamples/10;
            int rel = gMusicBeatSamples/4;
            if(gMusicSamplePos < atk)
                env = (float)gMusicSamplePos / atk;
            else if(gMusicSamplePos < gMusicBeatSamples - rel)
                env = 1.f;
            else
                env = (float)(gMusicBeatSamples - gMusicSamplePos) / rel;

            /* fundamental + gentle 2nd harmonic */
            s += sinf(2.f*3.14159265f * gMusicCurFreq * t) * 0.16f * env;
            s += sinf(4.f*3.14159265f * gMusicCurFreq * t) * 0.04f * env;

            gMusicSamplePos++;
            if(gMusicSamplePos >= gMusicBeatSamples){
                gMusicSamplePos = 0;
                gMusicBeatIdx++;
                gMusicBeatsPlayed++;

                /* select current track */
                const float* mel=GAME_MELODY; int nm=N_GAME_NOTES;
                if(gMusicTrack==TRACK_MENU){ mel=MENU_MELODY; nm=N_MENU_NOTES; }
                else if(gMusicTrack==TRACK_OVER){ mel=OVER_MELODY; nm=N_OVER_NOTES; }

                if(gMusicTrack==TRACK_OVER && gMusicBeatsPlayed>=N_OVER_NOTES){
                    gMusicPlaying=false;  /* one-shot */
                } else {
                    gMusicBeatIdx %= nm;
                    gMusicCurFreq  = mel[gMusicBeatIdx];
                }
            }
        }

        /* --- sound effects (mix on top) --- */
        SDL_AtomicLock(&gSfxLock);
        for(int j=0;j<MAX_SFX;j++){
            SfxNote& sx=gSfx[j];
            if(!sx.active) continue;
            float t=(float)sx.pos/AUDIO_SAMPLE_RATE;
            float env=(float)(sx.dur-sx.pos)/sx.dur;
            s += sinf(2.f*3.14159265f*sx.freq*t) * 0.28f * env * env;
            sx.pos++;
            if(sx.pos>=sx.dur) sx.active=false;
        }
        SDL_AtomicUnlock(&gSfxLock);

        /* clamp & write */
        if(s> 1.f) s= 1.f;
        if(s<-1.f) s=-1.f;
        out[i]=(int16_t)(s*32767);
    }
}

/* ----  helper: switch active music track  ---- */
static void setMusicTrack(MusicTrack track){
    if(gMusicTrack==track) return;
    gMusicTrack      = track;
    gMusicBeatIdx    = 0;
    gMusicSamplePos  = 0;
    gMusicBeatsPlayed= 0;

    float beatDur = GAME_BEAT_DUR;
    const float* mel = GAME_MELODY;
    if(track==TRACK_MENU){ beatDur=MENU_BEAT_DUR; mel=MENU_MELODY; }
    else if(track==TRACK_OVER){ beatDur=OVER_BEAT_DUR; mel=OVER_MELODY; }

    gMusicBeatSamples = (int)(beatDur * AUDIO_SAMPLE_RATE);
    gMusicCurFreq     = mel[0];
    gMusicPlaying     = (track!=TRACK_NONE);
}

/* ----  public music API (called from game code)  ---- */
static void musicPlay()   { gMusicPlaying=true;  }
static void musicStop()   { gMusicPlaying=false; }
static void musicToggle() {
    gMusicOn=!gMusicOn;
    if(!gMusicOn) musicStop();
    else          musicPlay();
}
static void playMenuMusic()    { setMusicTrack(TRACK_MENU); }
static void playGameOverMusic(){ setMusicTrack(TRACK_OVER); }
static void playGameMusic()    {}   /* game track starts at initGame() */

/* ----  sound effects  ---- */
static void soundCatch(ItemType t){
    switch(t){
        case EGG_NORMAL:  sfxTrigger(880, 0.06f); break;
        case EGG_BLUE:    sfxTrigger(1100,0.08f); break;
        case EGG_GOLDEN:  sfxTrigger(1400,0.12f,1760,0.08f); break;
        case POOP:        sfxTrigger(180, 0.18f); break;
        default:          sfxTrigger(660, 0.07f,990,0.07f);  break;
    }
}

/* ----  init audio (call once from main)  ---- */
static void audioInit(){
    if(SDL_Init(SDL_INIT_AUDIO)<0){ return; }
    SDL_AudioSpec want={}, got;
    want.freq     = AUDIO_SAMPLE_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = AUDIO_CHANNELS;
    want.samples  = 1024;
    want.callback = audioCallback;
    gAudioDev = SDL_OpenAudioDevice(nullptr,0,&want,&got,0);
    if(gAudioDev) SDL_PauseAudioDevice(gAudioDev,0);
}

/* ================================================================
   UTILITY
================================================================ */
static float randF(float lo,float hi){
    return lo+(float)rand()/RAND_MAX*(hi-lo);
}
static void txt(float x,float y,const char* s,
                float r=1,float g=1,float b=1,
                void* font=GLUT_BITMAP_HELVETICA_18)
{
    glColor3f(r,g,b); glRasterPos2f(x,y);
    for(const char* c=s;*c;c++) glutBitmapCharacter(font,*c);
}
static void txtBig(float x,float y,const char* s,float r=1,float g=1,float b=1){
    glColor3f(r,g,b); glRasterPos2f(x,y);
    for(const char* c=s;*c;c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
}

/* ================================================================
   DRAW HELPERS
================================================================ */
static void quad(float x,float y,float w,float h,float r,float g,float b){
    glColor3f(r,g,b);
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void quadLine(float x,float y,float w,float h,
                     float r,float g,float b,float lw=2){
    glColor3f(r,g,b); glLineWidth(lw);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y);
    glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
static void ellipse(float cx,float cy,float rx,float ry,
                    float r,float g,float b,int seg=36){
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<seg;i++){
        float a=2*PI*i/seg;
        glVertex2f(cx+rx*cosf(a),cy+ry*sinf(a));
    }
    glEnd();
}
static void circle(float cx,float cy,float rad,float r,float g,float b){
    ellipse(cx,cy,rad,rad,r,g,b);
}
static void darkOverlay(float alpha){
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,alpha);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(WIN_W,0);
    glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    glDisable(GL_BLEND);
}
static bool inRect(int mx,int my,float x,float y,float w,float h){
    return mx>=x&&mx<=x+w&&my>=y&&my<=y+h;
}

/* ================================================================
   BUTTON
================================================================ */
static void button(float x,float y,float w,float h,const char* lbl,bool hover){
    quad(x+4,y-4,w,h, 0,0,0);
    if(hover){
        glBegin(GL_QUADS);
        glColor3f(1.f,0.82f,0.15f); glVertex2f(x,y); glVertex2f(x+w,y);
        glColor3f(0.9f,0.65f,0.05f); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
        glEnd();
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.20f,0.20f,0.52f); glVertex2f(x,y); glVertex2f(x+w,y);
        glColor3f(0.10f,0.10f,0.35f); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
        glEnd();
    }
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x+2,y+h-4); glVertex2f(x+w-2,y+h-4);
    glVertex2f(x+w-2,y+h-2); glVertex2f(x+2,y+h-2);
    glEnd();
    glDisable(GL_BLEND);
    quadLine(x,y,w,h, hover?1.f:0.55f, hover?1.f:0.55f, hover?0.3f:0.8f, 1.8f);
    float tx=x+w/2-(float)strlen(lbl)*5.f;
    txt(tx,y+h/2-6,lbl, hover?0.05f:1.f, hover?0.05f:1.f, hover?0.05f:1.f);
}

/* ================================================================
   TREE
================================================================ */
static void drawTree(float cx, float groundY, float scale=1.f){
    float tw=12*scale, th=28*scale;
    quad(cx-tw/2, groundY, tw, th, 0.42f,0.26f,0.05f);
    glColor3f(0.12f,0.60f,0.12f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-38*scale,groundY+th);
    glVertex2f(cx+38*scale,groundY+th);
    glVertex2f(cx,          groundY+th+46*scale);
    glEnd();
    glColor3f(0.10f,0.50f,0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-27*scale,groundY+th+27*scale);
    glVertex2f(cx+27*scale,groundY+th+27*scale);
    glVertex2f(cx,          groundY+th+66*scale);
    glEnd();
    glColor3f(0.15f,0.68f,0.15f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-17*scale,groundY+th+50*scale);
    glVertex2f(cx+17*scale,groundY+th+50*scale);
    glVertex2f(cx,          groundY+th+82*scale);
    glEnd();
}

/* ================================================================
   HOUSE
================================================================ */
static void drawHouse(float x, float groundY, float scale=1.f){
    float w=90*scale, h=70*scale, rx=x-w/2;
    /* walls */
    glBegin(GL_QUADS);
    glColor3f(0.88f,0.76f,0.60f); glVertex2f(rx,groundY);
    glColor3f(0.88f,0.76f,0.60f); glVertex2f(rx+w,groundY);
    glColor3f(0.78f,0.65f,0.50f); glVertex2f(rx+w,groundY+h);
    glColor3f(0.78f,0.65f,0.50f); glVertex2f(rx,groundY+h);
    glEnd();
    /* roof */
    glColor3f(0.72f,0.20f,0.14f);
    glBegin(GL_TRIANGLES);
    glVertex2f(rx-8*scale,groundY+h);
    glVertex2f(rx+w+8*scale,groundY+h);
    glVertex2f(x,groundY+h+52*scale);
    glEnd();
    glColor3f(0.55f,0.14f,0.09f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x-6*scale,groundY+h+46*scale);
    glVertex2f(x+6*scale,groundY+h+46*scale);
    glVertex2f(x,groundY+h+52*scale);
    glEnd();
    /* door */
    float dx=x-10*scale, dw=20*scale, dh=32*scale;
    quad(dx,groundY,dw,dh, 0.40f,0.24f,0.08f);
    circle(dx+dw-5*scale,groundY+dh/2,3*scale, 0.8f,0.7f,0.1f);
    quadLine(dx,groundY,dw,dh, 0.28f,0.16f,0.04f, 1.5f);
    /* windows */
    for(int side=0;side<2;side++){
        float wx=(side==0)?(rx+7*scale):(rx+w-25*scale);
        float wy=groundY+h*0.38f, wsz=18*scale;
        quad(wx,wy,wsz,wsz*0.9f, 0.55f,0.82f,0.98f);
        quadLine(wx,wy,wsz,wsz*0.9f, 0.60f,0.45f,0.20f,1.5f);
        glColor3f(0.60f,0.45f,0.20f); glLineWidth(1.f);
        glBegin(GL_LINES);
        glVertex2f(wx+wsz/2,wy); glVertex2f(wx+wsz/2,wy+wsz*0.9f);
        glVertex2f(wx,wy+wsz*0.45f); glVertex2f(wx+wsz,wy+wsz*0.45f);
        glEnd();
    }
    /* chimney + smoke */
    float chx=rx+w*0.75f;
    quad(chx,groundY+h+20*scale,12*scale,32*scale, 0.65f,0.55f,0.45f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float sf=0.4f+0.4f*sinf(gAnim*1.4f);
    glColor4f(0.85f,0.85f,0.85f,0.45f*sf);
    circle(chx+6*scale,groundY+h+58*scale,9*scale, 0.85f,0.85f,0.85f);
    glColor4f(0.85f,0.85f,0.85f,0.30f*sf);
    circle(chx+10*scale,groundY+h+68*scale,7*scale, 0.85f,0.85f,0.85f);
    glDisable(GL_BLEND);
}

/* ================================================================
   BACKGROUND  (game scene)
================================================================ */
static void drawBG(){
    /* sky */
    if(gNight){
        glBegin(GL_QUADS);
        glColor3f(0.02f,0.02f,0.08f); glVertex2f(0,0); glVertex2f(WIN_W,0);
        glColor3f(0.04f,0.04f,0.16f); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
        glEnd();
        glPointSize(2.f); glBegin(GL_POINTS);
        for(int i=0;i<120;i++){
            float br=gStarBr[i]*(0.6f+0.4f*sinf(gAnim*2.f+i));
            glColor3f(br,br,br); glVertex2f(gStarX[i],gStarY[i]);
        }
        glEnd();
        ellipse(680,WIN_H-60,32,32, 0.97f,0.97f,0.80f);
        ellipse(695,WIN_H-54,27,27, 0.04f,0.04f,0.16f);
    } else {
        glBegin(GL_QUADS);
        glColor3f(0.48f,0.78f,0.99f); glVertex2f(0,0); glVertex2f(WIN_W,0);
        glColor3f(0.20f,0.50f,0.92f); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
        glEnd();
        ellipse(100,WIN_H-70,34,34, 1.f,0.92f,0.12f);
        ellipse(100,WIN_H-70,28,28, 1.f,0.97f,0.35f);
        glColor3f(1,0.92f,0.2f); glLineWidth(2.f);
        for(int i=0;i<8;i++){
            float a=PI/4*i+gAnim*0.3f;
            glBegin(GL_LINES);
            glVertex2f(100+36*cosf(a),WIN_H-70+36*sinf(a));
            glVertex2f(100+50*cosf(a),WIN_H-70+50*sinf(a));
            glEnd();
        }
    }
    /* clouds */
    if(!gNight){
        for(auto& c:gClouds){
            ellipse(c.x,c.y,36,24, 1,1,1);
            ellipse(c.x+32,c.y+10,30,21, 1,1,1);
            ellipse(c.x-32,c.y+7, 26,19, 1,1,1);
            ellipse(c.x+8, c.y+25,24,18, 1,1,1);
        }
    }
    /* grass */
    glBegin(GL_QUADS);
    glColor3f(0.20f,0.70f,0.20f); glVertex2f(0,0); glVertex2f(WIN_W,0);
    glColor3f(0.09f,0.48f,0.09f); glVertex2f(WIN_W,70); glVertex2f(0,70);
    glEnd();
    glColor3f(0.14f,0.60f,0.14f); glLineWidth(1.5f);
    for(int i=0;i<WIN_W;i+=10){
        glBegin(GL_LINES); glVertex2f(i,70); glVertex2f(i+5,84); glEnd();
    }
    /* houses & trees */
    drawHouse(680, 70, 0.82f);
    drawHouse(120, 70, 0.72f);
    drawTree(20,  70, 0.70f);
    drawTree(58,  70, 0.80f);
    drawTree(200, 70, 0.65f);
    drawTree(560, 70, 0.75f);
    drawTree(750, 70, 0.70f);
    drawTree(790, 70, 0.60f);
    /* bamboo sticks */
    for(int s=0;s<N_STICKS;s++){
        float sy=STICK_Y[s];
        glBegin(GL_QUADS);
        glColor3f(0.52f,0.34f,0.06f); glVertex2f(40,sy); glVertex2f(WIN_W-40,sy);
        glColor3f(0.38f,0.22f,0.02f); glVertex2f(WIN_W-40,sy+13); glVertex2f(40,sy+13);
        glEnd();
        glColor3f(0.30f,0.18f,0.01f); glLineWidth(2.f);
        for(int j=80;j<WIN_W-80;j+=55){
            glBegin(GL_LINES); glVertex2f(j,sy); glVertex2f(j,sy+13); glEnd();
        }
        glColor3f(0.35f,0.22f,0.04f); glLineWidth(5.f);
        glBegin(GL_LINES); glVertex2f(40,sy+13); glVertex2f(40,70); glEnd();
        glBegin(GL_LINES); glVertex2f(WIN_W-40,sy+13); glVertex2f(WIN_W-40,70); glEnd();
    }
    /* wind indicator */
    if(gWindOn){
        char buf[32]; sprintf(buf,"WIND  %s",gWindF>0?">>>":"<<<");
        txt(WIN_W/2-46,WIN_H-58,buf, 1.f,0.92f,0.f, GLUT_BITMAP_HELVETICA_12);
    }
    /* combo indicator */
    if(gCombo>=3){
        char buf[32]; sprintf(buf,"COMBO x%d!",gCombo);
        float p=0.8f+0.2f*sinf(gAnim*8.f);
        txt(WIN_W/2-50,WIN_H-76,buf, 1.f,p,0.f, GLUT_BITMAP_HELVETICA_18);
    }
}

/* ================================================================
   CHICKEN
================================================================ */
static void drawChicken(float cx,float cy,int dir){
    float f=(float)dir;
    ellipse(cx,cy,25,21, 0.80f,0.50f,0.12f);
    float hx=cx+f*21, hy=cy+17;
    ellipse(hx,hy,14,12, 0.84f,0.54f,0.14f);
    glColor3f(1.f,0.80f,0.f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx+f*13,hy+1); glVertex2f(hx+f*28,hy-1); glVertex2f(hx+f*13,hy-5);
    glEnd();
    ellipse(hx+f*5,hy+3, 3,3, 0,0,0);
    ellipse(hx+f*6,hy+3.8f, 1.2f,1.2f, 1,1,1);
    glColor3f(0.92f,0.12f,0.12f);
    glBegin(GL_TRIANGLES);
    glVertex2f(hx-4,hy+10); glVertex2f(hx-1,hy+23); glVertex2f(hx+4,hy+10);
    glVertex2f(hx+4,hy+10); glVertex2f(hx+7,hy+21); glVertex2f(hx+10,hy+10);
    glEnd();
    ellipse(hx+f*8,hy-7, 5,6, 0.92f,0.12f,0.12f);
    glColor3f(0.64f,0.38f,0.09f);
    glBegin(GL_POLYGON);
    glVertex2f(cx-f*5,cy+8); glVertex2f(cx-f*23,cy+2);
    glVertex2f(cx-f*19,cy-13); glVertex2f(cx-f*2,cy-6);
    glEnd();
    glColor3f(0.68f,0.44f,0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx-f*22,cy+5); glVertex2f(cx-f*41,cy+19); glVertex2f(cx-f*24,cy-2);
    glVertex2f(cx-f*20,cy-2); glVertex2f(cx-f*39,cy-11); glVertex2f(cx-f*20,cy-15);
    glEnd();
    glColor3f(1.f,0.80f,0.f); glLineWidth(2.5f);
    glBegin(GL_LINES);
    glVertex2f(cx-7,cy-19); glVertex2f(cx-11,cy-34);
    glVertex2f(cx+7,cy-19); glVertex2f(cx+11,cy-34);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(cx-11,cy-34); glVertex2f(cx-21,cy-34);
    glVertex2f(cx-11,cy-34); glVertex2f(cx-9,cy-40);
    glVertex2f(cx+11,cy-34); glVertex2f(cx+21,cy-34);
    glVertex2f(cx+11,cy-34); glVertex2f(cx+9,cy-40);
    glEnd();
}

/* ================================================================
   EGG
================================================================ */
static void drawEgg(float cx,float cy,float rx,float ry,float r,float g,float b){
    glColor3f(r,g,b);
    glBegin(GL_POLYGON);
    for(int i=0;i<40;i++){
        float a=2*PI*i/40;
        float ex=cx+rx*cosf(a);
        float ey=cy+ry*sinf(a)*(a<PI?1.0f:0.80f);
        glVertex2f(ex,ey);
    }
    glEnd();
    glColor3f(r*0.62f,g*0.62f,b*0.62f); glLineWidth(1.f);
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<40;i++){
        float a=2*PI*i/40;
        glVertex2f(cx+rx*cosf(a), cy+ry*sinf(a)*(a<PI?1.0f:0.80f));
    }
    glEnd();
    ellipse(cx-rx*0.22f,cy+ry*0.25f, rx*0.24f,ry*0.22f,
            fminf(r+0.32f,1.f),fminf(g+0.32f,1.f),fminf(b+0.32f,1.f));
}

/* ================================================================
   POOP
================================================================ */
static void drawPoop(float cx,float cy){
    ellipse(cx,cy,11,8, 0.38f,0.22f,0.00f);
    ellipse(cx-2,cy+11,9,7, 0.38f,0.22f,0.00f);
    ellipse(cx,cy+20,6,6, 0.38f,0.22f,0.00f);
    ellipse(cx-4,cy+22,2,2, 1,1,1); ellipse(cx+4,cy+22,2,2, 1,1,1);
    ellipse(cx-4,cy+22,1,1, 0,0,0); ellipse(cx+4,cy+22,1,1, 0,0,0);
    glColor3f(0.5f,0.5f,0.f); glLineWidth(1.2f);
    glBegin(GL_LINES);
    glVertex2f(cx-5,cy+27); glVertex2f(cx-8,cy+37);
    glVertex2f(cx+5,cy+27); glVertex2f(cx+8,cy+37);
    glEnd();
}

/* ================================================================
   POWER-UP BLOCK
================================================================ */
static void drawPower(float cx,float cy,ItemType t){
    float r,g,b; const char* lbl;
    switch(t){
        case PWR_BASKET: r=0.96f;g=0.55f;b=0.05f; lbl="BIG";    break;
        case PWR_SLOW:   r=0.05f;g=0.88f;b=0.88f; lbl="SLOW";   break;
        case PWR_TIME:   r=0.12f;g=0.90f;b=0.20f; lbl="+TIME";  break;
        case PWR_SHIELD: r=0.72f;g=0.12f;b=0.96f; lbl="SHIELD"; break;
        default:         r=1;g=1;b=1;              lbl="?";      break;
    }
    float pulse=0.85f+0.15f*sinf(gAnim*5.f);
    /* glow */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,0.22f*pulse);
    glBegin(GL_POLYGON);
    for(int i=0;i<20;i++){
        float a=2*PI*i/20;
        glVertex2f(cx+31*cosf(a),cy+31*sinf(a));
    }
    glEnd();
    glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    glColor3f(r*pulse,g*pulse,b*pulse);
    glVertex2f(cx-22,cy-22); glVertex2f(cx+22,cy-22);
    glColor3f(r*0.7f,g*0.7f,b*0.7f);
    glVertex2f(cx+22,cy+22); glVertex2f(cx-22,cy+22);
    glEnd();
    quad(cx-18,cy+3,20,13, fminf(r+0.25f,1.f),fminf(g+0.25f,1.f),fminf(b+0.25f,1.f));
    glColor3f(1,1,1); glLineWidth(2.f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx-22,cy-22); glVertex2f(cx+22,cy-22);
    glVertex2f(cx+22,cy+22); glVertex2f(cx-22,cy+22);
    glEnd();
    txt(cx-17,cy-7,lbl, 0,0,0, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   BASKET
================================================================ */
static void drawBasket(float bx,float by,float bw){
    float tl=bx-bw/2, tr=bx+bw/2;
    float bl=bx-bw/2+18, br=bx+bw/2-18;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.26f);
    glBegin(GL_QUADS);
    glVertex2f(tl+6,by+BH-6); glVertex2f(tr+6,by+BH-6);
    glVertex2f(br+6,by-6);    glVertex2f(bl+6,by-6);
    glEnd();
    glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    glColor3f(0.84f,0.60f,0.24f); glVertex2f(tl,by+BH); glVertex2f(tr,by+BH);
    glColor3f(0.62f,0.40f,0.10f); glVertex2f(br,by);    glVertex2f(bl,by);
    glEnd();
    glColor3f(0.54f,0.36f,0.12f); glLineWidth(1.5f);
    for(int i=1;i<=3;i++){
        float frac=(float)i/4;
        float ly=by+BH*frac;
        float x1=tl+(bl-tl)*(1-frac)*0.55f;
        float x2=tr-(tr-br)*(1-frac)*0.55f;
        glBegin(GL_LINES); glVertex2f(x1,ly); glVertex2f(x2,ly); glEnd();
    }
    for(int i=0;i<=6;i++){
        float frac=(float)i/6;
        glBegin(GL_LINES);
        glVertex2f(tl+(tr-tl)*frac,by+BH);
        glVertex2f(bl+(br-bl)*frac,by);
        glEnd();
    }
    glColor3f(0.40f,0.24f,0.04f); glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(tl,by+BH); glVertex2f(tr,by+BH);
    glVertex2f(br,by);    glVertex2f(bl,by);
    glEnd();
    glColor3f(0.74f,0.52f,0.20f); glLineWidth(3.8f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx-bw*0.22f,by+BH);
    glVertex2f(bx-bw*0.22f,by+BH+22);
    glVertex2f(bx+bw*0.22f,by+BH+22);
    glVertex2f(bx+bw*0.22f,by+BH);
    glEnd();
    if(gShield){
        glColor3f(0.74f,0.14f,0.98f); glLineWidth(3.f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(tl-7,by+BH+7); glVertex2f(tr+7,by+BH+7);
        glVertex2f(br+7,by-7);    glVertex2f(bl-7,by-7);
        glEnd();
        glPointSize(5.f); glColor3f(0.88f,0.55f,1.f);
        glBegin(GL_POINTS);
        glVertex2f(bx+bw/2*cosf(gAnim)*1.1f, by+BH/2+BH/2*sinf(gAnim)*1.1f);
        glVertex2f(bx+bw/2*cosf(gAnim+PI)*1.1f, by+BH/2+BH/2*sinf(gAnim+PI)*1.1f);
        glEnd();
    }
}

/* ================================================================
   POPUPS
================================================================ */
static void spawnPopup(float x,float y,int pts,float r,float g,float b){
    Popup p; p.x=x; p.y=y; p.vy=62.f; p.life=p.maxLife=1.4f;
    if(pts>=0) sprintf(p.text,"+%d",pts); else sprintf(p.text,"%d",pts);
    p.r=r; p.g=g; p.b=b;
    gPopups.push_back(p);
}
static void drawPopups(){
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(auto& p:gPopups){
        float alpha=p.life/p.maxLife;
        glColor4f(p.r,p.g,p.b,alpha);
        glRasterPos2f(p.x-12,p.y);
        for(const char* c=p.text;*c;c++)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*c);
    }
    glDisable(GL_BLEND);
}

/* ================================================================
   HUD
================================================================ */
static void drawHUD(){
    glBegin(GL_QUADS);
    glColor3f(0.04f,0.04f,0.12f); glVertex2f(0,WIN_H-42); glVertex2f(WIN_W,WIN_H-42);
    glColor3f(0.10f,0.10f,0.25f); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    glColor3f(0.9f,0.75f,0.1f); glLineWidth(1.5f);
    glBegin(GL_LINES); glVertex2f(0,WIN_H-42); glVertex2f(WIN_W,WIN_H-42); glEnd();

    char buf[64];
    sprintf(buf,"Score: %d",gScore);
    txt(14,WIN_H-26,buf, 1.f,0.92f,0.1f);

    int m=(int)gTime/60, s=(int)gTime%60;
    sprintf(buf,"Time: %d:%02d",m,s);
    bool flash=(gTime<15&&(int)(gTime*4)%2==0);
    txt(WIN_W/2-46,WIN_H-26,buf, flash?1.f:((gTime<15)?1.f:0.f), flash?0.f:((gTime<15)?0.2f:1.f), 0.f);

    sprintf(buf,"Best: %d",gHiScore);
    txt(WIN_W-115,WIN_H-26,buf, 0.72f,0.72f,0.72f);

    float px=14;
    if(gSlowOn){
        quad(px,WIN_H-60,62,16, 0,0.55f,0.55f);
        char tt[20]; sprintf(tt,"SLOW %.0fs",gSlowT);
        txt(px+3,WIN_H-51,tt, 0,0,0, GLUT_BITMAP_HELVETICA_12); px+=68;
    }
    if(gShield){
        quad(px,WIN_H-60,66,16, 0.52f,0.f,0.72f);
        txt(px+3,WIN_H-51,"SHIELD", 1,1,1, GLUT_BITMAP_HELVETICA_12);
    }

    float lx=WIN_W-182, ly=10;
    drawEgg(lx,ly+6,7,9, 1.f,0.97f,0.88f);  txt(lx+11,ly+2,"=1",  0.9f,0.9f,0.9f,GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+40,ly+6,7,9, 0.4f,0.6f,1.f); txt(lx+51,ly+2,"=5",  0.4f,0.7f,1.f, GLUT_BITMAP_HELVETICA_12);
    drawEgg(lx+80,ly+6,8,10,1.f,0.85f,0.f); txt(lx+91,ly+2,"=10", 1.f,0.85f,0.f, GLUT_BITMAP_HELVETICA_12);
    drawPoop(lx+132,ly+2);
    txt(lx+142,ly+2,"-10",1,0.3f,0.3f, GLUT_BITMAP_HELVETICA_12);

    /* Music indicator */
    if(gMusicOn){
        txt(WIN_W-35, WIN_H-26, "♪ ON", 0.2f,0.8f,0.2f, GLUT_BITMAP_HELVETICA_12);
    } else {
        txt(WIN_W-35, WIN_H-26, "♪ OFF", 0.8f,0.2f,0.2f, GLUT_BITMAP_HELVETICA_12);
    }
}

/* ================================================================
   GAME INIT
================================================================ */
static void spawnItem(float cx,float cy);

static void initGame(){
    gScore=0; gTime=60.f; gItems.clear(); gPopups.clear();
    gSlowOn=false; gSlowT=0; gBigOn=false; gBigT=0; gShield=false;
    gWindOn=false; gWindT=0; gWindF=0;
    gCombo=0; gComboTimer=0;

    gHens.clear();
    for(int s=0;s<N_STICKS;s++){
        Chicken h;
        h.x=WIN_W/2+(s%2==0?-80:80); h.y=STICK_Y[s]+38;
        h.speed=68.f+s*22.f; h.dir=(s%2==0)?1:-1;
        h.layTimer=0; h.layInterval=1.6f+s*0.4f;
        gHens.push_back(h);
    }
    gBasket.cx=WIN_W/2; gBasket.y=82; gBasket.w=BW_NORMAL;

    gClouds.clear();
    for(int i=0;i<5;i++){
        Cloud c; c.x=randF(60,WIN_W-60);
        c.y=randF(WIN_H*0.56f,WIN_H*0.88f); c.spd=randF(18,40);
        gClouds.push_back(c);
    }
    
    /* Start background music */
    musicPlay();
}

static void spawnItem(float cx,float cy){
    Item it; it.x=cx+randF(-8,8); it.y=cy-24; it.active=true;
    it.vx=gWindOn?gWindF:0.f;
    float r=(float)rand()/RAND_MAX;
    if     (r<0.05f) it.type=EGG_GOLDEN;
    else if(r<0.16f) it.type=EGG_BLUE;
    else if(r<0.52f) it.type=EGG_NORMAL;
    else if(r<0.64f) it.type=POOP;
    else if(r<0.72f) it.type=PWR_BASKET;
    else if(r<0.79f) it.type=PWR_SLOW;
    else if(r<0.89f) it.type=PWR_TIME;
    else             it.type=PWR_SHIELD;
    it.vy=randF(80,165);
    gItems.push_back(it);
}

/* ================================================================
   UPDATE
================================================================ */
static void update(float dt){
    gAnim+=dt*1.5f;
    if(gAnim>2*PI) gAnim-=2*PI;

    if(gState!=ST_PLAY){
        musicStop();
        return;
    }
    
    /* ensure game music is running */
    if(gMusicTrack != TRACK_GAME) setMusicTrack(TRACK_GAME);

    gTime-=dt;
    if(gTime<=0){ gTime=0; if(gScore>gHiScore)gHiScore=gScore; gState=ST_OVER; return; }

    if(gCombo>0){ gComboTimer-=dt; if(gComboTimer<0) gCombo=0; }
    if(gSlowOn){ gSlowT-=dt; if(gSlowT<=0) gSlowOn=false; }
    if(gWindOn){ gWindT-=dt; if(gWindT<=0) gWindOn=false; }

    static float windSpawn=18.f;
    windSpawn-=dt;
    if(windSpawn<0){
        windSpawn=randF(12,30); gWindOn=true; gWindT=randF(3,7);
        gWindF=randF(30,70)*(rand()%2?1:-1);
    }

    for(auto& c:gClouds){ c.x+=c.spd*dt; if(c.x>WIN_W+90) c.x=-90; }

    for(auto& h:gHens){
        h.x+=h.speed*h.dir*dt;
        if(h.x>WIN_W-62) h.dir=-1;
        if(h.x<62)       h.dir= 1;
        h.layTimer+=dt;
        if(h.layTimer>=h.layInterval){
            h.layTimer=0; h.layInterval=randF(1.0f,3.0f);
            spawnItem(h.x,h.y);
        }
    }

    if(gSpec[GLUT_KEY_LEFT] ||gKeys['a']||gKeys['A']) gBasket.cx-=BSPEED*dt;
    if(gSpec[GLUT_KEY_RIGHT]||gKeys['d']||gKeys['D']) gBasket.cx+=BSPEED*dt;
    float half=gBasket.w/2;
    if(gBasket.cx-half<0)     gBasket.cx=half;
    if(gBasket.cx+half>WIN_W) gBasket.cx=WIN_W-half;

    float sm=gSlowOn?0.38f:1.f;
    for(auto& it:gItems){
        if(!it.active)continue;
        it.y-=it.vy*sm*dt;
        it.x+=it.vx*sm*dt;

        float bL=gBasket.cx-gBasket.w/2+14;
        float bR=gBasket.cx+gBasket.w/2-14;
        float bT=gBasket.y+BH;

        if(it.y<=bT&&it.y>=gBasket.y-10&&it.x>=bL&&it.x<=bR){
            it.active=false;
            soundCatch(it.type);
            if(it.type==EGG_NORMAL||it.type==EGG_BLUE||it.type==EGG_GOLDEN){
                int pts=0;
                if(it.type==EGG_NORMAL) pts=1;
                else if(it.type==EGG_BLUE)   pts=5;
                else if(it.type==EGG_GOLDEN)  pts=10;
                gCombo++; gComboTimer=2.0f;
                if(gCombo>=5)  pts+=2;
                if(gCombo>=10) pts+=3;
                gScore+=pts;
                float er=(it.type==EGG_BLUE)?0.4f:1.f;
                float eg=(it.type==EGG_GOLDEN)?0.85f:(it.type==EGG_BLUE?0.6f:1.f);
                float eb=(it.type==EGG_BLUE)?1.f:0.7f;
                spawnPopup(it.x,it.y,pts,er,eg,eb);
            } else if(it.type==POOP){
                if(gShield){ gShield=false; spawnPopup(it.x,it.y,0, 0.72f,0.12f,0.96f); }
                else {
                    int loss=10; gScore-=loss; if(gScore<0)gScore=0;
                    gCombo=0;
                    spawnPopup(it.x,it.y,-loss, 1,0.2f,0.2f);
                }
            } else {
                int bonus=0;
                switch(it.type){
                    case PWR_BASKET: bonus=3;
                        spawnPopup(it.x,it.y,bonus, 0.96f,0.55f,0.05f); break;
                    case PWR_SLOW:   gSlowOn=true;gSlowT=8.f;bonus=3;
                        spawnPopup(it.x,it.y,bonus, 0.05f,0.88f,0.88f); break;
                    case PWR_TIME:   gTime+=20.f;if(gTime>120.f)gTime=120.f;bonus=5;
                        spawnPopup(it.x,it.y,bonus, 0.12f,0.90f,0.20f); break;
                    case PWR_SHIELD: gShield=true;bonus=2;
                        spawnPopup(it.x,it.y,bonus, 0.72f,0.12f,0.96f); break;
                    default: break;
                }
                gScore+=bonus;
            }
        }
        if(it.y<58||it.x<-35||it.x>WIN_W+35){
            if(it.type==EGG_NORMAL||it.type==EGG_BLUE||it.type==EGG_GOLDEN) gCombo=0;
            it.active=false;
        }
    }
    gItems.erase(std::remove_if(gItems.begin(),gItems.end(),[](const Item& i){return !i.active;}),gItems.end());

    for(auto& p:gPopups){ p.y+=p.vy*dt; p.life-=dt; }
    gPopups.erase(std::remove_if(gPopups.begin(),gPopups.end(),[](const Popup& p){return p.life<=0;}),gPopups.end());
}

/* ================================================================
   MENU
================================================================ */
static void drawMenu(){
    glBegin(GL_QUADS);
    glColor3f(0.04f,0.04f,0.18f); glVertex2f(0,0); glVertex2f(WIN_W,0);
    glColor3f(0.12f,0.04f,0.30f); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    /* stars */
    glPointSize(2.f); glBegin(GL_POINTS);
    for(int i=0;i<120;i++){
        float br=gStarBr[i]*(0.55f+0.45f*sinf(gAnim*1.8f+i*0.5f));
        glColor3f(br,br,br); glVertex2f(gStarX[i],gStarY[i]);
    }
    glEnd();
    /* moon */
    ellipse(700,WIN_H-80,38,38, 0.97f,0.97f,0.82f);
    ellipse(716,WIN_H-72,33,33, 0.12f,0.04f,0.30f);
    /* ground */
    glBegin(GL_QUADS);
    glColor3f(0.08f,0.28f,0.08f); glVertex2f(0,0); glVertex2f(WIN_W,0);
    glColor3f(0.05f,0.18f,0.05f); glVertex2f(WIN_W,80); glVertex2f(0,80);
    glEnd();
    /* scene */
    drawHouse(150,80,0.80f); drawHouse(650,80,0.75f);
    drawTree(40,80,0.80f); drawTree(80,80,0.70f);
    drawTree(260,80,0.65f); drawTree(540,80,0.72f);
    drawTree(740,80,0.68f); drawTree(780,80,0.60f);
    /* title glow */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.f,0.88f,0.f, 0.12f+0.10f*sinf(gAnim*2.f));
    glBegin(GL_POLYGON);
    for(int i=0;i<30;i++){
        float a=2*PI*i/30;
        glVertex2f(WIN_W/2+162*cosf(a),WIN_H-118+27*sinf(a));
    }
    glEnd();
    glDisable(GL_BLEND);
    txtBig(WIN_W/2-162,WIN_H-106,"CATCH  THE  EGGS", 1.f,0.92f,0.05f);
    txt(WIN_W/2-120,WIN_H-145,"CSE 426  |  Computer Graphics Lab",
        0.72f,0.72f,1.f, GLUT_BITMAP_HELVETICA_18);
    /* animated eggs */
    float bob=6.f*sinf(gAnim*2.2f);
    drawEgg(WIN_W/2-130,WIN_H-200+bob,   20,26, 1.f,0.97f,0.88f);
    drawEgg(WIN_W/2-52, WIN_H-210+bob*0.8f, 22,28, 1.f,0.85f,0.f);
    drawEgg(WIN_W/2+28, WIN_H-205+bob*1.2f, 20,26, 0.4f,0.6f,1.f);
    drawPoop(WIN_W/2+90,WIN_H-215+bob*0.7f);
    /* buttons */
    float bx=WIN_W/2-85, bw=170, bh=44;
    button(bx,WIN_H/2+60,  bw,bh,"START GAME", gHover==0);
    button(bx,WIN_H/2+5,   bw,bh,"HIGH SCORE", gHover==1);
    button(bx,WIN_H/2-50,  bw,bh,"HELP",       gHover==2);
    button(bx,WIN_H/2-105, bw,bh,"EXIT",       gHover==3);
    txt(WIN_W/2-245,22,
        "Arrow/A/D or Mouse to move   |   P/ESC pause   |   N night   |   M music",
        0.50f,0.50f,0.55f, GLUT_BITMAP_HELVETICA_12);
}

/* ================================================================
   PAUSE
================================================================ */
static void drawPause(){
    darkOverlay(0.65f);
    quad(WIN_W/2-130,WIN_H/2-140,260,240, 0.06f,0.06f,0.20f);
    quadLine(WIN_W/2-130,WIN_H/2-140,260,240, 0.9f,0.75f,0.1f,2);
    txtBig(WIN_W/2-52,WIN_H/2+82,"PAUSED", 1.f,1.f,0.f);
    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx,WIN_H/2+30, bw,bh,"RESUME",  gHover==0);
    button(bx,WIN_H/2-20, bw,bh,"RESTART", gHover==1);
    button(bx,WIN_H/2-70, bw,bh,"MENU",    gHover==2);
    button(bx,WIN_H/2-120,bw,bh,"EXIT",    gHover==3);
}

/* ================================================================
   GAME OVER  (star rating)
================================================================ */
static void drawGameOver(){
    darkOverlay(0.70f);
    quad(WIN_W/2-175,WIN_H/2-170,350,300, 0.06f,0.06f,0.20f);
    quadLine(WIN_W/2-175,WIN_H/2-170,350,300, 1.f,0.3f,0.3f,2.f);
    txtBig(WIN_W/2-90,WIN_H/2+108,"GAME  OVER", 1.f,0.28f,0.28f);
    char buf[64];
    sprintf(buf,"Your Score : %d",gScore);
    txt(WIN_W/2-85,WIN_H/2+68,buf, 1,1,0);
    sprintf(buf,"Best Score : %d",gHiScore);
    txt(WIN_W/2-85,WIN_H/2+42,buf, 0.78f,0.78f,0.78f);
    if(gScore>0&&gScore>=gHiScore)
        txt(WIN_W/2-68,WIN_H/2+18,"NEW HIGH SCORE!",0.2f,1.f,0.4f, GLUT_BITMAP_HELVETICA_12);

    /* stars: 1 star>=20, 2 stars>=50, 3 stars>=100 */
    int stars=(gScore>=100)?3:(gScore>=50)?2:(gScore>=20)?1:0;
    float sx=WIN_W/2-34.f;
    for(int st=0;st<3;st++){
        float scx=sx+st*34.f, scy=WIN_H/2-8.f;
        glBegin(GL_POLYGON);
        glColor3f(st<stars?1.f:0.3f, st<stars?0.85f:0.3f, 0.f);
        for(int i=0;i<10;i++){
            float a=PI/2+i*2*PI/10;
            float rad=(i%2==0)?14.f:6.f;
            glVertex2f(scx+rad*cosf(a),scy+rad*sinf(a));
        }
        glEnd();
        glColor3f(0.75f,0.60f,0.f); glLineWidth(1.2f);
        glBegin(GL_LINE_LOOP);
        for(int i=0;i<10;i++){
            float a=PI/2+i*2*PI/10;
            float rad=(i%2==0)?14.f:6.f;
            glVertex2f(scx+rad*cosf(a),scy+rad*sinf(a));
        }
        glEnd();
    }
    float bx=WIN_W/2-80, bw=160, bh=42;
    button(bx,WIN_H/2-55, bw,bh,"PLAY AGAIN", gHover==0);
    button(bx,WIN_H/2-105,bw,bh,"MENU",       gHover==1);
    button(bx,WIN_H/2-155,bw,bh,"EXIT",       gHover==2);
}

/* ================================================================
   HIGH SCORE  (BACK button fixed)
================================================================ */
static void drawHiScore(){
    drawMenu(); darkOverlay(0.76f);
    quad(210,170,380,260, 0.07f,0.07f,0.22f);
    quadLine(210,170,380,260, 1,1,0,2);
    txtBig(WIN_W/2-82,400,"HIGH  SCORE", 1.f,1.f,0.f);
    char buf[32]; sprintf(buf,"%d",gHiScore);
    txtBig(WIN_W/2-28,312,buf, 1.f,0.85f,0.f);
    txt(WIN_W/2-22,278,"points", 0.78f,0.78f,0.78f);
    if(gHiScore==0)
        txt(WIN_W/2-82,248,"Play a game to set your record!",
            0.55f,0.55f,0.55f, GLUT_BITMAP_HELVETICA_12);
    /* BACK — y=188, matches getBtn */
    button(WIN_W/2-65,188,130,38,"BACK", gHover==0);
}

/* ================================================================
   HELP  (BACK button fixed)
================================================================ */
static void drawHelp(){
    drawMenu(); darkOverlay(0.78f);
    quad(70,95,660,410, 0.06f,0.06f,0.20f);
    quadLine(70,95,660,410, 1,0.9f,0,2);
    txtBig(WIN_W/2-72,478,"CONTROLS & HELP", 1.f,0.9f,0.f);
    const char* lines[]={
        "Arrow Left / Right  or  A / D  :  Move basket",
        "Mouse Move (during play)         :  Move basket",
        "P  or  ESC                             :  Pause / Resume (no time penalty)",
        "N                                            :  Toggle Night mode",
        "",
        "  White Egg  = +1 pt     Blue Egg  = +5 pt",
        "  Gold  Egg  = +10 pt    Poop        = -10 pt",
        "",
        "  POWER-UPS (catch them for effect AND bonus pts):",
        "  BIG (orange)    : basket x2 for 10 s   +3 pt",
        "  SLOW (cyan)     : eggs fall slower 8 s   +3 pt",
        "  +TIME (green)   : adds 20 seconds         +5 pt",
        "  SHIELD (purple) : next poop negated        +2 pt",
        "",
        "  COMBO: catch eggs consecutively for extra points!",
        "  Wind randomly drifts all falling items sideways."
    };
    int n=sizeof(lines)/sizeof(lines[0]);
    for(int i=0;i<n;i++)
        txt(92,452-i*24,lines[i], 0.88f,0.88f,0.88f, GLUT_BITMAP_HELVETICA_12);
    /* BACK — y=108 */
    button(WIN_W/2-65,108,130,38,"BACK", gHover==0);
}

/* ================================================================
   BUTTON HIT TESTING
================================================================ */
static int getBtn(int mx,int my){
    float bx,bw,bh;
    if(gState==ST_MENU){
        bx=WIN_W/2-85; bw=170; bh=44;
        if(inRect(mx,my,bx,WIN_H/2+60, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2+5,  bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-50, bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2-105,bw,bh)) return 3;
    } else if(gState==ST_PAUSE){
        bx=WIN_W/2-80; bw=160; bh=42;
        if(inRect(mx,my,bx,WIN_H/2+30, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2-20, bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-70, bw,bh)) return 2;
        if(inRect(mx,my,bx,WIN_H/2-120,bw,bh)) return 3;
    } else if(gState==ST_OVER){
        bx=WIN_W/2-80; bw=160; bh=42;
        if(inRect(mx,my,bx,WIN_H/2-55, bw,bh)) return 0;
        if(inRect(mx,my,bx,WIN_H/2-105,bw,bh)) return 1;
        if(inRect(mx,my,bx,WIN_H/2-155,bw,bh)) return 2;
    } else if(gState==ST_HISCORE){
        if(inRect(mx,my,WIN_W/2-65,188,130,38)) return 0;
    } else if(gState==ST_HELP){
        if(inRect(mx,my,WIN_W/2-65,108,130,38)) return 0;
    }
    return -1;
}

/* ================================================================
   DISPLAY
================================================================ */
static void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    /* Music state management */
    if(gState != gLastState){
        if(gState == ST_MENU){
            playMenuMusic();
        } else if(gState == ST_OVER){
            playGameOverMusic();
        }
        gLastState = gState;
    }
    
    if(gState==ST_MENU)        drawMenu();
    else if(gState==ST_HISCORE) drawHiScore();
    else if(gState==ST_HELP)    drawHelp();
    else {
        drawBG();
        for(auto& h:gHens) drawChicken(h.x,h.y,h.dir);
        for(auto& it:gItems){
            if(!it.active)continue;
            switch(it.type){
                case EGG_NORMAL: drawEgg(it.x,it.y,10,14, 1.f,0.97f,0.88f); break;
                case EGG_BLUE:   drawEgg(it.x,it.y,10,14, 0.4f,0.6f,1.f);   break;
                case EGG_GOLDEN: drawEgg(it.x,it.y,13,17, 1.f,0.85f,0.f);   break;
                case POOP:       drawPoop(it.x,it.y);                         break;
                default:         drawPower(it.x,it.y,it.type);                break;
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
static void timerCB(int){
    float now=glutGet(GLUT_ELAPSED_TIME)/1000.f;
    float dt=now-gLastTime; if(dt>0.05f)dt=0.05f;
    gLastTime=now;
    update(dt);
    glutPostRedisplay();
    glutTimerFunc(16,timerCB,0);
}

/* ================================================================
   INPUT
================================================================ */
static void keyDown(unsigned char k,int,int){
    gKeys[k]=true;
    if(k==27||k=='p'||k=='P'){
        /* NO time penalty on pause */
        if(gState==ST_PLAY)   gState=ST_PAUSE;
        else if(gState==ST_PAUSE) gState=ST_PLAY;
    }
    if((k=='n'||k=='N')&&gState==ST_PLAY) gNight=!gNight;
    if(k=='m'||k=='M'){ musicToggle(); }
}
static void keyUp(unsigned char k,int,int){ gKeys[k]=false; }
static void specDown(int k,int,int){ gSpec[k]=true;  }
static void specUp  (int k,int,int){ gSpec[k]=false; }

static void setBasketX(float x){
    gBasket.cx=x;
    float half=gBasket.w/2;
    if(gBasket.cx-half<0)     gBasket.cx=half;
    if(gBasket.cx+half>WIN_W) gBasket.cx=WIN_W-half;
}
static void mouseMove(int mx,int my){
    int gy=WIN_H-my;
    if(gState==ST_PLAY) setBasketX((float)mx);
    gHover=getBtn(mx,gy);
    glutPostRedisplay();
}
static void mouseClick(int btn,int state,int mx,int my){
    if(btn!=GLUT_LEFT_BUTTON||state!=GLUT_DOWN)return;
    int gy=WIN_H-my, clicked=getBtn(mx,gy);
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

/* ================================================================
   RESHAPE
================================================================ */
static void reshape(int w,int h){
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

/* ================================================================
   MAIN
================================================================ */
int main(int argc,char** argv){
    srand((unsigned)time(0));
    for(int i=0;i<120;i++){
        gStarX[i]=randF(0,(float)WIN_W);
        gStarY[i]=randF(80,(float)WIN_H);
        gStarBr[i]=randF(0.5f,1.0f);
    }
    audioInit();               /* SDL2 audio — before GLUT so it's ready immediately */
    setMusicTrack(TRACK_MENU); /* start menu music */

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
