#include	"l.h"

static struct {
	ulong	start;
	ulong	size;
} pool;

static void		checkpool(Prog*, int);
static void 	flushpool(Prog*, int);
static long	pcldr(long, int);

static Optab *badop;
static Oprang	oprange[ALAST];

void
span(void)
{
	Prog *p;
	Sym *setext, *s;
	Optab *o;
	int m, bflag, i;
	long c, otxt, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);

	bflag = 0;
	c = INITTEXT;
	otxt = c;
	for(p = firstp; p != P; p = p->link) {
		p->pc = c;
		o = oplook(p);
		m = o->size;
		if(m == 0) {
			if(p->as == ATEXT) {
				curtext = p;
				autosize = p->to.offset + PCSZ;
				if(p->from.sym != S)
					p->from.sym->value = c;
				/* need passes to resolve branches */
				if(c-otxt >= 1L<<20)
					bflag = 1;
				otxt = c;
				continue;
			}
			diag("zero-width instruction\n%P", p);
			continue;
		}
		switch(o->flag & (LFROM|LTO)) {
		case LFROM:
			addpool(p, &p->from);
			break;
		case LTO:
			addpool(p, &p->to);
			break;
		}
		if(p->as == AB || p->as == ARET || p->as == AERET || p->as == ARETURN)	/* TO DO: other unconditional operations */
			checkpool(p, 0);
		c += m;
		if(blitrl)
			checkpool(p, 1);
	}

	/*
	 * if any procedure is large enough to
	 * generate a large SBRA branch, then
	 * generate extra passes putting branches
	 * around jmps to fix. this is rare.
	 */
	while(bflag) {
		if(debug['v'])
			Bprint(&bso, "%5.2f span1\n", cputime());
		bflag = 0;
		c = INITTEXT;
		for(p = firstp; p != P; p = p->link) {
			p->pc = c;
			o = oplook(p);
/* very large branches
			if(o->type == 6 && p->cond) {
				otxt = p->cond->pc - c;
				if(otxt < 0)
					otxt = -otxt;
				if(otxt >= (1L<<17) - 10) {
					q = prg();
					q->link = p->link;
					p->link = q;
					q->as = AB;
					q->to.type = D_BRANCH;
					q->cond = p->cond;
					p->cond = q;
					q = prg();
					q->link = p->link;
					p->link = q;
					q->as = AB;
					q->to.type = D_BRANCH;
					q->cond = q->link->link;
					bflag = 1;
				}
			}
 */
			m = o->size;
			if(m == 0) {
				if(p->as == ATEXT) {
					curtext = p;
					autosize = p->to.offset + PCSZ;
					if(p->from.sym != S)
						p->from.sym->value = c;
					continue;
				}
				diag("zero-width instruction\n%P", p);
				continue;
			}
			c += m;
		}
	}

	if(debug['t']) {
		/* 
		 * add strings to text segment
		 */
		c = rnd(c, 8);
		for(i=0; i<NHASH; i++)
		for(s = hash[i]; s != S; s = s->link) {
			if(s->type != SSTRING)
				continue;
			v = s->value;
			while(v & 3)
				v++;
			s->value = c;
			c += v;
		}
	}

	c = rnd(c, 8);

	setext = lookup("etext", 0);
	if(setext != S) {
		setext->value = c;
		textsize = c - INITTEXT;
	}
	if(INITRND)
		INITDAT = rnd(c, INITRND);
	if(debug['v'])
		Bprint(&bso, "tsize = %lux\n", textsize);
	Bflush(&bso);
}

/*
 * when the first reference to the literal pool threatens
 * to go out of range of a 1Mb PC-relative offset
 * drop the pool now, and branch round it.
 */
static void
checkpool(Prog *p, int skip)
{
	if(pool.size >= 0xffff0 || pcldr(p->pc+4+pool.size - pool.start+8, 0) == 0)
		flushpool(p, skip);
	else if(p->link == P)
		flushpool(p, 2);
}

