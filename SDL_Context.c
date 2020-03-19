/*
 * File: SDL_Context.c
 * Autor: @ooichu
 * Description: Source file of SDL_Context library.
 */

/*
 * TODO LIST:
 * 1.  add translation to the bitmap struct
 *     and implement it in draw functions         [ ]
 * 2.  add audio stuff                            [ ]
 * 3.  add Lua support                            [ ]
 * 4.  drop assertions                            [ ]
 * 5.  drop mutexes in SDL_ContextUpdateInput or
 *     make it part of SDL_Context struct         [ ]
 * 6.  add ttf font support                       [ ]
 * 7.  optimize raster algorithms                 [ ]
 * 8.  support other image formats                [ ]
 * 9.  add scale by X and Y axis                  [x]
 * 10. add buffering modes                        [-]
 * 11. make safe exit on panic                    [ ]
 * 12. refactor project!                          [ ]
 * 13. add joystick support                       [ ]
 * 14. fix SDL_ContextBitmapCopy                  [ ]
 * 15. add way to determine bitmap overlapping    [ ]
 */

// TODO: do not PANIC() if asset load failed 
// TODO: add enableSound switch into SDL_Context structure

#include "SDL_Context.h"
#include "dynarr.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#define ALLOCATION_STEP (32)

//
// Utils
//

#define PANIC(msg) \
	do { \
		fprintf(stdout, "SDL_Context(%s): %s\n", __func__, msg); \
		abort(); \
	} while (0)

#define ASSERT(expr) \
	if (!(expr)) PANIC("Assetion failed!")

#define IN_BOUNDS(x, a, b) (((x) >= (a) && (x) <= (b)))
#define NOT_IN_BOUNDS(x, a, b) (((x) < (a) || (x) > (b)))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, a, b) ((x) < (a) ? (a) : (x) > (b) ? (b) : (x))
#define SWAP(a, b) \
	do { \
		register int __tmp = a; \
		a = b; \
		b = __tmp; \
	} while (0)

//
// Default callbacks
//

static bool nopevents(SDL_Context* ctx)
{
	static SDL_Event event;
	(void) ctx;

	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			return false;
	}

	return true;
}

static bool nopupdate(SDL_Context* ctx, float dt)
{
	(void) ctx;
	(void) dt;
	return false;
}

static void noprender(SDL_Context* ctx)
{
	(void) ctx;
}

//
// Internal functions
//

static inline void* xmalloc(size_t bytes)
{
	void* ptr = malloc(bytes);
	if (!ptr)
	{
		PANIC("Cannot allocate memory!");
		return NULL;
	}
#ifdef SDL_CONTEXT_DEBUG
	fprintf(stdout, "SDL_Context(xmalloc): allocated %lu bytes! Block: 0x%p\n", (unsigned long)bytes, ptr);
#endif
	return ptr;
}

static inline void* xcalloc(size_t n, size_t bytes)
{
	void* ptr = calloc(n, bytes);
	if (!ptr)
	{
		PANIC("Cannot allocate memory!");
		return NULL;
	}
#ifdef SDL_CONTEXT_DEBUG
	fprintf(stdout, "SDL_Context(xcalloc): allocated %lu bytes! Block: 0x%p\n", (unsigned long)(bytes * n), ptr);
#endif
	return ptr;
}

static inline void* xrealloc(void* ptr, size_t bytes)
{
	if (!(ptr = realloc(ptr, bytes)))
	{
		PANIC("Cannot allocate memory!");
		return NULL;	
	}
#ifdef SDL_CONTEXT_DEBUG
	fprintf(stdout, "SDL_Context(xrealloc): reallocated %lu bytes! Block: 0x%p\n", (unsigned long)bytes, ptr);
#endif
	return ptr;
}

static inline void xfree(void* ptr)
{
#ifdef SDL_CONTEXT_DEBUG
	if (ptr)
		fprintf(stdout, "SDL_Context(xfree): freed block 0x%p!\n", ptr);
#endif // SDL_CONTEXT_DEBUG
	free(ptr);
}

#ifndef SDL_CONTEXT_NO_AUDIO

static inline void addAudio(SDL_ContextAudio* root, SDL_ContextAudio* new)
{
	if (!root) return;
	while (root->next) root = root->next;
	root->next = new;
}

static inline void addMusic(SDL_ContextAudio* root, SDL_ContextAudio* new)
{
	bool musicFound = false;
	SDL_ContextAudio* rootNext = root->next;
	
	while (rootNext)
	{
		if (rootNext->loop)
		{
			if (rootNext->fade)
				musicFound = true;
			else
			{
				if (musicFound)
					rootNext->length = rootNext->volume = 0;
				rootNext->fade = true;
			}
		}
		
		rootNext = rootNext->next;
	}

	addAudio(root, new);
}

static inline void audioCallback(void* userdata, uint8_t* stream, int len)
{
	SDL_ContextAudio* audio = (SDL_ContextAudio*)userdata, *prev = audio;
	int tempLength = 0;
	
	SDL_memset(stream, 0, len);
	
	audio = audio->next;
	
	while (audio)
	{
		if (audio->length > 0)
		{
			if (audio->loop)
			{
				if (audio->fade)
				{
					if (audio->volume > 0) --audio->volume;
					else audio->length = 0;
				}
				else
				{
					tempLength = 0;
				}
			}
			else
			{
				tempLength = ((uint32_t) len > audio->length) ? audio->length : (uint32_t)len;
			}

			SDL_MixAudioFormat(stream, audio->buffer, SDL_CONTEXT_AUDIO_FORMAT, tempLength, audio->volume);
			audio->buffer += tempLength;
			audio->length -= tempLength;
			prev = audio;
			audio = audio->next;
		}
		else if (audio->loop && !audio->fade)
		{
			audio->buffer = audio->bufferTrue;
			audio->length = audio->lengthTrue;
		}
		else
		{
			prev->next = audio->next;
			
			// TODO: OUCH!
//			if (!audio->loop) --ctx->soundCount;
			
			audio->next = NULL;
			SDL_ContextFreeAudio(audio);

			audio = prev->next;
		}
	}
}

