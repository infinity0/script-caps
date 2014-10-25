#include	<sys/param.h>
#include	<sys/stat.h>
#include	<stdio.h>

#define		COMMENT		'#'
#define		MAGIC		'?'
#define		TILDE		'~'
#define		SUBST		'%'
#define		MAXLEN		256
#define		MAXARGV		1024

#ifdef	NCARGS
#if NCARGS < MAXARGV
#undef	MAXARGV
#define		MAXARGV		NCARGS
#endif	/* NCARGS < MAXARGV */
#endif	/* NCARGS */
