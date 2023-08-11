#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

int userdata_pointer(lua_State* L) {
  lua_pushinteger(L, (uint64_t)lua_touserdata(L, 1));
  return 1;
}

static void* getuserdatapointer(lua_State* L, int idx, int* size) {
  void* p = lua_touserdata(L, idx);
  if (lua_getiuservalue(L, idx, 1) == LUA_TNUMBER) {
    p = *(void**)p;
    if (size) *size = (int)lua_tointeger(L, -1);
  } else if (size) *size = (int)lua_rawlen(L, idx);
  lua_pop(L, 1);
  return p;
}

int userdata_string(lua_State* L) { //userdata:string(size, offset=0) / userdata:string("str", offset=0)
  int len = 0;
  char* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_type(L, 2) == LUA_TSTRING) { //write
    int size = (int)lua_rawlen(L, 2);
    if ((size <= 0) || (offset + size > len)) size = len - offset;
    memcpy(&p[offset], lua_tostring(L, 2), size);
    lua_pushvalue(L, 1);
  }
  else {  //read
    int size = 0;
    if (lua_isinteger(L, 2)) size = (int)lua_tointeger(L, 2);
    if ((size <= 0) || (offset + size > len)) size = len - offset;
    lua_pushlstring(L, &p[offset], size);
  }
  return 1;
}

int userdata_int(lua_State* L) { //userdata:int([size][, offset][, value])
  int len = 0;
  uint8_t* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int64_t v = lua_tointeger(L, 4);
  int size = len;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_isinteger(L, 2)) size = (int)lua_tointeger(L, 2);
  if (size > 8) size = 8;
  if ((size <=0) || (size*(offset+1) > len)) return 0;
  if (lua_isinteger(L, 4)) {
    memcpy(&p[size * offset], &v, size);
    lua_pushvalue(L, 1);
  }
  else {
    memcpy(&v, &p[size * offset], size);
    lua_pushinteger(L, v);
  }
  return 1;
}

int userdata_bool(lua_State* L) { //userdata:bool([size][, offset][, value])
  int len = 0;
  uint8_t* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int size = len;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_isinteger(L, 2)) size = (int)lua_tointeger(L, 2);
  if (size > 8) size = 8;
  if ((size <= 0) || (size * (offset + 1) > len)) return 0;
  if (lua_isboolean(L, 4)) {
    int64_t v = lua_toboolean(L, 4);
    memcpy(&p[size * offset], &v, size);
    lua_pushvalue(L, 1);
  }
  else {
    int64_t v = 0;
    memcpy(&v, &p[size * offset], size);
    lua_pushboolean(L, v!=0);
  }
  return 1;
}

int userdata_float(lua_State* L) {  //userdata:float([, offset][, value])
  int len = 0;
  float* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 2);
  if (sizeof(float) * (offset + 1) > len) return 0;
  if (lua_isnumber(L, 3)) {
    p[offset] = (float)lua_tonumber(L, 3);
    lua_pushvalue(L, 1);
  }
  else {
    lua_pushnumber(L, p[offset]);
  }
  return 1;
}

int userdata_double(lua_State* L) {
  int len = 0;
  double* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 2);
  if (sizeof(double) * (offset + 1) > len) return 0;
  if (lua_isnumber(L, 3)) {
    p[offset] = lua_tonumber(L, 3);
    lua_pushvalue(L, 1);
  }
  else {
    lua_pushnumber(L, p[offset]);
  }
  return 1;
}

static uint64_t read_bits(const uint64_t* data, size_t offset, uint8_t len) {
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

int userdata_bits(lua_State* L) {  //userdata:bits(offset[, size=1][,data: write if present, otherwise read])
  int len = 0;
  uint64_t* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int size = 1;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_isinteger(L, 2)) size = (int)lua_tointeger(L, 2);
  if (size > 64) size = 64;
  if ((size <= 0) || (size * (offset + 1) > len*8)) return 0;
  if (lua_isinteger(L, 4)) {
    write_bits(p, offset, size, lua_tointeger(L, 4));
    lua_pushvalue(L, 1);
  }
  else {
    lua_pushinteger(L, read_bits(p, offset, size));
  }
  return 1;
}

int userdata_pack(lua_State* L) {
  int len = 0;
  uint64_t* p = getuserdatapointer(L, 1, &len);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "pack");
  lua_insert(L, 2);  //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...  [-1]=string
  lua_pop(L, 1);     //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...
  lua_call(L, lua_gettop(L) - 2, 1);
  const char* s = lua_tostring(L, -1);
  if (len < lua_rawlen(L, -1)) len = (int)lua_rawlen(L, -1);
  if (s) memcpy(p, s, len);
  lua_pop(L, 1);        //remove result of string.pack and return self
  return 1;
}

