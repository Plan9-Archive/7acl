TEXT	_main(SB), $0
	MOV	$setSB(SB), R28
	MOV	a+8(FP), R1
	MOV	b-8(FP), R2
	ADD	R1, R2, R0
	RETURN	R30

TEXT	_start(SB), $0
	BL	_main(SB)
	RETURN

TEXT	_zot(SB), $-4
l1:
	MOV	a+8(FP), R1
	CASE	R3, R5
l0:	BCASE	l0
	BCASE	l1
	BCASE	l2
	BCASE	l3
	BCASE	l4
l4:
	ERET
l2:
	ERET
l3:
	ERET

TEXT	_bra(SB), $-4
	BEQ	l9
	BNE	l9
	BCS	l9
	BHS	l9
	BCC	l9
	BLO	l9
	BMI	l9
	BPL	l9
	BVS	l9
	BVC	l9
	BHI	l9
	BLS	l9
	BGE	l9
	BLT	l9
	BGT	l9
	BLE	l9
	B	l8
l9:
	BL	_main(SB)
l8:
	CBNZ	R3, l8
	CBNZW	R3, l9
	CBZ	R3, l9
	CBZW	R3, l9
	TBNZ	$7, R3, l9
	TBNZ	$37, R3, l9
	TBZ	$7, R3, l8
	TBZ	$37, R3, l8
	RETURN

TEXT	_excdeb(SB), $-4
	SVC
	SVC	$0x1234
	HVC
	HVC	$0x1234
	SMC
	SMC	$0x1234
	BRK	$0x1234
	HLT	$0x1234
	DCPS1	$0x1234
	DCPS2	$0x1234
	DCPS3	$0x1234
	CLREX
	CLREX	$0x7
	DRPS

	DMB	$0
	DMB	$5
	DSB	$7
	DSB	$0xF
	ISB	$8
	ISB	$9
	RETURN

TEXT	_sys(SB), $-4
	SYS	R5, 1, 2, 3, 4
	SYS	R5, $(1<<16 | 2<<12 | 3<<8 | 4<<5)	/* op1, Cn, Cm, op2 */
	SYS	1, 2, 3, 4
	SYS	$(1<<16 | 2<<12 | 3<<8 | 4<<5)
	SYSL	1, 2, 3, 4, R5
	SYSL	$(1<<16 | 2<<12 | 3<<8 | 4<<5), R5
	RETURN

TEXT	_msr(SB), $-4
	MRS	DAIF, R5
	MSR	R5, DAIF
	MRS	NZCV, R5
	MSR	R5, NZCV
	MRS	FPSR, R6
	MSR	R6, FPSR
	MRS	FPSR, R7
	MSR	R7, FPSR
	MRS	SPSR_EL2, R8
	MRS	CurrentEL, R9
	MRS	SP_EL0, R9
//	MRS	SP_EL1, R9
	MRS	SPSel, R9

	MOV	DAIF, R5
	MOV	R5, DAIF

	MOV	$3, DAIFSet
	MOV	$2, DAIFClr
	MOV	$1, SPSel

	RETURN

TEXT _movwide(SB), $-4
	// bit of a waste, because MOVZ/MOVN disassemble as MOV */
	MOVZ	$0x1234, R1
	MOVZ	$0x1234<<16, R1
	MOVZ	$0x1234<<32, R1
	MOVZ	$0x1234<<48, R1
//	MOVZ	$0x1234<<64, R1	// illegal
	MOVZW	$0x1234, R2
	MOVZW	$0x1234<<16, R2
//	MOVZW	$0x1234<<32, R2	// illegal
	MOVN	$0x1234, R1
	MOVN	$0x1234<<16, R1
	MOVN	$0x1234<<32, R1
	MOVN	$0x1234<<48, R1
	MOVNW	$0x1234, R2
	MOVNW	$0x1234<<16, R2
	MOVK	$0x1234, R1
	MOVK	$0x1234<<16, R1
	MOVK	$0x1234<<32, R1
	MOVK	$0x1234<<48, R1
	MOVKW	$0x1234, R2
	MOVKW	$0x1234<<16, R2
	RETURN

