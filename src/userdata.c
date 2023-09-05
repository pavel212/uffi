#include <stdint.h>
#include "lua.h"
#include "lauxlib.h"

//void* (*mem_cpy)(void*, const void*, size_t);
void* mem_cpy(void*, const void*, size_t);


static void* getuserdatapointer(lua_State* L, int idx, int* size) {
  void* p = lua_touserdata(L, idx);
  if (lua_getiuservalue(L, idx, 1) == LUA_TNUMBER) {
    p = *(void**)p;
    if (size) *size = (int)lua_tointeger(L, -1);
  } else if (size) *size = (int)lua_rawlen(L, idx);
  lua_pop(L, 1);
  return p;
}

int userdata_pointer(lua_State* L) {
  lua_pushinteger(L, (uint64_t)getuserdatapointer(L, 1, 0));
  return 1;
}

int userdata_string(lua_State* L) { //userdata:string(size, offset=0) / userdata:string("str", offset=0)
  int len = 0;
  char* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_type(L, 2) == LUA_TSTRING) { //write
    int size = (int)lua_rawlen(L, 2);
    if ((size <= 0) || (offset + size > len)) size = len - offset;
    mem_cpy(&p[offset], lua_tostring(L, 2), size);
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

int userdata_int(lua_State* L) { //userdata:int([value][, offset][, size=userdata length])
  int len = 0;
  uint8_t* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int64_t v = lua_tointeger(L, 2);
  int size = len;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_isinteger(L, 4)) size = (int)lua_tointeger(L, 4);
  if (size > 8) size = 8;
  if ((size <=0) || (size*(offset+1) > len)) return 0;
  if (lua_isinteger(L, 2)) {
    mem_cpy(&p[size * offset], &v, size);
    lua_pushvalue(L, 1);
  }
  else {
    mem_cpy(&v, &p[size * offset], size);
    lua_pushinteger(L, v);
  }
  return 1;
}

int userdata_float(lua_State* L) {  //userdata:float([value][, offset])
  int len = 0;
  float* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 3);
  if (sizeof(float) * (offset + 1) > len) return 0;
  if (lua_isnumber(L, 2)) {
    p[offset] = (float)lua_tonumber(L, 2);
    lua_pushvalue(L, 1);
  }
  else {
    lua_pushnumber(L, p[offset]);
  }
  return 1;
}

int userdata_double(lua_State* L) {    //userdata:float([value][, offset])
  int len = 0;
  double* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int offset = (int)lua_tointeger(L, 3);
  if (sizeof(double) * (offset + 1) > len) return 0;
  if (lua_isnumber(L, 2)) {
    p[offset] = lua_tonumber(L, 2);
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

int userdata_bits(lua_State* L) {  //userdata:bits([,data: write if present, otherwise read][offset=0][, size=1])
  int len = 0;
  uint64_t* p = getuserdatapointer(L, 1, &len);
  if (p == 0) return 0;
  int size = 1;
  int offset = (int)lua_tointeger(L, 3);
  if (lua_isinteger(L, 2)) size = (int)lua_tointeger(L, 4);
  if (size > 64) size = 64;
  if ((size <= 0) || (size * (offset + 1) > len*8)) return 0;
  if (lua_isinteger(L, 2)) {
    write_bits(p, offset, size, lua_tointeger(L, 2));
    lua_pushvalue(L, 1);
  }
  else {
    lua_pushinteger(L, read_bits(p, offset, size));
  }
  return 1;
}

int userdata_pack(lua_State* L) { //:pack(fmt, values....)
  int len = 0;
  uint64_t* p = getuserdatapointer(L, 1, &len);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "pack");
  lua_insert(L, 2);  //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...  [-1]=string
  lua_pop(L, 1);     //stack: [1]=self, [2]=string.pack, [3]=fmt, [4...]=values, ...
  lua_call(L, lua_gettop(L) - 2, 1);
  const char* s = lua_tostring(L, -1);
  if (len < lua_rawlen(L, -1)) len = (int)lua_rawlen(L, -1);
  if (s) mem_cpy(p, s, len);
  lua_pop(L, 1);        //remove result of string.pack and return self
  return 1;
}

int userdata_type(lua_State* L) {
  int type = lua_type(L, 2);
  lua_getmetatable(L, 1);

  if ((type == LUA_TNIL) || (type == LUA_TNONE)) { // get type
    lua_getfield(L, -1, "__type");    //stack: self, ..., [nil], metatable, type
    return 1;
  }

  if (type == LUA_TSTRING) {  //set __type to ffi.userdata[str]
    lua_getfield(L, -1, "__userdata");  //stack: self, ..., metatable, __index 
    lua_pushvalue(L, 2);                //stack: self, ..., metatable, __index, arg
    lua_rawget(L, -2);                  //stack: self, ..., metatable, __index, userdata[arg]
    lua_setfield(L, -3, "__type");
    lua_settop(L, 1);
    return 1;
  }
  
  if (type == LUA_TFUNCTION) {
    lua_pushvalue(L, 2);           //stack: self, ..., metatable, arg
    lua_setfield(L, -2, "__type");
    lua_settop(L, 1);
    return 1;
  }
  return 0;
}

int userdata_size(lua_State* L) {
  int type = lua_type(L, 2);
  lua_getmetatable(L, 1);
  if ((type == LUA_TNIL) || (type == LUA_TNONE)) { // get size
    lua_getfield(L, -1, "__size");    //stack: self, ..., [nil], metatable, type
    lua_remove(L, -2);  //remove metatable
    if (type == LUA_TNIL) lua_remove(L, -2); //remove nil
    return 1;
  }

  if (type == LUA_TNUMBER) {  //set __size for indexing
    lua_pushvalue(L, 2);            //stack: self, ..., metatable, arg
    lua_setfield(L, -2, "__size");
    lua_pop(L, 1);
  }
  return 0;
}

//__index(t,k) x = ffi.userdata(size):type("float");   x(0), x(1), x(2)
int userdata__index(lua_State* L) {  //x = ffi.userdata():type("float") -> x.getmetatable().__type = userdata.float; then x(n) -> x.metatable.__type(x, nil, n) -> x:float(nil, n)
  int type = lua_type(L, 2); //key
  lua_getmetatable(L, 1);
  if (type == LUA_TNUMBER) {
    if (lua_getfield(L, -1, "__type") != LUA_TFUNCTION) return 0;
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    lua_pushvalue(L, 2);
    lua_getfield(L, -5, "__size");
    lua_call(L, 4, 1);
//    lua_remove(L, -2);
    return 1;
  } else {
    lua_getfield(L, -1, "__userdata");
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    return 1;
  }
  return 0;
}
//__newindex(t,k,v) (ffi.userdata()) [k] = v
int userdata__newindex(lua_State* L) {  //x = ffi.userdata():type("float") -> userdata.metatable.__type = userdata.float; x[n] -> x.metatable.__type(x, value, n) -> x:float(value, n)
  int type = lua_type(L, 2); //key
  if (type == LUA_TNUMBER) {
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, "__type");
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 3);
    lua_pushvalue(L, 2);
    lua_getfield(L, -5, "__size");
    lua_call(L, 4, 1);
    //  lua_remove(L, -2);
  }
  return 0;
}

