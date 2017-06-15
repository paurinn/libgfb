/**
@addtogroup gfb
@{

Libgfb interface for Lua.
*/
#ifndef __LGFB_H__
#define __LGFB_H__

/**

*/
LUALIB_API int luaopen_libgfb (lua_State *L);

/**
Push the libgfb Lua interface.
*/
void PushLGfb(lua_State *L);

#endif //!__LGFB_H__
/** @} */

