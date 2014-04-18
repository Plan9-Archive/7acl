#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../7c/7.out.h"
#include	"../8l/elf.h"

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

void	addlibpath(char*);
int	fileexists(char*);
char*	findlib(char*);

typedef vlong int64;

typedef	struct	Adr	Adr;
typedef	struct	Sym	Sym;
typedef	struct	Autom	Auto;
typedef	struct	Prog	Prog;
typedef	struct	Optab	Optab;
typedef	struct	Oprang	Oprang;
typedef	uchar	Opcross[32][2][32];
typedef	struct	Count	Count;

#define	P		((Prog*)0)
#define	S		((Sym*)0)
#define	TNAME		(curtext&&curtext->from.sym?curtext->from.sym->name:noname)

struct	Adr
{
	union
	{
		long	u0offset;
		char*	u0sval;
		Ieee*	u0ieee;
	} u0;
	union
	{
		Auto*	u1autom;
		Sym*	u1sym;
	} u1;
	char	type;
	char	reg;
	char	name;
	char	class;
};

#define	offset	u0.u0offset
#define	sval	u0.u0sval
#define	ieee	u0.u0ieee

#define	autom	u1.u1autom
#define	sym	u1.u1sym

struct	Prog
{
	Adr	from;
	Adr	from3;	/* third register operand */
	Adr	to;
	union
	{
		long	u0regused;
		Prog*	u0forwd;
	} u0;
	Prog*	cond;
	Prog*	link;
	long	pc;
	long	line;
	uchar	mark;
	uchar	optab;
	ushort	as;
	uchar	reg;
};
#define	regused	u0.u0regused
#define	forwd	u0.u0forwd

struct	Sym
{
	char	*name;
	short	type;
	short	version;
	short	become;
	short	frame;
	uchar	subtype;
	ushort	file;
	long	value;
	long	sig;
	Sym*	link;
};

#define SIGNINTERN	(1729*325*1729)

struct	Autom
{
	Sym*	asym;
	Auto*	link;
	long	aoffset;
	short	type;
};
struct	Optab
{
	ushort	as;
	char	a1;
	char	a2;
	char	a3;
	char	type;
	char	size;
	char	param;
	char	flag;
};
struct	Oprang
{
	Optab*	start;
	Optab*	stop;
};
struct	Count
{
	long	count;
	long	outof;
};

enum
{
	STEXT		= 1,
	SDATA,
	SBSS,
	SDATA1,
	SXREF,
	SLEAF,
	SFILE,
	SCONST,
	SSTRING,
	SUNDEF,

	SIMPORT,
	SEXPORT,

	LFROM		= 1<<0,
	LTO		= 1<<1,
	LPOOL		= 1<<2,

	C_SCOND		= 0xF,

	C_NONE		= 0,
	C_REG,
	C_SHIFT,		/* D_SHIFT: shift type, amount, value */
	C_EXTREG,	/* D_EXTREG: reg, ext type, shift */
	C_FREG,
	C_SPR,
	C_FCR,
	C_COND,

	C_RCON,		/* rotated bitmask constant */
	C_NCON,		/* ~RCON */
	C_LCON,		/* 32-bit constant */
	C_FCON,		/* floating-point constant */
	C_VCON,		/* 64-bit constant */

	C_RACON,	/* rotated bitmask offset in auto constant $a(FP) */
	C_LACON,	/* 32-bit offset in auto constant $a(FP) */

	C_RECON,	/* rotated bitmask offset in extern constant $e(SB) */

	C_SBRA,
	C_LBRA,

	C_NPAUTO,	/* -512 <= x < 0, 0 mod 8 */
	C_NSAUTO,	/* -256 <= x < 0 */
	C_PSAUTO,	/* 0 to 255 */
	C_PPAUTO,	/* 0 to 504, 0 mod 8 */
	C_UAUTO4K,	/* 0 to 4095 */
	C_UAUTO8K,	/* 0 to 8190, 0 mod 2 */
	C_UAUTO16K,	/* 0 to 16380, 0 mod 4 */
	C_UAUTO32K,	/* 0 to 32760, 0 mod 8 */
	C_UAUTO64K,	/* 0 to 65520, 0 mod 16 */
	C_LAUTO,		/* any other 32-bit constant */

