/*
libgfb - Library of Graphic Routines for Frame Buffers.
Copyright (C) 2016  Kari Sigurjonsson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
Graphical Frame Buffer

@defgroup

Example code:
@code

//Fixed pointer to some hardware frame buffer (example)
#define SCREEN_FRAME_PTR	((uint8_t *)(0x87700000))

//Create screen surface.
gfb_surface_t *pscreen = NULL;
assert(gfb_surface_create(&screen, 640, 480, GFB_PIXELFORMAT_ARGB32, GFB_DOUBLEBUFFER, SCREEN_FRAME_PTR, NULL) == GFB_OK);

//Draw circle on screen.
gfb_color_t color = gfb_maprgba(pscreen, 0x00, 0xff, 0x99, 0x99);
gfb_circle(pscreen, 100, 100, 50, color);

//Make changes visible.
gfb_flip(pscreen);

//Destroy surface after use to free allocated memory.
gfb_surface_destroy(&pscreen);

@endcode
*/
#ifndef __LIBGFB_H__
#define __LIBGFB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/** Round n to next multiple of 4. */
#define gfb_round4(n) ((n + 3) & ~(3))

#define gfb_inside(x,a,b) (x >= a && x <= b)
#define gfb_outside(x,a,b) (x < a || x > b)

/**
Returns the larger integer of two.
@param a First integer.
@param b Second integer.
@return Returns the integer that has the larger value.
*/
static inline int gfb_maxi(int a, int b) {return a>b ? a:b;}

/**
Returns the smaller integer of two.
@param a First integer.
@param b Second integer.
@return Returns the integer that has the lesser value.
*/
static inline int gfb_mini(int a, int b) {return a<b ? a:b;}

/** Ensures that x is between the limits set by low and high. If low is greater than high the result is undefined. */
#define gfb_clampi(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/** Maximum number of font faces that can be loaded on run time. */
#define MAX_GFB_FONT	10

/** Number of glyph cache elements. */
#define MAX_GFB_GLYPH	256

/** Identifier for a loaded true-type font. */
typedef int gfb_font_id;

/** Enough for 32 and 16 bits per pixel. */
typedef uint32_t gfb_color_t;

/** Enumerated pixel formats. */
typedef enum gfb_pixelformat_id {
                            //R.G.B.A.X
//  ---------------------------------------
    GFB_PIXELFORMAT_RGB16,	//5.5.5.0.1
    GFB_PIXELFORMAT_RGB24,	//8.8.8.0.0
    GFB_PIXELFORMAT_RGB32,	//8.8.8.0.8
    GFB_PIXELFORMAT_ARGB32,	//8.8.8.8.0
	GFB_PIXELFORMAT_ALPHA,  //0.0.0.8.0
    //--
    MAX_GFB_PIXELFORMAT,
} gfb_pixelformat_id_t;

/** Pixel format configuration. */
typedef struct gfb_pixelformat {
    gfb_pixelformat_id_t id;	/**< Pixel format identifier. */
    uint32_t bitsperpixel;		/**< Number of bits per pixel. */
    uint32_t bytesperpixel;		/**< Number of bytes per pixel. */
    uint8_t ashift;				/**< How many left shifts for the alpha component. */
    uint8_t rshift;				/**< How many left shifts for the red component. */
    uint8_t gshift;				/**< How many left shifts for the green component. */
    uint8_t bshift;				/**< How many left shifts for the blue component. */
    uint32_t amask;				/**< Mask to select only alpha component bits. */
    uint32_t rmask;				/**< Mask to select only red component bits. */
    uint32_t gmask;				/**< Mask to select only green component bits. */
    uint32_t bmask;				/**< Mask to select only blue component bits. */
} gfb_pixelformat_t;

/** Pixel format configuration table. */
extern gfb_pixelformat_t gfb_pixelformats[MAX_GFB_PIXELFORMAT];

/** Control flags. */
typedef enum gfb_flag_id {
    GFB_ALPHABLEND		= (1),	/**< Use alpha blending blit. */
    GFB_SRCCOLORKEY		= (2),	/**< Skip pixels matching the color key. */
    GFB_PREALLOCATE		= (4),	/**< Pre-allocate surface pixel buffer. */
    GFB_DOUBLEBUFFER	= (8),	/**< Use double buffering. */
} gfb_flag_id_t;