#endif // SDL_CONTEXT_NO_AUDIO

//
// Lua support
//

#ifdef SDL_CONTEXT_LUA

#include "SDL_ContextLuaBindings.c"

//
// Compile Lua script and call sdlctx.load()
//

bool SDL_ContextLoadLua(SDL_Context* restrict ctx, const char filename[restrict static 1])
{
	if (luaL_loadfile(ctx->lua, filename) || lua_pcall(ctx->lua, 0, 0, 0))
	{
		fprintf(stdout, "SDL_Context(%s): Error on load %s!\n", __func__, filename);
		return false;
	}

	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushstring(ctx->lua, "load");
	lua_gettable(ctx->lua, -2);

	if (lua_type(ctx->lua, -1) != LUA_TNIL)
	{
		if (lua_pcall(ctx->lua, 0, 0, 0))
		{
			fprintf(stdout, "SDL_Context(%s): Error in sdlctx.load function!\n", __func__);
			return false;
		}
	}

	return true;
}

//
// Call sdlctx.update(dt)
//

bool SDL_ContextUpdateLua(SDL_Context* ctx, float dt)
{
	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushlstring(ctx->lua, "update", 6);
	lua_gettable(ctx->lua, -2);
	lua_pushnumber(ctx->lua, dt);
	if (lua_pcall(ctx->lua, 1, 0, 0) != LUA_OK)
	{
		fprintf(stdout, "SDL_Context(%s): Error in 'sdlctx.update' function!\n", __func__);
		return false;
	}

	// check close flag
	lua_settop(ctx->lua, 0);
	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushlstring(ctx->lua, "__quit__", 8);
	lua_gettable(ctx->lua, -2);

	return !lua_toboolean(ctx->lua, -1);
}

//
// Call sdlctx.render()
//

void SDL_ContextRenderLua(SDL_Context* ctx)
{
	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushlstring(ctx->lua, "render", 6);
	lua_gettable(ctx->lua, -2);
	if (lua_pcall(ctx->lua, 0, 0, 0) != LUA_OK)
		fprintf(stdout, "SDL_Context(%s): Error in 'sdlctx.render' function!\n", __func__);
}

#endif // SDL_CONTEXT_LUA

//
// Core
//

SDL_Context* SDL_CreateContext(
	const char title[restrict static 1],
	int w_width,
	int w_height,
	unsigned short scaleX,
	unsigned short scaleY,
	SDL_ContextUpdateFunc update,
	SDL_ContextRenderFunc render,
	SDL_ContextEventsFunc events)
{
	SDL_Context* restrict ctx = xcalloc(1, sizeof(SDL_Context));
	ctx->scaleX = scaleX, ctx->scaleY = scaleY;

	//
	// Init SDL stuff
	//

	if (!ctx)
	{
		fprintf(stdout, "SDL_Context(%s): Cannot allocate memory!\n", __func__);	
		goto __sdlctx_err_cleanup__;
	}

	if (!(ctx->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w_width * scaleX, w_height * scaleY, SDL_WINDOW_SHOWN)))
	{
		fprintf(stdout, "SDL_Context(%s): Window init error!\n", __func__);
		goto __sdlctx_err_cleanup__;
	}

#ifndef SDL_CONTEXT_RENDER_SOFTWARE
	if (!(ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED)))
	{
		fprintf(stdout, "SDL_Context(%s): Renderer init error!\n", __func__);
		goto __sdlctx_err_cleanup__;
	}

	SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);

	if (!(ctx->texture = SDL_CreateTexture(
		ctx->renderer,
		SDL_CONTEXT_PIXELFORMAT,
#ifndef SDL_CONTEXT_NO_GRAPHICS
		SDL_TEXTUREACCESS_STREAMING,
#else // SDL_CONTEXT_NO_GRAPHICS
		SDL_TEXTUREACCESS_TARGET,
#endif // SDL_CONTEXT_NO_GRAPHICS
		w_width,
		w_height)))
	{
		fprintf(stdout, "SDL_Context(%s): Texture init error!\n", __func__);
		goto __sdlctx_err_cleanup__;
	}

	// disable alpha blending for texture by default
	SDL_SetTextureBlendMode(ctx->texture, SDL_BLENDMODE_NONE);

#endif // SDL_CONTEXT_RENDER_SOFTWARE

#ifndef SDL_CONTEXT_NO_GRAPHICS

	//
	// Init frame buffer
	//
	
	if (!(ctx->bitmap = SDL_ContextCreateBitmap(w_width, w_height)))
	{
		fprintf(stdout, "SDL_Context(%s): Bitmap creation error!\n", __func__);
	}

#ifdef SDL_CONTEXT_RENDER_SOFTWARE

	//
	// Init software rendering
	//

	ctx->surface = SDL_CreateRGBSurfaceWithFormatFrom(ctx->bitmap->pixels, w_width, w_height, sizeof(uint32_t), sizeof(uint32_t) * w_width, SDL_CONTEXT_PIXELFORMAT);

	// disable blending for surface by default
	SDL_SetSurfaceBlendMode(ctx->surface, SDL_BLENDMODE_NONE);

#endif // SDL_CONTEXT_RENDER_SOFTWARE

#endif // SDL_CONTEXT_NO_GRAPHICS

