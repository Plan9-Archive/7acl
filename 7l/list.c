#include "l.h"

void
listinit(void)
{

	fmtinstall('A', Aconv);
	fmtinstall('D', Dconv);
	fmtinstall('P', Pconv);
	fmtinstall('S', Sconv);
	fmtinstall('N', Nconv);
}

void
prasm(Prog *p)
{
	print("%P\n", p);
}

int
Pconv(Fmt *fp)
{
	char str[STRINGSZ], *s;
	Prog *p;
	int a;

	p = va_arg(fp->args, Prog*);
	curp = p;
	a = p->as;
	switch(a) {
	default:
		s = str;
		s += sprint(s, "(%ld)", p->line);
		if(p->reg == NREG && p->from3.type == D_NONE)
			sprint(s, "	%A	%D,%D",
				a, &p->from, &p->to);
		else if(p->from.type != D_FREG){
			s += sprint(s, "	%A	%D", a, &p->from);
			if(p->from3.type != D_NONE)
				s += sprint(s, ",%D", &p->from3);
			if(p->reg != NREG)
				s += sprint(s, ",R%d", p->reg);
			sprint(s, ",%D", &p->to);
		}else
			sprint(s, "	%A	%D,F%d,%D",
				a, &p->from, p->reg, &p->to);
		break;

	case ADATA:
	case AINIT:
	case ADYNT:
		sprint(str, "(%ld)	%A	%D/%d,%D",
			p->line, a, &p->from, p->reg, &p->to);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Aconv(Fmt *fp)
{
	char *s;
	int a;

	a = va_arg(fp->args, int);
	s = "???";
	if(a >= AXXX && a < ALAST && anames[a])
		s = anames[a];
	return fmtstrcpy(fp, s);
}

char*	strcond[16] =
{
	"EQ",
	"NE",
	"HS",
	"LO",
	"MI",
	"PL",
	"VS",
	"VC",
	"HI",
	"LS",
	"GE",
	"LT",
	"GT",
	"LE",
	"AL",
	"NV"
};

int
Dconv(Fmt *fp)
{
	char str[STRINGSZ];
	char *op;
	Adr *a;
	long v;
	static char *extop[] = {".UB", ".UH", ".UW", ".UX", ".SB", ".SH", ".SW", ".SX"};

	a = va_arg(fp->args, Adr*);
	switch(a->type) {

	default:
		sprint(str, "GOK-type(%d)", a->type);
		break;

	case D_NONE:
		str[0] = 0;
		if(a->name != D_NONE || a->reg != NREG || a->sym != S)
			sprint(str, "%N(R%d)(NONE)", a, a->reg);
		break;

	case D_CONST:
		if(a->reg == NREG)
			sprint(str, "$%N", a);
		else
			sprint(str, "$%N(R%d)", a, a->reg);
		break;

	case D_SHIFT:
		v = a->offset;
		op = "<<>>->@>" + (((v>>5) & 3) << 1);
		if(v & (1<<4))
			sprint(str, "R%ld%c%cR%ld", v&15, op[0], op[1], (v>>8)&15);
		else
			sprint(str, "R%ld%c%c%ld", v&15, op[0], op[1], (v>>7)&31);
		if(a->reg != NREG)
			sprint(str+strlen(str), "(R%d)", a->reg);
		break;

	case D_OCONST:
		sprint(str, "$*$%N", a);
		if(a->reg != NREG)
			sprint(str, "%N(R%d)(CONST)", a, a->reg);
		break;

	case D_OREG:
		if(a->reg != NREG)
			sprint(str, "%N(R%d)", a, a->reg);
		else
			sprint(str, "%N", a);
		break;

	case D_XPRE:
		if(a->reg != NREG)
			sprint(str, "%N(R%d)!", a, a->reg);
		else
			sprint(str, "%N!", a);
		break;

	case D_XPOST:
		if(a->reg != NREG)
			sprint(str, "(R%d)%N!", a->reg, a);
		else
			sprint(str, "%N!", a);
		break;

	case D_EXTREG:
		v = a->offset;
		if(v & (7<<10))
			snprint(str, sizeof(str), "R%ld%s<<%ld", (v>>16)&31, extop[(v>>13)&7], (v>>10)&7);
		else
			snprint(str, sizeof(str), "R%ld%s", (v>>16)&31, extop[(v>>13)&7]);
		break;

	case D_ROFF:
		v = a->offset;
		if(v & (1<<16))
			snprint(str, sizeof(str), "(R%d)[R%ld%s]", a->reg, v&31, extop[(v>>8)&7]);
		else
			snprint(str, sizeof(str), "(R%d)(R%ld%s)", a->reg, v&31, extop[(v>>8)&7]);
		break;

	case D_REG:
		sprint(str, "R%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_COND:
		strcpy(str, strcond[a->reg & 0xF]);
		break;

	case D_FREG:
		sprint(str, "F%d", a->reg);
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(R%d)(REG)", a, a->reg);
		break;

	case D_SPR:
		switch(a->reg){
		case D_FPSR:
			sprint(str, "FPSR");
			break;
		case D_FPCR:
			sprint(str, "FPCR");
			break;
		case D_NZCV:
			sprint(str, "NZCV");
			break;
		default:
			sprint(str, "SPR%d", a->reg);
			break;
		}
		if(a->name != D_NONE || a->sym != S)
			sprint(str, "%N(SPR%d)(REG)", a, a->reg);
		break;

	case D_BRANCH:	/* botch */
		if(curp->cond != P) {
			v = curp->cond->pc;
			if(a->sym != S)
				sprint(str, "%s+%#.5lux(BRANCH)", a->sym->name, v);
			else
				sprint(str, "%.5lux(BRANCH)", v);
		} else
			if(a->sym != S)
				sprint(str, "%s+%ld(APC)", a->sym->name, a->offset);
			else
				sprint(str, "%ld(APC)", a->offset);
		break;

	case D_FCONST:
		sprint(str, "$%e", ieeedtod(a->ieee));
		break;

	case D_SCONST:
		sprint(str, "$\"%S\"", a->sval);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Nconv(Fmt *fp)
{
	char str[STRINGSZ];
	Adr *a;
	Sym *s;

	a = va_arg(fp->args, Adr*);
	s = a->sym;
	switch(a->name) {
	default:
		sprint(str, "GOK-name(%d)", a->name);
		break;

	case D_NONE:
		sprint(str, "%ld", a->offset);
		break;

	case D_EXTERN:
		if(s == S)
			sprint(str, "%ld(SB)", a->offset);
		else
			sprint(str, "%s+%ld(SB)", s->name, a->offset);
		break;

	case D_STATIC:
		if(s == S)
			sprint(str, "<>+%ld(SB)", a->offset);
		else
			sprint(str, "%s<>+%ld(SB)", s->name, a->offset);
		break;

	case D_AUTO:
		if(s == S)
			sprint(str, "%ld(SP)", a->offset);
		else
			sprint(str, "%s-%ld(SP)", s->name, -a->offset);
		break;

	case D_PARAM:
		if(s == S)
			sprint(str, "%ld(FP)", a->offset);
		else
			sprint(str, "%s+%ld(FP)", s->name, a->offset);
		break;
	}
	return fmtstrcpy(fp, str);
}

int
Sconv(Fmt *fp)
{
	int i, c;
	char str[STRINGSZ], *p, *a;

	a = va_arg(fp->args, char*);
	p = str;
	for(i=0; i<sizeof(long); i++) {
		c = a[i] & 0xff;
		if(c >= 'a' && c <= 'z' ||
		   c >= 'A' && c <= 'Z' ||
		   c >= '0' && c <= '9' ||
		   c == ' ' || c == '%') {
			*p++ = c;
			continue;
		}
		*p++ = '\\';
		switch(c) {
		case 0:
			*p++ = 'z';
			continue;
		case '\\':
		case '"':
			*p++ = c;
			continue;
		case '\n':
			*p++ = 'n';
			continue;
		case '\t':
			*p++ = 't';
			continue;
		}
		*p++ = (c>>6) + '0';
		*p++ = ((c>>3) & 7) + '0';
		*p++ = (c & 7) + '0';
	}
	*p = 0;
	return fmtstrcpy(fp, str);
}

void
diag(char *fmt, ...)
{
	char buf[STRINGSZ], *tn;
	va_list arg;

	tn = "??none??";
	if(curtext != P && curtext->from.sym != S)
		tn = curtext->from.sym->name;
	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	print("%s: %s\n", tn, buf);

	nerrors++;
	if(nerrors > 10) {
		print("too many errors\n");
		errorexit();
	}
}
