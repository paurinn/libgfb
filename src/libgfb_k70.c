/*
libgfb - Library of Graphic Routines for Frame Buffers.
Copyright (C) 2016-2017  Kari Sigurjonsson

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
Kinetis K70 Driver

@addtogroup libgfb
@{
*/
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "libgfb.h"

GFB_FLIP(gfb_k70_flip);

GFB_FLIP(gfb_k70_flip) {
	//FIXME, swap graphical window pointer and psurface->ppixel pointer.
	(void)psurface;
	return GFB_OK;
}

/** Map of software drawing operations. */
gfb_devop_t gfb_k70_devops = {
	.putpixel       = gfb_soft_putpixel,
	.getpixel       = gfb_soft_getpixel,
	.blit           = gfb_soft_blit,
	.flip           = gfb_k70_flip,
	.clear          = gfb_soft_clear,
	.line           = gfb_soft_line,
	.rectangle      = gfb_soft_rectangle,
	.circle         = gfb_soft_circle,
	.filledrectangle= gfb_soft_filledrectangle,
	.filledcircle   = gfb_soft_filledcircle,
	.floodfill		= gfb_soft_floodfill,
	.text           = gfb_soft_text,
};

/** @} */

