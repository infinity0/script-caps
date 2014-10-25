/*
 * my_varargs.h
 */
#ifndef MY_VARARGS_H
#define MY_VARARGS_H

#ifdef	__STDC__

#define		EXTERN_VARARGS(type, f, args)	extern	type	f args
#define		VARARGS(type, f, args)		type	f args
#define		VA_START(ap, type, start)	va_start(ap, start)
#include	<stdarg.h>

#else	/* __STDC__ */

#define		EXTERN_VARARGS(type, f, args)	extern	type	f()
#define		VARARGS(type, f, args)		type	f(va_alist) \
						va_dcl
#define		VA_START(ap, type, start)	va_start(ap); \
						start = va_arg(ap, type)
#include	<varargs.h>

#endif	/* __STDC__ */

#endif	/* !MY_VARARGS_H */
