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
Shared memory descriptor.
*/
#ifndef __SHMDATA_H__
#define __SHMDATA_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GDISPW        640	/**< Frame buffer width in pixels. */
#define GDISPH        480	/**< Frame buffer height in pixels. */
#define BYTESPERPIXEL   4	/**< Number of bytes per pixel. */

/** Data residing in shared memory (/dev/shm/gfb). */
struct shmdata_t {
	uint32_t width;			/**< Width in pixels. */
	uint32_t height;		/**< Height in pixels. */
	uint32_t bpp;			/**< Bytes per pixel (1-4). */
	uint32_t count;			/**< Running change counter. */
	uint32_t keyflag;		/**< Hack to accept keypresses. */
	uint32_t key;			/**< Current key. */

	/** Pixel data. */
	uint8_t pixels[GDISPW * GDISPH * BYTESPERPIXEL];
};

#ifdef __cplusplus
}
#endif

#endif //!__SHMDATA_H__