#ifndef SDL_CONTEXT_NO_AUDIO

	//
	// Init audio
	//

	ctx->soundCount = 0;
	ctx->audioEnabled = false;

	SDL_memset(&(ctx->spec), 0, sizeof(SDL_AudioSpec));
	ctx->spec.freq = SDL_CONTEXT_DEFAULT_FREQ;
	ctx->spec.format = SDL_CONTEXT_AUDIO_FORMAT;
	ctx->spec.channels = SDL_CONTEXT_DEFAULT_NUM_CHANNELS;
	ctx->spec.samples = SDL_CONTEXT_DEFAULT_NUM_SAMPLES;
	ctx->spec.callback = audioCallback;
	ctx->spec.userdata = xcalloc(1, sizeof(SDL_ContextAudio));

	ctx->root = (SDL_ContextAudio*)ctx->spec.userdata;
	ctx->root->buffer = NULL;
	ctx->root->next = NULL;

	if (!(ctx->device = SDL_OpenAudioDevice(NULL, 0, &(ctx->spec), NULL, 0)))
		fprintf(stdout, "SDL_Context(%s): Falied to open audio device!", __func__);
	else
	{
		ctx->audioEnabled = true;
		SDL_ContextResumeAudio(ctx);
	}

#endif // SDL_CONTEXT_NO_AUDIO

	//
	// Bind callbacks
	//

	ctx->update = update ? update : nopupdate;	
	ctx->render = render ? render : noprender;
	ctx->events = events ? events : nopevents;

#ifdef SDL_CONTEXT_LUA
	
	//
	// Init Lua
	//

	if (!(ctx->lua = luaL_newstate()))
	{
		fprintf(stdout, "SDL_Context(%s): Lua init error!\n", __func__);
		goto __sdlctx_err_cleanup__;
	}
	
	// require libs
	luaL_openlibs(ctx->lua);
	luaL_requiref(ctx->lua, "sdlctx", luaopen_sdlctx, 1);
	lua_settop(ctx->lua, 0);

	// TODO: add to Lua register
	// push SDL_Context instance
	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushlstring(ctx->lua, "ctx", 3);
	lua_pushlightuserdata(ctx->lua, ctx);
	lua_settable(ctx->lua, -3);
	lua_settop(ctx->lua, 0);

	// push close flag
	lua_getglobal(ctx->lua, "sdlctx");
	lua_pushlstring(ctx->lua, "__quit__", 4);
	lua_pushboolean(ctx->lua, 0);
	lua_settable(ctx->lua, -3);
	lua_settop(ctx->lua, 0);

#endif // SDL_CONTEXT_LUA

	return ctx;
__sdlctx_err_cleanup__:
	SDL_DestroyContext(ctx);
	return NULL;
}

#if 0 // Old bad loop

void SDL_ContextMainLoop(SDL_Context* const restrict ctx, unsigned short frameCap)
{
	(void) frameCap;
	unsigned timePoint1 = 0, timePoint2 = 0, frameCount = 0;
	float frameTimer = 0, elapsedTime;
	for (;;)
	{
		if (!ctx->events(ctx)) break;
		if (SDL_GetWindowFlags(ctx->window) & SDL_WINDOW_INPUT_FOCUS)
		{
			// timing
			timePoint2 = SDL_GetTicks();
			elapsedTime = (float)(timePoint2 - timePoint1) / 1000.0f;
			timePoint1 = timePoint2;

#ifdef SDL_CONTEXT_NO_GRAPHICS
			SDL_SetRenderTarget(ctx->renderer, ctx->texture);
#endif // SDL_CONTEXT_NO_GRAPHICS

			// updating
			if (!ctx->update(ctx, elapsedTime)) break;
		
			// rendering
			ctx->render(ctx);

			// timing
			frameTimer += elapsedTime;
			++frameCount;
			if (frameTimer >= 1.0f)
			{
				frameTimer -= 1.0f;
#ifdef SDL_CONTEXT_DEBUG
				fprintf(stdout, "FPS: %u\n", frameCount);
#endif // SDL_CONTEXT_DEBUG
				frameCount = 0;
			}
		}
#ifdef SDL_CONTEXT_RENDER_SOFTWARE
		SDL_UpdateWindowSurface(ctx->window);
#else // SDL_CONTEXT_RENDER_SOFTWARE
		SDL_RenderPresent(ctx->renderer);
#endif // SDL_CONTEXT_RENDER_SOFTWARE
	}
}

#else

void SDL_ContextMainLoop(SDL_Context* ctx, unsigned short frameCap)
{
	const float dt = 1.f / (float)(frameCap);
	uint32_t currentTime = SDL_GetTicks(), newTime;
	float accumulator = 0.f;

	for (;;)
	{
		// timing
		newTime = SDL_GetTicks();
		accumulator += (newTime - currentTime) / 1000.f;
		currentTime = newTime;
		
		// updating
		while (accumulator >= dt)
		{
			if (!(ctx->events(ctx) && ctx->update(ctx, dt))) goto __sdlctx_end_loop__;
			accumulator -= dt;
		}

		// rendering
		ctx->render(ctx);

#ifdef SDL_CONTEXT_RENDER_SOFTWARE
		SDL_UpdateWindowSurface(ctx->window);
#else // SDL_CONTEXT_RENDER_SOFTWARE
		SDL_RenderPresent(ctx->renderer);
#endif // SDL_CONTEXT_RENDER_SOFTWARE
		SDL_Delay(1);
	}
__sdlctx_end_loop__:
	return;
}

#endif

void SDL_DestroyContext(SDL_Context* ctx)
{
	if (!ctx) return;

	if (ctx->window) SDL_DestroyWindow(ctx->window);

#ifndef SDL_CONTEXT_RENDER_SOFTWARE
	if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
	if (ctx->texture) SDL_DestroyTexture(ctx->texture);
#else // SDL_CONTEXT_RENDER_SOFTWARE
	if (ctx->surface) SDL_FreeSurface(ctx->surface);
#endif // SDL_CONTEXT_RENDER_SOFTWARE

#ifndef SDL_CONTEXT_NO_GRAPHICS
	if (ctx->bitmap) SDL_ContextDestroyBitmap(ctx->bitmap);
#endif // SDL_CONTEXT_NO_GRAPHICS

#ifndef SDL_CONTEXT_NO_AUDIO

	if (ctx->audioEnabled)
	{
		SDL_ContextPauseAudio(ctx);
		SDL_ContextFreeAudio((SDL_ContextAudio*)(ctx->spec.userdata));
		SDL_CloseAudioDevice(ctx->device);
	}

#endif // SDL_CONTEXT_NO_AUDIO

#ifdef SDL_CONTEXT_LUA
	
	if (ctx->lua) lua_close(ctx->lua);

#endif // SDL_CONTEXT_LUA

	xfree(ctx);
}

