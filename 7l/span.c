#include	"l.h"

static struct {
	ulong	start;
	ulong	size;
} pool;

void	checkpool(Prog*, int);
void 	flushpool(Prog*, int);
long	pcldr(long, int);

static Optab *badop;

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
				autosize = p->to.offset + SAVESIZE;
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
					autosize = p->to.offset + SAVESIZE;
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
void
checkpool(Prog *p, int skip)
{
	if(pool.size >= 0xffff0 || pcldr(p->pc+4+pool.size - pool.start+8, 0) == 0)
		flushpool(p, skip);
	else if(p->link == P)
		flushpool(p, 2);
}

void
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

long
immrot(ulong)
{
	return 0;
}

long
pcldr(long v, int rt)
{
	if(v >= -0xfffff && v <= 0xfffff && (v&3) == 0)
		return (6<<27) | (((v>>2)&0x7ffff)<<5) | rt;
	return 0;
}

long
imms9(vlong v)
{
	if(v >= -256 && v < 256)
		return (7<<27) | ((v&0x1ff)<<12);
	return 0;
}

long
immaddr(vlong v, int w)
{
	if(w > 1 && (v&(w-1)) == 0)
		v /= w;
	if(v >= 0 && v <= 0xfff)
		return (7<<27) | (1<<24) | ((v&0xfff)<<10);
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
			instoffset = autosize + a->offset + SAVESIZE;
			return autoclass[constclass(instoffset)];

		case D_NONE:
			instoffset = a->offset;
			return oregclass[constclass(instoffset)];
		}
		return C_GOK;

	case D_SPR:
		if(a->reg == D_FPCR)
			return C_FCR;
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
			if(a->reg != NREG)
				goto aconsize;

			v = instoffset;
			if(v >= 0){
				if(v < 0xFFF || (v>>12) < 0xFFF)
					return C_ADDCON;
			}
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
				t = immrot(instoffset);
				if(t && instoffset != 0)
					return C_RECON;
			}
			instoffset = s->value + a->offset + INITDAT;
			return C_LCON;

		case D_AUTO:
			instoffset = autosize + a->offset;
			goto aconsize;

		case D_PARAM:
			instoffset = autosize + a->offset + SAVESIZE;
		aconsize:
			t = immrot(instoffset);
			if(t)
				return C_RACON;
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
	char *c1, *c3;
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
	c3 = xcmp[a3];
	for(; o<e; o++)
		if(o->a2 == a2)
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
	case C_LCON:
		if(b == C_RCON || b == C_NCON || b == C_ADDCON || b == C_MOVCON)
			return 1;
		break;

	case C_MOVCON:
		if(b == C_ADDCON)
			return 1;
		break;

	case C_LACON:
		if(b == C_RACON)
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
		if(b == C_ZOREG || b == C_PSAUTO || b == C_PPAUTO)
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

