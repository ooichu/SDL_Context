/* 
 * File: SDL_Context.h
 * Autor: @ooichu
 * Description: Header file of SDL_Context library.
 * Useful defines:
 *  - SDL_CONTEXT_DEBUG - print debug information and unlock some assertions.
 *  - SDL_CONTEXT_NO_GRAPHICS - disable context graphics features.
 *  - SDL_CONTEXT_RENDER_SOFTWARE - set software render mode.
 *  - SDL_CONTEXT_NO_INPUT - disable context input implementation.
 *  - SDL_CONTEXT_GLOBAL_CACHES - make input caches global. If SDL_CONTEXT_NO_INPUT not defined does nothing.
 *  - SDL_CONTEXT_NO_AUDIO - disable context audio implementation.
 *  - SDL_CONTEXT_LUA - plug lua.
 */

#ifndef __SDL_CONTEXT_H__
#define __SDL_CONTEXT_H__

#define SDL_CONTEXT_DEBUG
#define SDL_CONTEXT_NO_AUDIO
// #define SDL_CONTEXT_RENDER_SOFTWARE

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

#if defined(_MSC_VER)
#define inline __inline
#endif

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef SDL_CONTEXT_LUA
#include <lua53/lua.h>
#endif

#ifndef SDL_CONTEXT_NO_AUDIO

//
// Audio definess
//

#define SDL_CONTEXT_DEFAULT_FREQ (44100)
#define SDL_CONTEXT_AUDIO_FORMAT AUDIO_S16
#define SDL_CONTEXT_DEFAULT_NUM_CHANNELS (2)
#define SDL_CONTEXT_DEFAULT_NUM_SAMPLES (4096)
// #define SDL_CONTEXT_MAX_SOUNDS 24

typedef struct SDL_ContextAudio
{
	uint32_t length, lengthTrue;
	uint8_t* buffer, *bufferTrue;
	int volume;
	bool loop, fade, free;
	SDL_AudioSpec spec;
	struct SDL_ContextAudio* next;
}
SDL_ContextAudio;

#endif // SDL_CONTEXT_NO_AUDIO

#ifndef SDL_CONTEXT_NO_GRAPHICS

//
// Graphics defines
//

#define SDL_BLENDMODE_MASK 4

typedef struct SDL_ContextBitmap
{
	uint32_t* pixels;
	struct { int x1, y1, x2, y2, w, h; } clip;
	int width, height, pitch;
	float tx, ty; // translation
	uint32_t mask;
	uint8_t blendMode;
	bool shared;
}
SDL_ContextBitmap;

#endif // SDL_CONTEXT_NO_GRAPHICS

//
// Core
//

typedef struct SDL_Context
{
	// SDL stuff
	SDL_Window* window;
#ifndef SDL_CONTEXT_RENDER_SOFTWARE
	SDL_Renderer* renderer;
	SDL_Texture* texture;
#else // SDL_CONTEXT_RENDER_SOFTWARE
	SDL_Surface* surface;
#endif // SDL_CONTEXT_RENDER_SOFTWARE
	// User callbacks
	bool (*update)(struct SDL_Context* ctx, float dt);
	void (*render)(struct SDL_Context* ctx);
	bool (*events)(struct SDL_Context* ctx);
#ifndef SDL_CONTEXT_NO_GRAPHICS
	// Frame buffer
	SDL_ContextBitmap* bitmap;
#endif // SDL_CONTEXT_NO_GRAPHICS
	unsigned short scaleX, scaleY;
#ifndef SDL_CONTEXT_NO_AUDIO
	SDL_ContextAudio* root;
	SDL_AudioDeviceID device;
	SDL_AudioSpec spec;
	uint32_t soundCount;
	bool audioEnabled;
#endif // SDL_CONTEXT_NO_AUDIO
#ifdef SDL_CONTEXT_LUA
	lua_State* lua;
#endif // SDL_CONTEXT_LUA
}
SDL_Context;

#define SDL_CONTEXT_PIXELFORMAT SDL_PIXELFORMAT_RGBA8888
#define SDL_CONTEXT_DEFAULT_FPS_CAP (60)

// getters
#ifndef SDL_CONTEXT_NO_GRAPHICS
#define SDL_ContextGetFramebuffer(ctx) ((SDL_ContextBitmap* const)(ctx)->bitmap)
#define SDL_ContextGetWidth(ctx) ((const int)SDL_ContextGetCurrentBuffer(ctx)->width)
#define SDL_ContextGetHeight(ctx) ((const int)SDL_ContextGetCurrentBuffer(ctx)->height)
#else // SDL_CONTEXT_NO_GRAPHICS
#define SDL_ContextGetWidth(ctx) ((const int)(ctx)->surface->clip_rect.w)
#define SDL_ContextGetHeight(ctx) ((const int)(ctx)->surface->clip_rect.h)
#endif // SDL_CONTEXT_NO_GRAPHICS

#define SDL_ContextGetScaleX(ctx) ((const unsigned short)ctx->scaleX)
#define SDL_ContextGetScaleY(ctx) ((const unsigned short)ctx->scaleY)

