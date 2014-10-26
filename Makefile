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
SCRIPT_CAP_NEEDED_PY := "$(subst cap_,,$(SCRIPT_CAP_NEEDED))"

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

test_1: runooni_1
	@if ./python -c "$(SCRIPT_CAP_TEST)"; then \
		echo "test failed! ./python is not supposed to be able to open raw sockets"; \
	else \
		echo "test passed! ./python succesfully failed to open a raw socket"; \
	fi
	cd test && PYTHONPATH=. "../$<" --version

test_2: runooni_2
	cd test && PYTHONPATH=. "../$<" --version

run_%: runooni_%
	"./$<" -i /usr/share/ooni/decks/complete.deck

runooni_1: runooni.c python Makefile
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
	sudo chown root:root "$@"
	sudo chmod 4755 "$@"

runooni_2.c: runooni_2.py Makefile
	cython "$<" --embed=CYTHON_MAIN_SENTINEL -Werror -Wextra -o "$@"
	sed -i \
	  -e 's/\(.*CYTHON_MAIN_SENTINEL.*{\)/\1 $(CYTHON_PRE_MAIN)/g' \
	  -e '/CYTHON_MAIN_SENTINEL[^{]*$$/,/{/s/{/{ $(CYTHON_PRE_MAIN)/g' \
	  -e 's/CYTHON_MAIN_SENTINEL/main/g' "$@"

runooni_2.py: runooni_2.py.m4 Makefile
	m4 < "$<" > "$@" \
	  -DSCRIPT_CAP_NEEDED='$(SCRIPT_CAP_NEEDED_PY)' \
	  -DSCRIPT_CAP_SCRIPT='$(SCRIPT_CAP_SCRIPT)' \
	  -DSCRIPT_CAP_TEST='$(SCRIPT_CAP_TEST)'

clean:
	rm -f python runooni_1 runooni_2 runooni_2.c runooni_2.py

.PHONY: clean all test test_% run_%