	C_SEXT,		/* 0 to 4095, direct */
	C_LEXT,

	C_NPOREG,	/* mirror NPAUTO etc, except for ZOREG */
	C_NSOREG,
	C_ZOREG,
	C_PSOREG,
	C_PPOREG,
	C_UOREG4K,
	C_UOREG8K,
	C_UOREG16K,
	C_UOREG32K,
	C_UOREG64K,
	C_LOREG,

	C_ADDR,		/* relocatable address for dynamic loading */
	C_ROFF,		/* register offset (inc register extended) */
	C_XPOST,
	C_XPRE,

	C_GOK,

/* mark flags */
	FOLL		= 1<<0,
	LABEL	= 1<<1,
	LEAF		= 1<<2,
	FLOAT		= 1<<3,
	BRANCH		= 1<<4,
	LOAD		= 1<<5,
	SYNC		= 1<<6,
	NOSCHED		= 1<<7,

	BIG		= (1<<12)-4,
	STRINGSZ	= 200,
	NHASH		= 10007,
	NHUNK		= 100000,
	MINSIZ		= 64,
	NENT		= 100,
	MAXIO		= 8192,
	MAXHIST		= 20,	/* limit of path elements for history symbols */

	Roffset	= 22,		/* no. bits for offset in relocation address */
	Rindex	= 10,		/* no. bits for index in relocation address */

	SAVESIZE	= 16,		/* size of save area at top of frame */
	STACKALIGN = 16,	/* alignment of stack */
	PCSZ=	8,	/* size of PC */
};

EXTERN union
{
	struct
	{
		uchar	obuf[MAXIO];			/* output buffer */
		uchar	ibuf[MAXIO];			/* input buffer */
	} u;
	char	dbuf[1];
} buf;

#define	cbuf	u.obuf
#define	xbuf	u.ibuf

EXTERN	long	HEADR;			/* length of header */
EXTERN	int	HEADTYPE;		/* type of header */
EXTERN	long	INITDAT;		/* data location */
EXTERN	long	INITRND;		/* data round above text location */
EXTERN	long	INITTEXT;		/* text location */
EXTERN	long	INITTEXTP;		/* text location (physical) */
EXTERN	char*	INITENTRY;		/* entry point */
EXTERN	long	autosize;
EXTERN	Biobuf	bso;
EXTERN	long	bsssize;
EXTERN	int	cbc;
EXTERN	uchar*	cbp;
EXTERN	int	cout;
EXTERN	Auto*	curauto;
EXTERN	Auto*	curhist;
EXTERN	Prog*	curp;
EXTERN	Prog*	curtext;
EXTERN	Prog*	datap;
EXTERN	long	datsize;
EXTERN	char	debug[128];
EXTERN	Prog*	etextp;
EXTERN	Prog*	firstp;
EXTERN	char	fnuxi4[4];
EXTERN	char	fnuxi8[8];
EXTERN	char*	noname;
EXTERN	Sym*	hash[NHASH];
EXTERN	Sym*	histfrog[MAXHIST];
EXTERN	int	histfrogp;
EXTERN	int	histgen;
EXTERN	char*	library[50];
EXTERN	char*	libraryobj[50];
EXTERN	int	libraryp;
EXTERN	int	xrefresolv;
EXTERN	char*	hunk;
EXTERN	char	inuxi1[1];
EXTERN	char	inuxi2[2];
EXTERN	char	inuxi4[4];
EXTERN	Prog*	lastp;
EXTERN	long	lcsize;
EXTERN	char	literal[32];
EXTERN	int	nerrors;
EXTERN	long	nhunk;
EXTERN	vlong	instoffset;
EXTERN	Opcross	opcross[8];
EXTERN	Oprang	oprange[ALAST];
EXTERN	char*	outfile;
EXTERN	long	pc;
EXTERN	uchar	repop[ALAST];
EXTERN	long	symsize;
EXTERN	Prog*	textp;
EXTERN	long	textsize;
EXTERN	long	thunk;
EXTERN	int	version;
EXTERN	char	xcmp[C_GOK+1][C_GOK+1];
EXTERN	Prog	zprg;
EXTERN	int	dtype;