/** API constants. */
typedef enum gfb_return {
    GFB_OK				= ( 0),	/**< Generic API success return value. */
    GFB_ERROR			= (-1),	/**< Generic API failure return value. */
    GFB_EARGUMENT		= (-2),	/**< Argument to API function was invalid. */
    GFB_ENOMEM			= (-3),	/**< API function ran out of memory i.e. malloc() failed. */
    GFB_ENOTSUPPORTED	= (-4),	/**< API function not supported. */
    GFB_EFILEOPEN		= (-5), /**< File open error. */
    GFB_EFILEREAD		= (-6), /**< File read error. */
    GFB_EFILEWRITE		= (-7), /**< File write error. */
} gfb_return_t;

/** 2D position. */
typedef struct gfb_point {
    int x;	/**< Top pixel position. */
    int y;	/**< Left pixel position. */
} gfb_point_t;

/** 2D rectangle. */
typedef struct gfb_rect {
    int x;		/**< Top pixel position. */
    int y;		/**< Left pixel position. */
    int w;		/**< Width in pixels. */
    int h;		/**< Height in pixels. */
} gfb_rect_t;

/** Polygon. */
typedef struct gfb_poly {
    int count;				/**< Tell how many items used in points[]. */
    gfb_point_t *points;	/**< Every point in the polygon. */
} gfb_poly_t;

/** Cached glyph. */
typedef struct gfb_glyph {
	int     ptsize; /**< Point size. */
	FT_UInt index;  /**< Glyph index. */

	FT_Glyph bitmap;  /**< Fully rendered glyph bitmap. */
} gfb_glyph_t;


///////////////////////////////////////////////////////////////////////////////////////////////////


//Forward declaration for the macros below.
struct gfb_surface;

/** Macro to encode a pixel in 16 bits RGBAX=5.5.5.0.1 */
#define GFB_MAP_PIXELFORMAT_16BIT_RGB(_red, _green, _blue) (0 | ((_red << 10) & 0x7c00) | ((_green << 5) & 0x03e0) | (_blue  & 0x001f))

/** Macro to encode a pixel in 24 bits RGBAX=8.8.8.0.0 */
#define GFB_MAP_PIXELFORMAT_24BIT_RGB(_red, _green, _blue) (0 | ((_red << 16) & 0xff0000) | ((_green << 8) & 0xff00) | (_blue & 0xff))

/** Macro to encode a pixel in 32 bits RGBAX=8.8.8.0.8 */
#define GFB_MAP_PIXELFORMAT_32BIT_RGB(_red, _green, _blue) (0xff000000 | ((_red << 16) & 0xff0000) | ((_green << 8) & 0xff00) | (_blue & 0xff))

/** Macro to encode a pixel in 32 bits RGBAX=8.8.8.0.8 */
#define GFB_MAP_PIXELFORMAT_32BIT_ARGB(_alpha, _red, _green, _blue) (((_alpha << 24) & 0xff000000) | ((_red << 16) & 0xff0000) | ((_green << 8) & 0xff00) | (_blue & 0xff))

/** Macro to define and declare a routine that copies a pixel out to surface frame buffer. */
#define GFB_PUTPIXEL(_gfb_putpixel_name) int (_gfb_putpixel_name)(struct gfb_surface *psurface, int x, int y, gfb_color_t color)

/** Function pointer type to a put pixel routine. */
typedef GFB_PUTPIXEL(*gfb_putpixel_t);

/** Macro to define and declare a routine that retrieves a pixel from surface frame buffer. */
#define GFB_GETPIXEL(_gfb_getpixel_name) gfb_color_t (_gfb_getpixel_name)(struct gfb_surface *psurface, int x, int y)

/** Function pointer type to a get pixel routine. */
typedef GFB_GETPIXEL(*gfb_getpixel_t);

/** Macro to define and declare a routine that copies pixels from a surface into surface frame buffer. */
#define GFB_BLIT(_gfb_blit_name) int (_gfb_blit_name)(struct gfb_surface *pdest, struct gfb_rect *pdestrect, struct gfb_surface *psource, struct gfb_rect *psourcerect)

/** Function pointer type to a blit surface routine. */
typedef GFB_BLIT(*gfb_blit_t);

/** Macro to define and declare a routine that flips between primary and secondary buffers (double buffering). */
#define GFB_FLIP(_gfb_flip_name) int (_gfb_flip_name)(struct gfb_surface *psurface)