TEXT	_addcon(SB), $-4
	ADD	$0x123, R1, R2
	ADDS $0x123000, R1, R2
	ADDW	$0x123, R1, R2
	ADDSW	$0x123000, R1, R2
	SUB	$0x123, R1, R2
	SUBS	$0x123000, R1, R2
	SUBW	$0x123, R1, R2
	SUBSW	$0x123000, R1, R2
	ADD	$0x100, SP
	SUB	$0x100, SP
	CMP	$0x100, SP
	CMN	$0x100, SP
	CMP	$0x123000,SP
	CMN	$0x123000,SP
	RETURN

TEXT	_muls(SB), $-4
	MADD	R1, R2, R3, R4	// R4=R3+(R2*R1)
	MADDW	R1, R2, R3, R4
	MSUB	R1, R2, R3, R4
	MSUBW	R1, R2, R3, R4
	MUL	R1, R2, R3
	MUL	R1, R3
	MULW	R1, R2, R3
	MULW	R1, R3
	MNEG	R1, R2
	MNEGW	R1, R2, R3
	MNEGW	R1, R2
	SMADDL	R1, R2, R3, R4
	SMSUBL	R1, R2, R3, R4
	SMNEGL	R1, R2, R3
	SMULL	R1, R2, R3
	SMULH	R1, R2, R3
	UMADDL	R1, R2, R3, R4
	UMSUBL	R1, R2, R3, R4
	UMNEGL	R1, R2, R3
	UMULL	R1, R2, R3
	UMULH	R1, R2, R3
	RETURN

TEXT	_ccmn(SB), $-4
	CCMN	NE, R1, R2, $5
	CCMNW	EQ, R2, R3, $15
	CCMN	NE, $0x1F,R2,$5
	CCMNW	EQ, $0x1F,R2,$15
	CCMP	NE, R1, R2, $5
	CCMPW	EQ, R2, R3, $15
	CCMP	NE, $0x1F,R2, $5
	CCMPW	EQ, $0x1F, R2, $15
	RETURN

TEXT	_movcon(SB), $-4
	MOVW	$0x123, R5
	MOV	$0x123, R5
	MOVW	$0x1230000, R4
	MOV	$0x1230000, R4
	MOV	$~0x1230000, R4
	MOV $~0x123, R4
	MOV	$~0, R4
	MOV	$0, R4
	MOV	$-1, R4
	MOVW	$-1, R4
//	MVN	$0x123, R4
//	MOV	$0x123(SP), R4
	RETURN

TEXT	_arithshifted(SB), $-4
	ADD	R1<<32, R2, R3
	ADD	R1, R2, R3
	ADDW	R1<<16, R2, R3
	ADDW	R1, R2, R3
	ADDS	R1<<32, R2, R3
	ADDSW	R1<<16, R2, R3
	ADD		R1<<24,R3,R4	/* logical left */
	ADD		R1>>24,R3,R4	/* logical right */
	ADD		R1->24,R3,R4	/* arithmetic right */
	/* (no rotate for arithmetic ops) */
	SUB	R1<<32, R2, R3
	SUB	R1, R2, R3
	SUBW	R1<<16, R2, R3
	SUBS	R1<<32, R2, R3
	SUBS	R1, R2, R3
	SUBSW	R1<<16, R2, R3
	CMN	R1<<16, R2
	CMNW	R1<<16, R2
	CMP	R1<<16, R2
	CMP	R1, R2
	NEG	R1, R2
	NEG	R1<<16, R2
	NEGW	R1<<16, R2
	NEGS	R1<<16, R2
	NEGSW	R1<<16, R2
	RETURN

TEXT	_arithextended(SB), $-4
	ADD		R3.UXTB << 2,R3,R4	/* extended register */
	ADD		R3.SXTX << 2, R3, R4
	ADDW	R3.UXTW << 2, R3, R4
	ADDW	R3.UXTW, R3, R4
	ADD	R3, SP
	SUB	R1, SP
	ADDS	R3, SP, R4
	ADDSW	R1.UXTX << 2, R3, R4
	ADD	R3.UXTX<<2, R2, R1
	SUB	R3.UXTB<<2, R2, R1
	SUBS	R3.UXTX<<4, SP, R1
	CMN	R3.UXTX<<2, SP
	CMP	R3.UXTX<<2, SP
	CMPW	R3.UXTW, R4
	CMP	R5.SXTW, R4
	RETURN