void SDL_ContextSwapBuffers(SDL_Context* restrict ctx)
{
#ifndef SDL_CONTEXT_RENDER_SOFTWARE
#ifndef SDL_CONTEXT_NO_GRAPHICS
	SDL_UpdateTexture(ctx->texture, NULL, ctx->bitmap->pixels, ctx->bitmap->pitch);
#else // SDL_CONTEXT_NO_GRAPHICS
	SDL_SetRenderTarget(ctx->renderer, NULL);
#endif // SDL_CONTEXT_NO_GRAPHICS
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
#else // SDL_CONTEXT_RENDER_SOFTWARE
	memcpy(ctx->surface->pixels, ctx->bitmap->pixels, ctx->bitmap->pitch * ctx->bitmap->height);
	register SDL_Surface* restrict wind = SDL_GetWindowSurface(ctx->window);
	if (SDL_MUSTLOCK(wind)) SDL_LockSurface(wind);
	SDL_BlitScaled(ctx->surface, NULL, wind, NULL);
	if (SDL_MUSTLOCK(wind)) SDL_UnlockSurface(wind);
#endif // SDL_CONTEXT_RENDER_SOFTWARE
}

#ifndef SDL_CONTEXT_NO_INPUT

//
// Input caches
//

#ifdef SDL_CONTEXT_GLOBAL_CACHES
#define FUNCTION_TYPE extern inline
#else
static bool
	_sdlctx_keyDown[SDL_NUM_SCANCODES],
	_sdlctx_keyPress[SDL_NUM_SCANCODES],
	_sdlctx_mouseDown[SDL_MOUSE_KEYS],
	_sdlctx_mousePress[SDL_MOUSE_KEYS];
static int 
	_sdlctx_mouseX,
	_sdlctx_mouseY,
	_sdlctx_wheel;
#define FUNCTION_TYPE extern
#endif

//
// Caching input
//

FUNCTION_TYPE void SDL_ContextResetInput(void)
{
	memset(_sdlctx_keyPress, 0, sizeof(_sdlctx_keyPress));
}

void SDL_ContextUpdateInput(const SDL_Event* event)
{
	switch (event->type)
	{
		case SDL_KEYDOWN:
			_sdlctx_keyPress[event->key.keysym.scancode] = !_sdlctx_keyDown[event->key.keysym.scancode];
			_sdlctx_keyDown[event->key.keysym.scancode] = true;
			break;
		case SDL_KEYUP:
			_sdlctx_keyDown[event->key.keysym.scancode] = false;
			break;
		case SDL_MOUSEMOTION:
			_sdlctx_mouseX = event->button.x;
			_sdlctx_mouseY = event->button.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
			_sdlctx_mousePress[event->button.button] = !_sdlctx_mouseDown[event->button.button];
			_sdlctx_mouseDown[event->button.button] = true;
			break;
		case SDL_MOUSEBUTTONUP:
			_sdlctx_mouseDown[event->button.button] = false;
			_sdlctx_mouseDown[event->button.button] = _sdlctx_mousePress[event->button.button] = false;
			break;
		case SDL_MOUSEWHEEL:
			_sdlctx_wheel += event->wheel.y;
			break;
		default:
			break;
	}
}

//
// Get data from cache
//

/* keyboard */

FUNCTION_TYPE bool SDL_ContextKeyIsDown(unsigned key)
{
	return key < SDL_NUM_SCANCODES ? _sdlctx_keyDown[key] : false ;
}

FUNCTION_TYPE bool SDL_ContextKeyIsUp(unsigned key)
{
	return key < SDL_NUM_SCANCODES ?  _sdlctx_keyPress[key] : false;
}

FUNCTION_TYPE bool SDL_ContextKeyIsPress(unsigned key)
{
	return key < SDL_NUM_SCANCODES ? _sdlctx_keyPress[key] : false ;
}

/* mouse */

FUNCTION_TYPE bool SDL_ContextMouseIsPress(unsigned key)
{
	return key < SDL_MOUSE_KEYS ? !_sdlctx_mouseDown[key] : false;
}

FUNCTION_TYPE bool SDL_ContextMouseIsDown(unsigned key)
{
	return key < SDL_MOUSE_KEYS ? !_sdlctx_mouseDown[key] : false;
}

FUNCTION_TYPE bool SDL_ContextMouseIsUp(unsigned key)
{
	return key < SDL_MOUSE_KEYS ? !_sdlctx_mouseDown[key] : false;
}

FUNCTION_TYPE int SDL_ContextGetMouseWheel(void)
{
	return _sdlctx_wheel;
}

FUNCTION_TYPE int SDL_ContextGetMouseX(const SDL_Context* ctx)
{
	return _sdlctx_mouseX / ctx->scaleX;
}

FUNCTION_TYPE int SDL_ContextGetMouseY(const SDL_Context* ctx)
{
	return _sdlctx_mouseY / ctx->scaleY;
}

#endif // SDL_CONTEXT_NO_INPUT

#ifndef SDL_CONTEXT_NO_GRAPHICS

//
// Graphics
//

