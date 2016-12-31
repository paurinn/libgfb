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
@addtogroup libgfb
@{
*/
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "libgfb.h"

/** Configuration of each pixel format. */
gfb_pixelformat_t gfb_pixelformats[MAX_GFB_PIXELFORMAT] = {
//               Identifier,  bpp, Bpp, ashift, rshift, gshift, bshift,       amask,      rmask,      gmask,      gmask
	{ GFB_PIXELFORMAT_RGB16,   16,   2,      0,     10,      5,      0,  0x00000000, 0x00007c00, 0x000003e0, 0x0000001f },
	{ GFB_PIXELFORMAT_RGB24,   24,   3,      0,     16,      8,      0,  0x00000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
	{ GFB_PIXELFORMAT_RGB32,   32,   4,      0,     16,      8,      0,  0x00000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
	{ GFB_PIXELFORMAT_ARGB32,  32,   4,     24,     16,      8,      0,  0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
	{ GFB_PIXELFORMAT_ALPHA,   32,   4,     24,      0,      0,      0,  0xff000000, 0x00000000, 0x00000000, 0x00000000 }
};


///////////////////////////////////////////////////////////////////////////////////////////////////

/** Handle to freetype2. */
static FT_Library libft2 = NULL;

/**< Handles to face objects. */
static FT_Face gfb_fontstore[MAX_GFB_FONT] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };

#define gfb_gcindex(x) (x % MAX_GFB_GLYPH)

/**< Glyph cache. */
//FIXME static gfb_glyph_t gfb_glyphstore[MAX_GFB_GLYPH] = {{0}};

/** Returns the sign of the given integer, -1, 0 or 1. */
static inline int sgn(int x) {
	return (0 < x) - (x < 0);
}

/**
Make sure the rectangle pointed to by prect1 is inside rectangle prect2.
@param prect1 Pointer to the rectangle that must fit within prect1.
@param prect2 Pointer to the container rectangle.
@return On success, returns the clipped rectangle.
@return On failure, returns a zero rectangle.
*/
static inline gfb_rect_t gfb_cliprect(gfb_rect_t *prect1, gfb_rect_t *prect2) {
	gfb_rect_t r = { .x = 0, .y = 0, .w = 0, .h = 0 };

	if (prect1 == NULL || prect2 == NULL) {
	    return r; //Invalid argument.
	}

	r.x = gfb_clampi(prect1->x, prect2->x, prect2->x + prect2->w);
	r.y = gfb_clampi(prect1->y, prect2->y, prect2->y + prect2->h);

	r.w = gfb_mini(prect1->w, (prect2->x + prect2->w) - prect1->x);
	r.h = gfb_mini(prect1->h, (prect2->y + prect2->h) - prect1->y);

	return r;
}

/* blit using per-pixel alpha, ignoring any colour key */
static inline void gfb_alphablit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];

	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			gfb_color_t color;
			//Fetch and decode source pixel.
			memcpy(&color, srcpix, psource->pformat->bytesperpixel);

			uint8_t sa = (uint8_t)((color & psource->pformat->amask) >> psource->pformat->ashift);
			uint8_t sr = (uint8_t)((color & psource->pformat->rmask) >> psource->pformat->rshift);
			uint8_t sg = (uint8_t)((color & psource->pformat->gmask) >> psource->pformat->gshift);
			uint8_t sb = (uint8_t)((color & psource->pformat->bmask) >> psource->pformat->bshift);

			//Fetch and decode destination pixel.
			memcpy(&color, dstpix, pdest->pformat->bytesperpixel);

			uint8_t da = (uint8_t)((color & pdest->pformat->amask) >> pdest->pformat->ashift);
			uint8_t dr = (uint8_t)((color & pdest->pformat->rmask) >> pdest->pformat->rshift);
			uint8_t dg = (uint8_t)((color & pdest->pformat->gmask) >> pdest->pformat->gshift);
			uint8_t db = (uint8_t)((color & pdest->pformat->bmask) >> pdest->pformat->bshift);

			float a = (float)sa / 255.0f;

			//Encode and write to destination.
			gfb_color_t destcolor = gfb_maprgba(
				pdest,
				(uint8_t)(a * sr + (1 - a) * dr),
				(uint8_t)(a * sg + (1 - a) * dg),
				(uint8_t)(a * sb + (1 - a) * db),
				da
			);
			memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);

			//Advance pixel positions.
			srcpix += psource->pformat->bytesperpixel;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

/* blit using the colour key AND the per-surface alpha value */
static inline void gfb_alphacolorkeyblit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];
	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			gfb_color_t color;
			gfb_color_t destcolor;

			//Fetch and decode pixels.
			memcpy(&color, srcpix, psource->pformat->bytesperpixel);
			memcpy(&destcolor, dstpix, pdest->pformat->bytesperpixel);

			//Skip colors matching the color key.
			if ((color | psource->pformat->amask) != (psource->colorkey | psource->pformat->amask)) {
				uint8_t sa = psource->alpha; //(uint8_t)((color & psource->pformat->amask) >> psource->pformat->ashift);
				uint8_t sr = (uint8_t)((color & psource->pformat->rmask) >> psource->pformat->rshift);
				uint8_t sg = (uint8_t)((color & psource->pformat->gmask) >> psource->pformat->gshift);
				uint8_t sb = (uint8_t)((color & psource->pformat->bmask) >> psource->pformat->bshift);

				uint8_t da = (uint8_t)((destcolor & pdest->pformat->amask) >> pdest->pformat->ashift);
				uint8_t dr = (uint8_t)((destcolor & pdest->pformat->rmask) >> pdest->pformat->rshift);
				uint8_t dg = (uint8_t)((destcolor & pdest->pformat->gmask) >> pdest->pformat->gshift);
				uint8_t db = (uint8_t)((destcolor & pdest->pformat->bmask) >> pdest->pformat->bshift);

				float a = (float)sa / 255.0f;

				dr = (uint8_t)(a * (float)sr + (1.0f - a) * (float)dr);
				dg = (uint8_t)(a * (float)sg + (1.0f - a) * (float)dg);
				db = (uint8_t)(a * (float)sb + (1.0f - a) * (float)db);

				//Encode and write to destination.
				destcolor = gfb_maprgba(pdest, dr, dg, db, da);
				memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);
			}

			//Advance pixel positions.
			srcpix += psource->pformat->bytesperpixel;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

/* blit using the per-surface alpha value */
static inline void gfb_srcalphablit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];
	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			gfb_color_t color;
			gfb_color_t destcolor;
			//Fetch and decode pixels.
			memcpy(&color, srcpix, psource->pformat->bytesperpixel);
			memcpy(&destcolor, dstpix, pdest->pformat->bytesperpixel);

			uint8_t sa = psource->alpha;
			uint8_t sr = (uint8_t)((color & psource->pformat->rmask) >> psource->pformat->rshift);
			uint8_t sg = (uint8_t)((color & psource->pformat->gmask) >> psource->pformat->gshift);
			uint8_t sb = (uint8_t)((color & psource->pformat->bmask) >> psource->pformat->bshift);

			uint8_t da = (uint8_t)((destcolor & pdest->pformat->amask) >> pdest->pformat->ashift);
			uint8_t dr = (uint8_t)((destcolor & pdest->pformat->rmask) >> pdest->pformat->rshift);
			uint8_t dg = (uint8_t)((destcolor & pdest->pformat->gmask) >> pdest->pformat->gshift);
			uint8_t db = (uint8_t)((destcolor & pdest->pformat->bmask) >> pdest->pformat->bshift);

			float a = (float)sa / 255.0f;

			dr = (uint8_t)(a * (float)sr + (1.0f - a) * (float)dr);
			dg = (uint8_t)(a * (float)sg + (1.0f - a) * (float)dg);
			db = (uint8_t)(a * (float)sb + (1.0f - a) * (float)db);

			//Encode and write to destination.
			destcolor = gfb_maprgba(pdest, dr, dg, db, da);
			memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);

			//Advance pixel positions.
			srcpix += psource->pformat->bytesperpixel;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