TEXT	_logshifted(SB), $-4
	/* logical left */
	AND	R1<<32, R2, R3
	ANDS R1<<7, R2, R3
	ANDW	R1<<5, R2, R3
	ANDSW	R1<<16, R2, R3
	ORR	R1<<15, R2, R3
	ORRW	R1<<5, R2, R3
	ORN	R1<<15, R2, R3
	ORNW	R1<<5, R2, R3
	EOR	R1<<17, R2, R3
	EORW	R1<<7, R2, R3
	EON	R1<<19, R2, R3
	EONW	R1<<9, R2, R3
	BIC	R1<<17, R2, R3
	BICW	R1<<7, R2, R3
	BICS	R1<<17, R2, R3
	BICSW	R1<<7, R2, R3

	/* logical right */
	AND	R1>>32, R2, R3
	ANDS R1>>7, R2, R3
	ANDW	R1>>5, R2, R3
	ANDSW	R1>>16, R2, R3
	ORR	R1>>15, R2, R3
	ORRW	R1>>5, R2, R3
	ORN	R1>>15, R2, R3
	ORNW	R1>>5, R2, R3
	EOR	R1>>17, R2, R3
	EORW	R1>>7, R2, R3
	EON	R1>>19, R2, R3
	EONW	R1>>9, R2, R3
	BIC	R1>>17, R2, R3
	BICW	R1>>7, R2, R3
	BICS	R1>>17, R2, R3
	BICSW	R1>>7, R2, R3

	/* arithmetic right */
	AND	R1->32, R2, R3
	ANDS R1->7, R2, R3
	ANDW	R1->5, R2, R3
	ANDSW	R1->16, R2, R3
	ORR	R1->15, R2, R3
	ORRW	R1->5, R2, R3
	ORN	R1->15, R2, R3
	ORNW	R1->5, R2, R3
	EOR	R1->17, R2, R3
	EORW	R1->7, R2, R3
	EON	R1->19, R2, R3
	EONW	R1->9, R2, R3
	BIC	R1->17, R2, R3
	BICW	R1->7, R2, R3
	BICS	R1->17, R2, R3
	BICSW	R1->7, R2, R3

	/* rotate right */
	AND	R1@>32, R2, R3
	ANDS R1@>7, R2, R3
	ANDW	R1@>5, R2, R3
	ANDSW	R1@>16, R2, R3
	ORR	R1@>15, R2, R3
	ORRW	R1@>5, R2, R3
	ORN	R1@>15, R2, R3
	ORNW	R1@>5, R2, R3
	EOR	R1@>17, R2, R3
	EORW	R1@>7, R2, R3
	EON	R1@>19, R2, R3
	EONW	R1@>9, R2, R3
	BIC	R1@>17, R2, R3
	BICW	R1@>7, R2, R3
	BICS	R1@>17, R2, R3
	BICSW	R1@>7, R2, R3

	RETURN

TEXT _movreg(SB), $-4
	MOV	R3, SP
	MOV	SP, R3
	MOV	R1, R2
	MOVW	R1, R2
	MVN	R1, R2
	MVNW	R1, R2
	RETURN

TEXT	_ldxr(SB), $-4
	LDXR	(R5), R4
	STXR	R4, (R5), R3
	RETURN

TEXT	_shrrr(SB), $-4
	LSL	R1, R2, R3
	LSLW	R1, R2, R3
	LSL	ZR,	R1, R2
	ROR	R1, R2, R3
	RORW	R1, R2, R3
	ASR	R1, R2, R3
	ASRW	R1, R2, R3
	RETURN