int userdata_unpack(lua_State* L) {
  int len = 0;
  uint8_t* p = getuserdatapointer(L, 1, &len);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "unpack");
  lua_insert(L, 2);     //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,    [-1]=string
  lua_pop(L, 1);        //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,  
  lua_pushlstring(L, p, len);
  lua_insert(L, 4);
  lua_call(L, lua_gettop(L) - 2, 1);
  return 1;
}

int ffi_userdata(lua_State* L) {
  switch (lua_type(L, 1)) {
    case LUA_TNUMBER: {
      int length = (int)lua_tointeger(L, 1);
      int depth = lua_gettop(L);
      
      for (int i = 2; i <= depth; i++) {
        if (LUA_TNUMBER == lua_type(L, i)) length *= (int)lua_tointeger(L, i);
        else {
          depth = i - 1;
          break;
        }
      }
      if (length <= 0) return 0;
      void* data = lua_newuserdatauv(L, length, 0);
      memset(data, 0, length);
    } break;

    case LUA_TSTRING: {
      size_t length = lua_rawlen(L, 1);
      char* data = lua_newuserdatauv(L, length + 1, 0);
      memcpy(data, lua_tostring(L, 1), length + 1);
    } break;

    case LUA_TTABLE: {
      size_t length = lua_rawlen(L, 1);
      uintptr_t* data = lua_newuserdatauv(L, sizeof(uintptr_t) * length, 0);
      for (size_t i = 0; i < length; i++) {
        lua_pushcfunction(L, ffi_userdata);
        lua_rawgeti(L, 1, i + 1);
        lua_call(L, 1, 1);
        data[i] = (uintptr_t)lua_touserdata(L, -1);
      }
      
    } break;

    case LUA_TLIGHTUSERDATA: {
      void* p = lua_touserdata(L, 1);
      void** data = lua_newuserdatauv(L, sizeof(void*), 1);
      lua_pushvalue(L, 2);
      lua_setiuservalue(L, -2, 1);
      *data = p;
    } break;

    case LUA_TUSERDATA: {
      lua_settop(L, 1); //remove other arguments, if any
    } break;
    default: return 0;
  }

  lua_createtable(L, 0, 2); //userdata metatable

  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "__tostring");

  lua_createtable(L, 0, 9);  //__index table
  {
    lua_pushcfunction(L, userdata_pointer);
    lua_setfield(L, -2, "pointer");
    lua_pushcfunction(L, userdata_string);
    lua_setfield(L, -2, "string");
    lua_pushcfunction(L, userdata_int);
    lua_setfield(L, -2, "int");
    lua_pushcfunction(L, userdata_bool);
    lua_setfield(L, -2, "bool");
    lua_pushcfunction(L, userdata_float);
    lua_setfield(L, -2, "float");
    lua_pushcfunction(L, userdata_double);
    lua_setfield(L, -2, "double");
    lua_pushcfunction(L, userdata_bits);
    lua_setfield(L, -2, "bits");
    lua_pushcfunction(L, userdata_pack);
    lua_setfield(L, -2, "pack");
    lua_pushcfunction(L, userdata_unpack);
    lua_setfield(L, -2, "unpack");
  }
  lua_setfield(L, -2, "__index");

  lua_setmetatable(L, -2);
  return 1;
}