static inline void blendPixel(const SDL_ContextBitmap* restrict bmp, uint32_t* restrict dest, uint32_t src)
{
	switch (bmp->blendMode)
	{
		case SDL_BLENDMODE_MASK:
			if (src & 0xFF)
				*dest = SDL_ContextColor(
				 	SDL_ContextColorR(src) & SDL_ContextColorR(bmp->mask),
					SDL_ContextColorG(src) & SDL_ContextColorG(bmp->mask),
					SDL_ContextColorB(src) & SDL_ContextColorB(bmp->mask),
					0xFF);
			break;
		case SDL_BLENDMODE_BLEND:
			{
			register const float a = (float)(SDL_ContextColorA(src)) / 255.f * (float)(bmp->mask & 0xFF) / 255.f;
			register const float c = 1.f - a;
			src = SDL_ContextColor(
				(uint8_t)(a * (float)SDL_ContextColorR(src) + c * (float)SDL_ContextColorR(*dest)),
				(uint8_t)(a * (float)SDL_ContextColorG(src) + c * (float)SDL_ContextColorG(*dest)),
				(uint8_t)(a * (float)SDL_ContextColorB(src) + c * (float)SDL_ContextColorB(*dest)),
				(uint8_t)(a * 255.f));
			src = ((src & 0xFF) & (bmp->mask & 0xFF)) | ((src & 0xFF00) & (bmp->mask & 0xFF00)) | ((src & 0xFF0000) & (bmp->mask & 0xFF0000)) | ((src & 0xFF000000) & (bmp->mask & 0xFF000000));
			*dest = src;
			}
			break;
		case SDL_BLENDMODE_NONE:
		default:
			*dest = src;
			break;
	}
}

extern inline SDL_ContextBitmap* SDL_ContextCreateBitmap(int width, int height)
{
	SDL_ContextBitmap* bmp = xmalloc(sizeof(SDL_ContextBitmap));
	bmp->width = width;
	bmp->height = height;
	bmp->clip.x1 = bmp->clip.y1 = 0;
	bmp->clip.x2 = width - 1, bmp->clip.y2 = height - 1;
	bmp->clip.w = width, bmp->clip.h = height;
	bmp->pitch = width * sizeof(uint32_t);
	bmp->pixels = xcalloc(1, sizeof(uint32_t[width][height]));
	bmp->mask = 0xFFFFFFFF;
	bmp->blendMode = SDL_BLENDMODE_NONE;
	bmp->tx = bmp->ty = bmp->shared = 0;
	return bmp;
}

extern inline SDL_ContextBitmap* SDL_ContextCloneBitmap(const SDL_ContextBitmap* bmp)
{
	SDL_ContextBitmap* clone = xmalloc(sizeof(SDL_ContextBitmap));
	clone->width = bmp->width;
	clone->height = bmp->height;
	clone->pitch = bmp->pitch;
	clone->mask = bmp->mask;
	clone->blendMode = bmp->blendMode;
	clone->clip = bmp->clip;
	clone->tx = bmp->tx;
	clone->ty = bmp->ty;
	if ((clone->shared = bmp->shared))
		clone->pixels = bmp->pixels;
	else
	{
		clone->pixels = xmalloc(sizeof(uint32_t[clone->width][clone->height]));
		memcpy(clone->pixels, bmp->pixels, bmp->pitch * bmp->height);
	}
	return clone;
}

extern inline SDL_ContextBitmap* SDL_ContextLoadBitmap(const char path[restrict static 1])
{
	SDL_Surface* restrict tmp = SDL_LoadBMP(path);
	if (!tmp)
	{
		fprintf(stdout, "SDL_Context(%s): Load image failed! File: %s\n", __func__, path);
		return NULL;
	}
	tmp = SDL_ConvertSurfaceFormat(tmp, SDL_CONTEXT_PIXELFORMAT, 0);
	SDL_ContextBitmap* bmp = SDL_ContextCreateBitmap(tmp->clip_rect.w, tmp->clip_rect.h);
	memcpy(bmp->pixels, tmp->pixels, bmp->pitch * bmp->height);
	SDL_FreeSurface(tmp);
	return bmp;
}

extern inline SDL_ContextBitmap* SDL_ContextLoadBitmapWithTransparent(const char path[restrict static 1], uint32_t transparent)
{
	SDL_ContextBitmap* bmp = SDL_ContextLoadBitmap(path);
	if (!bmp) return NULL;
	for (register uint32_t* restrict p = bmp->pixels; p < bmp->pixels + bmp->width * bmp->height; ++p)
		if (*p == transparent) *p = 0;
	return bmp;
}

extern inline SDL_ContextBitmap* SDL_ContextCreateBitmapShared(uint32_t pixels[], int width, int height)
{
	SDL_ContextBitmap* bmp = xmalloc(sizeof(SDL_ContextBitmap));
	bmp->width = width;
	bmp->height = height;
	bmp->clip.x1 = bmp->clip.y1 = 0;
	bmp->clip.x2 = width - 1;
	bmp->clip.y2 = height - 1;
	bmp->clip.w = width;
	bmp->clip.h = height;
	bmp->pitch = width * sizeof(uint32_t);
	bmp->pixels = pixels;
	bmp->tx = bmp->ty = 0;
	bmp->mask = 0xFFFFFFFF;
	bmp->blendMode = SDL_BLENDMODE_NONE;
	bmp->shared = true;
	return bmp;
}

extern inline void SDL_ContextDestroyBitmap(SDL_ContextBitmap* bmp)
{
	if (!bmp) return;
	if (!bmp->shared) xfree(bmp->pixels);
	xfree(bmp);
}

void SDL_ContextBitmapCopy(SDL_ContextBitmap* restrict dest, const SDL_ContextBitmap* restrict src, int x, int y)
{
	register int x2 = x + src->clip.w - 1, y2 = y + src->clip.h - 1;
	if ((x < dest->clip.x1 && x2 < dest->clip.x1) || (x > dest->clip.x2 && x2 > dest->clip.x2) || (y < dest->clip.y1 && y2 < dest->clip.y1) || (y > dest->clip.y2 && y2 > dest->clip.y2))
		return;

	const int xofs = x < 0 ? -x : 0, yofs = y < 0 ? -y : 0;

	x = CLAMP(x, dest->clip.x1, dest->clip.x2);
	y = CLAMP(y, dest->clip.y1, dest->clip.y2);
	x2 = CLAMP(x2, dest->clip.x1, dest->clip.x2);
	y2 = CLAMP(y2, dest->clip.y1, dest->clip.y2);

	for (register int tx, ty = y; ty <= y2; ++ty)
		for (tx = x; tx <= x2; ++tx)
			blendPixel(dest, dest->pixels + tx + ty * dest->width,
			*(src->pixels + (tx - x + xofs + src->clip.x1) + (ty - y + yofs + src->clip.y1) * src->width));
}	

