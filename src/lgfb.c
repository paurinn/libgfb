/**
@addtogroup gfb
@{
*/
#include <stdint.h>
#include <stdbool.h>
#define LUA_LIB
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "libgfb.h"
#include "lgfb.h"

//From libgfb.c.
extern gfb_pixelformat_t gfb_pixelformats[MAX_GFB_PIXELFORMAT];

static int LuaGfb_pusherror(lua_State *L, int e) {

	//Known errors.
	switch (e) {
		case GFB_OK:
			lua_pushboolean(L, 1);
			return 1;
		case GFB_ERROR:
			lua_pushnil(L);
			lua_pushstring(L, "error");
			return 2;
		case GFB_EARGUMENT:
			lua_pushnil(L);
			lua_pushstring(L, "argument");
			return 2;
		case GFB_ENOMEM:
			lua_pushnil(L);
			lua_pushstring(L, "nomem");
			return 2;
		case GFB_ENOTSUPPORTED:
			lua_pushnil(L);
			lua_pushstring(L, "support");
			return 2;
		case GFB_EFILEOPEN:
			lua_pushnil(L);
			lua_pushstring(L, "file open");
			return 2;
		case GFB_EFILEREAD:
			lua_pushnil(L);
			lua_pushstring(L, "file read");
			return 2;
		case GFB_EFILEWRITE:
			lua_pushnil(L);
			lua_pushstring(L, "file write");
			return 2;
	};

	//Fall back.
	lua_pushnil(L);
	lua_pushstring(L, "unspecified");
	return 2;
}

