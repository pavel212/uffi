#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

int userdata_string(lua_State* L) {
  int len = lua_tointeger(L, 2);
  if ((len <= 0) || (len > lua_rawlen(L, 1))) len = lua_rawlen(L, 1);
  lua_pushlstring(L, lua_touserdata(L, 1), len);
  return 1;
}

int userdata_int(lua_State* L) {
  int64_t i = 0;
  int len = lua_rawlen(L, 1);
  int size = len;
  if (lua_isnumber(L, 2)) size = lua_tointeger(L, 2);
  if (size > len) size = len;
  if (size <= 0) return 0;
  if (size > 8) size = 8;
  memcpy(&i, lua_touserdata(L, 1), size);
  lua_pushinteger(L, i);
  return 1;
}

int userdata_boolean(lua_State* L) {
  char* p = lua_touserdata(L, 1);
  if (p == 0) return 0;
  uint64_t i = 0;
  int len = lua_tointeger(L, 2);
  if (len > 8) len = 8;
  if (len <= 0) len = 1;
  memcpy(&i, p, len);
  lua_pushboolean(L, i != 0);
  return 1;
}

int userdata_float(lua_State* L) {
  float* p = lua_touserdata(L, 1);
  return p ? (lua_pushnumber(L, *p), 1) : 0;
}

int userdata_double(lua_State* L) {
  double* p = lua_touserdata(L, 1);
  return p ? (lua_pushnumber(L, *p), 1) : 0;
}

static uint64_t read_bits(uint64_t* data, size_t offset, uint8_t len) {
  data += offset >> 6;
  uint8_t pos = offset & 0x3F;
  uint64_t ret = data[0] >> pos;
  if ((pos + len) > 64) ret |= data[1] << (64 - pos);
  return ret & ((1ULL << len) - 1);
}

static void write_bits(uint64_t* data, size_t offset, uint8_t len, uint64_t value) {
  data += offset >> 6;
  uint8_t pos = offset & 0x3F;
  uint64_t mask = (((1ULL << len) - 1) << pos);
  data[0] &= ~mask;
  data[0] |= (value << pos) & mask;
  if ((pos + len) > 64) {
    len = pos + len - 64;
    uint64_t mask = ((1ULL << len) - 1);
    data[1] &= ~mask;
    data[1] |= (value >> (64 - pos)) & mask;
  }
}

int userdata_bits(lua_State* L) {  //userdata:bit(offset[, size=1][,data write if present otherwise read])
  void* p = lua_touserdata(L, 1);
  int64_t offset = lua_tointeger(L, 2) - 1;
  if ((offset < 0) || (offset >> 6) > lua_rawlen(L, 1)) return 0;
  size_t size = lua_isinteger(L, 3) ? lua_tointeger(L, 3) : 1;
  return lua_isinteger(L, 4) ? (write_bits(p, offset, size, lua_tointeger(L, 4)), 0) : (lua_pushinteger(L, read_bits(p, offset, size)), 1);
}

int userdata_pack(lua_State* L) {
  void* d = lua_touserdata(L, 1);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "pack");
  lua_insert(L, 2);  //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...  [-1]=string
  lua_pop(L, 1);     //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...
  lua_call(L, lua_gettop(L) - 2, 1);
  const char* s = lua_tostring(L, -1);
  if (s) memcpy(d, s, lua_rawlen(L, -1));
  lua_pop(L, 1);        //remove result of string.pack and return self
  return 1;
}

int userdata_unpack(lua_State* L) {
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "unpack");
  lua_insert(L, 2);     //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,    [-1]=string
  lua_pop(L, 1);        //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,  
  lua_pushlstring(L, lua_touserdata(L, 1), lua_rawlen(L, 1));
  lua_insert(L, 4);
  lua_call(L, lua_gettop(L) - 2, 1);
  return 1;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int userdata__index(lua_State* L) {  //array indexing
  if (lua_isinteger(L, 2)) {
    uint8_t* p = lua_touserdata(L, 1);
    size_t offset = lua_tointeger(L, 2);
    int dim = 0;
    while (LUA_TNONE != lua_getiuservalue(L, 1, dim++));
    uint8_t* ud = lua_newuserdatauv(L, 0, dim + 2);  
    switch(lua_tointeger(L, 3)) {
      case 'i': break;
      case 't': break;
      case 's': break;
    }
    return 1;
  }
  return 0;
}