// some utils
#define SDL_ContextColor(r, g, b, a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))
#define SDL_ContextColorR(c) (((c) >> 24) & 0xFF)
#define SDL_ContextColorG(c) (((c) >> 16) & 0xFF)
#define SDL_ContextColorB(c) (((c) >> 8) & 0xFF)
#define SDL_ContextColorA(c) ((c) & 0xFF)

typedef bool (*SDL_ContextEventsFunc)(SDL_Context* ctx);
typedef bool (*SDL_ContextUpdateFunc)(SDL_Context* ctx, float dt);
typedef void (*SDL_ContextRenderFunc)(SDL_Context* ctx);

SDL_Context* SDL_CreateContext(
	const char* title,
	int w_width,
	int w_height,
	unsigned short scaleX,
	unsigned short scaleY,
	SDL_ContextUpdateFunc update,
	SDL_ContextRenderFunc render,
	SDL_ContextEventsFunc events);
void SDL_ContextMainLoop(SDL_Context* ctx, unsigned short frameCap);
void SDL_ContextMainLoopFixed(SDL_Context* ctx, unsigned short frameCap);
void SDL_ContextMainLoopFixe(SDL_Context* ctx, unsigned short frameCap);
void SDL_DestroyContext(SDL_Context* ctx);
void SDL_ContextSwapBuffers(SDL_Context* ctx);
#define SDL_ContextCopyBuffer(ctx) SDL_ContextSwapBuffers(ctx)

#ifdef SDL_CONTEXT_LUA

//
// Lua defines
//

bool SDL_ContextLoadLua(SDL_Context* restrict ctx, const char* filename);
bool SDL_ContextUpdateLua(SDL_Context* ctx, float dt);
void SDL_ContextRenderLua(SDL_Context* ctx);

#endif // SDL_CONTEXT_LUA

#ifndef SDL_CONTEXT_NO_INPUT

#define SDL_MOUSE_KEYS (5)

//
// Input caches
//

#ifdef SDL_CONTEXT_GLOBAL_CACHES

static bool
	_sdlctx_keyDown[SDL_NUM_SCANCODES],
	_sdlctx_keyPress[SDL_NUM_SCANCODES],
	_sdlctx_mouseDown[SDL_MOUSE_KEYS],
	_sdlctx_mousePress[SDL_MOUSE_KEYS];
static int
	_sdlctx_mouseX,
	_sdlctx_mouseY,
	_sdlctx_wheel;

#endif // SDL_CONTEXT_GLOBAL_CACHES

//
// Caching input
//

void SDL_ContextResetInput(void);
void SDL_ContextUpdateInput(const SDL_Event* event);

//
// Get input caches
//

// keyboard
bool SDL_ContextKeyIsDown(unsigned key);
bool SDL_ContextKeyIsUp(unsigned key);
bool SDL_ContextKeyIsPress(unsigned key);

// mouse
bool SDL_ContextMouseIsPress(unsigned key);
bool SDL_ContextMouseIsDown(unsigned key);
bool SDL_ContextMouseIsUp(unsigned key);
int SDL_ContextGetMouseWheel(void);
int SDL_ContextGetMouseX(const SDL_Context* ctx);
int SDL_ContextGetMouseY(const SDL_Context* ctx);

#endif // SDL_CONTEXT_NO_INPUT

#ifndef SDL_CONTEXT_NO_GRAPHICS

//
// Graphics
//

typedef enum SDL_ContextTransform
{
	SDL_TRANSFORM_NONE = 0,
	SDL_FLIP_V = 0x01,
	SDL_FLIP_H = 0x02,
	SDL_ROTATE = 0x04,
	SDL_ROTATE_90 = SDL_ROTATE,
	SDL_ROTATE_180 = SDL_FLIP_H | SDL_FLIP_V,
	SDL_ROTATE_270 = SDL_ROTATE_90 | SDL_ROTATE_180,
	SDL_ROTATE_360 = SDL_TRANSFORM_NONE
}
SDL_ContextTransform;