/** Function pointer type to a flip routine. */
typedef GFB_FLIP(*gfb_flip_t);

/** Macro to define and declare a clear screen routine. */
#define GFB_CLEAR(_gfb_clear_name) int (_gfb_clear_name)(struct gfb_surface *psurface)

/** Function pointer type to a clear screen routine. */
typedef GFB_CLEAR(*gfb_clear_t);

/** Macro to define and declare a routine to draw a line. */
#define GFB_LINE(_gfb_line_name) int (_gfb_line_name)(struct gfb_surface *psurface, int x1, int y1, int x2, int y2, gfb_color_t color)

/** Function pointer type to a line drawing routine. */
typedef GFB_LINE(*gfb_line_t);

/** Macro to define and declare a routine to draw a rectangle. */
#define GFB_RECTANGLE(_gfb_rectangle_name) int (_gfb_rectangle_name)(struct gfb_surface *psurface, gfb_rect_t *prect, gfb_color_t color)

/** Function pointer to a rectangle drawing routine. */
typedef GFB_RECTANGLE(*gfb_rectangle_t);

/** Macro to define and declare a routine to draw a circle. */
#define GFB_CIRCLE(_gfb_circle_name) int (_gfb_circle_name)(struct gfb_surface *psurface, int x, int y, int radius, gfb_color_t color)

/** Function pointer to a circle drawin routine. */
typedef GFB_CIRCLE(*gfb_circle_t);

/** Macro to define and declare a routine to draw a filled rectangle. */
#define GFB_FILLEDRECTANGLE(_gfb_filledrectangle_name) int (_gfb_filledrectangle_name)(struct gfb_surface *psurface, gfb_rect_t *prect, gfb_color_t colorb)

/** Function pointer to a filled rectangle drawing routine. */
typedef GFB_FILLEDRECTANGLE(*gfb_filledrectangle_t);

/** Macro to define and declare a routine to draw a filled circle. */
#define GFB_FILLEDCIRCLE(_gfb_filledcircle_name) int (_gfb_filledcircle_name)(struct gfb_surface *psurface, int x, int y, int radius, gfb_color_t colorf, gfb_color_t colorb)

/** Function pointer to a filled circle drawing routine. */
typedef GFB_FILLEDCIRCLE(*gfb_filledcircle_t);

/** Macro to define and declare a routine to draw polygon. */
#define GFB_POLYGON(_gfb_polygon_name) int (_gfb_polygon_name)(struct gfb_surface *psurface, gfb_point_t *ppoints, size_t count, gfb_color_t color)

/** Function pointer to a draw polygon routine. */
typedef GFB_POLYGON(*gfb_polygon_t);

/** Macro to define and declare a routine to flood fill an area. */
#define GFB_FLOODFILL(_gfb_floodfill_name) int (_gfb_floodfill_name)(struct gfb_surface *psurface, int x, int y, gfb_color_t color)

/** Function pointer to a flood fill routine. */
typedef GFB_FLOODFILL(*gfb_floodfill_t);

/** Macro to define and declare a routine to render out Unicode array. */
#define GFB_TEXT(_gfb_text_name) int (_gfb_text_name)(struct gfb_surface *psurface, gfb_font_id fontid, int x, int y, uint16_t *text, size_t count, gfb_color_t colorf, gfb_color_t colorb)

/** Function pointer to a Unicode array render. */
typedef GFB_TEXT(*gfb_text_t);


///////////////////////////////////////////////////////////////////////////////////////////////////


GFB_PUTPIXEL(gfb_soft_putpixel);
GFB_GETPIXEL(gfb_soft_getpixel);
GFB_BLIT(gfb_soft_blit);
GFB_FLIP(gfb_soft_flip);
GFB_CLEAR(gfb_soft_clear);
GFB_LINE(gfb_soft_line);
GFB_RECTANGLE(gfb_soft_rectangle);
GFB_CIRCLE(gfb_soft_circle);
GFB_FILLEDRECTANGLE(gfb_soft_filledrectangle);
GFB_FILLEDCIRCLE(gfb_soft_filledcircle);
GFB_POLYGON(gfb_soft_polygon);
GFB_FLOODFILL(gfb_soft_floodfill);
GFB_TEXT(gfb_soft_text);


///////////////////////////////////////////////////////////////////////////////////////////////////