// TODO: refactor!
/* if you read this, sorry >_< */
void SDL_ContextBitmapCopyEx(SDL_ContextBitmap* restrict dest, const SDL_ContextBitmap* restrict src, int x, int y, int sx, int sy, SDL_ContextTransform transform)
{
	if (sx == 0 || sy == 0) return;

	register int
		x2 = x + src->clip.w * (transform & SDL_ROTATE ? sy : sx) - 1,
		y2 = y + src->clip.h * (transform & SDL_ROTATE ? sx : sy) - 1;

	if ((x < dest->clip.x1 && x2 < dest->clip.x1) || (x > dest->clip.x2 && x2 > dest->clip.x2) || (y < dest->clip.y1 && y2 < dest->clip.y1) || (y > dest->clip.y2 && y2 > dest->clip.y2))
		return;

	const int xofs = (x < 0 ? -x : 0), yofs = (y < 0 ? -y : 0);

	x = CLAMP(x, dest->clip.x1, dest->clip.x2);
	y = CLAMP(y, dest->clip.y1, dest->clip.y2);
	x2 = CLAMP(x2, dest->clip.x1, dest->clip.x2);
	y2 = CLAMP(y2, dest->clip.y1, dest->clip.y2);

	register int kx = transform & SDL_FLIP_V ? x2 + x : 0, ky = transform & SDL_FLIP_H ? y2 + y : 0;

	switch (transform & SDL_ROTATE)
	{
		case 0: // without rotation
			for (register int tx, ty = y; ty <= y2; ++ty)
				for (tx = x; tx <= x2; ++tx)
					blendPixel(dest, dest->pixels + (kx ? (kx - tx) : tx) + ((ky ? (ky - ty) : ty)) * dest->width, src->pixels[tx / sx - x + src->clip.x1 + xofs + (ty / sy - y + src->clip.y1 + yofs) * src->width]);
			break;
		default: // with rotation
			ky = ky ? 0 : y2 + y;
			for (register int tx, ty = y; ty <= y2; ++ty)
				for (tx = x; tx <= x2; ++tx)
					blendPixel(dest, dest->pixels + (kx ? (kx - tx) : tx) + ((ky ? (ky - ty) : ty)) * dest->width, src->pixels[ty / sy - y + src->clip.x1 + yofs + (tx / sx - x + src->clip.y1 + xofs) * src->width]);
			break;
	}
}

// TODO: remove this, refactor SDL_ContextBitmapDrawBitmap
static inline int max(int a, int b)
{
	return MAX(a, b);
}

static inline int min(int a, int b)
{
	return MIN(a, b);
}

// TODO: optimize!!!
// TODO: if (sx == 1 and sy == 1 and a == 0) => SDL_ContextBitmapCopy(...)
// TODO: if (sx > 1 and sy > 1 and a == 0) => fast copy scaled bitmap method
void SDL_ContextBitmapDrawBitmap(SDL_ContextBitmap* restrict dest, const SDL_ContextBitmap* restrict src,
	int x, int y, float a, int ox, int oy, float sx, float sy)
{
	const float s = -sinf(a), c = cosf(a);
	const float sx_cos = sx * c, sy_sin = sy * s;

	// left top corner
	const int rx1 = (-ox) * sx_cos + (-oy) * sy_sin;
	const int ry1 = (+ox) * sy_sin + (-oy) * sx_cos;
	
	// right top corner                                 
	const int rx2 = ( src->clip.w - ox) * sx_cos + (-oy) * sy_sin;
	const int ry2 = (-src->clip.w + ox) * sy_sin + (-oy) * sx_cos;

	// left bottom corner	                            
	const int rx3 = (-ox) * sx_cos + (src->clip.h - oy) * sy_sin;
	const int ry3 = (+ox) * sy_sin + (src->clip.h - oy) * sx_cos;

	// right bottom corner                         
	const int rx4 = ( src->clip.w - ox) * sx_cos + (src->clip.h - oy) * sy_sin;
	const int ry4 = (-src->clip.w + ox) * sy_sin + (src->clip.h - oy) * sx_cos;

	// final start (x, y) and end (x, y)
	int strx = min(rx1, min(rx2, min(rx3, rx4))), ex = max(rx1, max(rx2, max(rx3, rx4)));
	register int ty = min(ry1, min(ry2, min(ry3, ry4)));
	int ey = max(ry1, max(ry2, max(ry3, ry4)));

	sx = c / sx, sy = s / sy;

	register float nx, ny;
	for (; ty <= ey; ++ty)
		for (register int tx = strx; tx <= ex; ++tx)
		{
			nx = tx * sx - ty * sy + ox,
			ny = tx * sy + ty * sx + oy;
			if (IN_BOUNDS(nx, 0, src->clip.w - 1) && IN_BOUNDS(ny, 0, src->clip.h - 1))
				SDL_ContextBitmapDrawPoint(dest,
					tx + x, ty + y,
					src->pixels[(int)(nx + src->clip.x1 + 0.5f) + (int)(ny + src->clip.y1 + 0.5f) * src->width]);
		}
}

extern inline void SDL_ContextBitmapClip(SDL_ContextBitmap* bmp, int x, int y, int w, int h)
{
	bmp->clip.x1 = CLAMP(x, 0, bmp->width - 1);
	bmp->clip.y1 = CLAMP(y, 0, bmp->height - 1);
	bmp->clip.x2 = CLAMP(x + w, 0, bmp->width - 1);
	bmp->clip.y2 = CLAMP(y + h, 0, bmp->height - 1);
	bmp->clip.w = bmp->clip.x2 - bmp->clip.x1 + 1;
	bmp->clip.h = bmp->clip.y2 - bmp->clip.y1 + 1;
}

