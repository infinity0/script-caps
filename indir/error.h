/*
 * error.h
 */

#ifndef ERROR_H
#define ERROR_H

struct	error_s {
	int	status;
	char	*message;
};
typedef struct	error_s *error_p_type;

#include	"my_varargs.h"
EXTERN_VARARGS(void, error, (error_p_type error, ...));
extern	char	*geterr();

#ifndef EXTERN
#define EXTERN	extern
#define INIT_ERROR_S(value, message)
#else	/* !EXTERN */
#define INIT_ERROR_S(value, message)	= { { value, message } }
#endif	/* !EXTERN */

#define ERROR(error, value, message)	\
	EXTERN	struct	error_s error[] INIT_ERROR_S(value, message);

ERROR(E_opt,    1, "%s: -[ugbn] option expected\n")
ERROR(E_file,   2, "%s: file argument expected\n")
ERROR(E_open,   3, "%s: cannot open `%s': %s\n")
ERROR(E_name,   4, "%s: pathname of interpreter in `%s' is not absolute\n")
ERROR(E_mem,    5, "%s: malloc error for `%s': %s\n")
ERROR(E_subst,  6,
	"%s: `%c' substitution attempt under -[ugb] option in file `%s'\n")
ERROR(E_args,   7, "%s: too many arguments for `%s'\n")
ERROR(E_read,   8, "%s: read error in `%s': %s\n")
ERROR(E_fmt,    9, "%s: format error in `%s'\n")
ERROR(E_tilde, 10, "%s: cannot resolve `%s' in `%s'\n")
ERROR(E_fstat, 11, "%s: cannot fstat `%s': %s\n")
ERROR(E_mode,  12, "%s: `%s' is set[ug]id, yet has checking disabled!\n")
ERROR(E_alarm, 13, "%s: `%s' is a fake!\n")
ERROR(E_exec,  14, "%s: cannot execute `%s' in `%s': %s\n")

#endif	/* !ERROR_H */
