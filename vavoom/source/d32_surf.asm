 .486P
 .model FLAT
 externdef _d_sdivzstepu
 externdef _d_tdivzstepu
 externdef _d_zistepu
 externdef _d_sdivzstepv
 externdef _d_tdivzstepv
 externdef _d_zistepv
 externdef _d_sdivzorigin
 externdef _d_tdivzorigin
 externdef _d_ziorigin
 externdef _sadjust
 externdef _tadjust
 externdef _bbextents
 externdef _bbextentt
 externdef _cacheblock
 externdef _cachewidth
 externdef _ds_transluc
 externdef _ds_transluc16
 externdef _r_lightptr
 externdef _r_lightptrr
 externdef _r_lightptrg
 externdef _r_lightptrb
 externdef _r_lightwidth
 externdef _r_numvblocks
 externdef _r_sourcemax
 externdef _r_stepback
 externdef _prowdestbase
 externdef _pbasesource
 externdef _sourcetstep
 externdef _surfrowbytes
 externdef _lightright
 externdef _lightrightstep
 externdef _lightdeltastep
 externdef _lightdelta
 externdef _lightrleft
 externdef _lightrright
 externdef _lightrleftstep
 externdef _lightrrightstep
 externdef _lightgleft
 externdef _lightgright
 externdef _lightgleftstep
 externdef _lightgrightstep
 externdef _lightbleft
 externdef _lightbright
 externdef _lightbleftstep
 externdef _lightbrightstep
 externdef sdivz8stepu
 externdef tdivz8stepu
 externdef zi8stepu
 externdef sdivz16stepu
 externdef tdivz16stepu
 externdef zi16stepu
 externdef s
 externdef t
 externdef snext
 externdef tnext
 externdef sstep
 externdef tstep
 externdef sfracf
 externdef tfracf
 externdef spancountminus1
 externdef izi
 externdef izistep
 externdef advancetable
 externdef pbase
 externdef pz
 externdef reciprocal_table
 externdef pspantemp
 externdef counttemp
 externdef jumptemp
 externdef mmbuf
 externdef fp_64k
 externdef fp_8
 externdef fp_16
 externdef Float2ToThe31nd
 externdef FloatMinus2ToThe31nd
 externdef fp_64kx64k
 externdef float_1
 externdef float_particle_z_clip
 externdef float_point5
 externdef DP_u
 externdef DP_v
 externdef DP_32768
 externdef DP_Color
 externdef DP_Pix
 externdef ceil_cw
 externdef single_cw
 externdef _ylookup
 externdef _zbuffer
 externdef _scrn
 externdef _scrn16
 externdef _pal8_to16
 externdef _mmx_mask4
 externdef _mmx_mask8
 externdef _mmx_mask16
 externdef _d_rowbytes
 externdef _d_zrowbytes
 externdef _vieworg
 externdef _r_ppn
 externdef _r_pup
 externdef _r_pright
 externdef _centerxfrac
 externdef _centeryfrac
 externdef _d_particle_right
 externdef _d_particle_top
 externdef _d_pix_min
 externdef _d_pix_max
 externdef _d_pix_shift
 externdef _d_y_aspect_shift
 externdef _d_rgbtable
 externdef _rshift
 externdef _gshift
 externdef _bshift
 externdef _fadetable
 externdef _fadetable16
 externdef _fadetable16r
 externdef _fadetable16g
 externdef _fadetable16b
 externdef _fadetable32
 externdef _pr_globals
 externdef _pr_stackPtr
 externdef _pr_statements
 externdef _pr_functions
 externdef _pr_globaldefs
 externdef _pr_builtins
 externdef _current_func
 externdef _D_DrawZSpan
 externdef _PR_RFInvalidOpcode
_DATA SEGMENT
sb_v dd 0
_DATA ENDS
_TEXT SEGMENT
 align 4
 public _D_Surf32Start
_D_Surf32Start:
 align 4
 public _D_DrawSurfaceBlock32_mip0
_D_DrawSurfaceBlock32_mip0:
 push ebp
 push edi
 push esi
 push ebx
 mov ebx,ds:dword ptr[_r_lightptr]
 mov eax,ds:dword ptr[_r_numvblocks]
 mov ds:dword ptr[sb_v],eax
 mov edi,ds:dword ptr[_prowdestbase]
 mov esi,ds:dword ptr[_pbasesource]