extern inline uint32_t SDL_ContextBitmapGetPixel(const SDL_ContextBitmap* bmp, int x, int y)
{
	return (x < bmp->clip.x1 || y < bmp->clip.y1 || x > bmp->clip.x2 || y > bmp->clip.y2) ? 0 : bmp->pixels[x + y * bmp->width];
}

extern inline void SDL_ContextBitmapClear(SDL_ContextBitmap* restrict bmp, uint32_t val)
{
	register uint32_t* restrict p;
	for (register int y = bmp->clip.y1; y <= bmp->clip.y2; ++y)
		for (p = bmp->pixels + y * bmp->width + bmp->clip.x1; p <= bmp->pixels + y * bmp->width + bmp->clip.x2; ++p)
			*p = val;
}

extern inline void SDL_ContextBitmapDrawPoint(SDL_ContextBitmap* bmp, int x, int y, uint32_t val)
{
	if (x < bmp->clip.x1 || y < bmp->clip.y1 || x > bmp->clip.x2 || y > bmp->clip.y2)
		return;

	blendPixel(bmp, bmp->pixels + x + y * bmp->width, val);
}

// TODO: replace with Brethanham's line
// TODO: add some optimizations (when line is vertical, horizontal, etc.)
extern inline void SDL_ContextBitmapDrawLine(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, uint32_t val)
{
	const int dx = x2 - x1, dy = y2 - y1;
	register int step = ABS(dx) > ABS(dy) ? ABS(dx) : ABS(dy);
	float x_inc = dx / (float)step, y_inc = dy / (float)step;
	register float x = x1, y = y1;

	while (step-- >= 0)
	{
		SDL_ContextBitmapDrawPoint(bmp, round(x), round(y), val);
		x += x_inc, y += y_inc;	
	}
}

extern inline void SDL_ContextBitmapDrawRect(SDL_ContextBitmap* bmp, int x, int y, int w, int h, uint32_t val)
{
	--w; --h;
	SDL_ContextBitmapFillRect(bmp, x + 1, y, w, 1, val);
	SDL_ContextBitmapFillRect(bmp, x, y + h, w, 1, val);
	SDL_ContextBitmapFillRect(bmp, x, y, 1, h, val);
	SDL_ContextBitmapFillRect(bmp, x + w, y + 1, 1, h, val);
}

extern inline void SDL_ContextBitmapFillRect(SDL_ContextBitmap* bmp, int x, int y, int w, int h, uint32_t val)
{
	register int x2 = x + w, y2 = y + h;
	if ((x < bmp->clip.x1 && x2 < bmp->clip.x1) || (x > bmp->clip.x2 && x2 > bmp->clip.x2) || (y < bmp->clip.y1 && y2 < bmp->clip.y1) || (y > bmp->clip.y2 && y2 > bmp->clip.y2))
		return;

	x = CLAMP(x, bmp->clip.x1, bmp->clip.x2);
	x2 = CLAMP(x2, bmp->clip.x1, bmp->clip.x2);
	y = CLAMP(y, bmp->clip.y1, bmp->clip.y2);
	y2 = CLAMP(y2, bmp->clip.y1, bmp->clip.y2);

	for (register int tx, ty; y < y2; ++y)
	{
		ty = bmp->width * y;
		for (tx = x + ty; tx < x2 + ty; ++tx)
			blendPixel(bmp, bmp->pixels + tx, val);
	}
}

// TODO: make circles shape identically?
extern inline void SDL_ContextBitmapDrawCircle(SDL_ContextBitmap* bmp, int xm, int ym, int r, uint32_t val)
{
	register int x = -r, y = 0, err = 2 - 2 * r;
	if (xm + x < bmp->clip.x1 || xm - x > bmp->clip.x2 || ym - x < bmp->clip.y1 || ym - y > bmp->clip.y2)
		return;
	do 
	{
		SDL_ContextBitmapDrawPoint(bmp, xm - x, ym + y, val);
		SDL_ContextBitmapDrawPoint(bmp, xm - y, ym - x, val);
		SDL_ContextBitmapDrawPoint(bmp, xm + x, ym - y, val);
		SDL_ContextBitmapDrawPoint(bmp, xm + y, ym + x, val);

		r = err;
		if (r <= y) err += ++y * 2 + 1;
		if (r > x || err > y) err += ++x * 2 + 1;
	}
	while (x < 0);
}

extern inline void SDL_ContextBitmapFillCircle(SDL_ContextBitmap* bmp, int xm, int ym, int r, uint32_t val)
{
	register int x, y;
	r *= r;
	for (x = xm - r; x <= xm + r; ++x)
		for (y = ym - r; y <= ym + r; ++y)
			if ((pow(x - xm, 2) + pow(y - ym, 2)) <= r)
				SDL_ContextBitmapDrawPoint(bmp, x, y, val);
}

extern inline void SDL_ContextBitmapDrawTriangle(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t val)
{
	SDL_ContextBitmapDrawLine(bmp, x1, y1, x2, y2, val);
	SDL_ContextBitmapDrawLine(bmp, x2, y2, x3, y3, val);
	SDL_ContextBitmapDrawLine(bmp, x3, y3, x1, y1, val);
}