static void
flushpool(Prog *p, int skip)
{
	Prog *q;

	if(blitrl) {
		if(skip){
			if(debug['v'] && skip == 1)
				print("note: flush literal pool at %#llux: len=%lud ref=%lux\n", p->pc+4, pool.size, pool.start);
			q = prg();
			q->as = AB;
			q->to.type = D_BRANCH;
			q->cond = p->link;
			q->link = blitrl;
			blitrl = q;
		}
		else if(p->pc+pool.size-pool.start < 1024*1024)
			return;
		elitrl->link = p->link;
		p->link = blitrl;
		blitrl = 0;	/* BUG: should refer back to values until out-of-range */
		elitrl = 0;
		pool.size = 0;
		pool.start = 0;
	}
}

/*
 * TO DO: hash
 */
void
addpool(Prog *p, Adr *a)
{
	Prog *q, t;
	int c;

	c = aclass(a);

	t = zprg;
	t.as = AWORD;	/* TO DO: DWORD */
	if(p->as == AMOV)
		t.as = ADWORD;

	switch(c) {
	default:
		t.to = *a;
		break;

	case C_PSAUTO:
	case C_PPAUTO:
	case C_UAUTO4K:
	case C_UAUTO8K:
	case C_UAUTO16K:
	case C_UAUTO32K:
	case C_UAUTO64K:
	case C_NSAUTO:
	case C_NPAUTO:
	case C_LAUTO:
	case C_PPOREG:
	case C_PSOREG:
	case C_UOREG4K:
	case C_UOREG8K:
	case C_UOREG16K:
	case C_UOREG32K:
	case C_UOREG64K:
	case C_NSOREG:
	case C_NPOREG:
	case C_LOREG:
		t.to.type = D_CONST;
		t.to.offset = instoffset;
		break;
	}

	for(q = blitrl; q != P; q = q->link)	/* could hash on t.t0.offset */
		if(memcmp(&q->to, &t.to, sizeof(t.to)) == 0) {
			p->cond = q;
			return;
		}

	q = prg();
	*q = t;
	q->pc = pool.size;

	if(blitrl == P) {
		blitrl = q;
		pool.start = p->pc;
	} else
		elitrl->link = q;
	elitrl = q;
	pool.size += 4;

	p->cond = q;
}

void
xdefine(char *p, int t, long v)
{
	Sym *s;

	s = lookup(p, 0);
	if(s->type == 0 || s->type == SXREF) {
		s->type = t;
		s->value = v;
	}
}

long
regoff(Adr *a)
{

	instoffset = 0;
	aclass(a);
	return instoffset;
}

long
bitrot(vlong v)
{
	int i;

	for(i=0; i<64; i++) {
		if((v & ~0xff) == 0)
			return (i<<8) | v | (1<<25);
		v = (v<<2) | (v>>30);
	}
	return 0;
}

static int
isaddcon(vlong v)
{
	if(v < 0)
		return 0;
	if((v & 0xFFF) == 0)
		v >>= 12;
	return v < 0xFFF;
}

static long
pcldr(long v, int rt)
{
	if(v >= -0xfffff && v <= 0xfffff && (v&3) == 0)
		return (6<<27) | (((v>>2)&0x7ffff)<<5) | rt;
	return 0;
}

static int	autoclass[] = {C_PSAUTO, C_NSAUTO, C_NPAUTO, C_PSAUTO, C_PPAUTO, C_UAUTO4K, C_UAUTO8K, C_UAUTO16K, C_UAUTO32K, C_UAUTO64K, C_LAUTO};
static int	oregclass[] = {C_ZOREG, C_NSOREG, C_NPOREG, C_PSOREG, C_PPOREG, C_UOREG4K, C_UOREG8K, C_UOREG16K, C_UOREG32K, C_UOREG64K, C_LOREG};
static int	sextclass[] = {C_SEXT1, C_LEXT, C_LEXT, C_SEXT1, C_SEXT1, C_SEXT1, C_SEXT2, C_SEXT4, C_SEXT8, C_SEXT16, C_LEXT};

/*
 * return appropriate index into tables above
 */
