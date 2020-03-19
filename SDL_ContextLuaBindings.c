/*
 * Title: SDL_ContextLua.c
 * Autor: @ooichu
 * Description: Lua binding module, part of SDL_Context library
 */

#include <lua53/lua.h>
#include <lua53/lualib.h>
#include <lua53/lauxlib.h>

//
// Lua bindings
//

static inline SDL_Context* getContext(lua_State* L)
{
	lua_getglobal(L, "sdlctx");
	lua_pushlstring(L, "ctx", 3);
	lua_gettable(L, -2);
	return (SDL_Context*)lua_touserdata(L, -1);
}

static inline bool getQuitFlag(lua_State* L)
{
	lua_getglobal(L, "sdlctx");
	lua_pushlstring(L, "__quit__", 8);
	lua_gettable(L, -2);
	return (bool)lua_toboolean(L, -1);
}

//
// sdlctx.gfx module
//

static inline int l_clear(lua_State* L)
{
	SDL_ContextClear(getContext(L), lua_gettop(L) > 0 ? lua_tointeger(L, -1) : 0x000000FF);
	return 0;
}

static inline int l_copybuffer(lua_State* L)
{
	SDL_ContextCopyBuffer(getContext(L));
	return 0;
}

static inline int l_clip(lua_State* L)
{
	if (lua_gettop(L) >= 4)
		SDL_ContextSetClip(getContext(L), lua_tointeger(L, -4), lua_tointeger(L, -3), lua_tointeger(L, -2), lua_tointeger(L, -1));
	else
		fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);

	return 0;
}

static inline int l_getpixel(lua_State* L)
{
	if (lua_gettop(L) == 2)
		lua_pushinteger(L, SDL_ContextGetPixel(getContext(L), lua_tointeger(L, -2), lua_tointeger(L, -1)));
	else
		lua_pushinteger(L, 0);

	return 1;
}

static inline int l_drawpoint(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 2:
			SDL_ContextDrawPoint(getContext(L), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 3:
			SDL_ContextDrawPoint(getContext(L), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
			
	return 0;
}

static inline int l_drawline(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 4:
			SDL_ContextDrawLine(getContext(L), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 5:
			SDL_ContextDrawLine(getContext(L), lua_tonumber(L, -5), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
	return 0;
}

static inline int l_drawrect(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 4:
			SDL_ContextDrawRect(getContext(L), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 5:
			SDL_ContextDrawRect(getContext(L), lua_tonumber(L, -5), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
	return 0;
}

static inline int l_fillrect(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 4:
			SDL_ContextFillRect(getContext(L), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 5:
			SDL_ContextFillRect(getContext(L), lua_tonumber(L, -5), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
	return 0;
}

static inline int l_drawcircle(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 3:
			SDL_ContextDrawCircle(getContext(L), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 4:
			SDL_ContextDrawCircle(getContext(L), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
	return 0;
}

static inline int l_fillcircle(lua_State* L)
{
	switch (lua_gettop(L))
	{
		case 3:
			SDL_ContextFillCircle(getContext(L), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1), 0x000000FF);
			break;
		case 4:
			SDL_ContextFillCircle(getContext(L), lua_tonumber(L, -4), lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
			break;
		default:
			fprintf(stdout, "SDL_Context(%s): Syntax error!\n", __func__);
			break;
	}
	return 0;
}


static inline int l_drawtriangle(lua_State* L)
{
	(void) L;
	return 0;
}

static inline int l_filltriangle(lua_State* L)
{
	(void) L;
	return 0;
}

static inline int luaopen_gfx(lua_State* L)
{
	static const luaL_Reg lib[] = {
		{"clear", l_clear},
		{"copybuffer", l_copybuffer},
		{"clip", l_clip},
		{"getpixel", l_getpixel},
		{"drawpoint", l_drawpoint},
		{"drawline", l_drawline},
		{"drawrect", l_drawrect},
		{"fillrect", l_fillrect},
		{"drawcircle", l_drawcircle},
		{"fillcircle", l_fillcircle},
		{"drawtriangle", l_drawtriangle},
		{"filltriangle", l_filltriangle},
//		{"drawbitmap", l_drawbitmap},
		{NULL, NULL}
	};

	luaL_newlib(L, lib);
	return 1;
}

//
// sdlctx.audio module
//

// TODO: add audio support

//
// sdlctx.sys module
//

static inline int l_quit(lua_State* L)
{
	lua_getglobal(L, "sdlctx");
	lua_pushstring(L, "__quit__");
	lua_pushboolean(L, 1);
	lua_settable(L, -3);
	return 0;
}

// TODO: think what it must be
static inline int luaopen_sys(lua_State* L)
{
	static const luaL_Reg lib[] = {
		{"quit", l_quit},
		{NULL, NULL}
	};
	luaL_newlib(L, lib);
	return 1;
}

//
// sdlctx module
//

// TODO: get real version
static inline int l_getVersion(lua_State* L)
{
	lua_pushstring(L, "0.0.1");
	return 1;
}

static inline int luaopen_sdlctx(lua_State* L)
{
	static const luaL_Reg lib[] = {
		{"getVersion", l_getVersion},
		{NULL, NULL}
	};

	luaL_newlib(L, lib);

	static const struct { const char* name; int (*func)(lua_State* L); } libs[] = {
//		{"audio", luaopen_audio},
//		{"input", luaopen_input},
		{"sys", luaopen_sys},
		{"gfx", luaopen_gfx},
		{NULL, NULL}
	};

	for (int i = 0; libs[i].name; ++i)
	{
		libs[i].func(L);
		lua_setfield(L, -2, libs[i].name);
	}
	
	return 1;
}

