static	char	sccsid[] = "@(#)indir.c 2.0 89/11/01 Maarten Litmaath";

/*
 * indir.c
 * execute (setuid/setgid) (shell) scripts indirectly
 * see indir.1
 */

#include	"indir.h"
#include	"error.h"


static	char	*Env[] = {
	"PATH=/bin:/usr/bin:/usr/ucb",
	0
};
static	char	*Prog, *File, *Interpreter, *Newargv[MAXARGV];
static	int	Uid_check = 1, Gid_check = 1;
static	uid_t	Uid;


main(argc, argv)
int	argc;
char	**argv;
{
	extern	char	*strrchr();
	extern	int	execve(), execvp();
	FILE	*fp;
	int	c;
	struct	stat	st;


	if (!(Prog = strrchr(argv[0], '/')))
		Prog = argv[0];
	else
		++Prog;

	if (!argv[1] || *argv[1] != '-' || !argv[1][1] || argv[1][2])
		error(E_opt, Prog);

	switch (argv[1][1]) {
	case 'n':
		Uid_check = Gid_check = 0;
		break;
	case 'g':
		Uid_check = 0;
		break;
	case 'u':
		Gid_check = 0;
		break;
	case 'b':
		break;
	default:
		error(E_opt, Prog);
		break;
	}

	if (argc < 3)
		error(E_file, Prog);

	File = argv[2];

	if (!(fp = fopen(File, "r")))
		error(E_open, Prog, File, geterr());

	Uid = getuid();

	while ((c = getc(fp)) != '\n' && c != EOF)	/* skip #! line */
		;

	if (!(c = getnewargv(fp))) {
		if (ferror(fp))
			error(E_read, Prog, File, geterr());
		error(E_fmt, Prog, File);
	}

	argv += 3;			/* skip Prog, option and File */

	while (*argv && c < MAXARGV - 1)
		Newargv[c++] = *argv++;

	if (*argv)
		error(E_args, Prog, File);

	Newargv[c] = 0;

#ifdef	DEBUG
	fprintf(stderr, "Interpreter=`%s'\n", Interpreter);
	for (c = 0; Newargv[c]; c++)
		fprintf(stderr, "Newargv[%d]=`%s'\n", c, Newargv[c]);
#endif	/* DEBUG */

	if (fstat(fileno(fp), &st) != 0)
		error(E_fstat, Prog, File, geterr());
	/*
	 * List of possible Uid_check/setuid combinations:
	 *
	 * !Uid_check !setuid -> OK: ordinary script
	 * !Uid_check  setuid -> security hole: checking should be enabled
	 *  Uid_check !setuid -> fake
	 *  Uid_check  setuid -> check if st_uid == euid
	 */

	if (!Uid_check) {
		/*
		 * If the file is setuid, consistency should be checked;
		 * however, checking has been disabled for this file (leading
		 * to security holes again!), so warn the user and exit.
		 */
		if (st.st_mode & S_ISUID)
			error(E_mode, Prog, File);
	} else {
		/*
		 * Check if the file we're reading from is setuid and owned
		 * by geteuid().  If this test fails, the file is a fake,
		 * else it MUST be ok!
		 */
		if (!(st.st_mode & S_ISUID) || st.st_uid != geteuid())
			error(E_alarm, Prog, File);
	}

	/* setgid checks */

	if (!Gid_check) {
		if (st.st_mode & S_ISGID)
			error(E_mode, Prog, File);
	} else {
		if (!(st.st_mode & S_ISGID) || st.st_gid != getegid())
			error(E_alarm, Prog, File);
	}

	/*
	 * If we're executing a set[ug]id file, replace the complete
	 * environment by a save default, else permit the PATH to be
	 * searched too.
	 */
	if (st.st_mode & (S_ISUID | S_ISGID))
		execve(Interpreter, Newargv, Env);
	else
		execvp(Interpreter, Newargv);

	error(E_exec, Prog, Interpreter, File, geterr());
}


static	int	getnewargv(fp)
FILE	*fp;
{
	static	char	buf[MAXLEN];
	char	*skipblanks(), *skiptoblank(), *strrchr(), *malloc(), *tilde();
	register char	*p;
	register int	i = 0;


	for (;;) {
		if (!fgets(buf, sizeof buf, fp) || !*buf
			|| buf[strlen(buf) - 1] != '\n')
			return 0;

		p = skipblanks(buf);

		switch (*p++) {
		case '\0':			/* skip empty lines */
			continue;
		case COMMENT:
			break;
		default:
			return 0;
		}

		if (*p++ == MAGIC)		/* skip ordinary comments */
			break;
	}

	p = skipblanks(p);

	if (*p == TILDE)
		p = tilde(p, &Interpreter);
	else {
		if (*p != '/') {
			if (!*p)
				return 0;
			if (Uid_check || Gid_check)
				error(E_name, Prog, File);
		}
		Interpreter = p;
		p = skiptoblank(p);
		*p++ = '\0';
	}

	if (!(Newargv[0] = strrchr(Interpreter, '/')))
		Newargv[0] = Interpreter;
	else
		++Newargv[0];

	while (i < MAXARGV - 2) {
		p = skipblanks(p);

		if (!*p)
			break;

		if (*p == '\\' && p[1] == '\n') {
			if (!(p = malloc(MAXLEN)))
				error(E_mem, Prog, File, geterr());
			if (!fgets(p, MAXLEN, fp) || !*p
				|| p[strlen(p) - 1] != '\n')
				return 0;
			p = skipblanks(p);
			if (*p++ != COMMENT || *p++ != MAGIC)
				return 0;
			continue;
		}

		if (*p == TILDE)
			p = tilde(p, &Newargv[++i]);
		else if (*p++ == SUBST
			&& (*p == '\n' || *p == ' ' || *p == '\t')) {
			if (Uid_check || Gid_check)
				error(E_subst, Prog, SUBST, File);
			Newargv[++i] = File;
		} else {
			Newargv[++i] = --p;
			p = skiptoblank(p);
			*p++ = '\0';
		}
	}

	if (*p)
		error(E_args, Prog, File);

	Newargv[++i] = 0;
	return i;
}


static	char	*skipblanks(p)
register char	*p;
{
	register int	c;

	while ((c = *p++) == ' ' || c == '\t' || c == '\n')
		;
	return --p;
}


static	char	*skiptoblank(p)
register char	*p;
{
	register int	c;

	while ((c = *p++) != ' ' && c != '\t' && c != '\n')
		;
	return --p;
}


#include	<pwd.h>


static	char	*tilde(p, pp)
register char	*p;
char	**pp;
{
	register char	*q, c;
	struct	passwd	*pw, *getpwuid(), *getpwnam();
	char	*orig, save, *buf, *strcpy(), *strncpy(), *malloc();
	int	n;


	orig = p++;
	for (q = p; (c = *q++) != '/' && c != '\n' && c != ' ' && c != '\t'; )
		;
	if (--q == p)
		pw = getpwuid(Uid);
	else {
		save = *q;
		*q = '\0';
		pw = getpwnam(p);
		*(p = q) = save;
	}
	q = skiptoblank(q);
	save = *q;
	*q = '\0';
	if (!pw)
		error(E_tilde, Prog, orig, File);
	if (!(buf = malloc((n = strlen(pw->pw_dir)) + q - p + 1)))
		error(E_mem, Prog, File, geterr());
	strcpy(buf, pw->pw_dir);
	strcpy(buf + n, p);
	*pp = buf;
	*q = save;
	return q;
}