/**
Low level operations.
Hardware or software implementations of graphic routines.
*/
typedef struct gfb_devop {
	gfb_putpixel_t putpixel;				/**< Write pixel into frame buffer memory. */
	gfb_getpixel_t getpixel;				/**< Read pixel from frame buffer memory. */
	gfb_blit_t blit;						/**< Copy pixels from one surface to another. */
	gfb_flip_t flip;						/**< Flip between primary and secondary frame buffers (double buffering). */
	gfb_clear_t clear;						/**< Clear whole frame buffer area. */
	gfb_line_t line;						/**< Draw a line. */
	gfb_rectangle_t rectangle;				/**< Draw a rectangle. */
	gfb_circle_t circle;					/**< Draw a circle. */
	gfb_filledrectangle_t filledrectangle;	/**< Draw a filled rectangle. */
	gfb_filledcircle_t filledcircle;		/**< Draw a filled circle. */
	gfb_polygon_t polygon;					/**< Draw all lines in a polygon. */
	gfb_floodfill_t floodfill;				/**< Fill area of mathing color. */
	gfb_text_t text;						/**< Render UTF8 encoded NUL terminated string. */
} gfb_devop_t;

/** Graphical surface descriptor. */
typedef struct gfb_surface {
	gfb_flag_id_t flags;		/**< Control flags. */
	gfb_color_t colorkey;		/**< Transparent color key. */
	gfb_rect_t cliprect;		/**< Surface clip rectangle. */
	gfb_pixelformat_t *pformat;	/**< Layout of pixels in this frame buffer. */
	int w;						/**< Width of surface in pixels. */
	int h;						/**< Height of surface in pixels. */
	unsigned int pitch;			/**< Number of bytes per scanline. */
	uint8_t alpha;				/**< Overall surface alpha value. */
	unsigned int refcount;		/**< Reference counter. */
	gfb_devop_t *op;			/**< Device accelerated operations or software equivalent. */
	uint8_t *ppixelmemory;		/**< Pointer to the pixel buffer, pointer returned by calloc(). */
	//The variables ppixels and pbuffer are swapped when surface is flipped.
	//Both variables must point to memory within ppixelmemory[].
	uint8_t *ppixels;			/**< Pointer to the primary pixel buffer. This is what gfb_blit() copies from. */
	uint8_t *pbuffer;			/**< Pointer to the secondary pixel buffer. This is what all operations use, besides gfb_blit(). */
	uint32_t *prowoffsets;		/**< Array of offsets into `ppixels[]` where each pixel row starts. */
	uint32_t *pcoloffsets;		/**< Array of offsets into `ppixels[]` where each pixel column starts. */
} gfb_surface_t;


///////////////////////////////////////////////////////////////////////////////////////////////////


/** Bitmap Header. */
typedef struct __attribute__ ((__packed__)) gfb_bmpv3_header {
	char bm[2];			/**< Must be 'B' and 'M'. */
	uint32_t filesize;	/**< Size of the BMP file in bytes. */
	uint16_t reserved1;	/**< Reserved. */
	uint16_t reserved2;	/**< Reserved. */
	uint32_t offset;	/**< Offset relative to header start where pixel data starts. */
} gfb_bmpv3_header_t;

/** Bitmap Info Header. */
typedef struct __attribute__ ((__packed__)) gfb_bmpv3_dib {
	uint32_t size;			/**< Size of this DIB header (40 bytes). */
	int32_t width;			/**< Width of the bitmap. */
	int32_t height;			/**< Height of the bitmap. */
	uint16_t colorplanes;	/**< The number of color planes. Must be 1. */
	uint16_t bpp;			/**< The number of bits per pixel, which is the color depth of the image. Typical values are 1, 4, 8, 16, 24 and 32. */
	uint32_t compression;	/**< The compression method being used. */
	uint32_t imagesize;		/**< The image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps. */
	int32_t res_horizontal;	/**< The horizontal resolution of the image. (pixel per meter, signed integer) */
	int32_t res_vertical;	/**< The vertical resolution of the image. (pixel per meter, signed integer) */
	uint32_t ncolors;		/**< The number of colors in the color palette, or 0 to default to 2n. */
	uint32_t nicolors;		/**< The number of important colors used, or 0 when every color is important; generally ignored. */
} gfb_bmpv3_dib_t;

/**
Pixel data for 24 bit pixels.
@note The order of the components in file is BGR.
*/
typedef struct __attribute__ ((__packed__)) gfb_bmpv3_pixel24 {
    uint8_t b; /**< Blue component. */
    uint8_t g; /**< Green component. */
    uint8_t r; /**< Red component. */
} gfb_bmpv3_pixel24_t;

