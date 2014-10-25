SRCS		= indir.c error.c
OBJS		= indir.o error.o
MAN		= indir.1

# a few defines for testing purposes; if you test in the current
# directory the paths needn't be absolute
# the path of indir will be used in #! lines -> max. 32 characters for
# #! + name + mandatory option
PATH_OF_INDIR	= ./indir
TEST_DIRECTORY	= .

CC		= cc
# if your C library doesn't have strrchr()
# STRRCHR	= -Dstrrchr=rindex

CFLAGS		= -O $(STRRCHR)

indir:		exec_ok $(OBJS)
		$(CC) -O -o indir $(OBJS)

exec_ok:
		exec_test

test:		exec_ok
		test -f tests_made || PATH_OF_INDIR=$(PATH_OF_INDIR) \
			TEST_DIRECTORY=$(TEST_DIRECTORY) make_tests
		do_tests

lint:
		lint $(SRCS)

man:
		nroff -man $(MAN) > indir.man

shar:
		shar README Intro setuid.txt $(MAN) Makefile $(SRCS) indir.h \
			error.h my_varargs.h exec_test make_tests do_tests \
			file_argument > indir.sh

clean:
		/bin/rm -f *.o indir.man wrong.* ok.* core exec_ok tests_made

indir.o:	indir.c indir.h error.h my_varargs.h
error.o:	error.c error.h my_varargs.h
