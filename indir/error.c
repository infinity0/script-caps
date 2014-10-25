/*
 * error.c
 */
#define EXTERN
#include	"error.h"
#include	"my_varargs.h"
#include	<stdio.h>

/* VARARGS1 */
VARARGS(void, error, (error_p_type error_p, ...))
{
#ifndef __STDC__
	error_p_type	error_p;
#endif	/* !__STDC__ */

	va_list ap;

	VA_START(ap, error_p_type, error_p);
	vfprintf(stderr, error_p->message, ap);
	va_end(ap);
	exit(error_p->status);
}


char	*geterr()
{
	extern	int	errno, sys_nerr;
	extern	char	*sys_errlist[];
	static	char	s[64];

	if ((unsigned) errno < sys_nerr)
		return sys_errlist[errno];
	(void) sprintf(s, "Unknown error %d", errno);
	return s;
}