static int
constclass(vlong l)
{
	if(l == 0)
		return 0;
	if(l < 0){
		if(l >= -256)
			return 1;
		if(l >= -512 && (l&7) == 0)
			return 2;
		return 10;
	}
	if(l <= 255)
		return 3;
	if(l <= 504 && (l&7) == 0)
		return 4;
	if(l <= 4095)
		return 5;
	if(l <= 8190 && (l&1) == 0)
		return 6;
	if(l <= 16380 && (l&3) == 0)
		return 7;
	if(l <= 32760 && (l&7) == 0)
		return 8;
	if(l <= 65520 && (l&0xF) == 0)
		return 9;
	return 10;
}

vlong
offsetshift(vlong v, int c)
{
	vlong vs;
	int s;
	static int shifts[] = {0, 1, 2, 3, 4};

	s = 0;
	if(c >= C_SEXT1 && c <= C_SEXT16)
		s = shifts[c-C_SEXT1];
	else if(c >= C_UAUTO4K && c <= C_UAUTO64K)
		s = shifts[c-C_UAUTO4K];
	else if(c >= C_UOREG4K && c <= C_UOREG64K)
		s = shifts[c-C_UOREG4K];
	vs = v>>s;
	if(vs<<s != v)
		diag("odd offset: %lld\n%P", v, curp);
	return vs;
}

int
movcon(vlong v)
{
	int s;

	for(s = 0; s < 64; s += 16)
		if((v & ~((uvlong)0xFFFF<<s)) == 0)
			return s/16;
	return -1;
}

int
aclass(Adr *a)
{
	vlong v;
	Sym *s;
	int t;

	instoffset = 0;
	switch(a->type) {
	case D_NONE:
		return C_NONE;

	case D_REG:
		return C_REG;

	case D_SP:
		return C_RSP;

	case D_COND:
		return C_COND;

	case D_SHIFT:
		return C_SHIFT;

	case D_EXTREG:
		return C_EXTREG;

	case D_ROFF:
		return C_ROFF;

	case D_XPOST:
		return C_XPOST;

	case D_XPRE:
		return C_XPRE;

	case D_FREG:
		return C_FREG;

	case D_OREG:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			if(a->sym == 0 || a->sym->name == 0) {
				print("null sym external\n");
				print("%D\n", a);
				return C_GOK;
			}
			s = a->sym;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			if(dlm) {
				switch(t) {
				default:
					instoffset = s->value + a->offset + INITDAT;
					break;
				case SUNDEF:
				case STEXT:
				case SCONST:
				case SLEAF:
				case SSTRING:
					instoffset = s->value + a->offset;
					break;
				}
				return C_ADDR;
			}
			instoffset = s->value + a->offset;
			if(instoffset >= 0)
				return sextclass[constclass(instoffset)];
			return C_LEXT;

		case D_AUTO:
			instoffset = autosize + a->offset;
			return autoclass[constclass(instoffset)];

		case D_PARAM:
			instoffset = autosize + a->offset + PCSZ;
			return autoclass[constclass(instoffset)];

		case D_NONE:
			instoffset = a->offset;
			return oregclass[constclass(instoffset)];
		}
		return C_GOK;

	case D_SPR:
		return C_SPR;

	case D_OCONST:
		switch(a->name) {
		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			t = s->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
			}
			instoffset = s->value + a->offset + INITDAT;
			if(s->type == STEXT || s->type == SLEAF || s->type == SUNDEF)
				instoffset = s->value + a->offset;
			return C_LCON;
		}
		return C_GOK;

	case D_FCONST:
		return C_FCON;

	case D_CONST:
		switch(a->name) {

		case D_NONE:
			instoffset = a->offset;
			if(a->reg != NREG && a->reg != REGZERO)
				goto aconsize;

			v = instoffset;
			if(v == 0)
				return C_ZCON;
			if(isaddcon(v))
				return C_ADDCON;
			t = movcon(v);
			if(t >= 0)
				return C_MOVCON;
			t = movcon(~v);
			if(t >= 0)
				return C_MOVCON;
			return C_LCON;

		case D_EXTERN:
		case D_STATIC:
			s = a->sym;
			if(s == S)
				break;
			t = s->type;
			switch(t) {
			case 0:
			case SXREF:
				diag("undefined external: %s in %s",
					s->name, TNAME);
				s->type = SDATA;
				break;
			case SUNDEF:
			case STEXT:
			case SSTRING:
			case SCONST:
			case SLEAF:
				instoffset = s->value + a->offset;
				return C_LCON;
			}
			if(!dlm) {
				instoffset = s->value + a->offset - BIG;
				if(isaddcon(instoffset))
					return C_AECON;
			}
			instoffset = s->value + a->offset + INITDAT;
			return C_LCON;

		case D_AUTO:
			instoffset = autosize + a->offset;
			goto aconsize;

		case D_PARAM:
			instoffset = autosize + a->offset + PCSZ;
		aconsize:
			if(isaddcon(instoffset))
				return C_AACON;
			return C_LACON;
		}
		return C_GOK;

	case D_BRANCH:
		return C_SBRA;
	}
	return C_GOK;
}