TEXT	_bits(SB), $-4
	BFMW	$3, $7, R1, R2
	BFM	$33, $37, R1, R2
	SBFMW	$3, $7, R1, R2
	SBFM	$33, $37, R1, R2
	UBFMW	$3, $7, R1, R2
	UBFM	$33, $37, R1, R2

	BFIW	$1, $7, R1, R2
	BFI		$1, $7, R1, R2
	BFXILW	$1, $7, R1, R2
	BFXIL	$1, $7, R1, R2
	SBFIZW	$1, $7, R1, R2
	SBFIZ	$1, $7, R1, R2
	SBFX	$1, $7, R1, R2
	SBFXW	$1, $7, R1, R2
	UBFIZ	$1, $7, R1, R2
	UBFIZW	$1, $7, R1, R2
	UBFX	$1, $7, R1, R2
	UBFXW	$1, $7, R1, R2

	EXTRW	$5, R1, R2, R3
	EXTR	$45, R1, R2, R3
	RETURN

TEXT	_sxt(SB), $-4
	SXTB	R1, R2
	SXTH	R1, R2
	SXTW	R1, R2
	UXTB	R1, R2
	UXTH	R1, R2
	UXTW	R1, R2
	SXTBW	R1, R2
	SXTHW	R1, R2
	UXTBW	R1, R2
	UXTHW	R1, R2
	RETURN

TEXT	_shirr(SB), $-4
	ASR	$5, R1, R2
	ASRW	$5, R1, R2
	ASR	$5, R1
	ASRW	$5, R1
	LSL	$5, R1, R2
	LSLW	$5, R1, R2
	LSL	$5, R1
	LSR	$5, R1, R2
	LSRW	$5, R1, R2
	LSR	$5, R1
	ROR	$5, R1, R2
	RORW	$5, R1, R2
	ROR	$5, R2
	RETURN

TEXT	_bitop(SB), $-4
	CLSW	R1, R2
	CLS	R1, R2
	CLZW	R1, R2
	CLZ	R1, R2
	RBITW	R1, R2
	RBIT	R1, R2
	REVW	R1, R2
	REV	R1, R2
	REV16W	R1, R2
	REV16	R1, R2
	REV32	R1, R2
	RETURN

TEXT	_adrs(SB), $-4
	ADR	l9, R5
	ADRP	l9, R5
//	ADR	$divide(SB), R5
	RETURN

TEXT _divide(SB), $-4
	SDIV	R1, R2, R3
	SDIVW	R1, R2, R3
	UDIV	R1, R2, R3
	UDIVW	R1, R2, R3
	SDIV	R1, R3
	RETURN

TEXT	_crc32(SB), $-4
	CRC32B	R1, R2, R3
	CRC32CB	R1, R2, R3
	CRC32CH	R1, R2, R3
	CRC32CW	R1, R2, R3
	CRC32CX	R1, R2, R3
	CRC32H	R1, R2, R3
	CRC32W	R1, R2, R3
	CRC32X	R1, R2, R3
	RETURN

TEXT	_rem(SB), $-4
	REM	R1, R2, R3
	REMW	R1, R2, R3
	REM	R1, R2
	REMW	R1, R2
	UREM	R1, R2, R3
	UREMW	R1, R2, R3
	UREM	R1, R2
	RETURN

TEXT	_nops(SB), $-4
	NOP
	NOP	R0
	NOP	,R0
	NOP	F0
	NOP	,F0
	YIELD
	WFE
	WFI
	SEV
	SEVL
	HINT	$0	// syn for NOP
	HINT	$6
	HINT	$0x7F
	RETURN

TEXT	_movext(SB), $-4
	MOV	ext6(SB), R1
	MOV	R1, ext6(SB)
	MOVB	ext1(SB), R1
	MOVBU	ext2(SB), R1
	MOVH	ext3(SB), R1
	MOVHU	ext4(SB), R1
	MOVW	ext5(SB), R1
	MOVWU	ext6(SB), R1
	MOVW	R1, ext6(SB)
	MOVWU	R1, ext6(SB)
	MOVH	R1, ext6(SB)
	MOVHU	R1, ext6(SB)
	MOVB	R1, ext6(SB)
	MOVBU	R1, ext6(SB)
	RETURN