static int LuaGfb_surfacecreate(lua_State *L) {
	if (
		   !lua_isnumber(L, 1)
		|| !lua_isnumber(L, 2)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = NULL;
	int rc = gfb_surface_create(&psurface, lua_tonumber(L, 1), lua_tonumber(L, 2), GFB_PIXELFORMAT_ARGB32, GFB_PREALLOCATE, NULL, NULL);

	if (rc != GFB_OK) {
		return LuaGfb_pusherror(L, rc);
	}

	lua_pushlightuserdata(L, psurface);
	return 1;
}

static int LuaGfb_surfacedestroy(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = lua_touserdata(L, 1);
	if (psurface != NULL) {
		gfb_surface_destroy(&psurface);
	}
	return 0;
}

//Calculate distance between two color values.
//The value is Euclidian distance.
//The pixel format of the colors is assumed to be 32 bit ARGB.
static int LuaGfb_colordistance(lua_State *L) {

	if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_color_t color1 = (gfb_color_t)lua_tonumber(L, 1);
	gfb_color_t color2 = (gfb_color_t)lua_tonumber(L, 2);

	gfb_pixelformat_t *pformat = &gfb_pixelformats[GFB_PIXELFORMAT_ARGB32];

	int r1 = (color1 && pformat->rmask) >> pformat->rshift;
	int g1 = (color1 && pformat->gmask) >> pformat->gshift;
	int b1 = (color1 && pformat->bmask) >> pformat->bshift;

	int r2 = (color2 && pformat->rmask) >> pformat->rshift;
	int g2 = (color2 && pformat->gmask) >> pformat->gshift;
	int b2 = (color2 && pformat->bmask) >> pformat->bshift;

	int dist_r = r1 - r2;
	int dist_g = g1 - g2;
	int dist_b = b1 - b2;


	lua_pushnumber(L,
		(dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b)
	);

	return 1;
}

/**
Create a pixel encoded in the given format.
@param red Red intensity.
@param green Green intensity.
@param blue Blue intensity.
@param alpha Alpha value.
@return Returns the encoded pixel.
*/
static int LuaGfb_maprgba(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
		|| !lua_isnumber(L, 3)
		|| !lua_isnumber(L, 4)
		|| !lua_isnumber(L, 5)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	lua_pushinteger(L,
		gfb_maprgba(
			(gfb_surface_t *)lua_touserdata(L, 1),
			(uint8_t)lua_tonumber(L, 2),
			(uint8_t)lua_tonumber(L, 3),
			(uint8_t)lua_tonumber(L, 4),
			(uint8_t)lua_tonumber(L, 5)
		)
	);
	return 1;
}

/**
Set clipping rectangle of a surface.
Pixels are only drawn inside this rectangle.
If prect is NULL the clipping rectangle is set to the full surface dimensions.
@param psurface Pointer to the surface to change.
@param prect Pointer to a rectangle or NULL to use psurface size.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_setcliprect(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
		|| !lua_isnumber(L, 3)
		|| !lua_isnumber(L, 4)
		|| !lua_isnumber(L, 5)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = lua_touserdata(L, 1);

	gfb_rect_t r = {
		.x = lua_tonumber(L, 2),
		.y = lua_tonumber(L, 3),
		.w = lua_tonumber(L, 4),
		.h = lua_tonumber(L, 5)
	};

	return LuaGfb_pusherror(L, gfb_setcliprect(psurface, &r));
}

/**
Set overall alpha value of a surface.
The surface flag GFB_ALPHABLEND is set automatically.
@param alpha The new alpha value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_setalpha(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = lua_touserdata(L, 1);

	return LuaGfb_pusherror(L, gfb_setalpha(psurface, lua_tonumber(L, 2)));
}

/**
Set color key value of a surface.
The surface flag GFB_SRCCOLORKEY is set automatically.
@param colorkey The new color key (see gfb_maprgba()).
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_setcolorkey(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = lua_touserdata(L, 1);

	return LuaGfb_pusherror(L, gfb_setcolorkey(psurface, (gfb_color_t)lua_tonumber(L, 2)));
}


///////////////////////////////////////////////////////////////////////////////////////////////////


/**
Draw a single pixel on surface.
@param psurface Pointer to the surface to draw on.
@param x Left pixel position.
@param y Top pixel position.
@param pixel The encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_putpixel(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
		|| !lua_isnumber(L, 3)
		|| !lua_isnumber(L, 4)
		|| !lua_isnumber(L, 5)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_putpixel(lua_touserdata(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), (gfb_color_t) lua_tonumber(L, 4)));
}

/**
Read pixel from surface.
@param psurface Pointer to the surface to read from.
@param x Left pixel position.
@param y Top pixel position.
@param palpha Pointer where to store the alpha component.
@param pred Pointer where to store the red component.
@param pgreen Pointer where to store the green component.
@param pblue Pointer where to store the blue component.
@return On success returns GFB_OK.
@return On failure returns error code (GFB_Exxx).
*/
static int LuaGfb_getpixel(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1)
		|| !lua_isnumber(L, 2)
		|| !lua_isnumber(L, 3)
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	uint8_t alpha, red, green, blue;
	gfb_color_t color = gfb_getpixel((gfb_surface_t *)lua_touserdata(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), &alpha, &red, &green, &blue);

	lua_pushnumber(L, color);
	lua_pushnumber(L, alpha);
	lua_pushnumber(L, red);
	lua_pushnumber(L, green);
	lua_pushnumber(L, blue);

	return 5;
}

/**
Copy pixels from one surface to another.
Very important that both surfaces are of the same pixel format.

@param pdest Pointer to destination surface
@param pdestrect Pointer to destination rectangle or NULL to use (0, 0).
@param psource Pointer to source surface.
@param psourcerect Pointer to source rectangle or NULL to copy whole surface.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).

gfb.blit(screen, 1, 1, atlas, 1, 1, 10, 10)

*/
static int LuaGfb_blit(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface
		|| !lua_isnumber  (L, 2) //Dest x
		|| !lua_isnumber  (L, 3) //Dest y
		|| !lua_isuserdata(L, 4) //Source surface
		|| !lua_isnumber  (L, 5) //Source x
		|| !lua_isnumber  (L, 6) //Source y
		|| !lua_isnumber  (L, 7) //Width
		|| !lua_isnumber  (L, 8) //Height
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_rect_t destrect = {
		.x = lua_tonumber(L, 2),
		.y = lua_tonumber(L, 3),
		.w = lua_tonumber(L, 7),
		.h = lua_tonumber(L, 8)
	};

	gfb_rect_t sourcerect = {
		.x = lua_tonumber(L, 5),
		.y = lua_tonumber(L, 6),
		.w = lua_tonumber(L, 7),
		.h = lua_tonumber(L, 8)
	};

	return LuaGfb_pusherror(L, gfb_blit((gfb_surface_t *)lua_touserdata(L, 1), &destrect, (gfb_surface_t *)lua_touserdata(L, 4), &sourcerect));
}