Optab*
oplook(Prog *p)
{
	int a1, a2, a3, r;
	char *c1, *c2, *c3;
	Optab *o, *e;

	a1 = p->optab;
	if(a1)
		return optab+(a1-1);
	a1 = p->from.class;
	if(a1 == 0) {
		a1 = aclass(&p->from) + 1;
		p->from.class = a1;
	}
	a1--;
	a3 = p->to.class;
	if(a3 == 0) {
		a3 = aclass(&p->to) + 1;
		p->to.class = a3;
	}
	a3--;
	a2 = C_NONE;
	if(p->reg != NREG)
		a2 = C_REG;
	r = p->as;
	o = oprange[r].start;
	if(o == 0) {
		a1 = opcross[repop[r]][a1][a2][a3];
		if(a1) {
			p->optab = a1+1;
			return optab+a1;
		}
		o = oprange[r].stop; /* just generate an error */
	}
	if(0) {
		print("oplook %A %d %d %d\n",
			(int)p->as, a1, a2, a3);
		print("		%d %d\n", p->from.type, p->to.type);
	}
	e = oprange[r].stop;
	c1 = xcmp[a1];
	c2 = xcmp[a2];
	c3 = xcmp[a3];
	for(; o<e; o++)
		if(o->a2 == a2 || c2[o->a2])
		if(c1[o->a1])
		if(c3[o->a3]) {
			p->optab = (o-optab)+1;
			return o;
		}
	diag("illegal combination %A %d %d %d",
		p->as, a1, a2, a3);
	prasm(p);
	o = badop;
	if(o == 0)
		errorexit();
	return o;
}