TEXT	_lstr9s(SB), $-1
	MOV		-8(R5), R1
	MOV		R1, -16(R5)
	MOVW 	-4(R5), R1
	MOVW	R1, -16(R5)
	MOVWU	-4(R5), R1
	MOVWU	R1, -16(R5)
	MOVB	-4(R5), R1
	MOVB	R1,	-16(R5)
	MOVBU	-4(R5), R1
	MOVBU	R1, -16(R5)
	MOVH	-4(R5), R1
	MOVH	R1, -16(R5)
	MOVHU	-4(R5), R1
	MOVHU	R1, -16(R5)
	RETURN

TEXT	_lstr12u(SB), $-1
	MOV	32760(R5), R1
	MOV	R1, 32760(R5)
	MOVW	16380(R5), R1
	MOVW	R1, 16380(R5)
	MOVH	8190(R5), R1
	MOVH	R1, 8190(R5)
	MOVB	4095(R5), R1
	MOVB	R1, 4095(R5)
	RETURN

#ifdef YYY
TEXT	_amodes(SB), $-4
	MOVW		R3, (R2)
	MOVW		R3, 10(R2)
	MOVW		R3, name(SB)
	MOVW		R3, name(SB)(R2)
	MOVW		R3, name(SB)[R2]
	MOVW		R3, (R2)
	MOVW		R3, (R2)[R1]
	MOV			R3, (R2)[R1]
	MOV			R3, (R2)(R1)
	MOVW		R3, (R2)(R1)
	MOVW		R3, (R2)[R1.SXTX]
	RETURN
#endif

TEXT	_adc(SB), $-4
	ADC	R1, R2, R3
	ADCW	R1, R2, R3
	ADCS	R1, R2, R3
	ADCSW	R1, R2, R3
	SBC	R1, R2, R3
	SBCW	R1, R2, R3
	SBCS	R1, R2, R3
	SBCSW	R1, R2, R3
	NGC	R1, R2
	NGCW	R1, R2
	NGCS	R1, R2
	NGCSW	R1, R2
	RETURN

TEXT	_csel(SB), $-4

	CSEL	EQ, R1, R2, R3
	CSEL	NE, R1, R2, R3
	CSEL	CS, R1, R2, R3
	CSEL	HS, R1, R2, R3
	CSEL	CC, R1, R2, R3
	CSEL	LO, R1, R2, R3
	CSEL	MI, R1, R2, R3
	CSEL	PL, R1, R2, R3
	CSEL	VS, R1, R2, R3
	CSEL	VC, R1, R2, R3
	CSEL	HI, R1, R2, R3
	CSEL	LS, R1, R2, R3
	CSEL	GE, R1, R2, R3
	CSEL	LT, R1, R2, R3
	CSEL	GT, R1, R2, R3
	CSEL	LE, R1, R2, R3
	CSEL	AL, R1, R2, R3

	CSEL	EQ, R1, R2, R3
	CSELW	NE, R1, R2, R3
	CSINC	EQ, R1, R2, R3
	CSINCW	NE, R1, R2, R3
	CSINV	EQ, R1, R2, R3
	CSINVW	NE, R1, R2, R3
	CSNEG	EQ, R1, R2, R3
	CSNEGW	NE, R1, R2, R3

	// aliases, REGZERO, !cond
	CSET	EQ, R1
	CSET	NE, R1
	CSETW	NE, R2
	CSETM	EQ,	R1
	CSETMW	NE, R2

	// aliases, Rm=Rn, !cond
	CINC	EQ, R1, R2
	CINCW	NE, R2, R3
	CINV	EQ, R1, R2
	CINVW	NE, R2, R3
	CNEG	EQ, R1, R2
	CNEGW	NE, R2, R3

	RETURN

TEXT	_fpload(SB), $-4
	FMOVS	-16(R5), F1
	FMOVS	F1, -16(R5)
	FMOVS	16380(R5), F1
	FMOVS	F1, 16380(R5)
	FMOVD	-16(R5), F1
	FMOVD	F1, -16(R5)
	FMOVD	32760(R5), F1
	FMOVD	F1, 32760(R5)

