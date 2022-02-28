    {
    case XED_ICLASS_PF2ID:              // 3DNOW
    case XED_ICLASS_PF2IW:              // 3DNOW
    case XED_ICLASS_PI2FD:              // 3DNOW
    case XED_ICLASS_PI2FW:              // 3DNOW
        return IB_cvt;
        
    case XED_ICLASS_PFACC:              // 3DNOW
    case XED_ICLASS_PAVGUSB:            // 3DNOW
    case XED_ICLASS_PFADD:              // 3DNOW
    case XED_ICLASS_PFNACC:             // 3DNOW
    case XED_ICLASS_PFPNACC:            // 3DNOW
        return IB_add;

    case XED_ICLASS_PFSUB:              // 3DNOW
    case XED_ICLASS_PFSUBR:             // 3DNOW
        return IB_sub;

    case XED_ICLASS_PFCMPEQ:            // 3DNOW
    case XED_ICLASS_PFCMPGE:            // 3DNOW
    case XED_ICLASS_PFCMPGT:            // 3DNOW
        return IB_cmp;
        
    case XED_ICLASS_PFRCP:              // 3DNOW
    case XED_ICLASS_PFCPIT1:            // 3DNOW
    case XED_ICLASS_PFRCPIT2:           // 3DNOW
        return IB_div;

    case XED_ICLASS_PFMAX:              // 3DNOW
    case XED_ICLASS_PFMIN:              // 3DNOW
        return IB_cmp;

    case XED_ICLASS_PFMUL:              // 3DNOW
    case XED_ICLASS_PMULHRW:            // 3DNOW
        return IB_mult;
        
    case XED_ICLASS_PFRSQIT1:           // 3DNOW
    case XED_ICLASS_PFSQRT:             // 3DNOW
        return IB_sqrt;
        
    case XED_ICLASS_PSWAPD:             // 3DNOW
        return IB_move;

    case XED_ICLASS_AESDEC:             // AES
    case XED_ICLASS_AESDECLAST:         // AES
    case XED_ICLASS_AESENC:             // AES
    case XED_ICLASS_AESENCLAST:         // AES
    case XED_ICLASS_AESIMC:             // AES
    case XED_ICLASS_AESKEYGENASSIST:    // AES
    case XED_ICLASS_VAESDEC:            // AES
    case XED_ICLASS_VAESDECLAST:        // AES
    case XED_ICLASS_VAESENC:            // AES
    case XED_ICLASS_VAESENCLAST:        // AES
    case XED_ICLASS_VAESIMC:            // AES
    case XED_ICLASS_VAESKEYGENASSIST:   // AES
        return IB_mult; // ???

    case XED_ICLASS_VADDPD:             // AVX
    case XED_ICLASS_VADDPS:             // AVX
    case XED_ICLASS_VADDSD:             // AVX
    case XED_ICLASS_VADDSS:             // AVX
    case XED_ICLASS_VADDSUBPD:          // AVX
    case XED_ICLASS_VADDSUBPS:          // AVX
    case XED_ICLASS_ADDPD:              // SSE
    case XED_ICLASS_ADDPS:              // SSE
    case XED_ICLASS_ADDSD:              // SSE
    case XED_ICLASS_ADDSS:              // SSE
    case XED_ICLASS_ADDSUBPD:           // SSE
    case XED_ICLASS_ADDSUBPS:           // SSE
        return IB_add;

    case XED_ICLASS_VBLENDPD:           // AVX
    case XED_ICLASS_VBLENDPS:           // AVX
    case XED_ICLASS_VBLENDVPD:          // AVX
    case XED_ICLASS_VBLENDVPS:          // AVX
    case XED_ICLASS_BLENDPD:            // SSE
    case XED_ICLASS_BLENDPS:            // SSE
    case XED_ICLASS_BLENDVPD:           // SSE
    case XED_ICLASS_BLENDVPS:           // SSE
        return IB_blend;
        
    case XED_ICLASS_VCMPPD:             // AVX
    case XED_ICLASS_VCMPPS:             // AVX
    case XED_ICLASS_VCMPSD:             // AVX
    case XED_ICLASS_VCMPSS:             // AVX
    case XED_ICLASS_VCOMISD:            // AVX
    case XED_ICLASS_VCOMISS:            // AVX
    case XED_ICLASS_CMPPD:              // SSE
    case XED_ICLASS_CMPPS:              // SSE
    case XED_ICLASS_CMPSD_XMM:          // SSE
    case XED_ICLASS_CMPSS:              // SSE
    case XED_ICLASS_COMISD:             // SSE
    case XED_ICLASS_COMISS:             // SSE
        return IB_cmp;
        
    case XED_ICLASS_CRC32:              // SSE
        return IB_popcnt; // ??? computes checksum...

    case XED_ICLASS_VDIVPD:             // AVX
    case XED_ICLASS_VDIVPS:             // AVX
    case XED_ICLASS_VDIVSD:             // AVX
    case XED_ICLASS_VDIVSS:             // AVX
    case XED_ICLASS_DIVPD:              // SSE
    case XED_ICLASS_DIVPS:              // SSE
    case XED_ICLASS_DIVSD:              // SSE
    case XED_ICLASS_DIVSS:              // SSE
        return IB_div;
        
    case XED_ICLASS_VDPPD:              // AVX
    case XED_ICLASS_VDPPS:              // AVX
    case XED_ICLASS_DPPD:               // SSE
    case XED_ICLASS_DPPS:               // SSE
        return IB_mult; // ??? computes dot-product...
        
    case XED_ICLASS_VEXTRACTI128:       // AVX2
    case XED_ICLASS_VEXTRACTF128:       // AVX
    case XED_ICLASS_VEXTRACTPS:         // AVX
    case XED_ICLASS_EXTRACTPS:          // SSE
        return IB_extract;

    case XED_ICLASS_FXRSTOR64:          // SSE
    case XED_ICLASS_FXRSTOR:            // SSE
        return IB_load_conf;
        
    case XED_ICLASS_FXSAVE64:           // SSE
    case XED_ICLASS_FXSAVE:             // SSE
        return IB_store_conf;
        
    case XED_ICLASS_VHADDPD:            // AVX
    case XED_ICLASS_VHADDPS:            // AVX
    case XED_ICLASS_VHSUBPD:            // AVX
    case XED_ICLASS_VHSUBPS:            // AVX
    case XED_ICLASS_VMPSADBW:           // AVX
    case XED_ICLASS_HADDPD:             // SSE
    case XED_ICLASS_HADDPS:             // SSE
    case XED_ICLASS_HSUBPD:             // SSE  horizontal SUB
    case XED_ICLASS_HSUBPS:             // SSE
        return IB_add;
        
    case XED_ICLASS_VINSERTI128:        // AVX2
    case XED_ICLASS_VINSERTF128:        // AVX
    case XED_ICLASS_VINSERTPS:          // AVX
    case XED_ICLASS_INSERTPS:           // SSE
        return IB_insert;
        
    case XED_ICLASS_VLDDQU:             // AVX
    case XED_ICLASS_LDDQU:              // SSE
        return IB_load;

    /* these load a single status register; use normal load type */
    case XED_ICLASS_VLDMXCSR:           // AVX
    case XED_ICLASS_LDMXCSR:            // SSE
        return IB_load_conf;
        
    case XED_ICLASS_VMASKMOVDQU:        // AVX
    case XED_ICLASS_VMASKMOVPD:         // AVX
    case XED_ICLASS_VMASKMOVPS:         // AVX
        return IB_move;
        
    case XED_ICLASS_VMAXPD:             // AVX
    case XED_ICLASS_VMAXPS:             // AVX
    case XED_ICLASS_VMAXSD:             // AVX
    case XED_ICLASS_VMAXSS:             // AVX
    case XED_ICLASS_VMINPD:             // AVX
    case XED_ICLASS_VMINPS:             // AVX
    case XED_ICLASS_VMINSD:             // AVX
    case XED_ICLASS_VMINSS:             // AVX
    case XED_ICLASS_MAXPD:              // SSE
    case XED_ICLASS_MAXPS:              // SSE
    case XED_ICLASS_MAXSD:              // SSE
    case XED_ICLASS_MAXSS:              // SSE
    case XED_ICLASS_MINPD:              // SSE
    case XED_ICLASS_MINPS:              // SSE
    case XED_ICLASS_MINSD:              // SSE
    case XED_ICLASS_MINSS:              // SSE
        return IB_cmp;
        
    case XED_ICLASS_VMOVHLPS:           // AVX
    case XED_ICLASS_VMOVLHPS:           // AVX
        return IB_shuffle;
        
    case XED_ICLASS_VMOVNTDQ:           // AVX
    case XED_ICLASS_VMOVNTDQA:          // AVX
    case XED_ICLASS_VMOVNTPD:           // AVX
    case XED_ICLASS_VMOVNTPS:           // AVX
    case XED_ICLASS_MOVNTDQA:           // SSE
        return IB_copy;

    case XED_ICLASS_VMOVSHDUP:          // AVX
    case XED_ICLASS_VMOVSLDUP:          // AVX
        return IB_move;

    case XED_ICLASS_MPSADBW:            // SSE
        return IB_add;
        
    case XED_ICLASS_VMULPD:             // AVX
    case XED_ICLASS_VMULPS:             // AVX
    case XED_ICLASS_VMULSD:             // AVX
    case XED_ICLASS_VMULSS:             // AVX
    case XED_ICLASS_MULPD:              // SSE
    case XED_ICLASS_MULPS:              // SSE
    case XED_ICLASS_MULSD:              // SSE
    case XED_ICLASS_MULSS:              // SSE
        return IB_mult;
        
    case XED_ICLASS_VPABSB:             // AVX
    case XED_ICLASS_VPABSD:             // AVX
    case XED_ICLASS_VPABSW:             // AVX
    case XED_ICLASS_PABSB:              // MMX SSE
    case XED_ICLASS_PABSD:              // MMX SSE
    case XED_ICLASS_PABSW:              // MMX SSE
        return IB_add;
        
    case XED_ICLASS_VPACKSSDW:          // AVX
    case XED_ICLASS_VPACKSSWB:          // AVX
    case XED_ICLASS_VPACKUSDW:          // AVX
    case XED_ICLASS_VPACKUSWB:          // AVX
    case XED_ICLASS_PACKSSDW:           // MMX SSE
    case XED_ICLASS_PACKSSWB:           // MMX SSE
    case XED_ICLASS_PACKUSWB:           // MMX SSE
    case XED_ICLASS_PACKUSDW:           // SSE
        return IB_move;
        
    case XED_ICLASS_VPADDB:             // AVX
    case XED_ICLASS_VPADDD:             // AVX
    case XED_ICLASS_VPADDQ:             // AVX
    case XED_ICLASS_VPADDSB:            // AVX
    case XED_ICLASS_VPADDSW:            // AVX
    case XED_ICLASS_VPADDUSB:           // AVX
    case XED_ICLASS_VPADDUSW:           // AVX
    case XED_ICLASS_VPADDW:             // AVX
    case XED_ICLASS_PADDB:              // MMX SSE
    case XED_ICLASS_PADDD:              // MMX SSE
    case XED_ICLASS_PADDQ:              // MMX SSE
    case XED_ICLASS_PADDSB:             // MMX SSE
    case XED_ICLASS_PADDSW:             // MMX SSE
    case XED_ICLASS_PADDUSB:            // MMX SSE
    case XED_ICLASS_PADDUSW:            // MMX SSE
    case XED_ICLASS_PADDW:              // MMX SSE
        return IB_add;
        
    case XED_ICLASS_VPALIGNR:           // AVX
    case XED_ICLASS_PALIGNR:            // MMX SSE
        return IB_shift;

    case XED_ICLASS_VPAVGB:             // AVX
    case XED_ICLASS_VPAVGW:             // AVX
    case XED_ICLASS_PAVGB:              // MMX SSE
    case XED_ICLASS_PAVGW:              // MMX SSE
        return IB_add;
        
    case XED_ICLASS_VPBLENDVB:          // AVX
    case XED_ICLASS_VPBLENDW:           // AVX
    case XED_ICLASS_PBLENDVB:           // SSE
    case XED_ICLASS_PBLENDW:            // SSE
        return IB_blend;
        
    case XED_ICLASS_VPCLMULQDQ:         // AVX
        return IB_mult;
        
    case XED_ICLASS_VPCMPEQB:           // AVX
    case XED_ICLASS_VPCMPEQD:           // AVX
    case XED_ICLASS_VPCMPEQQ:           // AVX
    case XED_ICLASS_VPCMPEQW:           // AVX
    case XED_ICLASS_VPCMPGTB:           // AVX
    case XED_ICLASS_VPCMPGTD:           // AVX
    case XED_ICLASS_VPCMPGTQ:           // AVX
    case XED_ICLASS_VPCMPGTW:           // AVX
    case XED_ICLASS_PCMPEQB:            // MMX SSE
    case XED_ICLASS_PCMPEQD:            // MMX SSE
    case XED_ICLASS_PCMPEQW:            // MMX SSE
    case XED_ICLASS_PCMPGTB:            // MMX SSE
    case XED_ICLASS_PCMPGTD:            // MMX SSE
    case XED_ICLASS_PCMPGTW:            // MMX SSE
    case XED_ICLASS_PCMPEQQ:            // SSE
    case XED_ICLASS_PCMPESTRI:          // SSE
    case XED_ICLASS_PCMPESTRM:          // SSE
    case XED_ICLASS_PCMPGTQ:            // SSE
    case XED_ICLASS_PCMPISTRI:          // SSE
    case XED_ICLASS_PCMPISTRM:          // SSE
        return IB_cmp;
        
    case XED_ICLASS_VPERM2F128:         // AVX
    case XED_ICLASS_VPERMILPD:          // AVX
    case XED_ICLASS_VPERMILPS:          // AVX
        return IB_shuffle;

    case XED_ICLASS_VPEXTRB:            // AVX
    case XED_ICLASS_VPEXTRD:            // AVX
    case XED_ICLASS_VPEXTRQ:            // AVX
    case XED_ICLASS_VPEXTRW:            // AVX
    case XED_ICLASS_PEXTRW:             // MMX SSE SSE
    case XED_ICLASS_PEXTRB:             // SSE
    case XED_ICLASS_PEXTRD:             // SSE
    case XED_ICLASS_PEXTRQ:             // SSE
        return IB_extract;

    case XED_ICLASS_VPHADDD:            // AVX
    case XED_ICLASS_VPHADDSW:           // AVX
    case XED_ICLASS_VPHADDW:            // AVX
    case XED_ICLASS_VPHMINPOSUW:        // AVX
    case XED_ICLASS_VPHSUBD:            // AVX    horizontal SUB
    case XED_ICLASS_VPHSUBSW:           // AVX
    case XED_ICLASS_VPHSUBW:            // AVX
    case XED_ICLASS_PHADDD:             // MMX SSE
    case XED_ICLASS_PHADDSW:            // MMX SSE
    case XED_ICLASS_PHADDW:             // MMX SSE
    case XED_ICLASS_PHSUBD:             // MMX SSE
    case XED_ICLASS_PHSUBSW:            // MMX SSE
    case XED_ICLASS_PHSUBW:             // MMX SSE
    case XED_ICLASS_PHMINPOSUW:         // SSE
        return IB_add;
        
    case XED_ICLASS_VPINSRB:            // AVX
    case XED_ICLASS_VPINSRD:            // AVX
    case XED_ICLASS_VPINSRQ:            // AVX
    case XED_ICLASS_VPINSRW:            // AVX
    case XED_ICLASS_PINSRW:             // MMX SSE
    case XED_ICLASS_PINSRB:             // SSE
    case XED_ICLASS_PINSRD:             // SSE
    case XED_ICLASS_PINSRQ:             // SSE
        return IB_insert;
        
    case XED_ICLASS_VPMADDUBSW:         // AVX
    case XED_ICLASS_VPMADDWD:           // AVX
    case XED_ICLASS_PMADDUBSW:          // MMX SSE
    case XED_ICLASS_PMADDWD:            // MMX SSE

    case XED_ICLASS_VFMADD132PD:        // AVX2
    case XED_ICLASS_VFMADD132PS:        // AVX2
    case XED_ICLASS_VFMADD132SD:        // AVX2
    case XED_ICLASS_VFMADD132SS:        // AVX2
    case XED_ICLASS_VFMADD213PD:        // AVX2
    case XED_ICLASS_VFMADD213PS:        // AVX2
    case XED_ICLASS_VFMADD213SD:        // AVX2
    case XED_ICLASS_VFMADD213SS:        // AVX2
    case XED_ICLASS_VFMADD231PD:        // AVX2
    case XED_ICLASS_VFMADD231PS:        // AVX2
    case XED_ICLASS_VFMADD231SD:        // AVX2
    case XED_ICLASS_VFMADD231SS:        // AVX2
    case XED_ICLASS_VFMADDPD:           // AVX2
    case XED_ICLASS_VFMADDPS:           // AVX2
    case XED_ICLASS_VFMADDSD:           // AVX2
    case XED_ICLASS_VFMADDSS:           // AVX2
    case XED_ICLASS_VFMADDSUB132PD:     // AVX2
    case XED_ICLASS_VFMADDSUB132PS:     // AVX2
    case XED_ICLASS_VFMADDSUB213PD:     // AVX2
    case XED_ICLASS_VFMADDSUB213PS:     // AVX2
    case XED_ICLASS_VFMADDSUB231PD:     // AVX2
    case XED_ICLASS_VFMADDSUB231PS:     // AVX2
    case XED_ICLASS_VFMADDSUBPD:        // AVX2
    case XED_ICLASS_VFMADDSUBPS:        // AVX2
    case XED_ICLASS_VFNMADD132PD:       // AVX2
    case XED_ICLASS_VFNMADD132PS:       // AVX2
    case XED_ICLASS_VFNMADD132SD:       // AVX2
    case XED_ICLASS_VFNMADD132SS:       // AVX2
    case XED_ICLASS_VFNMADD213PD:       // AVX2
    case XED_ICLASS_VFNMADD213PS:       // AVX2
    case XED_ICLASS_VFNMADD213SD:       // AVX2
    case XED_ICLASS_VFNMADD213SS:       // AVX2
    case XED_ICLASS_VFNMADD231PD:       // AVX2
    case XED_ICLASS_VFNMADD231PS:       // AVX2
    case XED_ICLASS_VFNMADD231SD:       // AVX2
    case XED_ICLASS_VFNMADD231SS:       // AVX2
    case XED_ICLASS_VFNMADDPD:          // AVX2
    case XED_ICLASS_VFNMADDPS:          // AVX2
    case XED_ICLASS_VFNMADDSD:          // AVX2
    case XED_ICLASS_VFNMADDSS:          // AVX2

    case XED_ICLASS_VFMSUB132PD:        // AVX2
    case XED_ICLASS_VFMSUB132PS:        // AVX2
    case XED_ICLASS_VFMSUB132SD:        // AVX2
    case XED_ICLASS_VFMSUB132SS:        // AVX2
    case XED_ICLASS_VFMSUB213PD:        // AVX2
    case XED_ICLASS_VFMSUB213PS:        // AVX2
    case XED_ICLASS_VFMSUB213SD:        // AVX2
    case XED_ICLASS_VFMSUB213SS:        // AVX2
    case XED_ICLASS_VFMSUB231PD:        // AVX2
    case XED_ICLASS_VFMSUB231PS:        // AVX2
    case XED_ICLASS_VFMSUB231SD:        // AVX2
    case XED_ICLASS_VFMSUB231SS:        // AVX2
    case XED_ICLASS_VFMSUBADD132PD:     // AVX2
    case XED_ICLASS_VFMSUBADD132PS:     // AVX2
    case XED_ICLASS_VFMSUBADD213PD:     // AVX2
    case XED_ICLASS_VFMSUBADD213PS:     // AVX2
    case XED_ICLASS_VFMSUBADD231PD:     // AVX2
    case XED_ICLASS_VFMSUBADD231PS:     // AVX2
    case XED_ICLASS_VFMSUBADDPD:        // AVX2
    case XED_ICLASS_VFMSUBADDPS:        // AVX2
    case XED_ICLASS_VFMSUBPD:           // AVX2
    case XED_ICLASS_VFMSUBPS:           // AVX2
    case XED_ICLASS_VFMSUBSD:           // AVX2
    case XED_ICLASS_VFMSUBSS:           // AVX2
    case XED_ICLASS_VFNMSUB132PD:       // AVX2
    case XED_ICLASS_VFNMSUB132PS:       // AVX2
    case XED_ICLASS_VFNMSUB132SD:       // AVX2
    case XED_ICLASS_VFNMSUB132SS:       // AVX2
    case XED_ICLASS_VFNMSUB213PD:       // AVX2
    case XED_ICLASS_VFNMSUB213PS:       // AVX2
    case XED_ICLASS_VFNMSUB213SD:       // AVX2
    case XED_ICLASS_VFNMSUB213SS:       // AVX2
    case XED_ICLASS_VFNMSUB231PD:       // AVX2
    case XED_ICLASS_VFNMSUB231PS:       // AVX2
    case XED_ICLASS_VFNMSUB231SD:       // AVX2
    case XED_ICLASS_VFNMSUB231SS:       // AVX2
    case XED_ICLASS_VFNMSUBPD:          // AVX2
    case XED_ICLASS_VFNMSUBPS:          // AVX2
    case XED_ICLASS_VFNMSUBSD:          // AVX2
    case XED_ICLASS_VFNMSUBSS:          // AVX2
        return IB_madd;
        
    case XED_ICLASS_VPMAXSB:            // AVX
    case XED_ICLASS_VPMAXSD:            // AVX
    case XED_ICLASS_VPMAXSW:            // AVX
    case XED_ICLASS_VPMAXUB:            // AVX
    case XED_ICLASS_VPMAXUD:            // AVX
    case XED_ICLASS_VPMAXUW:            // AVX
    case XED_ICLASS_VPMINSB:            // AVX
    case XED_ICLASS_VPMINSD:            // AVX
    case XED_ICLASS_VPMINSW:            // AVX
    case XED_ICLASS_VPMINUB:            // AVX
    case XED_ICLASS_VPMINUD:            // AVX
    case XED_ICLASS_VPMINUW:            // AVX
    case XED_ICLASS_PMAXSW:             // MMX SSE
    case XED_ICLASS_PMAXUB:             // MMX SSE
    case XED_ICLASS_PMINSW:             // MMX SSE
    case XED_ICLASS_PMINUB:             // MMX SSE
    case XED_ICLASS_PMAXSB:             // SSE
    case XED_ICLASS_PMAXSD:             // SSE
    case XED_ICLASS_PMAXUD:             // SSE
    case XED_ICLASS_PMAXUW:             // SSE
    case XED_ICLASS_PMINSB:             // SSE
    case XED_ICLASS_PMINSD:             // SSE
    case XED_ICLASS_PMINUD:             // SSE
    case XED_ICLASS_PMINUW:             // SSE
        return IB_cmp;
        
    case XED_ICLASS_VPMOVMSKB:          // AVX
    case XED_ICLASS_VPMOVSXBD:          // AVX
    case XED_ICLASS_VPMOVSXBQ:          // AVX
    case XED_ICLASS_VPMOVSXBW:          // AVX
    case XED_ICLASS_VPMOVSXDQ:          // AVX
    case XED_ICLASS_VPMOVSXWD:          // AVX
    case XED_ICLASS_VPMOVSXWQ:          // AVX
    case XED_ICLASS_VPMOVZXBD:          // AVX
    case XED_ICLASS_VPMOVZXBQ:          // AVX
    case XED_ICLASS_VPMOVZXBW:          // AVX
    case XED_ICLASS_VPMOVZXDQ:          // AVX
    case XED_ICLASS_VPMOVZXWD:          // AVX
    case XED_ICLASS_VPMOVZXWQ:          // AVX
    case XED_ICLASS_PMOVMSKB:           // MMX SSE
    case XED_ICLASS_PMOVSXBD:           // SSE
    case XED_ICLASS_PMOVSXBQ:           // SSE
    case XED_ICLASS_PMOVSXBW:           // SSE
    case XED_ICLASS_PMOVSXDQ:           // SSE
    case XED_ICLASS_PMOVSXWD:           // SSE
    case XED_ICLASS_PMOVSXWQ:           // SSE
    case XED_ICLASS_PMOVZXBD:           // SSE
    case XED_ICLASS_PMOVZXBQ:           // SSE
    case XED_ICLASS_PMOVZXBW:           // SSE
    case XED_ICLASS_PMOVZXDQ:           // SSE
    case XED_ICLASS_PMOVZXWD:           // SSE
    case XED_ICLASS_PMOVZXWQ:           // SSE
        return IB_move;
        
    case XED_ICLASS_VPMULDQ:            // AVX
    case XED_ICLASS_VPMULHRSW:          // AVX
    case XED_ICLASS_VPMULHUW:           // AVX
    case XED_ICLASS_VPMULHW:            // AVX
    case XED_ICLASS_VPMULLD:            // AVX
    case XED_ICLASS_VPMULLW:            // AVX
    case XED_ICLASS_VPMULUDQ:           // AVX
    case XED_ICLASS_PMULHRSW:           // MMX SSE
    case XED_ICLASS_PMULHUW:            // MMX SSE
    case XED_ICLASS_PMULHW:             // MMX SSE
    case XED_ICLASS_PMULLW:             // MMX SSE
    case XED_ICLASS_PMULUDQ:            // MMX SSE
    case XED_ICLASS_PMULDQ:             // SSE
    case XED_ICLASS_PMULLD:             // SSE
        return IB_mult;
        
    case XED_ICLASS_VPSADBW:            // AVX
    case XED_ICLASS_PSADBW:             // MMX SSE
        return IB_add;

    case XED_ICLASS_POPCNT:             // SSE
        return IB_popcnt;
        
    case XED_ICLASS_VPSHUFB:            // AVX
    case XED_ICLASS_VPSHUFD:            // AVX
    case XED_ICLASS_VPSHUFHW:           // AVX
    case XED_ICLASS_VPSHUFLW:           // AVX
    case XED_ICLASS_PSHUFB:             // MMX SSE
    case XED_ICLASS_PSHUFD:             // SSE
    case XED_ICLASS_PSHUFHW:            // SSE
    case XED_ICLASS_PSHUFLW:            // SSE
    case XED_ICLASS_PSHUFW:             // MMX
        return IB_shuffle;
        
    case XED_ICLASS_VPSIGNB:            // AVX
    case XED_ICLASS_VPSIGND:            // AVX
    case XED_ICLASS_VPSIGNW:            // AVX
    case XED_ICLASS_PSIGNB:             // MMX SSE
    case XED_ICLASS_PSIGND:             // MMX SSE
    case XED_ICLASS_PSIGNW:             // MMX SSE
        return IB_move;

    case XED_ICLASS_VPSLLD:             // AVX
    case XED_ICLASS_VPSLLDQ:            // AVX
    case XED_ICLASS_VPSLLQ:             // AVX
    case XED_ICLASS_VPSLLW:             // AVX
    case XED_ICLASS_VPSRAD:             // AVX
    case XED_ICLASS_VPSRAW:             // AVX
    case XED_ICLASS_VPSRLD:             // AVX
    case XED_ICLASS_VPSRLDQ:            // AVX
    case XED_ICLASS_VPSRLQ:             // AVX
    case XED_ICLASS_VPSRLW:             // AVX
    case XED_ICLASS_PSLLD:              // MMX SSE
    case XED_ICLASS_PSLLQ:              // MMX SSE
    case XED_ICLASS_PSLLW:              // MMX SSE
    case XED_ICLASS_PSRAD:              // MMX SSE
    case XED_ICLASS_PSRAW:              // MMX SSE
    case XED_ICLASS_PSRLD:              // MMX SSE
    case XED_ICLASS_PSRLQ:              // MMX SSE
    case XED_ICLASS_PSRLW:              // MMX SSE
    case XED_ICLASS_PSLLDQ:             // SSE
    case XED_ICLASS_PSRLDQ:             // SSE
        return IB_shift;
        
    case XED_ICLASS_VPSUBB:             // AVX
    case XED_ICLASS_VPSUBD:             // AVX
    case XED_ICLASS_VPSUBQ:             // AVX
    case XED_ICLASS_VPSUBSB:            // AVX
    case XED_ICLASS_VPSUBSW:            // AVX
    case XED_ICLASS_VPSUBUSB:           // AVX
    case XED_ICLASS_VPSUBUSW:           // AVX
    case XED_ICLASS_VPSUBW:             // AVX
    case XED_ICLASS_PSUBB:              // MMX SSE
    case XED_ICLASS_PSUBD:              // MMX SSE
    case XED_ICLASS_PSUBQ:              // MMX SSE
    case XED_ICLASS_PSUBSB:             // MMX SSE
    case XED_ICLASS_PSUBSW:             // MMX SSE
    case XED_ICLASS_PSUBUSB:            // MMX SSE
    case XED_ICLASS_PSUBUSW:            // MMX SSE
    case XED_ICLASS_PSUBW:              // MMX SSE
        return IB_sub;
        
    case XED_ICLASS_VPUNPCKHBW:         // AVX
    case XED_ICLASS_VPUNPCKHDQ:         // AVX
    case XED_ICLASS_VPUNPCKHQDQ:        // AVX
    case XED_ICLASS_VPUNPCKHWD:         // AVX
    case XED_ICLASS_VPUNPCKLBW:         // AVX
    case XED_ICLASS_VPUNPCKLDQ:         // AVX
    case XED_ICLASS_VPUNPCKLQDQ:        // AVX
    case XED_ICLASS_VPUNPCKLWD:         // AVX
    case XED_ICLASS_PUNPCKHBW:          // MMX SSE
    case XED_ICLASS_PUNPCKHDQ:          // MMX SSE
    case XED_ICLASS_PUNPCKHWD:          // MMX SSE
    case XED_ICLASS_PUNPCKLBW:          // MMX SSE
    case XED_ICLASS_PUNPCKLDQ:          // MMX SSE
    case XED_ICLASS_PUNPCKLWD:          // MMX SSE
    case XED_ICLASS_PUNPCKHQDQ:         // SSE
    case XED_ICLASS_PUNPCKLQDQ:         // SSE
        return IB_move;
        
    case XED_ICLASS_VRCPPS:             // AVX
    case XED_ICLASS_VRCPSS:             // AVX
    case XED_ICLASS_RCPPS:              // SSE
    case XED_ICLASS_RCPSS:              // SSE
        return IB_reciprocal;   // table lookup
        
    case XED_ICLASS_VROUNDPD:           // AVX
    case XED_ICLASS_VROUNDPS:           // AVX
    case XED_ICLASS_VROUNDSD:           // AVX
    case XED_ICLASS_VROUNDSS:           // AVX
    case XED_ICLASS_ROUNDPD:            // SSE
    case XED_ICLASS_ROUNDPS:            // SSE
    case XED_ICLASS_ROUNDSD:            // SSE
    case XED_ICLASS_ROUNDSS:            // SSE
        return IB_add;

    case XED_ICLASS_VRSQRTPS:           // AVX
    case XED_ICLASS_VRSQRTSS:           // AVX
    case XED_ICLASS_RSQRTPS:            // SSE
    case XED_ICLASS_RSQRTSS:            // SSE
        return IB_reciprocal; // table lookup

    case XED_ICLASS_VSHUFPD:            // AVX
    case XED_ICLASS_VSHUFPS:            // AVX
    case XED_ICLASS_SHUFPD:             // SSE
    case XED_ICLASS_SHUFPS:             // SSE
        return IB_shuffle;
        
    case XED_ICLASS_VSQRTPD:            // AVX
    case XED_ICLASS_VSQRTPS:            // AVX
    case XED_ICLASS_VSQRTSD:            // AVX
    case XED_ICLASS_VSQRTSS:            // AVX
    case XED_ICLASS_SQRTPD:             // SSE
    case XED_ICLASS_SQRTPS:             // SSE
    case XED_ICLASS_SQRTSD:             // SSE
    case XED_ICLASS_SQRTSS:             // SSE
        return IB_sqrt;
        
    /* these only save one state register; mark it as normal store
       since it should not be as expensive as a config store */
    case XED_ICLASS_VSTMXCSR:           // AVX
    case XED_ICLASS_STMXCSR:            // SSE
        return IB_store_conf;
        
    case XED_ICLASS_VSUBPD:             // AVX
    case XED_ICLASS_VSUBPS:             // AVX
    case XED_ICLASS_VSUBSD:             // AVX
    case XED_ICLASS_VSUBSS:             // AVX
    case XED_ICLASS_SUBPD:              // SSE
    case XED_ICLASS_SUBPS:              // SSE
    case XED_ICLASS_SUBSD:              // SSE
    case XED_ICLASS_SUBSS:              // SSE
        return IB_sub;

    case XED_ICLASS_VUCOMISD:           // AVX
    case XED_ICLASS_VUCOMISS:           // AVX
    case XED_ICLASS_UCOMISD:            // SSE
    case XED_ICLASS_UCOMISS:            // SSE
        return IB_cmp;
        
    case XED_ICLASS_VUNPCKHPD:          // AVX
    case XED_ICLASS_VUNPCKHPS:          // AVX
    case XED_ICLASS_VUNPCKLPD:          // AVX
    case XED_ICLASS_VUNPCKLPS:          // AVX
    case XED_ICLASS_UNPCKHPD:           // SSE
    case XED_ICLASS_UNPCKHPS:           // SSE
    case XED_ICLASS_UNPCKLPD:           // SSE
    case XED_ICLASS_UNPCKLPS:           // SSE
        return IB_move;
        
    case XED_ICLASS_VZEROALL:           // AVX
    case XED_ICLASS_VZEROUPPER:         // AVX
        return IB_move;

    case XED_ICLASS_ADD:                // BINARY
    case XED_ICLASS_TZCNT:                // BINARY
    case XED_ICLASS_DEC:                // BINARY
    case XED_ICLASS_INC:                // BINARY
        return IB_add;

    case XED_ICLASS_SUB:                // BINARY
    case XED_ICLASS_NEG:                // BINARY
        return IB_sub;

    case XED_ICLASS_ADC:                // BINARY
    case XED_ICLASS_SBB:                // BINARY
        return IB_add_cc;

    case XED_ICLASS_CMP:                // BINARY
        return IB_cmp;

    case XED_ICLASS_DIV:                // BINARY
    case XED_ICLASS_IDIV:               // BINARY
        return IB_div;

    case XED_ICLASS_IMUL:               // BINARY
    case XED_ICLASS_MUL:                // BINARY
        return IB_mult;

    case XED_ICLASS_BSF:                // BITBYTE
    case XED_ICLASS_BSR:                // BITBYTE
    case XED_ICLASS_LZCNT:              // BITBYTE
        return IB_popcnt;
        
    case XED_ICLASS_BT:                 // BITBYTE
    case XED_ICLASS_BTC:                // BITBYTE
    case XED_ICLASS_BTR:                // BITBYTE
    case XED_ICLASS_BTS:                // BITBYTE
        return IB_shift; // ??? bit manipulations...
    
    case XED_ICLASS_EXTRQ:              // BITBYTE
        return IB_extract; // ???

    case XED_ICLASS_INSERTQ:            // BITBYTE
        return IB_insert; // ???

    // conditional set immediate value
    case XED_ICLASS_SETB:               // BITBYTE
    case XED_ICLASS_SETBE:              // BITBYTE
    case XED_ICLASS_SETL:               // BITBYTE
    case XED_ICLASS_SETLE:              // BITBYTE
    case XED_ICLASS_SETNB:              // BITBYTE
    case XED_ICLASS_SETNBE:             // BITBYTE
    case XED_ICLASS_SETNL:              // BITBYTE
    case XED_ICLASS_SETNLE:             // BITBYTE
    case XED_ICLASS_SETNO:              // BITBYTE
    case XED_ICLASS_SETNP:              // BITBYTE
    case XED_ICLASS_SETNS:              // BITBYTE
    case XED_ICLASS_SETNZ:              // BITBYTE
    case XED_ICLASS_SETO:               // BITBYTE
    case XED_ICLASS_SETP:               // BITBYTE
    case XED_ICLASS_SETS:               // BITBYTE
    case XED_ICLASS_SETZ:               // BITBYTE
        return IB_move;

    case XED_ICLASS_VBROADCASTF128:     // BROADCAST
    case XED_ICLASS_VBROADCASTSD:       // BROADCAST
    case XED_ICLASS_VBROADCASTSS:       // BROADCAST
        return IB_move;  // it creates another IB_load uop.

    case XED_ICLASS_CALL_FAR:           // CALL
    case XED_ICLASS_CALL_NEAR:          // CALL
        return IB_jump;

    // conditional move
    case XED_ICLASS_CMOVB:              // CMOV
    case XED_ICLASS_CMOVBE:             // CMOV
    case XED_ICLASS_CMOVL:              // CMOV
    case XED_ICLASS_CMOVLE:             // CMOV
    case XED_ICLASS_CMOVNB:             // CMOV
    case XED_ICLASS_CMOVNBE:            // CMOV
    case XED_ICLASS_CMOVNL:             // CMOV
    case XED_ICLASS_CMOVNLE:            // CMOV
    case XED_ICLASS_CMOVNO:             // CMOV
    case XED_ICLASS_CMOVNP:             // CMOV
    case XED_ICLASS_CMOVNS:             // CMOV
    case XED_ICLASS_CMOVNZ:             // CMOV
    case XED_ICLASS_CMOVO:              // CMOV
    case XED_ICLASS_CMOVP:              // CMOV
    case XED_ICLASS_CMOVS:              // CMOV
    case XED_ICLASS_CMOVZ:              // CMOV
        return IB_move_cc;

    case XED_ICLASS_JB:                 // COND_BR
    case XED_ICLASS_JBE:                // COND_BR
    case XED_ICLASS_JL:                 // COND_BR
    case XED_ICLASS_JLE:                // COND_BR
    case XED_ICLASS_JNB:                // COND_BR
    case XED_ICLASS_JNBE:               // COND_BR
    case XED_ICLASS_JNL:                // COND_BR
    case XED_ICLASS_JNLE:               // COND_BR
    case XED_ICLASS_JNO:                // COND_BR
    case XED_ICLASS_JNP:                // COND_BR
    case XED_ICLASS_JNS:                // COND_BR
    case XED_ICLASS_JNZ:                // COND_BR
    case XED_ICLASS_JO:                 // COND_BR
    case XED_ICLASS_JP:                 // COND_BR
    case XED_ICLASS_JRCXZ:              // COND_BR
    case XED_ICLASS_JS:                 // COND_BR
    case XED_ICLASS_JZ:                 // COND_BR
    case XED_ICLASS_LOOP:               // COND_BR
    case XED_ICLASS_LOOPE:              // COND_BR
    case XED_ICLASS_LOOPNE:             // COND_BR
        return IB_br_CC;

    case XED_ICLASS_CBW:                // CONVERT
    case XED_ICLASS_CDQ:                // CONVERT
    case XED_ICLASS_CDQE:               // CONVERT
    case XED_ICLASS_CQO:                // CONVERT
    case XED_ICLASS_CWD:                // CONVERT
    case XED_ICLASS_CWDE:               // CONVERT
    case XED_ICLASS_CVTPD2PS:           // CONVERT
    case XED_ICLASS_CVTPS2PD:           // CONVERT
    case XED_ICLASS_CVTSD2SS:           // CONVERT
    case XED_ICLASS_CVTSS2SD:           // CONVERT
    case XED_ICLASS_VCVTPD2PS:          // CONVERT
    case XED_ICLASS_VCVTPS2PD:          // CONVERT
    case XED_ICLASS_VCVTSD2SS:          // CONVERT
    case XED_ICLASS_VCVTSS2SD:          // CONVERT
        return IB_cvt_prec;
        
    case XED_ICLASS_CVTDQ2PD:           // CONVERT
    case XED_ICLASS_CVTDQ2PS:           // CONVERT
    case XED_ICLASS_CVTPD2DQ:           // CONVERT
    case XED_ICLASS_CVTPD2PI:           // CONVERT
    case XED_ICLASS_CVTPI2PD:           // CONVERT
    case XED_ICLASS_CVTPI2PS:           // CONVERT
    case XED_ICLASS_CVTPS2DQ:           // CONVERT
    case XED_ICLASS_CVTPS2PI:           // CONVERT
    case XED_ICLASS_CVTSD2SI:           // CONVERT
    case XED_ICLASS_CVTSI2SD:           // CONVERT
    case XED_ICLASS_CVTSI2SS:           // CONVERT
    case XED_ICLASS_CVTSS2SI:           // CONVERT
    case XED_ICLASS_CVTTPD2DQ:          // CONVERT
    case XED_ICLASS_CVTTPD2PI:          // CONVERT
    case XED_ICLASS_CVTTPS2DQ:          // CONVERT
    case XED_ICLASS_CVTTPS2PI:          // CONVERT
    case XED_ICLASS_CVTTSD2SI:          // CONVERT
    case XED_ICLASS_CVTTSS2SI:          // CONVERT
    case XED_ICLASS_VCVTDQ2PD:          // CONVERT
    case XED_ICLASS_VCVTDQ2PS:          // CONVERT
    case XED_ICLASS_VCVTPD2DQ:          // CONVERT
    case XED_ICLASS_VCVTPS2DQ:          // CONVERT
    case XED_ICLASS_VCVTSD2SI:          // CONVERT
    case XED_ICLASS_VCVTSI2SD:          // CONVERT
    case XED_ICLASS_VCVTSI2SS:          // CONVERT
    case XED_ICLASS_VCVTSS2SI:          // CONVERT
    case XED_ICLASS_VCVTTPD2DQ:         // CONVERT
    case XED_ICLASS_VCVTTPS2DQ:         // CONVERT
    case XED_ICLASS_VCVTTSD2SI:         // CONVERT
    case XED_ICLASS_VCVTTSS2SI:         // CONVERT
        return IB_cvt;
        
    case XED_ICLASS_BSWAP:              // DATAXFER
    case XED_ICLASS_MASKMOVDQU:         // DATAXFER
    case XED_ICLASS_MASKMOVQ:           // DATAXFER
        return IB_move;

    case XED_ICLASS_MOV:                // DATAXFER
    case XED_ICLASS_MOVAPD:             // DATAXFER
    case XED_ICLASS_MOVAPS:             // DATAXFER
    case XED_ICLASS_MOVDQA:             // DATAXFER
    case XED_ICLASS_MOVDQU:             // DATAXFER
    case XED_ICLASS_MOVD:               // DATAXFER DATAXFER
    case XED_ICLASS_MOVQ:               // DATAXFER DATAXFER
    case XED_ICLASS_VMOVAPD:            // DATAXFER
    case XED_ICLASS_VMOVAPS:            // DATAXFER
    case XED_ICLASS_VMOVD:              // DATAXFER
    case XED_ICLASS_VMOVQ:              // DATAXFER
    case XED_ICLASS_VMOVDQA:            // DATAXFER
    case XED_ICLASS_VMOVDQA64:            // DATAXFER
    case XED_ICLASS_VMOVDQU:            // DATAXFER
    case XED_ICLASS_MOVNTDQ:            // DATAXFER
    case XED_ICLASS_MOVNTI:             // DATAXFER
    case XED_ICLASS_MOVNTPD:            // DATAXFER
    case XED_ICLASS_MOVNTPS:            // DATAXFER
    case XED_ICLASS_MOVNTQ:             // DATAXFER
    case XED_ICLASS_MOVNTSD:            // DATAXFER
    case XED_ICLASS_MOVNTSS:            // DATAXFER
    case XED_ICLASS_VMOVSD:             // DATAXFER
    case XED_ICLASS_VMOVSS:             // DATAXFER
    case XED_ICLASS_VMOVUPD:            // DATAXFER
    case XED_ICLASS_VMOVUPS:            // DATAXFER
    case XED_ICLASS_MOVSS:              // DATAXFER
    case XED_ICLASS_MOVSD_XMM:          // DATAXFER
    case XED_ICLASS_MOVUPD:             // DATAXFER
    case XED_ICLASS_MOVUPS:             // DATAXFER
        return IB_copy;

    case XED_ICLASS_MOVBE:              // DATAXFER
    case XED_ICLASS_MOVDDUP:            // DATAXFER
    case XED_ICLASS_MOVDQ2Q:            // DATAXFER
    case XED_ICLASS_MOVMSKPD:           // DATAXFER
    case XED_ICLASS_MOVMSKPS:           // DATAXFER
    case XED_ICLASS_MOVQ2DQ:            // DATAXFER
    case XED_ICLASS_MOVSHDUP:           // DATAXFER
    case XED_ICLASS_MOVSLDUP:           // DATAXFER
    case XED_ICLASS_MOVSX:              // DATAXFER
    case XED_ICLASS_MOVSXD:             // DATAXFER
    case XED_ICLASS_MOVZX:              // DATAXFER
    case XED_ICLASS_MOV_CR:             // DATAXFER
    case XED_ICLASS_MOV_DR:             // DATAXFER
    case XED_ICLASS_VMOVDDUP:           // DATAXFER
    case XED_ICLASS_VMOVMSKPD:          // DATAXFER
    case XED_ICLASS_VMOVMSKPS:          // DATAXFER
        return IB_move;

    case XED_ICLASS_MOVHPD:             // DATAXFER
    case XED_ICLASS_MOVHPS:             // DATAXFER
    case XED_ICLASS_MOVLPD:             // DATAXFER
    case XED_ICLASS_MOVLPS:             // DATAXFER
    case XED_ICLASS_VMOVHPD:            // DATAXFER
    case XED_ICLASS_VMOVHPS:            // DATAXFER
    case XED_ICLASS_VMOVLPD:            // DATAXFER
    case XED_ICLASS_VMOVLPS:            // DATAXFER
        return IB_copy;

    case XED_ICLASS_MOVHLPS:            // DATAXFER
    case XED_ICLASS_MOVLHPS:            // DATAXFER
        return IB_move;

    case XED_ICLASS_XCHG:               // DATAXFER
        return IB_xchg;

    case XED_ICLASS_AAA:                // DECIMAL
    case XED_ICLASS_AAD:                // DECIMAL
    case XED_ICLASS_AAM:                // DECIMAL
    case XED_ICLASS_AAS:                // DECIMAL
    case XED_ICLASS_DAA:                // DECIMAL
    case XED_ICLASS_DAS:                // DECIMAL
        return IB_add; // ??? BCD manipulations...
            
    case XED_ICLASS_FCMOVB:             // FCMOV
    case XED_ICLASS_FCMOVBE:            // FCMOV
    case XED_ICLASS_FCMOVE:             // FCMOV
    case XED_ICLASS_FCMOVNB:            // FCMOV
    case XED_ICLASS_FCMOVNBE:           // FCMOV
    case XED_ICLASS_FCMOVNE:            // FCMOV
    case XED_ICLASS_FCMOVNU:            // FCMOV
    case XED_ICLASS_FCMOVU:             // FCMOV
        return IB_move;
            
    case XED_ICLASS_CLC:                // FLAGOP
    case XED_ICLASS_CLD:                // FLAGOP
    case XED_ICLASS_CLI:                // FLAGOP
    case XED_ICLASS_CMC:                // FLAGOP
    case XED_ICLASS_LAHF:               // FLAGOP
    case XED_ICLASS_SAHF:               // FLAGOP
    case XED_ICLASS_SALC:               // FLAGOP
    case XED_ICLASS_STC:                // FLAGOP
    case XED_ICLASS_STD:                // FLAGOP
    case XED_ICLASS_STI:                // FLAGOP
        return IB_shift; // ??? flag manipulations...
            
    case XED_ICLASS_BOUND:              // INTERRUPT
    case XED_ICLASS_INT1:               // INTERRUPT
    case XED_ICLASS_INT3:               // INTERRUPT
    case XED_ICLASS_INT:                // INTERRUPT
    case XED_ICLASS_INTO:               // INTERRUPT
    case XED_ICLASS_INVALID:            // INVALID
        return IB_trap;

    case XED_ICLASS_IN:                 // IO
    case XED_ICLASS_INSB:               // IOSTRINGOP
    case XED_ICLASS_INSD:               // IOSTRINGOP
    case XED_ICLASS_INSW:               // IOSTRINGOP
        return IB_port_rd;
    
    case XED_ICLASS_OUT:                // IO
    case XED_ICLASS_OUTSB:              // IOSTRINGOP
    case XED_ICLASS_OUTSD:              // IOSTRINGOP
    case XED_ICLASS_OUTSW:              // IOSTRINGOP
        return IB_port_wr;

    case XED_ICLASS_AND:                // LOGICAL
    case XED_ICLASS_ANDNPD:             // LOGICAL
    case XED_ICLASS_ANDNPS:             // LOGICAL
    case XED_ICLASS_ANDPD:              // LOGICAL
    case XED_ICLASS_ANDPS:              // LOGICAL
    case XED_ICLASS_NOT:                // LOGICAL
    case XED_ICLASS_OR:                 // LOGICAL
    case XED_ICLASS_ORPD:               // LOGICAL
    case XED_ICLASS_ORPS:               // LOGICAL
    case XED_ICLASS_PTEST:              // LOGICAL
    case XED_ICLASS_TEST:               // LOGICAL
    case XED_ICLASS_VANDNPD:            // LOGICAL
    case XED_ICLASS_VANDNPS:            // LOGICAL
    case XED_ICLASS_VANDPD:             // LOGICAL
    case XED_ICLASS_VANDPS:             // LOGICAL
    case XED_ICLASS_VORPD:              // LOGICAL
    case XED_ICLASS_VORPS:              // LOGICAL
    case XED_ICLASS_VPAND:              // LOGICAL
    case XED_ICLASS_VPANDN:             // LOGICAL
    case XED_ICLASS_VPOR:               // LOGICAL
    case XED_ICLASS_VPTEST:             // LOGICAL
    case XED_ICLASS_VTESTPD:            // LOGICAL
    case XED_ICLASS_VTESTPS:            // LOGICAL
    case XED_ICLASS_PAND:               // LOGICAL LOGICAL
    case XED_ICLASS_PANDN:              // LOGICAL LOGICAL
    case XED_ICLASS_POR:                // LOGICAL LOGICAL
        return IB_logical;

    case XED_ICLASS_VPXOR:              // LOGICAL
    case XED_ICLASS_VXORPD:             // LOGICAL
    case XED_ICLASS_VXORPS:             // LOGICAL
    case XED_ICLASS_XOR:                // LOGICAL
    case XED_ICLASS_XORPD:              // LOGICAL
    case XED_ICLASS_XORPS:              // LOGICAL
    case XED_ICLASS_PXOR:               // LOGICAL LOGICAL
        return IB_xor;

    case XED_ICLASS_CLFLUSH:            // MISC
        return IB_store;

    case XED_ICLASS_CPUID:              // MISC
    case XED_ICLASS_PAUSE:              // MISC
    case XED_ICLASS_UD2:                // MISC
        return IB_nop;
        
    case XED_ICLASS_ENTER:              // MISC
    case XED_ICLASS_LEAVE:              // MISC
        return IB_move;

    case XED_ICLASS_LEA:                // MISC
        return IB_lea;
        
    case XED_ICLASS_LFENCE:             // MISC
    case XED_ICLASS_MFENCE:             // MISC
    case XED_ICLASS_SFENCE:             // MISC
    case XED_ICLASS_ADD_LOCK:             // MISC
    case XED_ICLASS_XADD_LOCK:             // MISC
    case XED_ICLASS_BTS_LOCK:             // MISC
    case XED_ICLASS_SUB_LOCK:             // MISC
    case XED_ICLASS_CMPXCHG_LOCK:             // MISC
    case XED_ICLASS_DEC_LOCK:             // MISC
    case XED_ICLASS_INC_LOCK:             // MISC
    case XED_ICLASS_OR_LOCK:             // MISC
        return IB_mem_fence;

    case XED_ICLASS_MONITOR:            // MISC
    case XED_ICLASS_MWAIT:              // MISC
        return IB_privl_op;

    case XED_ICLASS_XLAT:               // MISC
        return IB_misc;

    case XED_ICLASS_EMMS:               // MMX
    case XED_ICLASS_FEMMS:              // MMX
        return IB_move; // ??? clears MMX registers
        

    case XED_ICLASS_NOP:                // NOP WIDENOP
        return IB_nop;

    case XED_ICLASS_PCLMULQDQ:          // PCLMULQDQ
        return IB_mult;

    /* mark PUSH and POPs as ADDs because we create another
       load or store micro-op for each memory operand anyway.
       The described operation is the stack pointer increment. */
    case XED_ICLASS_POP:                // POP
    case XED_ICLASS_POPF:               // POP
    case XED_ICLASS_POPFD:              // POP
    case XED_ICLASS_POPFQ:              // POP
        return IB_load;

    case XED_ICLASS_POPA:               // POP
    case XED_ICLASS_POPAD:              // POP
        return IB_load_conf;
            
    case XED_ICLASS_PREFETCHNTA:        // PREFETCH
    case XED_ICLASS_PREFETCHT0:         // PREFETCH
    case XED_ICLASS_PREFETCHT1:         // PREFETCH
    case XED_ICLASS_PREFETCHT2:         // PREFETCH
    case XED_ICLASS_PREFETCHW:          // PREFETCH
    case XED_ICLASS_PREFETCH_EXCLUSIVE: // PREFETCH
    case XED_ICLASS_PREFETCH_RESERVED:  // PREFETCH
        return IB_prefetch;
            
    case XED_ICLASS_PUSH:               // PUSH
    case XED_ICLASS_PUSHF:              // PUSH
    case XED_ICLASS_PUSHFD:             // PUSH
    case XED_ICLASS_PUSHFQ:             // PUSH
        return IB_store;

    case XED_ICLASS_PUSHA:              // PUSH  general purpose registers
    case XED_ICLASS_PUSHAD:             // PUSH
        return IB_store_conf;
            
    case XED_ICLASS_IRET:               // RET
    case XED_ICLASS_IRETD:              // RET
    case XED_ICLASS_IRETQ:              // RET
    case XED_ICLASS_RET_FAR:            // RET
    case XED_ICLASS_RET_NEAR:           // RET
        return IB_ret;
            
    case XED_ICLASS_RCL:                // ROTATE
    case XED_ICLASS_RCR:                // ROTATE
        return IB_rotate_cc;
        
    case XED_ICLASS_ROL:                // ROTATE
    case XED_ICLASS_ROR:                // ROTATE
        return IB_rotate;
            
    case XED_ICLASS_LDS:                // SEGOP
    case XED_ICLASS_LES:                // SEGOP
    case XED_ICLASS_LFS:                // SEGOP
    case XED_ICLASS_LGS:                // SEGOP
    case XED_ICLASS_LSS:                // SEGOP
        return IB_load;
            
    case XED_ICLASS_CMPXCHG16B:         // SEMAPHORE
    case XED_ICLASS_CMPXCHG8B:          // SEMAPHORE
    case XED_ICLASS_CMPXCHG:            // SEMAPHORE
        return IB_cmp_xchg;

    case XED_ICLASS_XADD:               // SEMAPHORE
        return IB_xchg;
            
    case XED_ICLASS_SAR:                // SHIFT
    case XED_ICLASS_SHL:                // SHIFT
    case XED_ICLASS_SHLD:               // SHIFT
    case XED_ICLASS_SHR:                // SHIFT
    case XED_ICLASS_SHRD:               // SHIFT
        return IB_shift;

    case XED_ICLASS_CMPSB:              // STRINGOP
    case XED_ICLASS_REPE_CMPSB:              // STRINGOP
    case XED_ICLASS_CMPSD:              // STRINGOP
    case XED_ICLASS_CMPSQ:              // STRINGOP
    case XED_ICLASS_CMPSW:              // STRINGOP
        return IB_cmp;
    
    case XED_ICLASS_LODSB:              // STRINGOP
    case XED_ICLASS_LODSD:              // STRINGOP
    case XED_ICLASS_LODSQ:              // STRINGOP
    case XED_ICLASS_LODSW:              // STRINGOP
        return IB_load;

    case XED_ICLASS_MOVSB:              // STRINGOP
    case XED_ICLASS_REP_MOVSB:              // STRINGOP
    case XED_ICLASS_MOVSD:              // STRINGOP
    case XED_ICLASS_REP_MOVSD:              // STRINGOP
    case XED_ICLASS_MOVSQ:              // STRINGOP
    case XED_ICLASS_REP_MOVSQ:              // STRINGOP
    case XED_ICLASS_MOVSW:              // STRINGOP
    case XED_ICLASS_REP_MOVSW:              // STRINGOP
        return IB_move;

    case XED_ICLASS_SCASB:              // STRINGOP
    case XED_ICLASS_REPNE_SCASB:              // STRINGOP
    case XED_ICLASS_SCASD:              // STRINGOP
    case XED_ICLASS_SCASQ:              // STRINGOP
    case XED_ICLASS_SCASW:              // STRINGOP
        return IB_cmp;

    case XED_ICLASS_STOSB:              // STRINGOP
    case XED_ICLASS_REP_STOSB:              // STRINGOP
    case XED_ICLASS_STOSD:              // STRINGOP
    case XED_ICLASS_REP_STOSD:              // STRINGOP
    case XED_ICLASS_STOSQ:              // STRINGOP
    case XED_ICLASS_REP_STOSQ:              // STRINGOP
    case XED_ICLASS_STOSW:              // STRINGOP
    case XED_ICLASS_REP_STOSW:              // STRINGOP
        return IB_store;
        
    case XED_ICLASS_VPCMPESTRI:         // STTNI
    case XED_ICLASS_VPCMPESTRM:         // STTNI
    case XED_ICLASS_VPCMPISTRI:         // STTNI
    case XED_ICLASS_VPCMPISTRM:         // STTNI
        return IB_cmp;
        
    case XED_ICLASS_SYSCALL:            // SYSCALL
    case XED_ICLASS_SYSCALL_AMD:        // SYSCALL
    case XED_ICLASS_SYSENTER:           // SYSCALL
        return IB_jump;
    
    case XED_ICLASS_RSM:                // SYSRET
    case XED_ICLASS_SYSEXIT:            // SYSRET
    case XED_ICLASS_SYSRET:             // SYSRET
    case XED_ICLASS_SYSRET_AMD:         // SYSRET
        return IB_ret;
        
    case XED_ICLASS_ARPL:               // SYSTEM
    case XED_ICLASS_CLGI:               // SYSTEM
    case XED_ICLASS_CLTS:               // SYSTEM
    case XED_ICLASS_GETSEC:             // SYSTEM
    case XED_ICLASS_HLT:                // SYSTEM
    case XED_ICLASS_INVD:               // SYSTEM
    case XED_ICLASS_INVLPG:             // SYSTEM
    case XED_ICLASS_INVLPGA:            // SYSTEM
    case XED_ICLASS_LAR:                // SYSTEM
    case XED_ICLASS_LGDT:               // SYSTEM
    case XED_ICLASS_LIDT:               // SYSTEM
    case XED_ICLASS_LLDT:               // SYSTEM
    case XED_ICLASS_LMSW:               // SYSTEM
    case XED_ICLASS_LSL:                // SYSTEM
    case XED_ICLASS_LTR:                // SYSTEM
    case XED_ICLASS_RDMSR:              // SYSTEM
    case XED_ICLASS_RDPMC:              // SYSTEM
    case XED_ICLASS_RDTSC:              // SYSTEM
    case XED_ICLASS_RDTSCP:             // SYSTEM
    case XED_ICLASS_SGDT:               // SYSTEM
    case XED_ICLASS_SIDT:               // SYSTEM
    case XED_ICLASS_SKINIT:             // SYSTEM
    case XED_ICLASS_SLDT:               // SYSTEM
    case XED_ICLASS_SMSW:               // SYSTEM
    case XED_ICLASS_STGI:               // SYSTEM
    case XED_ICLASS_STR:                // SYSTEM
    case XED_ICLASS_SWAPGS:             // SYSTEM
    case XED_ICLASS_VERR:               // SYSTEM
    case XED_ICLASS_VERW:               // SYSTEM
    case XED_ICLASS_VMLOAD:             // SYSTEM
    case XED_ICLASS_VMMCALL:            // SYSTEM
    case XED_ICLASS_VMRUN:              // SYSTEM
    case XED_ICLASS_VMSAVE:             // SYSTEM
    case XED_ICLASS_WBINVD:             // SYSTEM
    case XED_ICLASS_WRMSR:              // SYSTEM
        return IB_privl_op;

    case XED_ICLASS_JMP:                // UNCOND_BR
    case XED_ICLASS_JMP_FAR:            // UNCOND_BR
        return IB_branch;
        
    case XED_ICLASS_INVEPT:             // VTX
    case XED_ICLASS_INVVPID:            // VTX
    case XED_ICLASS_VMCALL:             // VTX
    case XED_ICLASS_VMCLEAR:            // VTX
    case XED_ICLASS_VMLAUNCH:           // VTX
    case XED_ICLASS_VMPTRLD:            // VTX
    case XED_ICLASS_VMPTRST:            // VTX
    case XED_ICLASS_VMREAD:             // VTX
    case XED_ICLASS_VMRESUME:           // VTX
    case XED_ICLASS_VMWRITE:            // VTX
    case XED_ICLASS_VMXOFF:             // VTX
    case XED_ICLASS_VMXON:              // VTX
        return IB_privl_op;
        
    case XED_ICLASS_F2XM1:              // X87_ALU
        return IB_fn; // pow...
        
    case XED_ICLASS_FABS:               // X87_ALU
    case XED_ICLASS_FADD:               // X87_ALU
    case XED_ICLASS_FADDP:              // X87_ALU
    case XED_ICLASS_FIADD:              // X87_ALU
        return IB_add;
        
    case XED_ICLASS_FBLD:               // X87_ALU
        return IB_cvt;
        
    case XED_ICLASS_FBSTP:              // X87_ALU
        return IB_cvt;
        
    case XED_ICLASS_FCHS:               // X87_ALU
        return IB_add;
        
    case XED_ICLASS_FCOM:               // X87_ALU
    case XED_ICLASS_FCOMI:              // X87_ALU
    case XED_ICLASS_FCOMIP:             // X87_ALU
    case XED_ICLASS_FCOMP:              // X87_ALU
    case XED_ICLASS_FCOMPP:             // X87_ALU
    case XED_ICLASS_FICOM:              // X87_ALU
    case XED_ICLASS_FICOMP:             // X87_ALU
        return IB_cmp;
        
    case XED_ICLASS_FCOS:               // X87_ALU
        return IB_fn; // cosine...
         
    case XED_ICLASS_FDECSTP:            // X87_ALU
        return IB_add;
        
    case XED_ICLASS_FDISI8087_NOP:      // X87_ALU
        return IB_nop;
        
    case XED_ICLASS_FDIV:               // X87_ALU
    case XED_ICLASS_FDIVP:              // X87_ALU
    case XED_ICLASS_FDIVR:              // X87_ALU
    case XED_ICLASS_FDIVRP:             // X87_ALU
    case XED_ICLASS_FIDIV:              // X87_ALU
    case XED_ICLASS_FIDIVR:             // X87_ALU
        return IB_div;
        
    case XED_ICLASS_FENI8087_NOP:       // X87_ALU
        return IB_nop;

    case XED_ICLASS_FFREE:              // X87_ALU
    case XED_ICLASS_FFREEP:             // X87_ALU
        return IB_add;

    case XED_ICLASS_FILD:               // X87_ALU
        return IB_cvt;    // loads int value and converts it to FP
        
    case XED_ICLASS_FINCSTP:            // X87_ALU
        return IB_add;
        
    case XED_ICLASS_FIST:               // X87_ALU
    case XED_ICLASS_FISTP:              // X87_ALU
    case XED_ICLASS_FISTTP:             // X87_ALU
        return IB_cvt;  // converts FP value to int and stores it
        
    case XED_ICLASS_FLD1:               // X87_ALU
    case XED_ICLASS_FLD:                // X87_ALU
    case XED_ICLASS_FLDL2E:             // X87_ALU
    case XED_ICLASS_FLDL2T:             // X87_ALU
    case XED_ICLASS_FLDLG2:             // X87_ALU
    case XED_ICLASS_FLDLN2:             // X87_ALU
    case XED_ICLASS_FLDPI:              // X87_ALU
    case XED_ICLASS_FLDZ:               // X87_ALU
        return IB_move;  // loads immediate values into registers

    case XED_ICLASS_FLDCW:              // X87_ALU
        return IB_load;
        
    case XED_ICLASS_FLDENV:             // X87_ALU
        return IB_load_conf;
        
    case XED_ICLASS_FMUL:               // X87_ALU
    case XED_ICLASS_FMULP:              // X87_ALU
    case XED_ICLASS_FIMUL:              // X87_ALU
        return IB_mult;

    case XED_ICLASS_FNCLEX:             // X87_ALU
    case XED_ICLASS_FNINIT:             // X87_ALU
        return IB_add; // ??? manipulates flags & regs...
        
    case XED_ICLASS_FNOP:               // X87_ALU
        return IB_nop;
        
    case XED_ICLASS_FNSTCW:             // X87_ALU
        return IB_store;

    case XED_ICLASS_FNSTSW:             // X87_ALU
        return IB_move;
        
    case XED_ICLASS_FNSAVE:             // X87_ALU
    case XED_ICLASS_FNSTENV:            // X87_ALU
        return IB_store_conf;
        
    case XED_ICLASS_FPATAN:             // X87_ALU
        return IB_fn; // computes arctan...
        
    case XED_ICLASS_FPREM1:             // X87_ALU
    case XED_ICLASS_FPREM:              // X87_ALU
        return IB_div;
        
    case XED_ICLASS_FPTAN:              // X87_ALU
        return IB_fn; // computes tan...

    case XED_ICLASS_FRNDINT:            // X87_ALU
        return IB_add;
        
    case XED_ICLASS_FRSTOR:             // X87_ALU
        return IB_load_conf;

    case XED_ICLASS_FSCALE:             // X87_ALU
        return IB_add;  // adds power of two to exponent
        
    case XED_ICLASS_FSETPM287_NOP:      // X87_ALU
        return IB_nop;
        
    case XED_ICLASS_FSIN:               // X87_ALU
    case XED_ICLASS_FSINCOS:            // X87_ALU
        return IB_fn; // computes sin/sincos
        
    case XED_ICLASS_FSQRT:              // X87_ALU
        return IB_sqrt;
        
    case XED_ICLASS_FST:                // X87_ALU
    case XED_ICLASS_FSTP:               // X87_ALU
    case XED_ICLASS_FSTPNCE:            // X87_ALU
        return IB_move;   // copy value and then store it to mem

    case XED_ICLASS_FSUB:               // X87_ALU
    case XED_ICLASS_FSUBP:              // X87_ALU
    case XED_ICLASS_FSUBR:              // X87_ALU
    case XED_ICLASS_FSUBRP:             // X87_ALU
    case XED_ICLASS_FISUB:              // X87_ALU
    case XED_ICLASS_FISUBR:             // X87_ALU
        return IB_sub;

    case XED_ICLASS_FTST:               // X87_ALU
    case XED_ICLASS_FUCOM:              // X87_ALU
    case XED_ICLASS_FUCOMI:             // X87_ALU
    case XED_ICLASS_FUCOMIP:            // X87_ALU
    case XED_ICLASS_FUCOMP:             // X87_ALU
    case XED_ICLASS_FUCOMPP:            // X87_ALU
        return IB_cmp;
        
    case XED_ICLASS_FWAIT:              // X87_ALU
    case XED_ICLASS_FXAM:               // X87_ALU
        return IB_cmp;

    case XED_ICLASS_FXCH:               // X87_ALU
        return IB_move;
        
    case XED_ICLASS_FXTRACT:            // X87_ALU
        return IB_move;
        
    case XED_ICLASS_FYL2X:              // X87_ALU
    case XED_ICLASS_FYL2XP1:            // X87_ALU
        return IB_fn;
        
    case XED_ICLASS_XGETBV:             // XSAVE
    case XED_ICLASS_XRSTOR64:           // XSAVE
    case XED_ICLASS_XRSTOR:             // XSAVE
    case XED_ICLASS_XSAVE64:            // XSAVE
    case XED_ICLASS_XSAVE:              // XSAVE
    case XED_ICLASS_XSETBV:             // XSAVE
    case XED_ICLASS_XSAVEOPT64:         // XSAVEOPT
    case XED_ICLASS_XSAVEOPT:           // XSAVEOPT
        return IB_move;

    default:
        fprintf(stderr, "unknown opcode...\n");
        return IB_unknown;
    }