void SDL_ContextBitmapFillTriangle(SDL_ContextBitmap* bmp, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t val)
{
	// sort vertices coordinates
	int tmp;
	if (y1 > y2)
	{
		tmp = y1, y1 = y2, y2 = tmp;
		tmp = x1, x1 = x2, x2 = tmp;
	}
	if (y1 > y3)
	{
		tmp = y1, y1 = y3, y3 = tmp;
		tmp = x1, x1 = x3, x3 = tmp;
	}
	if (y2 > y3)
	{
		tmp = y2, y2 = y3, y3 = tmp;
		tmp = x2, x2 = x3, x3 = tmp;
	}

	// to shape identically with SDL_ContextBitmapDrawTriangle
	SDL_ContextBitmapDrawLine(bmp, x1, y1, x2, y2, val);
	SDL_ContextBitmapDrawLine(bmp, x2, y2, x3, y3, val);
	SDL_ContextBitmapDrawLine(bmp, x3, y3, x1, y1, val);
	
	if (y2 == y3) // draw bottom flat triangle
	{
		register const float invslope1 = (float)(x2 - x1) / (float)(y2 - y1);
		register const float invslope2 = (float)(x3 - x1) / (float)(y3 - y1);
	
		register float curx1 = x1, curx2 = x1;
	
		for (register int y = y1; y <= y2; ++y)
		{
			SDL_ContextBitmapDrawLine(bmp, curx1, y, curx2, y, val);
			curx1 += invslope1;
			curx2 += invslope2;
		}	
	}
	else if (y1 == y2) // draw top flat triangle
	{
		register const float invslope1 = (float)(x3 - x1) / (float)(y3 - y1);
		register const float invslope2 = (float)(x3 - x2) / (float)(y3 - y2);
	
		register float curx1 = x3, curx2 = x3;
	
		for (register int y = y3; y >= y1; --y)
		{
			SDL_ContextBitmapDrawLine(bmp, curx1, y, curx2, y, val);
			curx1 -= invslope1, curx2 -= invslope2;
		}	
	}
	else // draw arbitary triangle
	{
		const int x4 = (int)(x1 + ((float)(y2 - y1) / (float)(y3 - y1)) * (x3 - x1));
		register float invslope1, invslope2, curx1, curx2;

		invslope1 = (float)(x2 - x1) / (float)(y2 - y1);
		invslope2 = (float)(x4 - x1) / (float)(y2 - y1);
	
		curx1 = curx2 = x1;
	
		for (register int y = y1; y <= y2; ++y)
		{
			SDL_ContextBitmapDrawLine(bmp, curx1, y, curx2, y, val);
			curx1 += invslope1, curx2 += invslope2;
		}
		
		invslope1 = (float)(x3 - x2) / (float)(y3 - y2);
		invslope2 = (float)(x3 - x4) / (float)(y3 - y2);
	
		curx1 = curx2 = x3;
	
		for (register int y = y3; y > y2; --y)
		{
			SDL_ContextBitmapDrawLine(bmp, curx1, y, curx2, y, val);
			curx1 -= invslope1, curx2 -= invslope2;
		}	
	}
}

#endif // SDL_CONTEXT_NO_GRAPHICS

#ifndef SDL_CONTEXT_NO_AUDIO

//
// Audio
//

SDL_ContextAudio* SDL_ContextCreateAudio(const char* file, bool loop, int volume)
{
	if (!file)
	{
#ifdef SDL_CONTEXT_DEBUG
		fprintf(stdout, "SDL_Context(%s): File \"%s\" not found!\n", __func__, file);
#endif // SDL_CONTEXT_DEBUG
		return NULL;
	}
	
	SDL_ContextAudio* const audio = xmalloc(sizeof(SDL_ContextAudio));
	
	audio->next = NULL;
	audio->loop = loop;
	audio->fade = 0;
	audio->free = true;
	audio->volume = volume;

	if (SDL_LoadWAV(file, &(audio->spec), &(audio->bufferTrue), &(audio->lengthTrue)) == NULL)
	{
		fprintf(stdout, "SDL_Context(%s): Failed to open .WAV file: %s! Error: %s!\n", __func__, file, SDL_GetError());
		xfree(audio);
		return NULL;
	}
	
	audio->buffer = audio->bufferTrue;
	audio->length = audio->lengthTrue;
	audio->spec.callback = NULL;
	audio->spec.userdata = NULL;

	return audio;
}

void SDL_ContextFreeAudio(SDL_ContextAudio* audio)
{
	if (!audio) return;

	SDL_ContextAudio* tmp;

	while (audio)
	{
		if (audio->free)
			SDL_FreeWAV(audio->bufferTrue);

		tmp = audio;
		audio = audio->next;

		xfree(tmp);
	}	
}

// void SDL_ContextPlaySound(const char* file, int volume);
// void SDL_ContextPlayMusic(const char* file, int volume);
void SDL_ContextPlaySound(SDL_Context* const ctx, SDL_ContextAudio* audio, int volume)
{
	if (!ctx->audioEnabled) return;

	++ctx->soundCount;

	SDL_ContextAudio* new = xmalloc(sizeof(SDL_ContextAudio));
	memcpy(new, audio, sizeof(SDL_ContextAudio));
	new->volume = volume;
	new->loop = false;
	new->free = false;

	SDL_LockAudioDevice(ctx->device);
	
	// TODO: create addAudio
	addAudio(ctx->spec.userdata, new);
	
	SDL_UnlockAudioDevice(ctx->device);
}

void SDL_ContextPlayMusic(SDL_Context* const ctx, SDL_ContextAudio* audio, int volume)
{
	if (!ctx->audioEnabled) return;
	
	SDL_ContextAudio* new = xmalloc(sizeof(SDL_ContextAudio));
	memcpy(new, audio, sizeof(SDL_ContextAudio));
	new->volume = volume;
	new->loop = false;
	new->free = false;

	SDL_LockAudioDevice(ctx->device);
	
	// TODO: create addMusic
	addMusic(ctx->spec.userdata, new);
	
	SDL_UnlockAudioDevice(ctx->device);
}

//
// Bindings for SDL_Context
//

extern inline void SDL_ContextPauseAudio(SDL_Context* const ctx)
{
	if (ctx->audioEnabled) SDL_PauseAudioDevice(ctx->device, 1);
}

extern inline void SDL_ContextResumeAudio(SDL_Context* const ctx)
{
	if (ctx->audioEnabled) SDL_PauseAudioDevice(ctx->device, 0);
}

#endif // SDL_CONTEXT_NO_AUDIO