Lv_loop_mip0:
 mov eax,ds:dword ptr[ebx]
 mov edx,ds:dword ptr[4+ebx]
 mov ebp,eax
 mov ecx,ds:dword ptr[_r_lightwidth]
 mov ds:dword ptr[_lightright],edx
 sub ebp,edx
 and ebp,0FFFFFh
 lea ebx,ds:dword ptr[ebx+ecx*4]
 mov ds:dword ptr[_r_lightptr],ebx
 mov ecx,ds:dword ptr[4+ebx]
 mov ebx,ds:dword ptr[ebx]
 sub ebx,eax
 sub ecx,edx
 sar ecx,4
 or ebp,0F0000000h
 sar ebx,4
 mov ds:dword ptr[_lightrightstep],ecx
 sub ebx,ecx
 and ebx,0FFFFFh
 or ebx,0F0000000h
 sub ecx,ecx
 mov ds:dword ptr[_lightdeltastep],ebx
 sub ebx,ebx
Lblockloop16_mip0:
 mov ds:dword ptr[_lightdelta],ebp
 mov cl,ds:byte ptr[14+esi]
 sar ebp,4
 mov bh,dh
 mov bl,ds:byte ptr[15+esi]
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch0:
 mov bl,ds:byte ptr[13+esi]
 mov ds:dword ptr[60+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch1:
 mov cl,ds:byte ptr[12+esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[56+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch2:
 add edx,ebp
 mov ds:dword ptr[52+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch3:
 mov bl,ds:byte ptr[11+esi]
 mov cl,ds:byte ptr[10+esi]
 mov ds:dword ptr[48+edi],eax
 mov bh,dh
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch4:
 mov bl,ds:byte ptr[9+esi]
 mov ds:dword ptr[44+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch5:
 mov cl,ds:byte ptr[8+esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[40+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch6:
 add edx,ebp
 mov ds:dword ptr[36+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch7:
 mov bl,ds:byte ptr[7+esi]
 mov cl,ds:byte ptr[6+esi]
 mov ds:dword ptr[32+edi],eax
 mov bh,dh
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch8:
 mov bl,ds:byte ptr[5+esi]
 mov ds:dword ptr[28+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch9:
 mov cl,ds:byte ptr[4+esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[24+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch10:
 add edx,ebp
 mov ds:dword ptr[20+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch11:
 mov bl,ds:byte ptr[3+esi]
 mov cl,ds:byte ptr[2+esi]
 mov ds:dword ptr[16+edi],eax
 mov bh,dh
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch12:
 mov bl,ds:byte ptr[1+esi]
 mov ds:dword ptr[12+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch13:
 mov cl,ds:byte ptr[esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[8+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch14:
 mov edx,ds:dword ptr[_lightright]
 mov ds:dword ptr[4+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch15:
 mov ebp,ds:dword ptr[_lightdelta]
 mov ds:dword ptr[edi],eax
 add esi,ds:dword ptr[_sourcetstep]
 add edi,ds:dword ptr[_surfrowbytes]
 add edx,ds:dword ptr[_lightrightstep]
 add ebp,ds:dword ptr[_lightdeltastep]
 mov ds:dword ptr[_lightright],edx
 jc Lblockloop16_mip0
 cmp esi,ds:dword ptr[_r_sourcemax]
 jb LSkip_mip0
 sub esi,ds:dword ptr[_r_stepback]
LSkip_mip0:
 mov ebx,ds:dword ptr[_r_lightptr]
 dec ds:dword ptr[sb_v]
 jnz Lv_loop_mip0
 pop ebx
 pop esi
 pop edi
 pop ebp
 ret
 align 4
 public _D_DrawSurfaceBlock32_mip1
_D_DrawSurfaceBlock32_mip1:
 push ebp
 push edi
 push esi
 push ebx
 mov ebx,ds:dword ptr[_r_lightptr]
 mov eax,ds:dword ptr[_r_numvblocks]
 mov ds:dword ptr[sb_v],eax
 mov edi,ds:dword ptr[_prowdestbase]
 mov esi,ds:dword ptr[_pbasesource]
Lv_loop_mip1:
 mov eax,ds:dword ptr[ebx]
 mov edx,ds:dword ptr[4+ebx]
 mov ebp,eax
 mov ecx,ds:dword ptr[_r_lightwidth]
 mov ds:dword ptr[_lightright],edx
 sub ebp,edx
 and ebp,0FFFFFh
 lea ebx,ds:dword ptr[ebx+ecx*4]
 mov ds:dword ptr[_r_lightptr],ebx
 mov ecx,ds:dword ptr[4+ebx]
 mov ebx,ds:dword ptr[ebx]
 sub ebx,eax
 sub ecx,edx
 sar ecx,3
 or ebp,070000000h
 sar ebx,3
 mov ds:dword ptr[_lightrightstep],ecx
 sub ebx,ecx
 and ebx,0FFFFFh
 or ebx,0F0000000h
 sub ecx,ecx
 mov ds:dword ptr[_lightdeltastep],ebx
 sub ebx,ebx
Lblockloop16_mip1:
 mov ds:dword ptr[_lightdelta],ebp
 mov cl,ds:byte ptr[6+esi]
 sar ebp,3
 mov bh,dh
 mov bl,ds:byte ptr[7+esi]
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch22:
 mov bl,ds:byte ptr[5+esi]
 mov ds:dword ptr[28+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch23:
 mov cl,ds:byte ptr[4+esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[24+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch24:
 add edx,ebp
 mov ds:dword ptr[20+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch25:
 mov bl,ds:byte ptr[3+esi]
 mov cl,ds:byte ptr[2+esi]
 mov ds:dword ptr[16+edi],eax
 mov bh,dh
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch26:
 mov bl,ds:byte ptr[1+esi]
 mov ds:dword ptr[12+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch27:
 mov cl,ds:byte ptr[esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[8+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch28:
 mov edx,ds:dword ptr[_lightright]
 mov ds:dword ptr[4+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch29:
 mov ebp,ds:dword ptr[_lightdelta]
 mov ds:dword ptr[edi],eax
 mov eax,ds:dword ptr[_sourcetstep]
 add esi,eax
 mov eax,ds:dword ptr[_surfrowbytes]
 add edi,eax
 mov eax,ds:dword ptr[_lightrightstep]
 add edx,eax
 mov eax,ds:dword ptr[_lightdeltastep]
 add ebp,eax
 mov ds:dword ptr[_lightright],edx
 jc Lblockloop16_mip1
 cmp esi,ds:dword ptr[_r_sourcemax]
 jb LSkip_mip1
 sub esi,ds:dword ptr[_r_stepback]
LSkip_mip1:
 mov ebx,ds:dword ptr[_r_lightptr]
 dec ds:dword ptr[sb_v]
 jnz Lv_loop_mip1
 pop ebx
 pop esi
 pop edi
 pop ebp
 ret
 align 4
 public _D_DrawSurfaceBlock32_mip2
_D_DrawSurfaceBlock32_mip2:
 push ebp
 push edi
 push esi
 push ebx
 mov ebx,ds:dword ptr[_r_lightptr]
 mov eax,ds:dword ptr[_r_numvblocks]
 mov ds:dword ptr[sb_v],eax
 mov edi,ds:dword ptr[_prowdestbase]
 mov esi,ds:dword ptr[_pbasesource]
Lv_loop_mip2:
 mov eax,ds:dword ptr[ebx]
 mov edx,ds:dword ptr[4+ebx]
 mov ebp,eax
 mov ecx,ds:dword ptr[_r_lightwidth]
 mov ds:dword ptr[_lightright],edx
 sub ebp,edx
 and ebp,0FFFFFh
 lea ebx,ds:dword ptr[ebx+ecx*4]
 mov ds:dword ptr[_r_lightptr],ebx
 mov ecx,ds:dword ptr[4+ebx]
 mov ebx,ds:dword ptr[ebx]
 sub ebx,eax
 sub ecx,edx
 sar ecx,2
 or ebp,030000000h
 sar ebx,2
 mov ds:dword ptr[_lightrightstep],ecx
 sub ebx,ecx
 and ebx,0FFFFFh
 or ebx,0F0000000h
 sub ecx,ecx
 mov ds:dword ptr[_lightdeltastep],ebx
 sub ebx,ebx
Lblockloop16_mip2:
 mov ds:dword ptr[_lightdelta],ebp
 mov cl,ds:byte ptr[2+esi]
 sar ebp,2
 mov bh,dh
 mov bl,ds:byte ptr[3+esi]
 add edx,ebp
 mov ch,dh
 add edx,ebp
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch18:
 mov bl,ds:byte ptr[1+esi]
 mov ds:dword ptr[12+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch19:
 mov cl,ds:byte ptr[esi]
 mov bh,dh
 add edx,ebp
 mov ds:dword ptr[8+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch20:
 mov edx,ds:dword ptr[_lightright]
 mov ds:dword ptr[4+edi],eax
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch21:
 mov ebp,ds:dword ptr[_lightdelta]
 mov ds:dword ptr[edi],eax
 mov eax,ds:dword ptr[_sourcetstep]
 add esi,eax
 mov eax,ds:dword ptr[_surfrowbytes]
 add edi,eax
 mov eax,ds:dword ptr[_lightrightstep]
 add edx,eax
 mov eax,ds:dword ptr[_lightdeltastep]
 add ebp,eax
 mov ds:dword ptr[_lightright],edx
 jc Lblockloop16_mip2
 cmp esi,ds:dword ptr[_r_sourcemax]
 jb LSkip_mip2
 sub esi,ds:dword ptr[_r_stepback]
LSkip_mip2:
 mov ebx,ds:dword ptr[_r_lightptr]
 dec ds:dword ptr[sb_v]
 jnz Lv_loop_mip2
 pop ebx
 pop esi
 pop edi
 pop ebp
 ret
 align 4
 public _D_DrawSurfaceBlock32_mip3
_D_DrawSurfaceBlock32_mip3:
 push ebp
 push edi
 push esi
 push ebx
 mov ebx,ds:dword ptr[_r_lightptr]
 mov eax,ds:dword ptr[_r_numvblocks]
 mov ds:dword ptr[sb_v],eax
 mov edi,ds:dword ptr[_prowdestbase]
 mov esi,ds:dword ptr[_pbasesource]
Lv_loop_mip3:
 mov eax,ds:dword ptr[ebx]
 mov edx,ds:dword ptr[4+ebx]
 mov ebp,eax
 mov ecx,ds:dword ptr[_r_lightwidth]
 mov ds:dword ptr[_lightright],edx
 sub ebp,edx
 and ebp,0FFFFFh
 lea ebx,ds:dword ptr[ebx+ecx*4]
 mov ds:dword ptr[_lightdelta],ebp
 mov ds:dword ptr[_r_lightptr],ebx
 mov ecx,ds:dword ptr[4+ebx]
 mov ebx,ds:dword ptr[ebx]
 sub ebx,eax
 sub ecx,edx
 sar ecx,1
 sar ebx,1
 mov ds:dword ptr[_lightrightstep],ecx
 sub ebx,ecx
 and ebx,0FFFFFh
 sar ebp,1
 or ebx,0F0000000h
 mov ds:dword ptr[_lightdeltastep],ebx
 sub ebx,ebx
 mov bl,ds:byte ptr[1+esi]
 sub ecx,ecx
 mov bh,dh
 mov cl,ds:byte ptr[esi]
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch16:
 add edx,ebp
 mov ds:dword ptr[4+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch17:
 mov edx,ds:dword ptr[_lightright]
 mov ds:dword ptr[edi],eax
 mov eax,ds:dword ptr[_sourcetstep]
 add esi,eax
 mov eax,ds:dword ptr[_surfrowbytes]
 add edi,eax
 mov eax,ds:dword ptr[_lightdeltastep]
 mov ebp,ds:dword ptr[_lightdelta]
 mov cl,ds:byte ptr[esi]
 add ebp,eax
 mov eax,ds:dword ptr[_lightrightstep]
 sar ebp,1
 add edx,eax
 mov bh,dh
 mov bl,ds:byte ptr[1+esi]
 mov eax,ds:dword ptr[12345678h+ebx*4]
LBPatch30:
 add edx,ebp
 mov ds:dword ptr[4+edi],eax
 mov ch,dh
 mov eax,ds:dword ptr[12345678h+ecx*4]
LBPatch31:
 mov edx,ds:dword ptr[_sourcetstep]
 mov ds:dword ptr[edi],eax
 mov ebp,ds:dword ptr[_surfrowbytes]
 add esi,edx
 add edi,ebp
 cmp esi,ds:dword ptr[_r_sourcemax]
 jb LSkip_mip3
 sub esi,ds:dword ptr[_r_stepback]
LSkip_mip3:
 mov ebx,ds:dword ptr[_r_lightptr]
 dec ds:dword ptr[sb_v]
 jnz Lv_loop_mip3
 pop ebx
 pop esi
 pop edi
 pop ebp
 ret
 public _D_Surf32End
_D_Surf32End:
_TEXT ENDS
_DATA SEGMENT
 align 4
LPatchTable:
 dd LBPatch0-4
 dd LBPatch1-4
 dd LBPatch2-4
 dd LBPatch3-4
 dd LBPatch4-4
 dd LBPatch5-4
 dd LBPatch6-4
 dd LBPatch7-4
 dd LBPatch8-4
 dd LBPatch9-4
 dd LBPatch10-4
 dd LBPatch11-4
 dd LBPatch12-4
 dd LBPatch13-4
 dd LBPatch14-4
 dd LBPatch15-4
 dd LBPatch16-4
 dd LBPatch17-4
 dd LBPatch18-4
 dd LBPatch19-4
 dd LBPatch20-4
 dd LBPatch21-4
 dd LBPatch22-4
 dd LBPatch23-4
 dd LBPatch24-4
 dd LBPatch25-4
 dd LBPatch26-4
 dd LBPatch27-4
 dd LBPatch28-4
 dd LBPatch29-4
 dd LBPatch30-4
 dd LBPatch31-4
_DATA ENDS
_TEXT SEGMENT
 align 4
 public _D_Surf32Patch
_D_Surf32Patch:
 push ebx
 mov eax,ds:dword ptr[_fadetable32]
 mov ebx,offset LPatchTable
 mov ecx,32
LPatchLoop:
 mov edx,ds:dword ptr[ebx]
 add ebx,4
 mov ds:dword ptr[edx],eax
 dec ecx
 jnz LPatchLoop
 pop ebx
 ret
_TEXT ENDS
 END