int userdata_unpack(lua_State* L) { //:unpack(fmt, pos)
  int len = 0;
  uint8_t* p = getuserdatapointer(L, 1, &len);
  lua_getglobal(L, "string");
  lua_getfield(L, -1, "unpack");
  lua_insert(L, 2);     //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,    [-1]=string
  lua_pop(L, 1);        //stack: [1]=self, [2]=string.unpack, [3]=fmt, [4]=pos,  
  lua_pushlstring(L, p, len);
  lua_insert(L, 4);
  lua_call(L, lua_gettop(L) - 2, LUA_MULTRET);
  return lua_gettop(L) - 1;
}

int ffi_userdata(lua_State* L) {
  switch (lua_type(L, 2)) {
    case LUA_TNUMBER: {
      int length = (int)lua_tointeger(L, 2);
      if (length <= 0) return 0;
      void* data = lua_newuserdatauv(L, length, 0);
//      memset(data, 0, length);
    } break;

    case LUA_TSTRING: {
      size_t length = lua_rawlen(L, 2);
      char* data = lua_newuserdatauv(L, length + 1, 0);
      mem_cpy(data, lua_tostring(L, 2), length + 1);
    } break;

    case LUA_TTABLE: {
      size_t length = lua_rawlen(L, 2);
      uintptr_t* data = lua_newuserdatauv(L, sizeof(uintptr_t) * length, 0);
      for (size_t i = 0; i < length; i++) {
        lua_pushcfunction(L, ffi_userdata);
        lua_pushvalue(L, 1);
        lua_rawgeti(L, 1, i + 1);
        lua_call(L, 2, 1);
        data[i] = (uintptr_t)lua_touserdata(L, -1);
      }
      
    } break;

    case LUA_TLIGHTUSERDATA: {
      void* p = lua_touserdata(L, 2);
      void** data = lua_newuserdatauv(L, sizeof(void*), 1);
      lua_pushvalue(L, 3);
      lua_setiuservalue(L, -2, 1);
      *data = p;
    } break;

    case LUA_TUSERDATA: {
      lua_settop(L, 2); //remove other arguments, if any
    } break;
    default: return 0;
  }

  lua_createtable(L, 0, 5); //userdata metatable
    lua_pushcfunction(L, userdata_string);
    lua_setfield(L, -2, "__tostring");

    lua_pushvalue(L, 1);  //ffi.userdata
    lua_setfield(L, -2, "__userdata");  //for indexing of ffi.userdata functions
     
    lua_pushcfunction(L, userdata__index);
    lua_setfield(L, -2, "__call");   //for now () for indexing as array instead of []: http://lua-users.org/lists/lua-l/2023-08/msg00122.html

    lua_pushcfunction(L, userdata__index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, userdata__newindex);
    lua_setfield(L, -2, "__newindex");

  lua_setmetatable(L, -2);

  return 1;
}