int
cmp(int a, int b)
{

	if(a == b)
		return 1;
	switch(a) {
	case C_RSP:
		if(b == C_REG)
			return 1;
		break;

	case C_REG:
		if(b == C_ZCON)
			return 1;
		break;

	case C_ADDCON:
		if(b == C_ZCON)
			return 1;
		break;

	case C_LCON:
		if(b == C_ZCON || b == C_BITCON || b == C_ADDCON || b == C_MOVCON)
			return 1;
		break;

	case C_VCON:
		return cmp(C_LCON, b);

	case C_MOVCON:
		if(b == C_ADDCON || b == C_ZCON)
			return 1;
		break;

	case C_LACON:
		if(b == C_AACON)
			return 1;
		break;

	case C_SEXT2:
		if(b == C_SEXT1)
			return 1;
		break;

	case C_SEXT4:
		if(b == C_SEXT1 || b == C_SEXT2)
			return 1;
		break;

	case C_SEXT8:
		if(b >= C_SEXT1 && b <= C_SEXT4)
			return 1;
		break;

	case C_SEXT16:
		if(b >= C_SEXT1 && b <= C_SEXT8)
			return 1;
		break;

	case C_LEXT:
		if(b >= C_SEXT1 && b <= C_SEXT16)
			return 1;
		break;

	case C_PPAUTO:
		if(b == C_PSAUTO)
			return 1;
		break;

	case C_UAUTO4K:
		if(b == C_PSAUTO || b == C_PPAUTO)
			return 1;
		break;

	case C_UAUTO8K:
		return cmp(C_UAUTO4K, b);

	case C_UAUTO16K:
		return cmp(C_UAUTO8K, b);

	case C_UAUTO32K:
		return cmp(C_UAUTO16K, b);

	case C_UAUTO64K:
		return cmp(C_UAUTO32K, b);

	case C_NPAUTO:
		return cmp(C_NSAUTO, b);

	case C_LAUTO:
		return cmp(C_NPAUTO, b) || cmp(C_UAUTO64K, b);

	case C_PSOREG:
		if(b == C_ZOREG)
			return 1;
		break;

	case C_PPOREG:
		if(b == C_ZOREG || b == C_PSOREG)
			return 1;
		break;

	case C_UOREG4K:
		if(b == C_ZOREG || b == C_PSAUTO || b == C_PSOREG || b == C_PPAUTO || b == C_PPOREG)
			return 1;
		break;

	case C_UOREG8K:
		return cmp(C_UOREG4K, b);

	case C_UOREG16K:
		return cmp(C_UOREG8K, b);

	case C_UOREG32K:
		return cmp(C_UOREG16K, b);

	case C_UOREG64K:
		return cmp(C_UOREG32K, b);

	case C_NPOREG:
		return cmp(C_NSOREG, b);

	case C_LOREG:
		return cmp(C_NPOREG, b) || cmp(C_UOREG64K, b);

	case C_LBRA:
		if(b == C_SBRA)
			return 1;
		break;
	}
	return 0;
}

static int
ocmp(const void *a1, const void *a2)
{
	Optab *p1, *p2;
	int n;

	p1 = (Optab*)a1;
	p2 = (Optab*)a2;
	n = p1->as - p2->as;
	if(n)
		return n;
	n = p1->a1 - p2->a1;
	if(n)
		return n;
	n = p1->a2 - p2->a2;
	if(n)
		return n;
	n = p1->a3 - p2->a3;
	if(n)
		return n;
	return 0;
}