int
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

		switch(r)
		{
		default:
			diag("unknown op in build: %A", r);
			errorexit();
		case AXXX:
			break;
		case AADD:
			oprange[AADDS] = oprange[r];
			oprange[ASUB] = oprange[r];
			oprange[ASUBS] = oprange[r];
			oprange[AADDW] = oprange[r];
			oprange[AADDSW] = oprange[r];
			oprange[ASUBW] = oprange[r];
			oprange[ASUBSW] = oprange[r];
			break;
		case AAND:
			oprange[AEOR] = oprange[r];
			oprange[ASUB] = oprange[r];
			oprange[AORR] = oprange[r];
			oprange[ABIC] = oprange[r];
			break;
		case AADC:	/* rn=Rd */
			oprange[AADCW] = oprange[r];
			oprange[AADCS] = oprange[r];
			oprange[AADCSW] = oprange[r];
			oprange[ASBC] = oprange[r];
			oprange[ASBCW] = oprange[r];
			oprange[ASBCS] = oprange[r];
			oprange[ASBCSW] = oprange[r];
			break;
		case ANGC:	/* rn=REGZERO */
			oprange[ANGCW] = oprange[r];
			oprange[ANGCS] = oprange[r];
			oprange[ANGCSW] = oprange[r];
			break;
		case ACMP:
			oprange[ACMPW] = oprange[r];
//			oprange[ATST] = oprange[r];
//			oprange[ATEQ] = oprange[r];
//			oprange[ACMN] = oprange[r];
			break;
		case AMVN:
			break;
		case AMOVK:
			oprange[AMOVKW] = oprange[r];
			oprange[AMOVN] = oprange[r];
			oprange[AMOVNW] = oprange[r];
			oprange[AMOVZ] = oprange[r];
			oprange[AMOVZW] = oprange[r];
			break;
		case ABEQ:
			oprange[ABNE] = oprange[r];
			oprange[ABCS] = oprange[r];
			oprange[ABHS] = oprange[r];
			oprange[ABCC] = oprange[r];
			oprange[ABLO] = oprange[r];
			oprange[ABMI] = oprange[r];
			oprange[ABPL] = oprange[r];
			oprange[ABVS] = oprange[r];
			oprange[ABVC] = oprange[r];
			oprange[ABHI] = oprange[r];
			oprange[ABLS] = oprange[r];
			oprange[ABGE] = oprange[r];
			oprange[ABLT] = oprange[r];
			oprange[ABGT] = oprange[r];
			oprange[ABLE] = oprange[r];
			break;
		case ALSL:
			oprange[ALSLW] = oprange[r];
			oprange[ALSR] = oprange[r];
			oprange[ALSRW] = oprange[r];
			oprange[AASR] = oprange[r];
			oprange[AASRW] = oprange[r];
			oprange[AROR] = oprange[r];
			oprange[ARORW] = oprange[r];
			break;
		case ACLS:
			oprange[ACLSW] = oprange[r];
			oprange[ACLZ] = oprange[r];
			oprange[ACLZW] = oprange[r];
			oprange[ARBIT] = oprange[r];
			oprange[ARBITW] = oprange[r];
			oprange[AREV] = oprange[r];
			oprange[AREVW] = oprange[r];
			oprange[AREV16] = oprange[r];
			oprange[AREV16W] = oprange[r];
			oprange[AREV32] = oprange[r];
			break;
		case ASDIV:
			oprange[ASDIVW] = oprange[r];
			oprange[AUDIV] = oprange[r];
			oprange[AUDIVW] = oprange[r];
			oprange[ACRC32B] = oprange[r];
			oprange[ACRC32CB] = oprange[r];
			oprange[ACRC32CH] = oprange[r];
			oprange[ACRC32CW] = oprange[r];
			oprange[ACRC32CX] = oprange[r];
			oprange[ACRC32H] = oprange[r];
			oprange[ACRC32W] = oprange[r];
			oprange[ACRC32X] = oprange[r];
			break;
		case AMADD:
			oprange[AMADDW] = oprange[r];
			oprange[AMSUB] = oprange[r];
			oprange[AMSUBW] = oprange[r];
			oprange[ASMADDL] = oprange[r];
			oprange[ASMSUBL] = oprange[r];
			oprange[ASMULH] = oprange[r];
			oprange[AUMADDL] = oprange[r];
			oprange[AUMSUBL] = oprange[r];
			break;
		case AREM:
			oprange[AREMW] = oprange[r];
			oprange[AUREM] = oprange[r];
			oprange[AUREMW] = oprange[r];
			break;
		case AMUL:
			oprange[AMULW] = oprange[r];
			oprange[AMNEG] = oprange[r];
			oprange[AMNEGW] = oprange[r];
			oprange[ASMNEGL] = oprange[r];
			oprange[ASMULL] = oprange[r];
			oprange[ASMULH] = oprange[r];
			oprange[AUMNEGL] = oprange[r];
			oprange[AUMULH] = oprange[r];
			oprange[AUMULL] = oprange[r];
			break;
		case AMOVH:
			oprange[AMOVHU] = oprange[r];
			break;
		case AMOVW:
			oprange[AMOVWU] = oprange[r];
			break;
		case ABFM:
			oprange[ABFMW] = oprange[r];
			oprange[ASBFM] = oprange[r];
			oprange[ASBFMW] = oprange[r];
			oprange[AUBFM] = oprange[r];
			oprange[AUBFMW] = oprange[r];
			break;
		case ABFI:
			oprange[ABFIW] = oprange[r];
			oprange[ABFXIL] = oprange[r];
			oprange[ABFXILW] = oprange[r];
			oprange[ASBFIZ] = oprange[r];
			oprange[ASBFIZW] = oprange[r];
			oprange[ASBFX] = oprange[r];
			oprange[ASBFXW] = oprange[r];
			oprange[AUBFIZ] = oprange[r];
			oprange[AUBFIZW] = oprange[r];
			oprange[AUBFX] = oprange[r];
			oprange[AUBFXW] = oprange[r];
			break;
		case AEXTR:
			oprange[AEXTRW] = oprange[r];
			break;
		case ASXTB:
			oprange[ASXTBW] = oprange[r];
			oprange[ASXTH] = oprange[r];
			oprange[ASXTHW] = oprange[r];
			oprange[ASXTW] = oprange[r];
			oprange[AUXTB] = oprange[r];
			oprange[AUXTH] = oprange[r];
			oprange[AUXTW] = oprange[r];
			oprange[AUXTBW] = oprange[r];
			oprange[AUXTHW] = oprange[r];
			break;
		case ACCMN:
			oprange[ACCMNW] = oprange[r];
			oprange[ACCMP] = oprange[r];
			oprange[ACCMPW] = oprange[r];
			break;
		case ACSEL:
			oprange[ACSELW] = oprange[r];
			oprange[ACSINC] = oprange[r];
			oprange[ACSINCW] = oprange[r];
			oprange[ACSINV] = oprange[r];
			oprange[ACSINVW] = oprange[r];
			oprange[ACSNEG] = oprange[r];
			oprange[ACSNEGW] = oprange[r];
			// aliases Rm=Rn, !cond
			oprange[ACINC] = oprange[r];
			oprange[ACINCW] = oprange[r];
			oprange[ACINV] = oprange[r];
			oprange[ACINVW] = oprange[r];
			oprange[ACNEG] = oprange[r];
			oprange[ACNEGW] = oprange[r];
			break;
		case ACSET:
			// aliases, Rm=Rn=REGZERO, !cond
			oprange[ACSETW] = oprange[r];
			oprange[ACSETM] = oprange[r];
			oprange[ACSETMW] = oprange[r];
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
			oprange[ANOP] = oprange[r];
			oprange[AWFE] = oprange[r];
			oprange[AWFI] = oprange[r];
			oprange[AYIELD] = oprange[r];
			oprange[ASEV] = oprange[r];
			oprange[ASEVL] = oprange[r];
			break;
		case ACBZ:
			oprange[ACBZW] = oprange[r];
			oprange[ACBNZ] = oprange[r];
			oprange[ACBNZW] = oprange[r];
			break;
		case ATBZ:
			oprange[ATBNZ] = oprange[r];
			break;
		case AADR:
		case AADRP:
			break;
		case ACLREX:
			oprange[ADRPS] = oprange[r];
			break;
		case ASVC:
			oprange[AHLT] = oprange[r];
			oprange[AHVC] = oprange[r];
			oprange[ASMC] = oprange[r];
			oprange[ABRK] = oprange[r];
			oprange[ADCPS1] = oprange[r];
			oprange[ADCPS2] = oprange[r];
			oprange[ADCPS3] = oprange[r];
			break;
		case AFADDS:
			oprange[AFADDD] = oprange[r];
			oprange[AFSUBS] = oprange[r];
			oprange[AFSUBD] = oprange[r];
			oprange[AFMULS] = oprange[r];
			oprange[AFMULD] = oprange[r];
			oprange[AFDIVS] = oprange[r];
			oprange[AFDIVD] = oprange[r];
			oprange[AFCVTSD] = oprange[r];
			oprange[AFCVTDS] = oprange[r];
			break;
		case AFCMPS:
			oprange[AFCMPD] = oprange[r];
			oprange[AFCMPES] = oprange[r];
			oprange[AFCMPED] = oprange[r];
			break;
		case AFCCMPS:
			oprange[AFCCMPD] = oprange[r];
			oprange[AFCCMPES] = oprange[r];
			oprange[AFCCMPED] = oprange[r];
			break;

		case AFMOVS:
		case AFMOVD:
			break;

		case AFCVTZSD:
			oprange[AFCVTZSDW] = oprange[r];
			oprange[AFCVTZSS] = oprange[r];
			oprange[AFCVTZSSW] = oprange[r];
			oprange[AFCVTZUD] = oprange[r];
			oprange[AFCVTZUDW] = oprange[r];
			oprange[AFCVTZUS] = oprange[r];
			oprange[AFCVTZUSW] = oprange[r];
			break;
		case ASCVTFD:
			oprange[ASCVTFS] = oprange[r];
			oprange[ASCVTFWD] = oprange[r];
			oprange[ASCVTFWS] = oprange[r];
			oprange[AUCVTFD] = oprange[r];
			oprange[AUCVTFS] = oprange[r];
			oprange[AUCVTFWD] = oprange[r];
			oprange[AUCVTFWS] = oprange[r];
			break;

		case AMRS:
		case AMSR:
			/* TO DO */
			break;
		}
	}
}

/*
void
buildrep(int x, int as)
{
	Opcross *p;
	Optab *e, *s, *o;
	int a1, a2, a3, n;

	if(C_NONE != 0 || C_REG != 1 || C_GOK >= 32 || x >= nelem(opcross)) {
		diag("assumptions fail in buildrep");
		errorexit();
	}
	repop[as] = x;
	p = (opcross + x);
	s = oprange[as].start;
	e = oprange[as].stop;
	for(o=e-1; o>=s; o--) {
		n = o-optab;
		for(a2=0; a2<2; a2++) {
			if(a2) {
				if(o->a2 == C_NONE)
					continue;
			} else
				if(o->a2 != C_NONE)
					continue;
			for(a1=0; a1<32; a1++) {
				if(!xcmp[a1][o->a1])
					continue;
				for(a3=0; a3<32; a3++)
					if(xcmp[a3][o->a3])
						(*p)[a1][a2][a3] = n;
			}
		}
	}
	oprange[as].start = 0;
}
*/

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
asmdyn()
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
