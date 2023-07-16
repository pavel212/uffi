/*
#define ARGN(A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,...) A17
#define EXPAND_MSVC(x) x
//#define _B(x,...) EXPAND_MSVC(ARGN(__VA_ARGS__,B16,_B15,_B14,_B13,_B12,_B11,_B10,_B09,_B08,_B07,_B06,_B05,_B04,_B03,_B02,_B01) (x,__VA_ARGS__))
#define _B01(x,A)                                   *x++ = A
#define _B02(x,A,B)                             do{ _B01(x,A);                             _B01(x,B); }while(0)
#define _B03(x,A,B,C)                           do{ _B02(x,A,B);                           _B01(x,C); }while(0)
#define _B04(x,A,B,C,D)                         do{ _B03(x,A,B,C);                         _B01(x,D); }while(0)
#define _B05(x,A,B,C,D,E)                       do{ _B04(x,A,B,C,D);                       _B01(x,E); }while(0)
#define _B06(x,A,B,C,D,E,F)                     do{ _B05(x,A,B,C,D,E);                     _B01(x,F); }while(0)
#define _B07(x,A,B,C,D,E,F,G)                   do{ _B06(x,A,B,C,D,E,F);                   _B01(x,G); }while(0)
#define _B08(x,A,B,C,D,E,F,G,H)                 do{ _B07(x,A,B,C,D,E,F,G);                 _B01(x,H); }while(0)
#define _B09(x,A,B,C,D,E,F,G,H,I)               do{ _B08(x,A,B,C,D,E,F,G,H);               _B01(x,I); }while(0)
#define _B10(x,A,B,C,D,E,F,G,H,I,J)             do{ _B09(x,A,B,C,D,E,F,G,H,I);             _B01(x,J); }while(0)
#define _B11(x,A,B,C,D,E,F,G,H,I,J,K)           do{ _B10(x,A,B,C,D,E,F,G,H,I,J);           _B01(x,K); }while(0)
#define _B12(x,A,B,C,D,E,F,G,H,I,J,K,L)         do{ _B11(x,A,B,C,D,E,F,G,H,I,J,K);         _B01(x,L); }while(0)
#define _B13(x,A,B,C,D,E,F,G,H,I,J,K,L,M)       do{ _B12(x,A,B,C,D,E,F,G,H,I,J,K,L);       _B01(x,M); }while(0)
#define _B14(x,A,B,C,D,E,F,G,H,I,J,K,L,M,N)     do{ _B13(x,A,B,C,D,E,F,G,H,I,J,K,L,M);     _B01(x,N); }while(0)
#define _B15(x,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O)   do{ _B14(x,A,B,C,D,E,F,G,H,I,J,K,L,M,N);   _B01(x,O); }while(0)
#define _B16(x,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P) do{ _B15(x,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O); _B01(x,P); }while(0)
*/

//#define _DW(x) x, (x)>>8, (x)>>16, (x)>>24
//#define _QW(x) x, (x)>>8, (x)>>16, (x)>>24, (x)>>32, (x)>>40, (x)>>48, (x)>>56

//#define _ARGC(...) EXPAND_MSVC(ARGN(__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1))

#define _ARGC(...) sizeof((uint8_t[]){__VA_ARGS__})
#define _B(p,...) (memcpy(p, (const uint8_t[]){__VA_ARGS__}, _ARGC(__VA_ARGS__) ), (uint8_t*)p += _ARGC(__VA_ARGS__))

#define _W(x)  (x), (x)>>8
#define _DW(x) _W(x), _W((x)>>16)
#define _QW(x) _DW(x), _DW((x)>>32)

#define _push_rbx(p)           _B(p, 0x53)
#define _pop_rbx(p)            _B(p, 0x5B)
#define _sub_rsp_DW(p,x)       _B(p, 0x48, 0x81, 0xEC, _DW(x))
#define _add_rsp_DW(p,x)       _B(p, 0x48, 0x81, 0xC4, _DW(x))

#define _mov_rbx_rcx(p)        _B(p, 0x48, 0x89, 0xCB)
#define _mov_rcx_rbx(p)        _B(p, 0x48, 0x89, 0xD9)

#define _mov_rbx_rax(p)        _B(p, 0x48, 0x89, 0xC3)
#define _mov_rcx_rax(p)        _B(p, 0x48, 0x89, 0xC1)
#define _mov_rdx_rax(p)        _B(p, 0x48, 0x89, 0xC2)
#define _mov_r8_rax(p)         _B(p, 0x49, 0x89, 0xC0)
#define _mov_r9_rax(p)         _B(p, 0x49, 0x89, 0xC1)

#define _mov_rax_rbx(p)        _B(p, 0x48, 0x89, 0xD8)
#define _mov_rax_rcx(p)        _B(p, 0x48, 0x89, 0xC8)
#define _mov_rax_rdx(p)        _B(p, 0x48, 0x89, 0xD0)
#define _mov_rax_r8(p)         _B(p, 0x4C, 0x89, 0xC0)
#define _mov_rax_r9(p)         _B(p, 0x4C, 0x89, 0xC8)

