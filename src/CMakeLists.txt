# libgfb - Library of Graphic Routines for Frame Buffers.
# Copyright (C) 2016  Kari Sigurjonsson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.0)
project (libgfb)

set(CMAKE_BUILD_TYPE Debug)

SET(STANDARD_FLAGS "-std=c99 -Werror -Wall -Wextra -Wfatal-errors -D_POSIX_C_SOURCE=200112L -DFT2_BUILD_LIBRARY")

SET(CMAKE_C_FLAGS       "${STANDARD_FLAGS} ${CMAKE_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "${STANDARD_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")

#Use custom linker script.
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${libgfb_SOURCE_DIR}/linker.ld")

#FreeType2 source code folder.
set(FT2_Source_dir "/home/kari/projects/freetype-2.6")

include_directories(
	${FT2_Source_dir}/include
	${FT2_Source_dir}/include/freetype
	${FT2_Source_dir}/include/freetype/config
	${FT2_Source_dir}/include/freetype/config/internal
	${FT2_Source_dir}/include/freetype/config/internal/services
	${FT2_Source_dir}/src
	${FT2_Source_dir}/src/autofit
	${FT2_Source_dir}/src/base
	${FT2_Source_dir}/src/bdf
	${FT2_Source_dir}/src/bzip2
	${FT2_Source_dir}/src/cache
	${FT2_Source_dir}/src/cff
	${FT2_Source_dir}/src/cid
	${FT2_Source_dir}/src/gxvalid
	${FT2_Source_dir}/src/gzip
	${FT2_Source_dir}/src/lzw
	${FT2_Source_dir}/src/otvalid
	${FT2_Source_dir}/src/pcf
	${FT2_Source_dir}/src/pfr
	${FT2_Source_dir}/src/psaux
	${FT2_Source_dir}/src/pshinter
	${FT2_Source_dir}/src/psnames
	${FT2_Source_dir}/src/raster
	${FT2_Source_dir}/src/sfnt
	${FT2_Source_dir}/src/smooth
	${FT2_Source_dir}/src/tools
	${FT2_Source_dir}/src/tools/docmaker
	${FT2_Source_dir}/src/tools/ftfuzzer
	${FT2_Source_dir}/src/tools/ftrandom
	${FT2_Source_dir}/src/truetype
	${FT2_Source_dir}/src/type1
	${FT2_Source_dir}/src/type42
	${FT2_Source_dir}/src/winfonts
	${libgfb_SOURCE_DIR}
)

add_library(libgfb STATIC
	${FT2_Source_dir}/src/base/ftadvanc.c
	${FT2_Source_dir}/src/base/fttrigon.c
	${FT2_Source_dir}/src/base/ftrfork.c
	${FT2_Source_dir}/src/base/ftcalc.c
	${FT2_Source_dir}/src/base/ftoutln.c
	${FT2_Source_dir}/src/base/ftgloadr.c
	${FT2_Source_dir}/src/base/ftglyph.c
	${FT2_Source_dir}/src/base/ftstream.c
	${FT2_Source_dir}/src/base/ftutil.c
	${FT2_Source_dir}/src/base/ftsystem.c
	${FT2_Source_dir}/src/base/ftinit.c
	${FT2_Source_dir}/src/base/ftobjs.c
	#${FT2_Source_dir}/src/base/fthash.c
	${FT2_Source_dir}/src/base/ftbitmap.c
	#${FT2_Source_dir}/src/autofit/afshaper.c
	${FT2_Source_dir}/src/autofit/hbshim.c
	${FT2_Source_dir}/src/autofit/afloader.c
	${FT2_Source_dir}/src/autofit/afhints.c
	${FT2_Source_dir}/src/autofit/afranges.c
	${FT2_Source_dir}/src/autofit/afglobal.c
	${FT2_Source_dir}/src/autofit/afmodule.c
	${FT2_Source_dir}/src/autofit/afdummy.c
	${FT2_Source_dir}/src/autofit/aflatin.c
	${FT2_Source_dir}/src/autofit/afcjk.c
	${FT2_Source_dir}/src/autofit/afindic.c
	${FT2_Source_dir}/src/autofit/afangles.c
	${FT2_Source_dir}/src/autofit/afblue.c
	${FT2_Source_dir}/src/autofit/afwarp.c
	${FT2_Source_dir}/src/truetype/ttdriver.c
	${FT2_Source_dir}/src/truetype/ttgload.c
	${FT2_Source_dir}/src/truetype/ttobjs.c
	${FT2_Source_dir}/src/truetype/ttgxvar.c
	${FT2_Source_dir}/src/truetype/ttpload.c
	${FT2_Source_dir}/src/truetype/ttinterp.c
	${FT2_Source_dir}/src/type1/t1driver.c
	${FT2_Source_dir}/src/type1/t1afm.c
	${FT2_Source_dir}/src/type1/t1load.c
	${FT2_Source_dir}/src/type1/t1objs.c
	${FT2_Source_dir}/src/type1/t1gload.c
	${FT2_Source_dir}/src/type1/t1parse.c
	${FT2_Source_dir}/src/cff/cffdrivr.c
	${FT2_Source_dir}/src/cff/cffgload.c
	${FT2_Source_dir}/src/cff/cffload.c
	${FT2_Source_dir}/src/cff/cffobjs.c
	${FT2_Source_dir}/src/cff/cffcmap.c
	${FT2_Source_dir}/src/cff/cf2ft.c
	${FT2_Source_dir}/src/cff/cffparse.c
	${FT2_Source_dir}/src/cff/cf2font.c
	${FT2_Source_dir}/src/cff/cf2blues.c
	${FT2_Source_dir}/src/cff/cf2intrp.c
	${FT2_Source_dir}/src/cff/cf2error.c
	${FT2_Source_dir}/src/cff/cf2hints.c
	${FT2_Source_dir}/src/cff/cf2read.c
	${FT2_Source_dir}/src/cff/cf2stack.c
	${FT2_Source_dir}/src/cff/cf2arrst.c
	${FT2_Source_dir}/src/cid/cidgload.c
	${FT2_Source_dir}/src/cid/cidload.c
	${FT2_Source_dir}/src/cid/cidobjs.c
	${FT2_Source_dir}/src/cid/cidparse.c
	${FT2_Source_dir}/src/cid/cidriver.c
	${FT2_Source_dir}/src/cid/type1cid.c
	${FT2_Source_dir}/src/pfr/pfr.c
	${FT2_Source_dir}/src/pfr/pfrcmap.c
	${FT2_Source_dir}/src/pfr/pfrdrivr.c
	${FT2_Source_dir}/src/pfr/pfrgload.c
	${FT2_Source_dir}/src/pfr/pfrload.c
	${FT2_Source_dir}/src/pfr/pfrobjs.c
	${FT2_Source_dir}/src/pfr/pfrsbit.c
	${FT2_Source_dir}/src/type42/t42drivr.c
	${FT2_Source_dir}/src/type42/t42objs.c
	${FT2_Source_dir}/src/type42/t42parse.c
	${FT2_Source_dir}/src/type42/type42.c
	${FT2_Source_dir}/src/winfonts/winfnt.c
	${FT2_Source_dir}/src/pcf/pcf.c
	${FT2_Source_dir}/src/pcf/pcfdrivr.c
	${FT2_Source_dir}/src/pcf/pcfread.c
	${FT2_Source_dir}/src/pcf/pcfutil.c
	${FT2_Source_dir}/src/psaux/afmparse.c
	${FT2_Source_dir}/src/psaux/psaux.c
	${FT2_Source_dir}/src/psaux/psauxmod.c
	${FT2_Source_dir}/src/psaux/psconv.c
	${FT2_Source_dir}/src/psaux/psobjs.c
	${FT2_Source_dir}/src/psaux/t1cmap.c
	${FT2_Source_dir}/src/psaux/t1decode.c
	${FT2_Source_dir}/src/psnames/psmodule.c
	${FT2_Source_dir}/src/psnames/psnames.c
	${FT2_Source_dir}/src/psnames/pspic.c
	${FT2_Source_dir}/src/pshinter/pshalgo.c
	${FT2_Source_dir}/src/pshinter/pshglob.c
	${FT2_Source_dir}/src/pshinter/pshinter.c
	${FT2_Source_dir}/src/pshinter/pshmod.c
	${FT2_Source_dir}/src/pshinter/pshpic.c
	${FT2_Source_dir}/src/pshinter/pshrec.c
	${FT2_Source_dir}/src/raster/ftrend1.c
	${FT2_Source_dir}/src/raster/ftraster.c
	${FT2_Source_dir}/src/sfnt/sfdriver.c
	${FT2_Source_dir}/src/sfnt/ttpost.c
	${FT2_Source_dir}/src/sfnt/ttbdf.c
	${FT2_Source_dir}/src/sfnt/ttload.c
	${FT2_Source_dir}/src/sfnt/ttcmap.c
	${FT2_Source_dir}/src/sfnt/sfobjs.c
	${FT2_Source_dir}/src/sfnt/ttmtx.c
	${FT2_Source_dir}/src/sfnt/ttkern.c
	${FT2_Source_dir}/src/sfnt/ttsbit.c
	${FT2_Source_dir}/src/smooth/ftsmooth.c
	${FT2_Source_dir}/src/smooth/ftgrays.c
	${FT2_Source_dir}/src/bdf/bdf.c
	${FT2_Source_dir}/src/gzip/ftgzip.c
	${FT2_Source_dir}/src/lzw/ftlzw.c
	${libgfb_SOURCE_DIR}/libgfb.c
	${libgfb_SOURCE_DIR}/libgfb_k70.c
)