/** Bitmap structure. */
typedef struct __attribute__ ((__packed__)) gfb_bmpv3 {
    gfb_bmpv3_header_t header;	/**< File header. */
    gfb_bmpv3_dib_t dib;		/**< Description of the bitmap itself. */
} gfb_bmpv3_t;

/**
Initialize internal handle to libfreetype2 and other dependencies.

@note This function must be called once before the other library functions.

@return On success, returns GFB_OK.
@return On failure, such as libfreetype2 failure to initialize, returns GFB_ERROR.
*/
int gfb_initialize(void);


/**
Free up any internal resources and close the freetype2 library handle.
*/
void gfb_finalize(void);

/**
Allocate for and initialize a new frame buffer surface.

@param ppsurface Double pointer to make sure that caller gets NULL assigned pointer in case of failure.
@param width Width of the frame buffer in pixels.
@param height Height of the frame buffer in pixels.
@param format Format of the pixel buffer.
@param flags Control flags from enum gfb_flag_id.
@param ppixels Pointer to pixel buffer or NULL to malloc() automatically. Ignored if flags contains GFB_PREALLOCATE.
@param pdevop Pointer to device accelerated operations or NULL to use built-in software implemenations.

@return On success the pointer reference by ppsurface is assigned the address of the newly allocated surface and GFB_OK is returned.
@return On failure such as out of memory the pointer reference by ppsurface is assigned NULL and GFB_ERROR is returned.
*/
int gfb_surface_create(gfb_surface_t **ppsurface, int width, int height, gfb_pixelformat_id_t format, gfb_flag_id_t flags, uint8_t *ppixels, gfb_devop_t *pdevop);

/**
Free all memory allocated by a surface.
@param ppsurface Double pointer to make sure caller gets a NULL assigned pointer after free.
*/
void gfb_surface_destroy(gfb_surface_t **ppsurface);

/**
Allocate for a surface with the contents of a bitmap copied and converted to the given pixel format.
@param ppdevice Double pointer to make sure that caller gets a NULL assigned pointer in case of failure.
@param pbmpv3 Pointer to a microsoft bitmap version 3.

@return On success the pointer referenced by ppdevice is assigned the address of the new device and GFB_OK is returned.
@return On failure such as out of memory the pointer referenced by ppdevice is assigned NULL and GFB_ERROR is returned.
*/
int gfb_surface_from_bmp3(gfb_surface_t **ppsurface, gfb_pixelformat_id_t format, gfb_bmpv3_header_t *pbmpv3);

/**
Allocate for a surface large enough for a bitmap residing in file.
Loads the bitmap pixels from the file into the surface.
The bitmap file format must be version 3.
@param ppdevice Double pointer to make sure that caller gets a NULL assigned pointer in case of failure.
@param format Format of the surface pixel buffer. The bitmap pixels are converted as they are copied.
@param flags Control flags from enum gfb_flag_id.
@param ppixels Pointer to pixel buffer or NULL to malloc() automatically. Ignored if flags contains GFB_PREALLOCATE.
@param pdevop Pointer to device accelerated operations or NULL to use built-in software implemenations.
@param pzpath Pointer to a zero terminated string with a path to the bitmap file.
*/
int gfb_surface_load_bmp3(gfb_surface_t **ppsurface, gfb_pixelformat_id_t format, gfb_flag_id_t flags, uint8_t *ppixels, gfb_devop_t *pdevop, const char *pzpath);

/**
Dump surface pixels to BMP file.

@param psurface The surface to save.
@param Path the destination file.
@return On success, returns GFB_OK.
@return On failure, returns an error code (GFB_Exxx).
*/
int gfb_surface_save_bmp3(gfb_surface_t *psurface, const char *pzpath);

/**
Dump raw pixels in the pixel format of the surface.

The size of the resulting file will be exactly:
(psurface->w * psurface->h * psurface->pformat->bytesperpixel)

@param psurface The surface to dump.
@param Path the destination file.
@return On success, returns GFB_OK.
@return On failure, returns an error code (GFB_Exxx).
*/
int gfb_surface_save_raw(gfb_surface_t *psurface, const char *pzpath);


///////////////////////////////////////////////////////////////////////////////////////////////////


