#include	"l.h"

static	usize	nhunk;
static	usize	tothunk;
static	char*	hunk;

void
errorexit(void)
{

	if(nerrors) {
		if(cout >= 0)
			remove(outfile);
		exits("error");
	}
	exits(0);
}

void
undef(void)
{
	int i;
	Sym *s;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link)
		if(s->type == SXREF)
			diag("%s: not defined", s->name);
}

vlong
atolwhex(char *s)
{
	vlong n;
	int f;

	n = 0;
	f = 0;
	while(*s == ' ' || *s == '\t')
		s++;
	if(*s == '-' || *s == '+') {
		if(*s++ == '-')
			f = 1;
		while(*s == ' ' || *s == '\t')
			s++;
	}
	if(s[0]=='0' && s[1]){
		if(s[1]=='x' || s[1]=='X'){
			s += 2;
			for(;;){
				if(*s >= '0' && *s <= '9')
					n = n*16 + *s++ - '0';
				else if(*s >= 'a' && *s <= 'f')
					n = n*16 + *s++ - 'a' + 10;
				else if(*s >= 'A' && *s <= 'F')
					n = n*16 + *s++ - 'A' + 10;
				else
					break;
			}
		} else
			while(*s >= '0' && *s <= '7')
				n = n*8 + *s++ - '0';
	} else
		while(*s >= '0' && *s <= '9')
			n = n*10 + *s++ - '0';
	if(f)
		n = -n;
	return n;
}

vlong
rnd(vlong v, long r)
{
	long c;

	if(r <= 0)
		return v;
	v += r - 1;
	c = v % r;
	if(c < 0)
		c += r;
	v -= c;
	return v;
}

Prog*
prg(void)
{
	Prog *p;

	while(nhunk < sizeof(Prog))
		gethunk();
	p = (Prog*)hunk;
	nhunk -= sizeof(Prog);
	hunk += sizeof(Prog);

	*p = zprg;
	return p;
}

void*
halloc(usize n)
{
	void *p;

	n = (n+7)&~7;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
gethunk(void)
{
	char *h;
	long nh;

	nh = NHUNK;
	if(tothunk >= 5L*NHUNK) {
		nh = 5L*NHUNK;
		if(tothunk >= 25L*NHUNK)
			nh = 25L*NHUNK;
	}
	h = mysbrk(nh);
	if(h == (void*)-1) {
		diag("out of memory");
		errorexit();
	}

	hunk = h;
	nhunk = nh;
	tothunk += nh;
}

long
hunkspace(void)
{
	return tothunk;
}