EXTERN	int	doexp, dlm;
EXTERN	int	imports, nimports;
EXTERN	int	exports, nexports;
EXTERN	char*	EXPTAB;
EXTERN	Prog	undefp;

#define	UP	(&undefp)

extern	char*	anames[];
extern	Optab	optab[];

void	addpool(Prog*, Adr*);
EXTERN	Prog*	blitrl;
EXTERN	Prog*	elitrl;

EXTERN	Prog*	prog_div;
EXTERN	Prog*	prog_divu;
EXTERN	Prog*	prog_mod;
EXTERN	Prog*	prog_modu;

#pragma	varargck	type	"A"	int
#pragma	varargck	type	"A"	uint
#pragma	varargck	type	"C"	int
#pragma	varargck	type	"D"	Adr*
#pragma	varargck	type	"N"	Adr*
#pragma	varargck	type	"P"	Prog*
#pragma	varargck	type	"S"	char*

#pragma	varargck	argpos	diag 1

int	Aconv(Fmt*);
int	Cconv(Fmt*);
int	Dconv(Fmt*);
int	Nconv(Fmt*);
int	Pconv(Fmt*);
int	Sconv(Fmt*);
int	aclass(Adr*);
void	addhist(long, int);
void	addlibpath(char*);
void	append(Prog*, Prog*);
void	asmb(void);
void	asmdyn(void);
void	asmlc(void);
void	asmout(Prog*, Optab*);
void	asmsym(void);
long	atolwhex(char*);
Prog*	brloop(Prog*);
void	buildop(void);
void	buildrep(int, int);
void	cflush(void);
void	ckoff(Sym*, long);
int	chipfloat(Ieee*);
int	cmp(int, int);
int	compound(Prog*);
double	cputime(void);
void	datblk(long, long, int);
void	diag(char*, ...);
void	divsig(void);
void	dodata(void);
void	doprof1(void);
void	doprof2(void);
void	dynreloc(Sym*, long, int);
long	entryvalue(void);
void	errorexit(void);
void	exchange(Prog*);
void	export(void);
int	fileexists(char*);
int	find1(long, int);
char*	findlib(char*);
void	follow(void);
void	gethunk(void);
void	histtoauto(void);
double	ieeedtod(Ieee*);
long	ieeedtof(Ieee*);
void	import(void);
int	isnop(Prog*);
void	ldobj(int, long, char*);
void	loadlib(void);
void	listinit(void);
Sym*	lookup(char*, int);
void	cput(int);
void	llput(vlong);
void	llputl(vlong);
void	lput(long);
void	lputl(long);
void	mkfwd(void);
void*	mysbrk(ulong);
void	names(void);
void	nocache(Prog*);
void	nuxiinit(void);
void	objfile(char*);
int	ocmp(const void*, const void*);
long	opirr(int);
Optab*	oplook(Prog*);
void	patch(void);
void	prasm(Prog*);
void	prepend(Prog*, Prog*);
Prog*	prg(void);
int	pseudo(Prog*);
void	putsymb(char*, int, long, int);
void	readundefs(char*, int);
long	regoff(Adr*);
int	relinv(int);
long	rnd(long, long);
void	span(void);
void	strnput(char*, int);
void	undef(void);
void	undefsym(Sym*);
void	wput(long);
void	wputl(long);
void	xdefine(char*, int, long);
void	xfol(Prog*);
void	zerosig(char*);
void	noops(void);
long	immrot(ulong);
long	immaddr(vlong, int);