void
buildop(void)
{
	int i, n, r;
	Oprang t;

	for(i=0; i<C_GOK; i++)
		for(n=0; n<C_GOK; n++)
			xcmp[i][n] = cmp(n, i);
	for(n=0; optab[n].as != AXXX; n++)
		;
	badop = optab+n;
	qsort(optab, n, sizeof(optab[0]), ocmp);
	for(i=0; i<n; i++) {
		r = optab[i].as;
		oprange[r].start = optab+i;
		while(optab[i].as == r)
			i++;
		oprange[r].stop = optab+i;
		i--;

		t = oprange[r];
		switch(r){
		default:
			diag("unknown op in build: %A", r);
			errorexit();
		case AXXX:
			break;
		case AADD:
			oprange[AADDS] = t;
			oprange[ASUB] = t;
			oprange[ASUBS] = t;
			oprange[AADDW] = t;
			oprange[AADDSW] = t;
			oprange[ASUBW] = t;
			oprange[ASUBSW] = t;
			break;
		case AAND:	/* logical immediate, logical shifted register */
			oprange[AANDS] = t;
			oprange[AANDSW] = t;
			oprange[AANDW] = t;
			oprange[AEOR] = t;
			oprange[AEORW] = t;
			oprange[AORR] = t;
			oprange[AORRW] = t;
			break;
		case ABIC:	/* only logical shifted register */
			oprange[ABICS] = t;
			oprange[ABICSW] = t;
			oprange[ABICW] = t;
			oprange[AEON] = t;
			oprange[AEONW] = t;
			oprange[AORN] = t;
			oprange[AORNW] = t;
			break;
		case ANEG:
			oprange[ANEGS] = t;
			oprange[ANEGSW] = t;
			oprange[ANEGW] = t;
			break;
		case AADC:	/* rn=Rd */
			oprange[AADCW] = t;
			oprange[AADCS] = t;
			oprange[AADCSW] = t;
			oprange[ASBC] = t;
			oprange[ASBCW] = t;
			oprange[ASBCS] = t;
			oprange[ASBCSW] = t;
			break;
		case ANGC:	/* rn=REGZERO */
			oprange[ANGCW] = t;
			oprange[ANGCS] = t;
			oprange[ANGCSW] = t;
			break;
		case ACMP:
			oprange[ACMPW] = t;
			oprange[ACMN] = t;
			oprange[ACMNW] = t;
			break;
		case ATST:
			oprange[ATSTW] = t;
			break;
		case AMVN:
			/* register/register, and shifted */
			oprange[AMVNW] = t;
			break;
		case AMOVK:
			oprange[AMOVKW] = t;
			oprange[AMOVN] = t;
			oprange[AMOVNW] = t;
			oprange[AMOVZ] = t;
			oprange[AMOVZW] = t;
			break;
		case ABEQ:
			oprange[ABNE] = t;
			oprange[ABCS] = t;
			oprange[ABHS] = t;
			oprange[ABCC] = t;
			oprange[ABLO] = t;
			oprange[ABMI] = t;
			oprange[ABPL] = t;
			oprange[ABVS] = t;
			oprange[ABVC] = t;
			oprange[ABHI] = t;
			oprange[ABLS] = t;
			oprange[ABGE] = t;
			oprange[ABLT] = t;
			oprange[ABGT] = t;
			oprange[ABLE] = t;
			break;
		case ALSL:
			oprange[ALSLW] = t;
			oprange[ALSR] = t;
			oprange[ALSRW] = t;
			oprange[AASR] = t;
			oprange[AASRW] = t;
			oprange[AROR] = t;
			oprange[ARORW] = t;
			break;
		case ACLS:
			oprange[ACLSW] = t;
			oprange[ACLZ] = t;
			oprange[ACLZW] = t;
			oprange[ARBIT] = t;
			oprange[ARBITW] = t;
			oprange[AREV] = t;
			oprange[AREVW] = t;
			oprange[AREV16] = t;
			oprange[AREV16W] = t;
			oprange[AREV32] = t;
			break;
		case ASDIV:
			oprange[ASDIVW] = t;
			oprange[AUDIV] = t;
			oprange[AUDIVW] = t;
			oprange[ACRC32B] = t;
			oprange[ACRC32CB] = t;
			oprange[ACRC32CH] = t;
			oprange[ACRC32CW] = t;
			oprange[ACRC32CX] = t;
			oprange[ACRC32H] = t;
			oprange[ACRC32W] = t;
			oprange[ACRC32X] = t;
			break;
		case AMADD:
			oprange[AMADDW] = t;
			oprange[AMSUB] = t;
			oprange[AMSUBW] = t;
			oprange[ASMADDL] = t;
			oprange[ASMSUBL] = t;
			oprange[AUMADDL] = t;
			oprange[AUMSUBL] = t;
			break;
		case AREM:
			oprange[AREMW] = t;
			oprange[AUREM] = t;
			oprange[AUREMW] = t;
			break;
		case AMUL:
			oprange[AMULW] = t;
			oprange[AMNEG] = t;
			oprange[AMNEGW] = t;
			oprange[ASMNEGL] = t;
			oprange[ASMULL] = t;
			oprange[ASMULH] = t;
			oprange[AUMNEGL] = t;
			oprange[AUMULH] = t;
			oprange[AUMULL] = t;
			break;
		case AMOVH:
			oprange[AMOVHU] = t;
			break;
		case AMOVW:
			oprange[AMOVWU] = t;
			break;
		case ABFM:
			oprange[ABFMW] = t;
			oprange[ASBFM] = t;
			oprange[ASBFMW] = t;
			oprange[AUBFM] = t;
			oprange[AUBFMW] = t;
			break;
		case ABFI:
			oprange[ABFIW] = t;
			oprange[ABFXIL] = t;
			oprange[ABFXILW] = t;
			oprange[ASBFIZ] = t;
			oprange[ASBFIZW] = t;
			oprange[ASBFX] = t;
			oprange[ASBFXW] = t;
			oprange[AUBFIZ] = t;
			oprange[AUBFIZW] = t;
			oprange[AUBFX] = t;
			oprange[AUBFXW] = t;
			break;
		case AEXTR:
			oprange[AEXTRW] = t;
			break;
		case ASXTB:
			oprange[ASXTBW] = t;
			oprange[ASXTH] = t;
			oprange[ASXTHW] = t;
			oprange[ASXTW] = t;
			oprange[AUXTB] = t;
			oprange[AUXTH] = t;
			oprange[AUXTW] = t;
			oprange[AUXTBW] = t;
			oprange[AUXTHW] = t;
			break;
		case ACCMN:
			oprange[ACCMNW] = t;
			oprange[ACCMP] = t;
			oprange[ACCMPW] = t;
			break;
		case ACSEL:
			oprange[ACSELW] = t;
			oprange[ACSINC] = t;
			oprange[ACSINCW] = t;
			oprange[ACSINV] = t;
			oprange[ACSINVW] = t;
			oprange[ACSNEG] = t;
			oprange[ACSNEGW] = t;
			// aliases Rm=Rn, !cond
			oprange[ACINC] = t;
			oprange[ACINCW] = t;
			oprange[ACINV] = t;
			oprange[ACINVW] = t;
			oprange[ACNEG] = t;
			oprange[ACNEGW] = t;
			break;
		case ACSET:
			// aliases, Rm=Rn=REGZERO, !cond
			oprange[ACSETW] = t;
			oprange[ACSETM] = t;
			oprange[ACSETMW] = t;
			break;
		case AMOV:
		case AMOVB:
		case AMOVBU:
		case AB:
		case ABL:
		case AWORD:
		case ADWORD:
		case ARET:
		case ATEXT:
		case ACASE:
		case ABCASE:
			break;
		case AERET:
			oprange[ANOP] = t;
			oprange[AWFE] = t;
			oprange[AWFI] = t;
			oprange[AYIELD] = t;
			oprange[ASEV] = t;
			oprange[ASEVL] = t;
			oprange[ADRPS] = t;
			break;
		case ACBZ:
			oprange[ACBZW] = t;
			oprange[ACBNZ] = t;
			oprange[ACBNZW] = t;
			break;
		case ATBZ:
			oprange[ATBNZ] = t;
			break;
		case AADR:
		case AADRP:
			break;
		case ACLREX:
			break;
		case ASVC:
			oprange[AHLT] = t;
			oprange[AHVC] = t;
			oprange[ASMC] = t;
			oprange[ABRK] = t;
			oprange[ADCPS1] = t;
			oprange[ADCPS2] = t;
			oprange[ADCPS3] = t;
			break;
		case AFADDS:
			oprange[AFADDD] = t;
			oprange[AFSUBS] = t;
			oprange[AFSUBD] = t;
			oprange[AFMULS] = t;
			oprange[AFMULD] = t;
			oprange[AFDIVS] = t;
			oprange[AFDIVD] = t;
			oprange[AFCVTSD] = t;
			oprange[AFCVTDS] = t;
			break;
		case AFCMPS:
			oprange[AFCMPD] = t;
			oprange[AFCMPES] = t;
			oprange[AFCMPED] = t;
			break;
		case AFCCMPS:
			oprange[AFCCMPD] = t;
			oprange[AFCCMPES] = t;
			oprange[AFCCMPED] = t;
			break;

		case AFMOVS:
		case AFMOVD:
			break;

		case AFCVTZSD:
			oprange[AFCVTZSDW] = t;
			oprange[AFCVTZSS] = t;
			oprange[AFCVTZSSW] = t;
			oprange[AFCVTZUD] = t;
			oprange[AFCVTZUDW] = t;
			oprange[AFCVTZUS] = t;
			oprange[AFCVTZUSW] = t;
			break;
		case ASCVTFD:
			oprange[ASCVTFS] = t;
			oprange[ASCVTFWD] = t;
			oprange[ASCVTFWS] = t;
			oprange[AUCVTFD] = t;
			oprange[AUCVTFS] = t;
			oprange[AUCVTFWD] = t;
			oprange[AUCVTFWS] = t;
			break;

		case ASYS:
			oprange[AAT] = t;
			oprange[ADC] = t;
			oprange[AIC] = t;
			oprange[ATLBI] = t;
			break;

		case ASYSL:
		case AHINT:
			break;

		case ADMB:
			oprange[ADSB] = t;
			oprange[AISB] = t;
			break;

		case AMRS:
		case AMSR:
			break;

		case ALDXR:
			oprange[ALDXRB] = t;
			oprange[ALDXRH] = t;
			oprange[ALDXRW] = t;
			break;
		case ALDXP:
			oprange[ALDXPW] = t;
			break;
		case ASTXR:
			oprange[ASTXRB] = t;
			oprange[ASTXRH] = t;
			oprange[ASTXRW] = t;
			break;
		case ASTXP:
			oprange[ASTXPW] = t;
			break;
			
		}
	}
}