//	FMOVD	F1, F2
//	FMOVS	F1, F2
//	FMOVDS	F1, F2
//	FMOVSD	F1, F2
//	FMOVSW	F1, R2
//	FMOVSW	R2, F1
	RETURN

TEXT	_fpops(SB), $-4
	FCMPD	$0.0, F1
	FCMPS	$0.0, F1
	FCMPD	F1, F2
	FCMPS	F1, F2

	FCMPED	$0.0, F1
	FCMPES	$0.0, F1
	FCMPED	F1, F2
	FCMPES	F1, F2

	FCCMPD	EQ, F1, F2, $7
	FCCMPED	NE, F1, F2, $0xF
	FCCMPS	EQ, F1, F2, $7
	FCCMPES	NE, F1, F2, $0xF

	FADDS	F1, F2, F3
	FADDD	F1, F2, F3
	FSUBS	F1, F2, F3
	FSUBD	F1, F2, F3
	FMULS	F1, F2, F3
	FMULD	F1, F2, F3
	FDIVS	F1, F2, F3
	FDIVD	F1, F2, F3

	FCVTSD	F1, F2
	FCVTDS	F2, F1

	FCVTZSD	F2, R1
	FCVTZSDW	F2, R1
	FCVTZSS	F2, R1
	FCVTZSSW	F2, R1

	FCVTZUD	F2, R1
	FCVTZUDW	F2, R1
	FCVTZUS	F2, R1
	FCVTZUSW	F2, R1

	SCVTFD	R1, F2
	SCVTFS	R1, F2
	SCVTFWD	R1, F2
	SCVTFWS	R1, F2

	UCVTFD	R1, F2
	UCVTFS	R1, F2
	UCVTFWD	R1, F2
	UCVTFWS	R1, F2

	FMAXS	F1, F2, F3
	FMINS	F1, F2, F3
	FMAXD	F1, F2, F3
	FMIND	F1, F2, F3
	FMAXNMS	F1, F2, F3
	FMAXNMD	F1, F2, F3
	FMINNMS	F1, F2, F3
	FMINNMD	F1, F2, F3
	FNMULS	F1, F2, F3
	FNMULD	F1, F2, F3

	FCSELS	NE, F1, F2, F3
	FCSELD	NE, F1, F2, F3

	FABSS	F1, F2
	FABSD	F1, F2
	FNEGS	F1, F2
	FNEGD	F1, F2
	FSQRTS	F1, F2
	FSQRTD	F1, F2

	FRINTNS	F1, F2
	FRINTPS	F1, F2
	FRINTMS	F1, F2
	FRINTZS	F1, F2
	FRINTAS	F1, F2
	FRINTXS	F1, F2
	FRINTIS	F1, F2

	FRINTND	F1, F2
	FRINTPD	F1, F2
	FRINTMS	F1, F2
	FRINTZD	F1, F2
	FRINTAD	F1, F2
	FRINTXD	F1, F2
	FRINTID	F1, F2

	FCVTDH	F1, F2
	FCVTHS	F1, F2
	FCVTHD	F1, F2
	FCVTSH	F1, F2

	RETURN

TEXT	_aes(SB), $-4
	AESE	V1, V2
	AESD	V1, V2
	AESMC	V1, V2
	AESIMC	V1, V2

	SHA1H	V1, V2
	SHA1SU1	V1, V2
	SHA256SU0	V1, V2
	SHA1C	V1, V2, V3
	SHA1P	V1, V2, V3
	SHA1M	V1, V2, V3
	SHA1SU0	V1, V2, V3
	SHA256H	V1, V2, V3
	SHA256H2	V1, V2, V3
	SHA256SU1	V1, V2, V3
	RETURN

TEXT _movacon(SB), $-4
	MOV	$0x3e000, R1	// check that ADDCON<<12 doesn't confuse
	RETURN

GLOBL	ext1+0(SB), $1
GLOBL	ext2+0(SB), $2
GLOBL	ext3+0(SB), $4
GLOBL	ext4+0(SB), $4
GLOBL	ext5+0(SB), $4
GLOBL	ext6+0(SB), $16
GLOBL	name+0(SB), $8
	END