SDL_ContextBitmap* SDL_ContextCreateBitmap(int width, int height);
SDL_ContextBitmap* SDL_ContextCreateBitmapShared(uint32_t pixels[], int width, int height);
SDL_ContextBitmap* SDL_ContextCloneBitmap(const SDL_ContextBitmap* bmp);
SDL_ContextBitmap* SDL_ContextLoadBitmap(const char* path);
SDL_ContextBitmap* SDL_ContextLoadBitmapWithTransparent(const char* path, uint32_t transparent);
void SDL_ContextDestroyBitmap(SDL_ContextBitmap* bmp);
void SDL_ContextBitmapCopy(SDL_ContextBitmap* dest, const SDL_ContextBitmap* src, int x, int y);
void SDL_ContextBitmapCopyEx(SDL_ContextBitmap* dest, const SDL_ContextBitmap* src, int x, int y, int sx, int sy, SDL_ContextTransform transform);
void SDL_ContextBitmapDrawBitmap(SDL_ContextBitmap* restrict dest, const SDL_ContextBitmap* restrict src, int x, int y, float a, int ox, int oy, float sclx, float scly);
#define SDL_ContextBitmapTranslate(bmp, tx, ty) do { (bmp)->tx = (tx), (bmp)->ty = (ty); } while (0)
#define SDL_ContextBitmapTranslateX(bmp, tx) do { (bmp)->tx = (tx); } while (0)
#define SDL_ContextBitmapTranslateY(bmp, ty) do { (bmp)->ty = (ty); } while (0)
#define SDL_ContextBitmapSetAlpha(bmp, a) do { (bmp)->alpha = (a); } while (0)
#define SDL_ContextBitmapSetBlend(bmp, mode) do { (bmp)->blendMode = (mode); } while (0)
#define SDL_ContextBitmapSetMask(bmp, m) do { (bmp)->mask = (m); } while (0)
void SDL_ContextBitmapClip(SDL_ContextBitmap* bmp, int x, int y, int w, int h);
uint32_t SDL_ContextBitmapGetPixel(const SDL_ContextBitmap* bmp, int x, int y);
void SDL_ContextBitmapClear(SDL_ContextBitmap* restrict bmp, uint32_t val);
void SDL_ContextBitmapDrawPoint(SDL_ContextBitmap* bmp, int x, int y, uint32_t val);
void SDL_ContextBitmapDrawLine(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, uint32_t val);
void SDL_ContextBitmapDrawRect(SDL_ContextBitmap* bmp, int x, int y, int w, int h, uint32_t val);
void SDL_ContextBitmapFillRect(SDL_ContextBitmap* bmp, int x, int y, int w, int h, uint32_t val);
void SDL_ContextBitmapDrawCircle(SDL_ContextBitmap* bmp, int x, int y, int r, uint32_t val);
void SDL_ContextBitmapFillCircle(SDL_ContextBitmap* bmp, int x, int y, int r, uint32_t val);
void SDL_ContextBitmapDrawTriangle(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t val);
void SDL_ContextBitmapFillTriangle(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t val);

//
// Bindings for SDL_Context
//

#define SDL_ContextClear(ctx, val) SDL_ContextBitmapClear((ctx)->bitmap, val)
#define SDL_ContextTranslate(ctx, tx, ty) SDL_ContextBitmapTranslate((ctx)->bitmap, (tx), (ty))
#define SDL_ContextTranslateX(ctx, tx) SDL_ContextBitmapTranslateX((ctx)->bitmap, (tx))
#define SDL_ContextTranslateY(ctx, ty) SDL_ContextBitmapTranslateY((ctx)->bitmap, (ty))
#define SDL_ContextSetAlpha(ctx, a) SDL_ContextBitmapSetAlpha((ctx)->bitmap, (a))
#define SDL_ContextSetBlend(ctx, mode) SDL_ContextBitmapSetBlend((ctx)->bitmap, (mode))
#define SDL_ContextSetMask(ctx, m) SDL_ContextBitmapSetMask((ctx)->bitmap, (m))
#define SDL_ContextClip(ctx, ...) SDL_ContextBitmapClip((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextGetPixel(ctx, ...) SDL_ContextBitmapGetPixel((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawPoint(ctx, ...) SDL_ContextBitmapDrawPoint((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawLine(ctx, ...) SDL_ContextBitmapDrawLine((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawRect(ctx, ...) SDL_ContextBitmapDrawRect((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextFillRect(ctx, ...) SDL_ContextBitmapFillRect((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawCircle(ctx, ...) SDL_ContextBitmapDrawCircle((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextFillCircle(ctx, ...) SDL_ContextBitmapFillCircle((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawTriangle(ctx, ...) SDL_ContextBitmapDrawTriangle((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextFillTriangle(ctx, ...) SDL_ContextBitmapFillTriangle((ctx)->bitmap, __VA_ARGS__)
#define SDL_ContextDrawBitmap(ctx, ...) SDL_ContextBitmapDrawBitmap(SDL_ContextGetCurrentBuffer(ctx), __VA_ARGS__)

#endif // SDL_CONTEXT_NO_GRAPHICS

#ifndef SDL_CONTEXT_NO_AUDIO

//
// Audio
//

// Note: don't working correctly. Need to fix it!

SDL_ContextAudio* SDL_ContextCreateAudio(const char* file, bool loop, int volume);
void SDL_ContextFreeAudio(SDL_ContextAudio* audio);

//
// Bindings for SDL_Context
//

void SDL_ContextPlaySound(SDL_Context* const ctx, SDL_ContextAudio* audio, int volume);
void SDL_ContextPlayMusic(SDL_Context* const ctx, SDL_ContextAudio* audio, int volume);
void SDL_ContextResumeAudio(SDL_Context* const ctx);
void SDL_ContextPauseAudio(SDL_Context* const ctx);

#endif // SDL_CONTEXT_NO_AUDIO

#ifdef __cplusplus
}
#endif

#endif // __SDL_CONTEXT_H__