enum{
	ABSD = 0,
	ABSU = 1,
	RELD = 2,
	RELU = 3,
};

int modemap[4] = { 0, 1, -1, 2, };

typedef struct Reloc Reloc;

struct Reloc
{
	int n;
	int t;
	uchar *m;
	ulong *a;
};

Reloc rels;

static void
grow(Reloc *r)
{
	int t;
	uchar *m, *nm;
	ulong *a, *na;

	t = r->t;
	r->t += 64;
	m = r->m;
	a = r->a;
	r->m = nm = malloc(r->t*sizeof(uchar));
	r->a = na = malloc(r->t*sizeof(ulong));
	memmove(nm, m, t*sizeof(uchar));
	memmove(na, a, t*sizeof(ulong));
	free(m);
	free(a);
}

void
dynreloc(Sym *s, long v, int abs)
{
	int i, k, n;
	uchar *m;
	ulong *a;
	Reloc *r;

	if(v&3)
		diag("bad relocation address");
	v >>= 2;
	if(s != S && s->type == SUNDEF)
		k = abs ? ABSU : RELU;
	else
		k = abs ? ABSD : RELD;
	/* Bprint(&bso, "R %s a=%ld(%lx) %d\n", s->name, a, a, k); */
	k = modemap[k];
	r = &rels;
	n = r->n;
	if(n >= r->t)
		grow(r);
	m = r->m;
	a = r->a;
	for(i = n; i > 0; i--){
		if(v < a[i-1]){	/* happens occasionally for data */
			m[i] = m[i-1];
			a[i] = a[i-1];
		}
		else
			break;
	}
	m[i] = k;
	a[i] = v;
	r->n++;
}

