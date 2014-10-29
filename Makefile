# Wrappers for running ooniprobe as a non-root user.
#
# (1) `make test_1`
#
# Build-Depends: libcap-dev
#
# This builds a program that has file capabilities set on it. The program calls
# a python interpreter on the ooniprobe script. This child interpreter itself
# also needs file capabilities granted, i.e. the ability to "inherit caps from
# the wrapper". (That's just how Linux capabilities works, unfortunately). To
# be clear, the child can only *use* those caps when called by the wrapper.
#
# Our current approach is to copy the system python interpreter and set caps
# on the copy, rather than directly on the original. The path to the copy is
# hard-coded in the wrapper, but the system administrator can in theory set a
# different version of python to that path during runtime.
#
# Nothing runs as root; however we must maintain the child capabilities across
# upgrades. It also needs to be kept in sync when the system interpreter is
# auto-upgraded by the system package manager.
#
# (2) `make test_2`
#
# Build-Depends: cython, python-dev
#
# This builds a program that has file capabilities set on it. It uses libpython
# to launch ooniprobe in its own process via python's exec(), which avoids
# execve(2) so that capabilities are retained. The version of python is
# hard-coded into the wrapper at build time; making this dynamic is possible,
# but much more complex and not yet implemented.
#
# This way, we avoid needing a child interpreter process with capabilities set.
# Another advantage is that libpython would be automatically upgraded by the
# system package manager.
#
# In both (1) and (2), execution may be limited to a particular unix group by
# setting o-x,g+x.
#

SCRIPT_CAP_NEEDED ?= cap_net_admin,cap_net_raw
SCRIPT_CAP_INTERPRETER ?= "$(shell realpath ./python)"
SCRIPT_CAP_SCRIPT ?= "/usr/bin/ooniprobe"
SCRIPT_CAP_RUN ?= "python", "-Es", $(SCRIPT_CAP_SCRIPT)
SCRIPT_CAP_TEST ?= import socket; s=socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW); s.close(); del s;

PYTHON := python
PYINCPATH := $(shell $(PYTHON) -c "from distutils import sysconfig; print(sysconfig.get_python_inc())")
PYLIBPATH := $(shell $(PYTHON) -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBPL'))")
PYLIB := $(shell $(PYTHON) -c "from distutils import sysconfig; print(sysconfig.get_config_var('LIBRARY')[3:-2])")

SCRIPT_CAP_NEEDED_MACRO := $(shell echo "$(SCRIPT_CAP_NEEDED)" | tr '[a-z]' '[A-Z]')

# Unfortunately cython --embed ignores the arguments in the shebang line
# So we need to patch the generated code ourselves.
CYTHON_PRE_MAIN = extern int Py_IgnoreEnvironmentFlag; \
                  Py_IgnoreEnvironmentFlag++; \
                  extern int Py_NoUserSiteDirectory; \
                  Py_NoUserSiteDirectory++;

all: runooni_1 runooni_2

test: test_pre test_1 test_2

test_pre:
	grep -q "os.getuid() != 0" "$$(python -c "import ooni.utils; print ooni.utils.__file__")" \
	  && { echo "Ooni not patched: Tor bug #13497"; exit 1; } || true

test_1: runooni_1 python
	@if ./python -c "$(SCRIPT_CAP_TEST)"; then \
		echo "test failed! ./python is not supposed to be able to directly exercise its capabilities"; \
	else \
		echo "test passed! ./python succesfully failed to directly exercise its capabilities"; \
	fi
	cd test && PYTHONPATH=. "../$<" --version

test_2: runooni_2
	cd test && PYTHONPATH=. "../$<" --version

run_%: runooni_%
	"./$<" -i /usr/share/ooni/decks/complete.deck

runooni_1: runooni_1.c Makefile
	$(CC) "$<" -Wall -std=c11 -l cap -o "$@" $(CFLAGS) \
	  -DSCRIPT_CAP_NEEDED='$(SCRIPT_CAP_NEEDED_MACRO)' \
	  -DSCRIPT_CAP_INTERPRETER='$(SCRIPT_CAP_INTERPRETER)' \
	  -DSCRIPT_CAP_RUN='$(SCRIPT_CAP_RUN)'
	sudo setcap $(SCRIPT_CAP_NEEDED)+p "$@"

python: $(shell which $(PYTHON)) Makefile
	cp "$<" "$@"
	sudo setcap $(SCRIPT_CAP_NEEDED)+ei "$@"

runooni_2: runooni_2.c Makefile
	$(CC) -L$(PYLIBPATH) -l$(PYLIB) -I$(PYINCPATH) "$<" -o "$@"
	sudo setcap $(SCRIPT_CAP_NEEDED)+eip "$@"

runooni_2.c: runooni_2.py Makefile
	cython "$<" --embed=CYTHON_MAIN_SENTINEL -Werror -Wextra -o "$@"
	sed -i \
	  -e 's/\(.*CYTHON_MAIN_SENTINEL.*{\)/\1 $(CYTHON_PRE_MAIN)/g' \
	  -e '/CYTHON_MAIN_SENTINEL[^{]*$$/,/{/s/{/{ $(CYTHON_PRE_MAIN)/g' \
	  -e 's/CYTHON_MAIN_SENTINEL/main/g' "$@"

runooni_2.py: Makefile
	rm -f "$@" && touch "$@"
	echo '$(SCRIPT_CAP_TEST)' >> "$@"
	echo 'exec(open($(SCRIPT_CAP_SCRIPT)).read(), {})' >> "$@"

clean:
	rm -f python runooni_1 runooni_2 runooni_2.c runooni_2.py

.PHONY: clean all test test_% run_%