/* blit using the colour key */
static inline void gfb_colorkeyblit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];
	gfb_color_t color, destcolor;
	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			//Fetch and decode source pixel.
			memcpy(&color, srcpix, psource->pformat->bytesperpixel);
			if ((color | psource->pformat->amask) != (psource->colorkey | psource->pformat->amask)) {
				uint8_t r, g, b, a;
				a = (uint8_t)((color & psource->pformat->amask) >> psource->pformat->ashift);
				r = (uint8_t)((color & psource->pformat->rmask) >> psource->pformat->rshift);
				g = (uint8_t)((color & psource->pformat->gmask) >> psource->pformat->gshift);
				b = (uint8_t)((color & psource->pformat->bmask) >> psource->pformat->bshift);

				//Encode and write to destination.
				destcolor = gfb_maprgba(pdest, r, g, b, a);
				memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);
			}

			//Advance pixel positions.
			srcpix += psource->pformat->bytesperpixel;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

/* Opaque rectangular blit raster copy */
static inline void gfb_rasterblit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	int count = ncols * psource->pformat->bytesperpixel;
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];
	for (; nlines >= 0; nlines--) {
		memmove((uint8_t *)pdestrow, (uint8_t *)psourcerow, count);
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

/* Opaque rectangular blit for different formats */
static inline void gfb_convertblit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);
	uint8_t *psourcerow = &psource->ppixels[psourcerect->y * psource->pitch + (psourcerect->x * psource->pformat->bytesperpixel)];
	uint8_t *pdestrow = &pdest->ppixels[pdestrect->y * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];
	gfb_color_t color, destcolor;
	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			//Fetch and decode source pixel.
			memcpy(&color, srcpix, psource->pformat->bytesperpixel);
			uint8_t r = 0, g = 0, b = 0, a = 0;

			if (psource->pformat->amask) a = (uint8_t)((color & psource->pformat->amask) >> psource->pformat->ashift);
			if (psource->pformat->rmask) r = (uint8_t)((color & psource->pformat->rmask) >> psource->pformat->rshift);
			if (psource->pformat->gmask) g = (uint8_t)((color & psource->pformat->gmask) >> psource->pformat->gshift);
			if (psource->pformat->bmask) b = (uint8_t)((color & psource->pformat->bmask) >> psource->pformat->bshift);

			//Encode and write to destination.
			destcolor = gfb_maprgba(pdest, r, g, b, a);
			memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);

			//Advance pixel positions.
			srcpix += psource->pformat->bytesperpixel;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

static inline void dumprect(gfb_rect_t *prect) {
	fprintf(stderr, "xy(%d, %d) : wh(%d, %d)\n", prect->x, prect->y, prect->w, prect->h);
}