#define _mov_rax_QW(p,x)       _B(p, 0x48, 0xB8, _QW(x))
#define _mov_rbx_QW(p,x)       _B(p, 0x48, 0xBB, _QW(x))
#define _mov_rcx_QW(p,x)       _B(p, 0x48, 0xB9, _QW(x))
#define _mov_rdx_QW(p,x)       _B(p, 0x48, 0xBA, _QW(x))
#define _mov_r8_QW(p,x)        _B(p, 0x49, 0xB8, _QW(x))
#define _mov_r9_QW(p,x)        _B(p, 0x49, 0xB9, _QW(x))

#define _mov_eax_DW(p,x)       _B(p, 0xB8,             _DW(x))
#define _mov_rax_DW(p,x)       _B(p, 0x48, 0xC7, 0xC0, _DW(x))
#define _mov_rbx_DW(p,x)       _B(p, 0x48, 0xC7, 0xC3, _DW(x))
#define _mov_rcx_DW(p,x)       _B(p, 0x48, 0xC7, 0xC1, _DW(x))
#define _mov_rdx_DW(p,x)       _B(p, 0x48, 0xC7, 0xC2, _DW(x))
#define _mov_r8_DW(p,x)        _B(p, 0x49, 0xC7, 0xC0, _DW(x))
#define _mov_r9_DW(p,x)        _B(p, 0x49, 0xC7, 0xC1, _DW(x))

#define _clr_rax(p)            _B(p, 0x48, 0x31, 0xC0)
#define _clr_rbx(p)            _B(p, 0x48, 0x31, 0xDB)
#define _clr_rcx(p)            _B(p, 0x48, 0x31, 0xC9)
#define _clr_rdx(p)            _B(p, 0x48, 0x31, 0xD2)
#define _clr_r8(p)             _B(p, 0x4D, 0x31, 0xC0)
#define _clr_r9(p)             _B(p, 0x4D, 0x31, 0xC9)

#define _ld_rax(p,x)           _B(p,       0x48, 0x8B, 0x84, 0x24, _DW(x))
#define _ld_rbx(p,x)           _B(p,       0x48, 0x8B, 0x9C, 0x24, _DW(x))
#define _ld_rcx(p,x)           _B(p,       0x48, 0x8B, 0x8C, 0x24, _DW(x))
#define _ld_rdx(p,x)           _B(p,       0x48, 0x8B, 0x94, 0x24, _DW(x))
#define _ld_r8(p,x)            _B(p,       0x4C, 0x8B, 0x84, 0x24, _DW(x))
#define _ld_r9(p,x)            _B(p,       0x4C, 0x8B, 0x8C, 0x24, _DW(x))
#define _ld_xmm0(p,x)          _B(p, 0xF3, 0x0F, 0x7E, 0x84, 0x24, _DW(x))
#define _ld_xmm1(p,x)          _B(p, 0xF3, 0x0F, 0x7E, 0x8C, 0x24, _DW(x))
#define _ld_xmm2(p,x)          _B(p, 0xF3, 0x0F, 0x7E, 0x94, 0x24, _DW(x))
#define _ld_xmm3(p,x)          _B(p, 0xF3, 0x0F, 0x7E, 0x9C, 0x24, _DW(x))

#define _st_rax(p,x)           _B(p,       0x48, 0x89, 0x84, 0x24, _DW(x))
#define _st_rbx(p,x)           _B(p,       0x48, 0x89, 0x9C, 0x24, _DW(x))
#define _st_rcx(p,x)           _B(p,       0x48, 0x89, 0x8C, 0x24, _DW(x))
#define _st_rdx(p,x)           _B(p,       0x48, 0x89, 0x94, 0x24, _DW(x))
#define _st_r8(p,x)            _B(p,       0x4C, 0x89, 0x84, 0x24, _DW(x))
#define _st_r9(p,x)            _B(p,       0x4C, 0x89, 0x8C, 0x24, _DW(x))
#define _st_xmm0(p,x)          _B(p, 0x66, 0x0F, 0xD6, 0x84, 0x24, _DW(x))
#define _st_xmm1(p,x)          _B(p, 0x66, 0x0F, 0xD6, 0x8C, 0x24, _DW(x))
#define _st_xmm2(p,x)          _B(p, 0x66, 0x0F, 0xD6, 0x94, 0x24, _DW(x))
#define _st_xmm3(p,x)          _B(p, 0x66, 0x0F, 0xD6, 0x9C, 0x24, _DW(x))

#define _mov_xmm0f_xmm0(p)     _B(p, 0xF2, 0x0F, 0x5A, 0xC0)
#define _mov_xmm1_xmm0f(p)     _B(p, 0xF3, 0x0F, 0x5A, 0xC8)
#define _mov_xmm1_xmm1f(p)     _B(p, 0xF3, 0x0F, 0x5A, 0xC9)
#define _mov_xmm1_xmm0(p)      _B(p, 0xF3, 0x0F, 0x7E, 0xC8)
#define _mov_xmm0_xmm1(p)      _B(p, 0xF3, 0x0F, 0x7E, 0xC1)

#define _call_rax(p)           _B(p, 0xFF, 0xD0)
#define _ret(p)                _B(p, 0xC3)

#define _call(p,f) do { _mov_rax_QW(p, (uintptr_t)(f)); _call_rax(p); }while(0)  