/**
Clear whole frame buffer area to black.
@param psurface Pointer to the surface to draw on.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_clear(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_clear((gfb_surface_t *)lua_touserdata(L, 1)));
}

/** Draw a line.
@param psurface Pointer to the surface to draw on.
@param x1 Left pixel position of start point of line.
@param y1 Top pixel position of start point of line.
@param x2 Left pixel position of end point of line.
@param y2 Top pixel position of end point of line.
@param color Encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_line(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface.
		|| !lua_isnumber  (L, 2) //Source x
		|| !lua_isnumber  (L, 3) //Source y
		|| !lua_isnumber  (L, 4) //Dest x
		|| !lua_isnumber  (L, 5) //Dest y
		|| !lua_isnumber  (L, 6) //Color
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_line((gfb_surface_t *)lua_touserdata(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), (gfb_color_t)lua_tonumber(L, 6)));
}

/**
Draw a rectangle.
@param psurface Pointer to the surface to draw on.
@param prect Pointer to the rectangle configuration.
@param color Encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_rectangle(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface.
		|| !lua_isnumber  (L, 2) //Dest x
		|| !lua_isnumber  (L, 3) //Dest y
		|| !lua_isnumber  (L, 4) //Width
		|| !lua_isnumber  (L, 5) //Height
		|| !lua_isnumber  (L, 6) //Color
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_rect_t destrect = {
		.x = lua_tonumber(L, 2),
		.y = lua_tonumber(L, 3),
		.w = lua_tonumber(L, 4),
		.h = lua_tonumber(L, 5)
	};

	return LuaGfb_pusherror(L, gfb_rectangle((gfb_surface_t *)lua_touserdata(L, 1), &destrect, (gfb_color_t)lua_tonumber(L, 6)));
}

/**
Draw a circle.
@param psurface Pointer to the surface to draw on.
@param x Left pixel position of circle center.
@param y Top pixel position of circle center.
@param radius Circle radius.
@param color Encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_circle(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface.
		|| !lua_isnumber  (L, 2) //Dest x
		|| !lua_isnumber  (L, 3) //Dest y
		|| !lua_isnumber  (L, 4) //Radius
		|| !lua_isnumber  (L, 5) //Color
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_circle((gfb_surface_t *)lua_touserdata(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), (gfb_color_t)lua_tonumber(L, 5)));
}

/**
Draw a filled rectangle.
@param psurface Pointer to the surface to draw on.
@param prect Pointer to the rectangle configuration.
@param colorb Encoded pixel value of the fill color.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_filledrectangle(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface
		|| !lua_isnumber  (L, 2) //Dest x
		|| !lua_isnumber  (L, 3) //Dest y
		|| !lua_isnumber  (L, 4) //Width
		|| !lua_isnumber  (L, 5) //Height
		|| !lua_isnumber  (L, 6) //Fill color
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_rect_t r = {
		.x = lua_tonumber(L, 2),
		.y = lua_tonumber(L, 3),
		.w = lua_tonumber(L, 4),
		.h = lua_tonumber(L, 5)
	};

	return LuaGfb_pusherror(L, gfb_filledrectangle((gfb_surface_t *)lua_touserdata(L, 1), &r, (gfb_color_t)lua_tonumber(L, 6)));
}

/**
Flip between primary and secondary frame buffers (double buffering).
@param pdest Pointer to the surface to flip.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_flip(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_flip((gfb_surface_t *)lua_touserdata(L, 1)));
}

/**
Draw a filled circle.
@param psurface Pointer to the surface to draw on.
@param x Left pixel position.
@param y Top pixel position.
@param radius Circle radius.
@param colorf Encoded pixel value of the outline color.
@param colorb Encoded pixel value of the fill color.
*/
static int LuaGfb_filledcircle(lua_State *L) {
	if (
		   !lua_isuserdata(L, 1) //Destination surface
		|| !lua_isnumber  (L, 2) //Dest x
		|| !lua_isnumber  (L, 3) //Dest y
		|| !lua_isnumber  (L, 4) //Radius
		|| !lua_isnumber  (L, 5) //Outline color
		|| !lua_isnumber  (L, 6) //Fill color
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	return LuaGfb_pusherror(L, gfb_filledcircle((gfb_surface_t *)lua_touserdata(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), (gfb_color_t)lua_tonumber(L, 5), (gfb_color_t)lua_tonumber(L, 6)));
}

#if 0
/**
Draw lines between the given polygon points.
@param psurface Pointer to the surface to draw on.
@param ppoints Pointer to an array of gfb_point_t.
@param count Number of items in ppoints array.
@param color Encoded pixel value of the outline color.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_polygon(struct gfb_surface *psurface, gfb_point_t *ppoints, size_t count, gfb_color_t color);

/**
Fill area of mathing color.
@param psurface Pointer to the surface to draw on.
@param x Left pixel position.
@param y Top pixel position.
@param color Encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_floodfill(gfb_surface_t *psurface, int x, int y, gfb_color_t color);
#endif

/**
Load true-type file from memory.
@param pttf Pointer to the true-type file in memory.
@param ttfsize Number of bytes in pttf[].
@return On success, returns an Id to the stored font (0 to MAX_GFB_FONT).
@return On failure, returns a negative error code (GFB_Exxx).
*/
static int LuaGfb_loadfont(lua_State *L) {
	uint8_t *pttf;
	size_t ttfsize;

	//int gfb_font_id gfb_ttf_load_memory(uint8_t *pttf, size_t ttfsize);

	return ;
}

#if 0
/**
Render an array of Unicode glyphs.
This is a front-end for FreeType2.
@param psurface Pointer to the surface to draw on.
@param fontid Id of the font to use, see gfb_ttf_load_memory().
@param ptsize Font point size.
@param x Left pixel position.
@param y Top pixel position.
@param punicode Pointer to an array of Unicode glyphs.
@param count how many characters (not bytes) to print from punicode[].
@param colorf Color of the text.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_textu(gfb_surface_t *psurface, gfb_font_id fontid, uint8_t ptsize, int x, int y, uint16_t *punicode, size_t count, gfb_color_t colorf);
#endif

/**
Render UTF8 encoded NUL terminated string.
This is a front-end for FreeType2.

@param psurface Pointer to the surface to draw on.
@param fontid Id of the font to use, see gfb_ttf_load_memory().
@param ptsize Font point size.
@param x Left pixel position.
@param y Top pixel position.
@param pzutf8 Pointer to NUL terminated UTF8 encoded string.
@param count how many characters (not bytes) to print from pzutf8.
@param colorf Color of the text.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
static int LuaGfb_text(lua_State *L) {
#if 0
	//FIXME, lock surface!
	if (!IsModel(M_APP_SCREEN)) {
		lua_pushnil(L);
		lua_pushstring(L, "access");
		return 2;
	}
#endif

	if (
		   !lua_isuserdata(L, 1) //Destination surface
		|| !lua_isnumber  (L, 2) //Font Id
		|| !lua_isnumber  (L, 3) //Point size
		|| !lua_isnumber  (L, 4) //Dest x
		|| !lua_isnumber  (L, 5) //Dest y
		|| !lua_isstring  (L, 6) //text
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_color_t colorf = 0x000000;
	gfb_color_t colorb = 0xffffff;

	if (lua_isnumber(L, 7)) {
		//Foreground color
		colorf = (gfb_color_t)lua_tonumber(L, 7);

		if (lua_isnumber(L, 8)) {
			//Background color
			colorb = (gfb_color_t)lua_tonumber(L, 8);
		}
	}

	lua_len(L, 6);
	size_t len = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return LuaGfb_pusherror(L, gfb_text((gfb_surface_t *)lua_touserdata(L, 1), (gfb_font_id)lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), (char *)lua_tostring(L, 6), len, colorf, colorb));
}

static int LuaGfb_surfacefrombmp(lua_State *L) {
	if (
		   !lua_isstring(L, 1) //Path to bitmap.
	) {
		return LuaGfb_pusherror(L, GFB_EARGUMENT);
	}

	gfb_surface_t *psurface = NULL;
	int rc = gfb_surface_load_bmp3(&psurface, GFB_PIXELFORMAT_ARGB32, GFB_PREALLOCATE, NULL, NULL, lua_tostring(L, 1));
	if (rc != GFB_OK) {
		return LuaGfb_pusherror(L, rc);
	}

	lua_pushlightuserdata(L, psurface);
	return 1;
}

//int LuaGfb_surfacetobmp(gfb_surface_t *psurface, const char *pzpath) {

//}

luaL_Reg libGfb[] = {
	{ .name = "surfaceCreate",		.func = LuaGfb_surfacecreate },
	{ .name = "surfaceDestroy",		.func = LuaGfb_surfacedestroy },
	{ .name = "surfaceFromBmp",		.func = LuaGfb_surfacefrombmp },
	//{ .name = "surfaceToBmp",		.func = LuaGfb_surfacetobmp },
	{ .name = "colorDistance",      .func = LuaGfb_colordistance },
	{ .name = "mapRGBA",            .func = LuaGfb_maprgba },
	{ .name = "setCliprect",        .func = LuaGfb_setcliprect },
	{ .name = "setAlpha",           .func = LuaGfb_setalpha },
	{ .name = "setColorkey",        .func = LuaGfb_setcolorkey },
	{ .name = "putPixel",           .func = LuaGfb_putpixel },
	{ .name = "getPixel",           .func = LuaGfb_getpixel },
	{ .name = "blit",               .func = LuaGfb_blit },
	{ .name = "clear",              .func = LuaGfb_clear },
	{ .name = "line",               .func = LuaGfb_line },
	{ .name = "rectangle",          .func = LuaGfb_rectangle },
	{ .name = "circle",             .func = LuaGfb_circle },
	{ .name = "filledRectangle",    .func = LuaGfb_filledrectangle },
	{ .name = "flip",               .func = LuaGfb_flip },
	{ .name = "filledCircle",       .func = LuaGfb_filledcircle },
	{ .name = "loadFont",           .func = LuaGfb_loadfont },
	{ .name = "text",               .func = LuaGfb_text },
	//--
	{ .name = NULL, .func = NULL }
};

/**
Open GFB library.
*/
LUALIB_API int luaopen_libgfb (lua_State *L) {
	luaL_newlib(L, libGfb);
	return 1;
}

#if 0
void AppPushLibGfb(lua_State *L) {
	luaL_newlib(L, libGfb);
	lua_setglobal(L, "gfb");

	lua_getglobal(L, "gfb");

	lua_pushlightuserdata(L, pscreen);
	lua_setfield (L, -2, "screen");
	lua_pushlightuserdata(L, patlas);
	lua_setfield (L, -2, "atlas");
	lua_pushnumber(L, fontid1);
	lua_setfield (L, -2, "font1");
	lua_pushnumber(L, fontid2);
	lua_setfield (L, -2, "font2");

	lua_pop(L, 1);
}
#endif

/** @} */