int userdata_dim(lua_State* L) {
  int i = 3;
  while (LUA_TNONE != lua_getiuservalue(L, 1, i++));
  lua_pop(L, 1); //removel last nil
  return i-4;
}

//uservalues of userdata: [1] - type, [2] - sizeof element in bytes, [3...] array dimensions
int ffi_userdata(lua_State* L) {
  if (lua_type(L, 1) == LUA_TNUMBER) {
    int len = lua_tointeger(L, 1);
    int depth = lua_gettop(L);
    for (int i = 2; i <= depth; i++) {
      if (LUA_TNUMBER == lua_type(L, i)) len *= lua_tointeger(L, i);
      else {
        depth = i - 1;
        break;
      }
    }
    if (len > 0) {
      memset(lua_newuserdatauv(L, len, depth + 2), 0, len);
    }
    else return 0;

    lua_pushinteger(L, 'i');      //array type, integer by default
    lua_setiuservalue(L, -2, 1);

    lua_pushinteger(L, 1);       //element size 1 by default
    lua_setiuservalue(L, -2, 2);

    for (int i = 1; i <= depth; i++) {
      lua_pushvalue(L, i);
      lua_setiuservalue(L, -2, i + 2);
    }
  }

  if (lua_type(L, 1) == LUA_TSTRING) {
    size_t len = lua_rawlen(L, 1);
    memcpy(lua_newuserdatauv(L, len, 3), lua_tostring(L, 1), len);
    lua_pushinteger(L, 's');      //userdata type string
    lua_setiuservalue(L, -2, 1);
    lua_pushinteger(L, 1);        //1 byte
    lua_setiuservalue(L, -2, 2);
    lua_pushinteger(L, len);
    lua_setiuservalue(L, -2, 3);
  }

  if (lua_type(L, 1) == LUA_TTABLE) {
    size_t len = lua_rawlen(L, 1);
    uintptr_t* data = lua_newuserdatauv(L, sizeof(uintptr_t) * len, len + 3); //array store pointers also in uservalues to prevent GC
    lua_pushinteger(L, 't');      //table type string
    lua_setiuservalue(L, -2, 1);
    lua_pushinteger(L, sizeof(void*));        //8 bytes
    lua_setiuservalue(L, -2, 2);
    lua_pushinteger(L, len);
    lua_setiuservalue(L, -2, 3);
    for (size_t i = 0; i < len; i++) {
      lua_pushcfunction(L, ffi_userdata);
      lua_rawgeti(L, 1, i + 1);
      lua_call(L, 1, 1);
      data[i] = lua_touserdata(L, -1);
      lua_setiuservalue(L, -2, i + 4);
    }
  }

  lua_createtable(L, 0, 2); //userdata metatable
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "__tostring");

  lua_createtable(L, 0, 7);  //__index table
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "string");
  lua_pushcfunction(L, userdata_int);
  lua_setfield(L, -2, "int");
  lua_pushcfunction(L, userdata_boolean);
  lua_setfield(L, -2, "bool");
  lua_pushcfunction(L, userdata_float);
  lua_setfield(L, -2, "float");
  lua_pushcfunction(L, userdata_double);
  lua_setfield(L, -2, "double");
  lua_pushcfunction(L, userdata_pack);
  lua_setfield(L, -2, "pack");
  lua_pushcfunction(L, userdata_unpack);
  lua_setfield(L, -2, "unpack");
  lua_pushcfunction(L, userdata_bits);
  lua_setfield(L, -2, "bits");
  lua_pushcfunction(L, userdata_dim);
  lua_setfield(L, -2, "dim");

  lua_createtable(L, 0, 1);  //metatable for __index which is called if no metamethod is found (e.g. for x[1])
  lua_pushcfunction(L, userdata__index);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);

  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  return 1;
}