/*
blit using per-pixel alpha, ignoring any colour key
*/
static inline void gfb_ftbitmapblit( gfb_surface_t *pdest, gfb_rect_t *pdestrect, FT_Bitmap *psource, gfb_rect_t *psourcerect, gfb_color_t colorf, gfb_color_t colorb) {
	int ncols = gfb_mini(pdestrect->w, psourcerect->w);
	int nlines = gfb_mini(pdestrect->h, psourcerect->h);

	uint8_t *psourcerow = &psource->buffer[((psourcerect->y) * psource->pitch) + psourcerect->x];
	uint8_t *pdestrow = &pdest->ppixels[(pdestrect->y) * pdest->pitch + (pdestrect->x * pdest->pformat->bytesperpixel)];

	(void)colorf;
	uint8_t sr = (uint8_t)((colorf & pdest->pformat->rmask) >> pdest->pformat->rshift);
	uint8_t sg = (uint8_t)((colorf & pdest->pformat->gmask) >> pdest->pformat->gshift);
	uint8_t sb = (uint8_t)((colorf & pdest->pformat->bmask) >> pdest->pformat->bshift);

	for (; nlines >= 0; nlines--) {
		int i;

		uint8_t *srcpix = psourcerow;
		uint8_t *dstpix = pdestrow;

		for (i = 0; i < ncols; i++) {
			uint8_t sa = *srcpix;

			//Fetch and decode destination pixel.
			uint8_t da = (uint8_t)((colorb & pdest->pformat->amask) >> pdest->pformat->ashift);
			uint8_t dr = (uint8_t)((colorb & pdest->pformat->rmask) >> pdest->pformat->rshift);
			uint8_t dg = (uint8_t)((colorb & pdest->pformat->gmask) >> pdest->pformat->gshift);
			uint8_t db = (uint8_t)((colorb & pdest->pformat->bmask) >> pdest->pformat->bshift);

			float a = (float)sa / 255.0f;

			//Encode and write to destination.
			gfb_color_t destcolor = gfb_maprgba(
				pdest,
				(uint8_t)(a * sr + (1 - a) * dr),
				(uint8_t)(a * sg + (1 - a) * dg),
				(uint8_t)(a * sb + (1 - a) * db),
				da
			);
			memcpy(dstpix, &destcolor, pdest->pformat->bytesperpixel);

			//Advance pixel positions.
			srcpix += 1;
			dstpix += pdest->pformat->bytesperpixel;
		}
		pdestrow += pdest->pitch;
		psourcerow += psource->pitch;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////


GFB_PUTPIXEL(gfb_soft_putpixel) {
	memcpy(
		(uint8_t *)&psurface->pbuffer[ psurface->prowoffsets[y] + psurface->pcoloffsets[x] ],
		&color,
		psurface->pformat->bytesperpixel
	);
	return GFB_OK;
}

GFB_GETPIXEL(gfb_soft_getpixel) {
	gfb_color_t rc;
	memcpy(&rc, (uint8_t *)&psurface->pbuffer[y * psurface->pitch + (x * psurface->pformat->bytesperpixel)], psurface->pformat->bytesperpixel);
	return rc;
}

GFB_BLIT(gfb_soft_blit) {
	if (psource->flags & GFB_ALPHABLEND) {
		if (psource->pformat->amask != 0) {
			/* blit using per-pixel alpha, ignoring any colour key */
			gfb_alphablit(pdest, pdestrect, psource, psourcerect);
		} else {
			if (psource->flags & GFB_SRCCOLORKEY) {
				/* blit using the colour key AND the per-surface alpha value */
				gfb_alphacolorkeyblit(pdest, pdestrect, psource, psourcerect);
			} else {
				/* blit using the per-surface alpha value */
				gfb_srcalphablit(pdest, pdestrect, psource, psourcerect);
			}
		}
	} else {
		if (psource->flags & GFB_SRCCOLORKEY) {
			/* blit using the colour key */
			gfb_colorkeyblit(pdest, pdestrect, psource, psourcerect);
		} else {
			if (pdest->pformat->bytesperpixel == psource->pformat->bytesperpixel) {
				/* Opaque rectangular blit raster copy */
				gfb_rasterblit(pdest, pdestrect, psource, psourcerect);
			} else {
				/* Opaque rectangular blit for different formats */
				gfb_convertblit(pdest, pdestrect, psource, psourcerect);
			}
		}
	}


	return GFB_OK;
}

GFB_FLIP(gfb_soft_flip) {
	uint8_t *tmp = psurface->pbuffer;
	psurface->pbuffer = psurface->ppixels;
	psurface->ppixels = tmp;
	return GFB_OK;
}

GFB_CLEAR(gfb_soft_clear) {
	int rc = GFB_OK;

	if (
		   psurface->cliprect.x == 0
		&& psurface->cliprect.y == 0
		&& psurface->cliprect.w == psurface->w
		&& psurface->cliprect.h == psurface->h
	) {
		memset(psurface->pbuffer , 0x00, (psurface->pitch * psurface->h));
	} else {
		rc = gfb_filledrectangle(psurface, NULL, 0x000000);
	}
	return rc;
}

GFB_LINE(gfb_soft_line) {
	//both points on line must lie within surface cliprect.
	if (
		   gfb_outside(x1, psurface->cliprect.x, psurface->cliprect.x + psurface->cliprect.w)
		|| gfb_outside(y1, psurface->cliprect.y, psurface->cliprect.y + psurface->cliprect.h)
		|| gfb_outside(x2, psurface->cliprect.x, psurface->cliprect.x + psurface->cliprect.w)
		|| gfb_outside(y2, psurface->cliprect.y, psurface->cliprect.y + psurface->cliprect.h)
	) {
		return GFB_EARGUMENT; //Line out of bounds.
	}

	int dx    = x2 - x1;	/* the horizontal distance of the line */
	int dy    = y2 - y1;	/* the vertical distance of the line */
	int dxabs = abs(dx);
	int dyabs = abs(dy);
	int sdx   = sgn(dx);
	int sdy   = sgn(dy);
	int x     = dyabs >> 1;
	int y     = dxabs >> 1;
	int px    = x1;
	int py    = y1;

	int i, rc;

	psurface->op->putpixel(psurface, px,py, color);

	if (dxabs >= dyabs) {
		/* the line is more horizontal than vertical */
		for (i = 0; i < dxabs;i++) {
			y += dyabs;
			if (y >= dxabs) {
				y -= dxabs;
				py += sdy;
			}
			px += sdx;
			rc = psurface->op->putpixel(psurface, px, py, color);
			if (rc != GFB_OK) return rc;
		}
	} else {
		/* the line is more vertical than horizontal */
		for (i=0;i<dyabs;i++) {
			x+=dxabs;
			if (x>=dyabs) {
				x-=dyabs;
				px+=sdx;
			}
			py+=sdy;
			rc = psurface->op->putpixel(psurface, px,py, color);
			if (rc != GFB_OK) return rc;
		}
	}

	return GFB_OK;
}

GFB_RECTANGLE(gfb_soft_rectangle) {
	int rc;
	if ((rc = psurface->op->line(psurface, prect->x, prect->y, prect->x + prect->w, prect->y, color)) != GFB_OK) return rc;
	if ((rc = psurface->op->line(psurface, prect->x + prect->w, prect->y, prect->x + prect->w, prect->y + prect->h, color)) != GFB_OK) return rc;
	if ((rc = psurface->op->line(psurface, prect->x, prect->y, prect->x, prect->y + prect->h, color)) != GFB_OK) return rc;
	if ((rc = psurface->op->line(psurface, prect->x, prect->y + prect->h, prect->x + prect->w, prect->y + prect->h, color)) != GFB_OK) return rc;
	return rc;
}

GFB_CIRCLE(gfb_soft_circle) {
	int X = radius;
	int Y = 0;
	int decisionOver2 = 1 - X;   // Decision criterion divided by 2 evaluated at X=radius, Y=0

	while( Y <= X ) {
		gfb_putpixel(psurface,  X + x,  Y + y, color); // Octant 1
		gfb_putpixel(psurface,  Y + x,  X + y, color); // Octant 2
		gfb_putpixel(psurface, -X + x,  Y + y, color); // Octant 4
		gfb_putpixel(psurface, -Y + x,  X + y, color); // Octant 3
		gfb_putpixel(psurface, -X + x, -Y + y, color); // Octant 5
		gfb_putpixel(psurface, -Y + x, -X + y, color); // Octant 6
		gfb_putpixel(psurface,  X + x, -Y + y, color); // Octant 7
		gfb_putpixel(psurface,  Y + x, -X + y, color); // Octant 8
		Y++;
		if (decisionOver2<=0) {
			decisionOver2 += 2 * Y + 1;   // Change in decision criterion for Y -> Y+1
		} else {
			X--;
			decisionOver2 += 2 * (Y - X) + 1;   // Change for Y -> Y+1, X -> X-1
		}
	}

	return GFB_OK;
}

GFB_FILLEDRECTANGLE(gfb_soft_filledrectangle) {
	uint32_t yoff = prect->y * psurface->pitch; //Byte offset where the line starts.
	uint32_t xoff = prect->x * psurface->pformat->bytesperpixel; //Byte offset relative to yoff.
	uint32_t idx1 = (yoff + xoff); //Byte offset of the first line.
	uint32_t idx;
	int x, y;
	int pixcount = ((prect->x + prect->w) - prect->x)+1; //Number of pixels per line.
	uint32_t pixbytes = pixcount * psurface->pformat->bytesperpixel; //Number of bytes per line.

	//Prepare the first line.
	idx = idx1; //Byte offset of first line
	for (x = 0; x < pixcount; x++) {
	    memcpy((uint8_t *)&psurface->pbuffer[idx], &colorb, psurface->pformat->bytesperpixel);
	    idx += psurface->pformat->bytesperpixel; //Move to next pixel
	}

	//Copy first line over the rest of the rectangle.
	idx = ((yoff + psurface->pitch) + xoff); //Byte offset of second line.
	for (y = prect->y; y < prect->y + prect->h; y++) {
	    memcpy((uint8_t *)&psurface->pbuffer[idx], (uint8_t *)&psurface->pbuffer[idx1], pixbytes);
	    idx += psurface->pitch; //Move to next line.
	}

	return GFB_OK;
}

GFB_FILLEDCIRCLE(gfb_soft_filledcircle) {
	int X = radius;
	int Y = 0;
	int decisionOver2 = 1 - X;   // Decision criterion divided by 2 evaluated at X=radius, Y=0

	while( Y <= X ) {
		psurface->op->line(psurface, -X + x,  Y + y, X + x,  Y + y, colorb);
		psurface->op->line(psurface, -Y + x,  X + y, Y + x,  X + y, colorb);
		psurface->op->line(psurface, -X + x, -Y + y, X + x, -Y + y, colorb);
		psurface->op->line(psurface, -Y + x, -X + y, Y + x, -X + y, colorb);

		Y++;
		if (decisionOver2<=0) {
			decisionOver2 += 2 * Y + 1;   // Change in decision criterion for Y -> Y+1
		} else {
			X--;
			decisionOver2 += 2 * (Y - X) + 1;   // Change for Y -> Y+1, X -> X-1
		}
	}

	X = radius;
	Y = 0;
	decisionOver2 = 1 - X;   // Decision criterion divided by 2 evaluated at X=radius, Y=0
	while( Y <= X ) {
		psurface->op->putpixel(psurface,  X + x,  Y + y, colorf); // Octant 1
		psurface->op->putpixel(psurface, -X + x,  Y + y, colorf); // Octant 4

		psurface->op->putpixel(psurface,  Y + x,  X + y, colorf); // Octant 2
		psurface->op->putpixel(psurface, -Y + x,  X + y, colorf); // Octant 3

		psurface->op->putpixel(psurface, -X + x, -Y + y, colorf); // Octant 5
		psurface->op->putpixel(psurface,  X + x, -Y + y, colorf); // Octant 7

		psurface->op->putpixel(psurface, -Y + x, -X + y, colorf); // Octant 6
		psurface->op->putpixel(psurface,  Y + x, -X + y, colorf); // Octant 8

		Y++;
		if (decisionOver2<=0) {
			decisionOver2 += 2 * Y + 1;   // Change in decision criterion for Y -> Y+1
		} else {
			X--;
			decisionOver2 += 2 * (Y - X) + 1;   // Change for Y -> Y+1, X -> X-1
		}
	}


	return GFB_OK;
}

GFB_POLYGON(gfb_soft_polygon) {
	size_t i;
	int x1, y1, x2, y2;
	int rc;

	for (i = 0; i < count-1; i++) {
		x1 = ppoints[i].x;
		y1 = ppoints[i].y;
		x2 = ppoints[i+1].x;
		y2 = ppoints[i+1].y;
		if ((rc = psurface->op->line(psurface, x1, y1, x2, y2, color)) != GFB_OK) {
			return rc;
		}
	}

	return GFB_OK;
}

#if 0
void gfb_floodfill_helper(gfb_surface_t *psurface, int x1, int x2, int y, gfb_color_t seed_color, gfb_color_t fill_color) {
	int xL, xR;

	if ( y < psurface->cliprect.y1 || y > psurface->cliprect.y2) return;
	//if (( x1 < psurface->cliprect.x1 || x1 > psurface->cliprect.x2) || ( x2 < psurface->cliprect.x1 || x2 > psurface->cliprect.x2)) return;

	for ( xL = x1; xL >= psurface->cliprect.x1; --xL ) { // scan left
		if ( psurface->op->getpixel(psurface, xL,y) != seed_color ) break;
		psurface->op->putpixel(psurface, xL,y,fill_color);
	}

	if ( xL < x1 ) {
		gfb_floodfill_helper(psurface, xL, x1, y-1, seed_color, fill_color); // fill child
		gfb_floodfill_helper(psurface, xL, x1, y+1, seed_color, fill_color); // fill child
		++x1;
	}

	for ( xR = x2;  xR <= psurface->cliprect.x2; ++xR ) { // scan right
		if ( psurface->op->getpixel(psurface, xR,y) !=  seed_color ) break;
		psurface->op->putpixel(psurface, xR,y,fill_color);
	}

	if ( xR > x2 ) {
		gfb_floodfill_helper(psurface, x2, xR, y-1, seed_color, fill_color); // fill child
		gfb_floodfill_helper(psurface, x2, xR, y+1, seed_color, fill_color); // fill child
		--x2;
	}

	for ( xR = x1; xR <= x2 && xR <= psurface->cliprect.x2; ++xR ) {  // scan betweens
		if ( psurface->op->getpixel(psurface, xR,y) == seed_color )
			psurface->op->putpixel(psurface, xR,y,fill_color);
		else {
			if ( x1 < xR ) {
				// fill child
				gfb_floodfill_helper(psurface, x1, xR-1, y-1, seed_color, fill_color);
				// fill child
				gfb_floodfill_helper(psurface, x1, xR-1, y+1, seed_color, fill_color);
				x1 = xR;
			}
			// Note: This function still works if this step is removed.
			for( ; xR <= x2 && xR <= psurface->cliprect.x2; ++xR) { // skip over border
				if( psurface->op->getpixel(psurface, xR,y) == seed_color ) {
					x1 = xR--;
					break;
				}
			}
		}
	}
}
#endif

//From https://en.wikipedia.org/wiki/Flood_fill
//Flood-fill (node, target-color, replacement-color):
static inline void gfb_floodfill_recurse(gfb_surface_t *psurface, int x, int y, gfb_color_t target_color, gfb_color_t replacement_color) {
	if (x < psurface->cliprect.x || x > psurface->cliprect.x + psurface->cliprect.w || y < psurface->cliprect.y || y > psurface->cliprect.y + psurface->cliprect.h) return;

	//1. If target-color is equal to replacement-color, return.
	if (target_color == replacement_color) return;

	//2. If the color of node is not equal to target-color, return.
	if (psurface->op->getpixel(psurface, x, y) != target_color) return;

	//3. Set the color of node to replacement-color.
	psurface->op->putpixel(psurface, x, y, replacement_color);

	//4.
	//4.1 Perform Flood-fill (one step to the south of node, target-color, replacement-color).
	if (y+1 >= psurface->cliprect.y && y+1 <= psurface->cliprect.y + psurface->cliprect.h) gfb_floodfill_recurse(psurface, x, y+1, target_color, replacement_color);
	//4.2 Perform Flood-fill (one step to the north of node, target-color, replacement-color).
	if (y-1 >= psurface->cliprect.y && y-1 <= psurface->cliprect.y + psurface->cliprect.h) gfb_floodfill_recurse(psurface, x, y-1, target_color, replacement_color);
	//4.3 Perform Flood-fill (one step to the west of node, target-color, replacement-color).
	if (x-1 >= psurface->cliprect.x && x-1 <= psurface->cliprect.x + psurface->cliprect.w) gfb_floodfill_recurse(psurface, x-1, y, target_color, replacement_color);
	//4.4 Perform Flood-fill (one step to the east of node, target-color, replacement-color).
	if (x+1 >= psurface->cliprect.x && x+1 <= psurface->cliprect.x + psurface->cliprect.w) gfb_floodfill_recurse(psurface, x+1, y, target_color, replacement_color);
}

GFB_FLOODFILL(gfb_soft_floodfill) {
	(void)psurface;
	(void)x;
	(void)y;
	(void)color;
	/*
	gfb_color_t seed_color = psurface->op->getpixel(psurface, x,y);
	int rc = GFB_OK;
	if (color != seed_color) {
		gfb_floodfill_recurse(psurface, x, y, seed_color, color);
	}
	return rc;
	*/
	return GFB_ENOTSUPPORTED;
}

/** See if there are trailing bytes after the character (c). */
#define is_trail(c) (c > 0x7F && c < 0xC0)

static inline int utf8_get_next_char(const unsigned char *str, size_t str_size, size_t *cursor, int *is_valid, unsigned int *code_point)
{
	size_t pos = *cursor;
	size_t rest_size = str_size - pos;
	unsigned char c;
	unsigned char min;
	unsigned char max;

	*code_point = 0;
	*is_valid = 1;

	if (*cursor >= str_size) {
		return 0;
	}

	c = str[pos];

	if (rest_size < 1) {
		*is_valid = 0;
		pos += 1;
	} else if (c < 0x80) {
		*code_point = str[pos];
		*is_valid = 1;
		pos += 1;
	} else if (c < 0xC2) {
		*is_valid = 0;
		pos += 1;
	} else if (c < 0xE0) {

		if (rest_size < 2 || !is_trail(str[pos + 1])) {
			*is_valid = 0;
			pos += 1;
		} else {
			*code_point = ((str[pos] & 0x1F) << 6) | (str[pos + 1] & 0x3F);
			*is_valid = 1;
			pos += 2;
		}

	} else if (c < 0xF0) {

		min = (c == 0xE0) ? 0xA0 : 0x80;
		max = (c == 0xED) ? 0x9F : 0xBF;

		if (rest_size < 2 || str[pos + 1] < min || max < str[pos + 1]) {
			*is_valid = 0;
			pos += 1;
		} else if (rest_size < 3 || !is_trail(str[pos + 2])) {
			*is_valid = 0;
			pos += 2;
		} else {
			*code_point = ((str[pos]     & 0x1F) << 12)
					   | ((str[pos + 1] & 0x3F) <<  6)
					   |  (str[pos + 2] & 0x3F);
			*is_valid = 1;
			pos += 3;
		}

	} else if (c < 0xF5) {

		min = (c == 0xF0) ? 0x90 : 0x80;
		max = (c == 0xF4) ? 0x8F : 0xBF;

		if (rest_size < 2 || str[pos + 1] < min || max < str[pos + 1]) {
			*is_valid = 0;
			pos += 1;
		} else if (rest_size < 3 || !is_trail(str[pos + 2])) {
			*is_valid = 0;
			pos += 2;
		} else if (rest_size < 4 || !is_trail(str[pos + 3])) {
			*is_valid = 0;
			pos += 3;
		} else {
			*code_point = ((str[pos]     &  0x7) << 18)
					   | ((str[pos + 1] & 0x3F) << 12)
					   | ((str[pos + 2] & 0x3F) << 6)
					   |  (str[pos + 3] & 0x3F);
			*is_valid = 1;
			pos += 4;
		}

	} else {
		*is_valid = 0;
		pos += 1;
	}

	*cursor = pos;

	return 1;
}

//FIXME, kerning and glyph cache!
GFB_TEXT(gfb_soft_text) {
	FT_GlyphSlot slot = gfb_fontstore[fontid]->glyph;
	FT_Vector pen;

	/* set up matrix */
	/*
	FT_Matrix matrix;
	double angle = 0;//6.25;
	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );
	*/

	/* the pen position in 26.6 cartesian space coordinates; */
	/* start at (x,y) relative to the upper left corner  */
	pen.x = x * 64;
	pen.y = y * 64;

	gfb_rect_t destrect;
	gfb_rect_t srcrect;

	size_t n;
	for ( n = 0; n < count; n++ ) {
		/* set transformation */
		FT_Set_Transform( gfb_fontstore[fontid], /*&matrix*/ 0, &pen );

		/* load glyph image into the slot (erase previous one) */
		FT_Error error = FT_Load_Char( gfb_fontstore[fontid], text[n], FT_LOAD_RENDER);// | FT_LOAD_FORCE_AUTOHINT );
		if ( error ) continue; /* ignore errors */

		if (slot->bitmap.width > 0 && slot->bitmap.rows > 0) {
			/* now, draw to our target surface (convert position) */
			destrect.x = pen.x / 64;
			destrect.y = (pen.y / 64) - (slot->metrics.horiBearingY / 64);
			destrect.w = slot->bitmap.width;
			destrect.h = slot->bitmap.rows;

			srcrect.x = 0;
			srcrect.y = 0;
			srcrect.w = slot->bitmap.width;
			srcrect.h = slot->bitmap.rows-1;

			gfb_ftbitmapblit( psurface, &destrect, &slot->bitmap, &srcrect, colorf, colorb);
		}

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	return GFB_OK;
}

/** Map of software drawing operations. */
gfb_devop_t gfb_soft_devops = {
	.putpixel       = gfb_soft_putpixel,
	.getpixel       = gfb_soft_getpixel,
	.blit           = gfb_soft_blit,
	.flip           = gfb_soft_flip,
	.clear          = gfb_soft_clear,
	.line           = gfb_soft_line,
	.rectangle      = gfb_soft_rectangle,
	.circle         = gfb_soft_circle,
	.filledrectangle= gfb_soft_filledrectangle,
	.filledcircle   = gfb_soft_filledcircle,
	.polygon		= gfb_soft_polygon,
	.floodfill		= gfb_soft_floodfill,
	.text			= gfb_soft_text
};


///////////////////////////////////////////////////////////////////////////////////////////////////


int gfb_surface_create(gfb_surface_t **ppsurface, int width, int height, gfb_pixelformat_id_t format, gfb_flag_id_t flags, uint8_t *ppixels, gfb_devop_t *pdevop) {
	//Basic argument check.
	if (ppsurface == NULL || width < 0 || height < 0) return GFB_EARGUMENT;

	//User must provide pointer to pixel buffer if pre-allocate flag is NOT set.
	if (!(flags & GFB_PREALLOCATE) && ppixels == NULL) return GFB_EARGUMENT;

	//Allocate for the surface.
	(*ppsurface) = calloc(1, sizeof(gfb_surface_t));
	if ((*ppsurface) == NULL) return GFB_ENOMEM;

	//Pre-allocate pixel buffer if flagged so.
	if (flags & GFB_PREALLOCATE) {
	    size_t size = (width * height * gfb_pixelformats[format].bytesperpixel);

		if (flags & GFB_DOUBLEBUFFER)
			size *= 2;

	    (*ppsurface)->ppixelmemory = calloc(1, size);
	    if ((*ppsurface)->ppixelmemory == NULL) {
	        free(*ppsurface);
	        *ppsurface = NULL;
	        return GFB_ENOMEM;
	    }

	    (*ppsurface)->ppixels = (*ppsurface)->ppixelmemory;

		if (flags & GFB_DOUBLEBUFFER) {
			(*ppsurface)->pbuffer = (*ppsurface)->ppixels + (size / 2);
		} else {
			(*ppsurface)->pbuffer = (*ppsurface)->ppixels;
		}
	} else {
	    //User provided pixel buffer.
	    (*ppsurface)->ppixelmemory = ppixels;
	    (*ppsurface)->ppixels = ppixels;

		if (flags & GFB_DOUBLEBUFFER) {
			size_t size = (width * height * gfb_pixelformats[format].bytesperpixel);
			(*ppsurface)->pbuffer = (uint8_t *)ppixels + size;
		} else {
			(*ppsurface)->pbuffer = (*ppsurface)->ppixels;
		}
	}

	(*ppsurface)->flags = flags;

	(*ppsurface)->pformat = &gfb_pixelformats[format];

	(*ppsurface)->w = width;
	(*ppsurface)->h = height;

	(*ppsurface)->cliprect.x = 0;
	(*ppsurface)->cliprect.y = 0;
	(*ppsurface)->cliprect.w = width;
	(*ppsurface)->cliprect.h = height;

	(*ppsurface)->pitch = (width * gfb_pixelformats[format].bytesperpixel);

	(*ppsurface)->prowoffsets = calloc(1, sizeof(uint32_t) * height);
	if ((*ppsurface)->prowoffsets == NULL) {
		if (flags & GFB_PREALLOCATE) {
			free((*ppsurface)->ppixels);
		}
		free(*ppsurface);
		*ppsurface = NULL;
		return GFB_ENOMEM;
	}

	(*ppsurface)->pcoloffsets = calloc(1, sizeof(uint32_t) * width);
	if ((*ppsurface)->pcoloffsets == NULL) {
		if (flags & GFB_PREALLOCATE) {
			free((*ppsurface)->ppixels);
		}
		free((*ppsurface)->prowoffsets);
		free(*ppsurface);
		*ppsurface = NULL;
		return GFB_ENOMEM;
	}

	for (int i = 0; i < height; i++) {
		(*ppsurface)->prowoffsets[i] = (*ppsurface)->pitch * i;
	}

	for (int i = 0; i < width; i++) {
		(*ppsurface)->pcoloffsets[i] = gfb_pixelformats[format].bytesperpixel * i;
	}
	if (pdevop != NULL) {
		(*ppsurface)->op = pdevop;
	} else {
		(*ppsurface)->op = &gfb_soft_devops;
	}

	return GFB_OK;
}

void gfb_surface_destroy(gfb_surface_t **ppsurface) {
	if (ppsurface == NULL || *ppsurface == NULL) return; //Already freed.

	if (*ppsurface != NULL) {
		if ((*ppsurface)->flags & GFB_PREALLOCATE && (*ppsurface)->ppixelmemory != NULL) {
			free((*ppsurface)->ppixelmemory);
			(*ppsurface)->ppixelmemory = NULL;
			(*ppsurface)->ppixels = NULL;
			(*ppsurface)->pbuffer = NULL;
		}
		if ((*ppsurface)->prowoffsets != NULL) {
			free((*ppsurface)->prowoffsets);
			(*ppsurface)->prowoffsets = NULL;
		}
		if ((*ppsurface)->pcoloffsets != NULL) {
			free((*ppsurface)->pcoloffsets);
			(*ppsurface)->pcoloffsets = NULL;
		}
	}

	*ppsurface = NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////////////


int gfb_surface_load_bmp3(gfb_surface_t **ppsurface, gfb_pixelformat_id_t format, gfb_flag_id_t flags, uint8_t *ppixels, gfb_devop_t *pdevop, const char *pzpath) {
	FILE *F = fopen(pzpath, "r");
	if (F == NULL) {
	    return GFB_EFILEOPEN;
	}

	fseek(F, 0, SEEK_END);
	size_t filesize = ftell(F);
	fseek(F, 0, SEEK_SET);

	if (filesize < sizeof(gfb_bmpv3_header_t)) {
	    fclose(F);
	    return GFB_EFILEREAD;
	}

	uint8_t *fdata = calloc(1, filesize);
	if (fdata == NULL) {
	    fclose(F);
	    return GFB_ENOMEM;
	}

	size_t readsize = fread(fdata, sizeof(gfb_bmpv3_t), 1, F);
	if (readsize < 1) {
		fclose(F);
	    free(fdata);
	    return GFB_EFILEREAD;
	}

	//Access the byte buffer as a bitmap structure.
	gfb_bmpv3_t *pbmp = (gfb_bmpv3_t *)fdata;

	if (
		//Header marker is two consequtive bytes: 'B' and 'M'.
	    (pbmp->header.bm[0] != 'B' || pbmp->header.bm[1] != 'M')
	    || pbmp->dib.size != 40 //Version 3 DIB is 40 bytes.
	) {
		fclose(F);
	    free(fdata);
	    return GFB_EFILEREAD;
	}

	int height = abs(pbmp->dib.height);

	//Create a new surface the same size as the bitmap.
	gfb_surface_create(ppsurface, pbmp->dib.width, height, format, flags, ppixels, pdevop);
	if ((*ppsurface) == NULL) {
		fclose(F);
	    free(fdata);
	    return GFB_ENOMEM;
	}

	//Rows in BMP files are padded to 4 bytes.
	uint32_t rowsize = gfb_round4(pbmp->dib.width * (pbmp->dib.bpp / 8));

	size_t fileoff = pbmp->header.offset;	//To seek between rows in file.
	size_t memoff = 0;						//To seek between pixels in surface memory.
	size_t memrowoff = 0;					//To seek between pixel rows in surface memory.
	size_t memrowstride = (*ppsurface)->w * (*ppsurface)->pformat->bytesperpixel;

	//If height is positive then pixel rows are ordered bottom to top.
	//Start at the first byte of the last pixel row in the surface and move backwards in memory.
	if (pbmp->dib.height > 0) {
		memoff = (((*ppsurface)->w * (*ppsurface)->h) * (*ppsurface)->pformat->bytesperpixel) - memrowstride;
		memrowoff = memoff;
		memrowstride = -memrowstride;
	}

	gfb_bmpv3_pixel24_t pixel;

	int x, y;
	for (y = 0; y < height; y++) {
	    //Seek to start of row.
	    fseek(F, fileoff, SEEK_SET);
	    if (feof(F)) {
	        gfb_surface_destroy(ppsurface);
	        *ppsurface = NULL;
			fclose(F);
			free(fdata);
	        return GFB_EFILEREAD;
	    }

	    //Convert pixels in row one by one.
	    for (x = 0; x < pbmp->dib.width; x++) {
	        //Read one 3 byte pixel from file.
	        if (fread(&pixel, sizeof(gfb_bmpv3_pixel24_t), 1, F) < 1) {
	            gfb_surface_destroy(ppsurface);
	            *ppsurface = NULL;
				fclose(F);
				free(fdata);
	            return GFB_EFILEREAD;
	        }

	        //Write pixel converted to destination pixel format.
	        gfb_color_t color = gfb_maprgba(*ppsurface, pixel.r, pixel.g, pixel.b, 0xff);
	        memcpy((*ppsurface)->pbuffer + memoff, &color, (*ppsurface)->pformat->bytesperpixel);

	        //Advance to next pixel within surface memory.
	        memoff += (*ppsurface)->pformat->bytesperpixel;
	    }

	    //Advance to next row within file.
	    fileoff += rowsize;
		//Advance to next (or previous) row in surface memory.
		memrowoff += memrowstride;
		memoff = memrowoff;
	}

	fclose(F);
	free(fdata);

	return GFB_OK;
}

int gfb_surface_save_bmp3(gfb_surface_t *psurface, const char *pzpath) {
	gfb_bmpv3_t bmp = {
		.header = {
			.bm = {'B', 'M'},
			.filesize = 0, //Set later.
			.reserved1 = 0,
			.reserved2 = 0,
			.offset = sizeof(gfb_bmpv3_t)
		},
		.dib = {
			.size = 40, //BMP version 3 size is 40 bytes.
			.width = psurface->w,
			.height = -psurface->h, //Progress from top to bottom.
			.colorplanes = 1,
			.bpp = 24,
			.compression = 0,
			.imagesize = 0,
			.res_horizontal = 0,
			.res_vertical = 0,
			.ncolors = 0,
			.nicolors = 0
		}
	};

	//3 bytes per pixel but row must be multiple of 4.
	uint32_t rowsize = gfb_round4(bmp.dib.width * 3);

	int height = abs(bmp.dib.height);

	bmp.header.filesize = sizeof(bmp) + rowsize * height;
	bmp.dib.imagesize = rowsize * height;

	FILE *F = fopen(pzpath, "w+");
	if (F == NULL) {
		return GFB_EFILEOPEN;
	}

	//Write header.
	int rc = fwrite(&bmp, sizeof(gfb_bmpv3_t), 1, F);
	if (rc != 1) {
		fclose(F);
		return GFB_EFILEWRITE;
	}

	//How many bytes to write after each pixel line to pad byte count to 4.
	uint32_t padding = 0;
	uint32_t padlen = rowsize - (bmp.dib.width * 3);
	gfb_bmpv3_pixel24_t pixel;

	int x, y;
	for (y = 0; y < psurface->h; y++) {
		for (x = 0; x < psurface->w; x++) {
			gfb_getpixel(psurface, x, y, NULL, &pixel.r, &pixel.g, &pixel.b);
			if (fwrite(&pixel, sizeof(pixel), 1, F) != 1) {
				fclose(F);
				return GFB_EFILEWRITE;
			}
		}

		//Write out padding if any.
		if (padlen > 0) {
			if (fwrite(&padding, padlen, 1, F) != 1) {
				fclose(F);
				return GFB_EFILEWRITE;
			}
		}
	}

	fclose(F);

	return GFB_OK;
}

int gfb_surface_save_raw(gfb_surface_t *psurface, const char *pzpath) {
	(void)psurface;
	(void)pzpath;
	return GFB_ENOTSUPPORTED;
}

///////////////////////////////////////////////////////////////////////////////////////////////////


gfb_color_t gfb_maprgba(gfb_surface_t *psurface, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
	if (psurface == NULL) return GFB_EARGUMENT;

	switch (psurface->pformat->id) {
	    case GFB_PIXELFORMAT_RGB16: //5.5.5.0.1
	        return GFB_MAP_PIXELFORMAT_16BIT_RGB(red, green, blue);
	    case GFB_PIXELFORMAT_RGB24: //8.8.8.0.0
	        return GFB_MAP_PIXELFORMAT_24BIT_RGB(red, green, blue);
	    case GFB_PIXELFORMAT_RGB32: //8.8.8.0.8
	        return GFB_MAP_PIXELFORMAT_32BIT_RGB(red, green, blue);
	    case GFB_PIXELFORMAT_ARGB32://8.8.8.8.0
	        return GFB_MAP_PIXELFORMAT_32BIT_ARGB(alpha, red, green, blue);

	    default:
	        return 0;
	}

	return 0;
}

int gfb_setcliprect(gfb_surface_t *psurface, gfb_rect_t *prect) {
	if (psurface == NULL) return GFB_EARGUMENT;

	//Copy or resize cliprect.
	if (prect == NULL) {
	    psurface->cliprect.x = 0;
	    psurface->cliprect.y = 0;
	    psurface->cliprect.w = psurface->w;
	    psurface->cliprect.h = psurface->h;
	} else {
	    memcpy(&psurface->cliprect, prect, sizeof(gfb_rect_t));
	}

	return GFB_OK;
}

int gfb_setcolorkey(gfb_surface_t *psurface, gfb_color_t colorkey) {
	if (psurface == NULL) return GFB_EARGUMENT;

	psurface->colorkey = colorkey;
	psurface->flags |= GFB_SRCCOLORKEY;

	return GFB_OK;
}


int gfb_setalpha(gfb_surface_t *psurface, uint8_t alpha) {
	if (psurface == NULL) return GFB_EARGUMENT;

	psurface->alpha = alpha;
	psurface->flags |= GFB_ALPHABLEND;

	return GFB_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////////////


int gfb_putpixel(gfb_surface_t *psurface, int x, int y, gfb_color_t color) {
	if (psurface == NULL || x < psurface->cliprect.x || y < psurface->cliprect.y || x >= (psurface->cliprect.x + psurface->cliprect.w) || y >= (psurface->cliprect.y + psurface->cliprect.h)) return GFB_EARGUMENT;
	return psurface->op->putpixel(psurface, x, y, color);
}

int gfb_getpixel(gfb_surface_t *psurface, int x, int y, uint8_t *palpha, uint8_t *pred, uint8_t *pgreen, uint8_t *pblue) {
	if (psurface == NULL || x < psurface->cliprect.x || y < psurface->cliprect.y || x >= (psurface->cliprect.x + psurface->cliprect.w) || y >= (psurface->cliprect.y + psurface->cliprect.h)) return 0;
	gfb_color_t color = psurface->op->getpixel(psurface, x, y);

	if (palpha != NULL) *palpha = (uint8_t)((color & psurface->pformat->amask) >> psurface->pformat->ashift);
	if (pred   != NULL) *pred   = (uint8_t)((color & psurface->pformat->rmask) >> psurface->pformat->rshift);
	if (pgreen != NULL) *pgreen = (uint8_t)((color & psurface->pformat->gmask) >> psurface->pformat->gshift);
	if (pblue  != NULL) *pblue  = (uint8_t)((color & psurface->pformat->bmask) >> psurface->pformat->bshift);

	return GFB_OK;
}

int gfb_blit(gfb_surface_t *pdest, gfb_rect_t *pdestrect, gfb_surface_t *psource, gfb_rect_t *psourcerect) {
	if (pdest == NULL || psource == NULL) return GFB_EARGUMENT;

	gfb_rect_t sr = { .x = psource->cliprect.x, .y = psource->cliprect.y, .w = psource->cliprect.w, .h = psource->cliprect.h };
	gfb_rect_t dr = { .x = pdest->cliprect.x, .y = pdest->cliprect.y, .w = pdest->cliprect.w, .h = pdest->cliprect.h };

	if (pdestrect != NULL) {
	    memcpy(&dr, pdestrect, sizeof(gfb_rect_t));
	}

	if (psourcerect != NULL) {
	    memcpy(&sr, psourcerect, sizeof(gfb_rect_t));
	}

	sr = gfb_cliprect(&sr, &psource->cliprect);
	dr = gfb_cliprect(&dr, &pdest->cliprect);

	return pdest->op->blit(pdest, &dr, psource, &sr);
}

int gfb_flip(gfb_surface_t *psurface) {
	if (psurface == NULL || psurface->ppixels == NULL || psurface->pbuffer == NULL) return GFB_EARGUMENT;
	return psurface->op->flip(psurface);
}

int gfb_clear(gfb_surface_t *psurface) {
	if (psurface == NULL) return GFB_EARGUMENT;
	return psurface->op->clear(psurface);
}

int gfb_line(gfb_surface_t *psurface, int x1, int y1, int x2, int y2, gfb_color_t color) {
	if (psurface == NULL) return GFB_EARGUMENT;
	x1 = gfb_clampi(x1, psurface->cliprect.x, psurface->cliprect.x + psurface->cliprect.w);
	x2 = gfb_clampi(x2, psurface->cliprect.x, psurface->cliprect.x + psurface->cliprect.w);
	y1 = gfb_clampi(y1, psurface->cliprect.y, psurface->cliprect.y + psurface->cliprect.h);
	y2 = gfb_clampi(y2, psurface->cliprect.y, psurface->cliprect.y + psurface->cliprect.h);
	return psurface->op->line(psurface, x1, y1, x2, y2, color);
}

int gfb_rectangle(gfb_surface_t *psurface, gfb_rect_t *prect, gfb_color_t color) {
	if (psurface == NULL) return GFB_EARGUMENT;

	gfb_rect_t rect;

	if (prect != NULL) {
	    rect = gfb_cliprect(prect, &psurface->cliprect);
	} else {
	    rect.x = psurface->cliprect.x;
	    rect.w = psurface->cliprect.w;
	    rect.y = psurface->cliprect.y;
	    rect.h = psurface->cliprect.h;
	}

	return psurface->op->rectangle(psurface, &rect, color);
}

int gfb_filledrectangle(gfb_surface_t *psurface, gfb_rect_t *prect, gfb_color_t colorb) {
	if (psurface == NULL) return GFB_EARGUMENT;

	gfb_rect_t rect;

	if (prect != NULL) {
	    rect = gfb_cliprect(prect, &psurface->cliprect);
	} else {
	    rect.x = psurface->cliprect.x;
	    rect.w = psurface->cliprect.w;
	    rect.y = psurface->cliprect.y;
	    rect.h = psurface->cliprect.h;
	}

	return psurface->op->filledrectangle(psurface, &rect, colorb);
}

int gfb_circle(gfb_surface_t *psurface, int x, int y, int radius, gfb_color_t color) {
	int t, l, r, b;

	if (psurface == NULL) return GFB_EARGUMENT;

	t = y - radius;
	b = y + radius;
	l = x - radius;
	r = x + radius;

	if (   (l < psurface->cliprect.x || r >= (psurface->cliprect.x + psurface->cliprect.w))
	    || (t < psurface->cliprect.y || b >= (psurface->cliprect.y + psurface->cliprect.h))
	) return GFB_EARGUMENT;

	return psurface->op->circle(psurface, x, y, radius, color);
}

int gfb_filledcircle(gfb_surface_t *psurface, int x, int y, int radius, gfb_color_t colorf, gfb_color_t colorb) {
	if (
		psurface == NULL
		|| ((x - radius) < psurface->cliprect.x || (x + radius) >= (psurface->cliprect.x + psurface->cliprect.w))
		|| ((y - radius) < psurface->cliprect.y || (y + radius) >= (psurface->cliprect.y + psurface->cliprect.h))
	) return GFB_EARGUMENT;

	return psurface->op->filledcircle(psurface, x, y, radius, colorf, colorb);
}

int gfb_polygon(struct gfb_surface *psurface, gfb_point_t *ppoints, size_t count, gfb_color_t color) {
	if (psurface == NULL || ppoints == NULL || count < 3) {
		return GFB_EARGUMENT;
	}

	return psurface->op->polygon(psurface, ppoints, count, color);
}

int gfb_floodfill(gfb_surface_t *psurface, int x, int y, gfb_color_t color) {
	if (psurface == NULL || x < psurface->cliprect.x || x >= (psurface->cliprect.x + psurface->cliprect.w) || y < psurface->cliprect.y || y >= (psurface->cliprect.y + psurface->cliprect.h)) {
		return GFB_EARGUMENT;
	}

	psurface->op->floodfill(psurface, x, y, color);
	return GFB_OK;
}

gfb_font_id gfb_ttf_load_memory(uint8_t *pttf, size_t ttfsize) {
	if (pttf == NULL || ttfsize == 0) {
		return GFB_EARGUMENT;
	}

	//Find free slot in the font store.
	int i;
	for (i = 0; i < MAX_GFB_FONT; i++) {
		if (gfb_fontstore[i] == NULL) {
			break;
		}
	}

	if (i < 0 || i >= MAX_GFB_FONT || gfb_fontstore[i] != NULL) {
		//All slots occupied.
		return GFB_ERROR;
	}

	//Load font into library state.
	FT_Error e = FT_New_Memory_Face(libft2, pttf, ttfsize, 0, &gfb_fontstore[i]);

	if ( e ) {
		gfb_fontstore[i] = NULL;
		return GFB_ERROR;
	}

	e = FT_Set_Char_Size(
		gfb_fontstore[i],	/* Handle to face object.			*/
		0,					/* Char_width in 1/64th of points.	*/
		32*64,				/* Char_height in 1/64th of points.	*/
		72,				/* Horizontal device resolution.	*/
		72					/* Vertical device resolution.		*/
	);

	if (e) {
		fprintf(stderr, "Error %d setting font size.\n", e);
	}

	return i;
}

//FIXME, solve this buffering
static uint16_t text[1024];

int gfb_textu(gfb_surface_t *psurface, gfb_font_id fontid, uint8_t ptsize, int x, int y, uint16_t *punicode, size_t count, gfb_color_t colorf, gfb_color_t colorb) {
	if (
		   psurface == NULL
		|| fontid >= MAX_GFB_FONT
		|| gfb_fontstore[fontid] == NULL
		|| ptsize < 1
		|| punicode == NULL
		|| count == 0
	) {
		return GFB_EARGUMENT;
	}

	/* FIXME using 75dpi */
	FT_Error error = FT_Set_Char_Size( gfb_fontstore[fontid], ptsize * 64, 0, 100, 0 );
	if (error) return GFB_ERROR;

	return psurface->op->text(psurface, fontid, x, y, punicode, count, colorf, colorb);
}

int gfb_text(gfb_surface_t *psurface, gfb_font_id fontid, uint8_t ptsize, int x, int y, char *pzutf8, size_t count, gfb_color_t colorf, gfb_color_t colorb) {
	if (
		   psurface == NULL
		|| fontid >= MAX_GFB_FONT
		|| gfb_fontstore[fontid] == NULL
		|| ptsize < 1
		|| pzutf8 == NULL
		|| count == 0
	) {
		return GFB_EARGUMENT;
	}

	/* FIXME using 75dpi */
	FT_Error error = FT_Set_Char_Size( gfb_fontstore[fontid], ptsize * 64, 0, 100, 0 );
	if (error) return GFB_ERROR;

	size_t cursor = 0;
	int is_valid = 0;
	unsigned int code = 0;
	size_t zlen = strlen(pzutf8);

	unsigned int n = count;
	unsigned int i = 0;

	//Decode UTF-8 string into text[].
	while (utf8_get_next_char((uint8_t *)pzutf8, zlen, &cursor, &is_valid, &code)) {
		if (n == 0) break;

		if (is_valid && code) {
			text[i++] = code;
		}

		if (n > 0) --n;
	}

	return psurface->op->text(psurface, fontid, x, y, &text[0], i, colorf, colorb);
}


///////////////////////////////////////////////////////////////////////////////////////////////////


int gfb_initialize(void) {

	//Initialize library state.
	if (libft2 == NULL) {
		FT_Error e = FT_Init_FreeType( &libft2 );
		if ( e ) {
			libft2 = NULL;
			return GFB_ERROR;
		}
	}

	return GFB_OK;
}

void gfb_finalize(void) {
	int i;
	for (i = 0; i < MAX_GFB_FONT; i++) {
		if (gfb_fontstore[i] != NULL) {
			FT_Done_Face(gfb_fontstore[i]);
			gfb_fontstore[i] = NULL;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////


static const char const * gfb_copyright_text = "\n"
"libgfb - Library of Graphic Routines for Frame Buffers.\n"
"Copyright (C) 2016  Kari Sigurjonsson\n"
"\n"
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU Lesser General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU Lesser General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU Lesser General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
"\n";

const char *gfb_about(void) {
	return gfb_copyright_text;
}

/** @} */