/*


#define USERDATA_TUINT1   0x00
#define USERDATA_TUINT2   0x01
#define USERDATA_TUINT4   0x02
#define USERDATA_TUINT8   0x03
#define USERDATA_TINT1    0x10
#define USERDATA_TINT2    0x11
#define USERDATA_TINT4    0x12
#define USERDATA_TINT8    0x13
#define USERDATA_TFLOAT   0x22 
#define USERDATA_TDOUBLE  0x23
#define USERDATA_TSTRING  0x40
#define USERDATA_TPOINTER 0x80


int userdata__index(lua_State* L) {  //array indexing, read
  if (lua_isinteger(L, 2)) {
    uint8_t* p = lua_touserdata(L, 1);
    size_t offset = lua_tointeger(L, 2);
    int dim = 0;
    while (LUA_TNONE != lua_getiuservalue(L, 1, dim++));
    switch(lua_type(L, 4)) {
      case LUA_TNUMBER: 
        lua_getiuservalue(L, 3, 1);
        void* data = lua_touserdata(L, -1);
        size_t size = 1 << (lua_tointeger(L, 4) & 0x03);
//        lua_newuserdatauv(L, ???)

        if (dim <= 1) {  //one dimensional array

        }
        else {

        }
        lua_pushlightuserdata(L, &((uint8_t*)data)[offset * size]);
        lua_setiuservalue(L, -2, 1);

      break;

      case LUA_TTABLE: //indexing of multidimensional array
//        lua_newuserdatauv(L, );
//        lua_rawgeti(L, -1, offset);
//        lua_setiuservalue(L, ? ? ? , 1);
      break;
    }
    return 1;
  }
  return 0;
}

int userdata__newindex(lua_State* L) {  //array indexing, write
  return 0;
}

int userdata_dim(lua_State* L) {
  int i = 3;
  while (LUA_TNONE != lua_getiuservalue(L, 1, i++));
  lua_pop(L, 1); //removel last nil
  return i-4;
}


//uservalues of userdata: [1] - type, [2...] array dimensions
int ffi_userdata(lua_State* L) {
  if (lua_type(L, 1) == LUA_TNUMBER) {
    int length = lua_tointeger(L, 1);
    int depth = lua_gettop(L);
    void* data = 0;
    for (int i = 2; i <= depth; i++) {
      if (LUA_TNUMBER == lua_type(L, i)) length *= lua_tointeger(L, i);
      else {
        depth = i - 1;
        break;
      }
    }
    if (length > 0) {
      data = lua_newuserdatauv(L, length, depth + 2);
      memset(data, 0, length);
    }
    else return 0;

    lua_pushlightuserdata(L, data);       
    lua_setiuservalue(L, -2, 1);

    lua_pushinteger(L, USERDATA_TUINT1);       //element size 1 by default
    lua_setiuservalue(L, -2, 2);

    for (int i = 1; i <= depth; i++) {
      lua_pushvalue(L, lua_tointeger(L, i));
      lua_setiuservalue(L, -2, i+2);
    }
  }

  if (lua_type(L, 1) == LUA_TSTRING) {
    size_t length = lua_rawlen(L, 1);
    char* data = lua_newuserdatauv(L, length + 1, 2);
    memcpy(data, lua_tostring(L, 1), length + 1);
    lua_pushlightuserdata(L, data);
    lua_setiuservalue(L, -2, 1);

    lua_pushinteger(L, USERDATA_TSTRING);       //element size 1 by default
    lua_setiuservalue(L, -2, 2);
  }

  if (lua_type(L, 1) == LUA_TTABLE) {
    size_t length = lua_rawlen(L, 1);
    uintptr_t* data = lua_newuserdatauv(L, sizeof(uintptr_t) * length, 2); //array store pointers also in uservalues to prevent GC
    lua_pushlightuserdata(L, data);
    lua_setiuservalue(L, -2, 1);

    lua_pushvalue(L, 1);          //table type
    lua_setiuservalue(L, -2, 2);
    for (size_t i = 0; i < length; i++) {
      lua_pushcfunction(L, ffi_userdata);
      lua_rawgeti(L, 1, i + 1);
      lua_call(L, 1, 1);
      data[i] = lua_touserdata(L, -1);
    }
  }

  lua_createtable(L, 0, 2); //userdata metatable
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "__tostring");

  lua_createtable(L, 0, 7);  //__index table
  lua_pushcfunction(L, userdata_pointer);
  lua_setfield(L, -2, "pointer");
  lua_pushcfunction(L, userdata_string);
  lua_setfield(L, -2, "string");
  lua_pushcfunction(L, userdata_int);
  lua_setfield(L, -2, "int");
  lua_pushcfunction(L, userdata_bool);
  lua_setfield(L, -2, "bool");
  lua_pushcfunction(L, userdata_float);
  lua_setfield(L, -2, "float");
  lua_pushcfunction(L, userdata_double);
  lua_setfield(L, -2, "double");
  lua_pushcfunction(L, userdata_bits);
  lua_setfield(L, -2, "bits");

//  lua_pushcfunction(L, userdata_pack);
//  lua_setfield(L, -2, "pack");
//  lua_pushcfunction(L, userdata_unpack);
//  lua_setfield(L, -2, "unpack");
//  lua_pushcfunction(L, userdata_dim);
//  lua_setfield(L, -2, "dim");

  lua_createtable(L, 0, 2);  //metatable for __index which is called if no metamethod is found (e.g. for x[1])
  lua_pushcfunction(L, userdata__index);
  lua_setfield(L, -2, "__index");
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__index");

//  lua_pushcfunction(L, userdata__newindex);
//  lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2);
  return 1;
}

*/
