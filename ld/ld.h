#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"../ld/elf.h"

typedef vlong int64;

typedef	struct	Auto	Auto;
typedef	struct	Sym	Sym;

#ifndef	EXTERN
#define	EXTERN	extern
#endif

#define	LIBNAMELEN	300

#define SIGNINTERN	(1729*325*1729)

struct	Auto
{
	Sym*	asym;
	Auto*	link;
	vlong	aoffset;
	short	type;
};

void	addlibpath(char*);
int	fileexists(char*);
char*	findlib(char*);
uchar*	readsome(int, uchar*, uchar*, uchar*, int);
void	collapsefrog(Sym*);
void	addlib(char*);
void	addlibroot(void);