static int
sput(char *s)
{
	char *p;

	p = s;
	while(*s)
		cput(*s++);
	cput(0);
	return  s-p+1;
}

void
asmdyn(void)
{
	int i, n, t, c;
	Sym *s;
	ulong la, ra, *a;
	vlong off;
	uchar *m;
	Reloc *r;

	cflush();
	off = seek(cout, 0, 1);
	lput(0);
	t = 0;
	lput(imports);
	t += 4;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->type == SUNDEF){
				lput(s->sig);
				t += 4;
				t += sput(s->name);
			}
	
	la = 0;
	r = &rels;
	n = r->n;
	m = r->m;
	a = r->a;
	lput(n);
	t += 4;
	for(i = 0; i < n; i++){
		ra = *a-la;
		if(*a < la)
			diag("bad relocation order");
		if(ra < 256)
			c = 0;
		else if(ra < 65536)
			c = 1;
		else
			c = 2;
		cput((c<<6)|*m++);
		t++;
		if(c == 0){
			cput(ra);
			t++;
		}
		else if(c == 1){
			wput(ra);
			t += 2;
		}
		else{
			lput(ra);
			t += 4;
		}
		la = *a++;
	}

	cflush();
	seek(cout, off, 0);
	lput(t);

	if(debug['v']){
		Bprint(&bso, "import table entries = %d\n", imports);
		Bprint(&bso, "export table entries = %d\n", exports);
	}
}