/**
Create a pixel encoded in the given format.
@param red Red intensity.
@param green Green intensity.
@param blue Blue intensity.
@param alpha Alpha value.
@return Returns the encoded pixel.
*/
gfb_color_t gfb_maprgba(gfb_surface_t *psurface, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

/**
Set clipping rectangle of a surface.
Pixels are only drawn inside this rectangle.
If prect is NULL the clipping rectangle is set to the full surface dimensions.
@param psurface Pointer to the surface to change.
@param prect Pointer to a rectangle or NULL to use psurface size.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_setcliprect(gfb_surface_t *psurface, gfb_rect_t *prect);

/**
Set overall alpha value of a surface.
The surface flag GFB_ALPHABLEND is set automatically.
@param alpha The new alpha value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_setalpha(gfb_surface_t *psurface, uint8_t alpha);

/**
Set color key value of a surface.
The surface flag GFB_SRCCOLORKEY is set automatically.
@param colorkey The new color key (see gfb_maprgba()).
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_setcolorkey(gfb_surface_t *psurface, gfb_color_t colorkey);


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
int gfb_putpixel(gfb_surface_t *psurface, int x, int y, gfb_color_t color);

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
int gfb_getpixel(gfb_surface_t *psurface, int x, int y, uint8_t *palpha, uint8_t *pred, uint8_t *pgreen, uint8_t *pblue);

/**
Copy pixels from one surface to another.
Very important that both surfaces are of the same pixel format.

@param pdest Pointer to destination surface
@param pdestrect Pointer to destination rectangle or NULL to use (0, 0).
@param psource Pointer to source surface.
@param psourcerect Pointer to source rectangle or NULL to copy whole surface.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_blit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect);

/**
Flip between primary and secondary frame buffers (double buffering).
@param pdest Pointer to the surface to flip.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_flip(gfb_surface_t *psurface);

/**
Clear whole frame buffer area to black.
@param psurface Pointer to the surface to draw on.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_clear(gfb_surface_t *psurface);

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
int gfb_line(gfb_surface_t *psurface, int x1, int y1, int x2, int y2, gfb_color_t color);

/**
Draw a rectangle.
@param psurface Pointer to the surface to draw on.
@param prect Pointer to the rectangle configuration.
@param color Encoded pixel value.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_rectangle(gfb_surface_t *psurface, gfb_rect_t *prect, gfb_color_t color);

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
int gfb_circle(gfb_surface_t *psurface, int x, int y, int radius, gfb_color_t color);

/**
Draw a filled rectangle.
@param psurface Pointer to the surface to draw on.
@param prect Pointer to the rectangle configuration.
@param colorb Encoded pixel value of the fill color.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_filledrectangle(gfb_surface_t *psurface, gfb_rect_t *prect, gfb_color_t colorb);

/**
Draw a filled circle.
@param psurface Pointer to the surface to draw on.
@param x Left pixel position.
@param y Top pixel position.
@param radius Circle radius.
@param colorf Encoded pixel value of the outline color.
@param colorb Encoded pixel value of the fill color.
*/
int gfb_filledcircle(gfb_surface_t *psurface, int x, int y, int radius, gfb_color_t colorf, gfb_color_t colorb);

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

/**
Load true-type file from memory.
@param pttf Pointer to the true-type file in memory.
@param ttfsize Number of bytes in pttf[].
@return On success, returns an Id to the stored font (0 to MAX_GFB_FONT).
@return On failure, returns a negative error code (GFB_Exxx).
*/
gfb_font_id gfb_ttf_load_memory(uint8_t *pttf, size_t ttfsize);

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
@param colorb Color of the backgroun.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_textu(gfb_surface_t *psurface, gfb_font_id fontid, uint8_t ptsize, int x, int y, uint16_t *punicode, size_t count, gfb_color_t colorf, gfb_color_t colorb);

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
@param colorb Color of the backgroun.
@return On success returns GFB_OK.
@return On failure a negative error code is returned (GFB_Exxx).
*/
int gfb_text(gfb_surface_t *psurface, gfb_font_id fontid, uint8_t ptsize, int x, int y, char *pzutf8, size_t count, gfb_color_t colorf, gfb_color_t colorb);

/**
Returns a short copyright and license clause.
@return Const pointer to the NUL terminated text.
*/
const char *gfb_about(void);

#ifdef __cplusplus
}
#endif

#endif //!__LIBGFB_H__

/** @} */
