SCRIPT_CAP_NEEDED ?= cap_net_admin,cap_net_raw
SCRIPT_CAP_INTERPRETER ?= "$(shell realpath ./python)"
SCRIPT_CAP_SCRIPT ?= "python", "-Es", "/usr/bin/ooniprobe"

SCRIPT_CAP_NEEDED_MACRO := $(shell echo "$(SCRIPT_CAP_NEEDED)" | tr '[a-z]' '[A-Z]')

all: runooni

test: python
	@if ./python -c "import socket; socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)"; then \
		echo "test failed! ./python is not supposed to be able to open raw sockets"; \
	else \
		echo "test passed! ./python succesfully failed to open a raw socket"; \
	fi
	@echo "now run 'make run' to run complete.deck using the wrapper"

run: all
	./runooni -i /usr/share/ooni/decks/complete.deck

runooni: runooni.c python Makefile
	gcc "$<" -Wall -std=c11 -l cap -o "$@" $(CFLAGS) \
	  -DSCRIPT_CAP_NEEDED='$(SCRIPT_CAP_NEEDED_MACRO)' \
	  -DSCRIPT_CAP_INTERPRETER='$(SCRIPT_CAP_INTERPRETER)' \
	  -DSCRIPT_CAP_SCRIPT='$(SCRIPT_CAP_SCRIPT)'
	sudo setcap $(SCRIPT_CAP_NEEDED)+p "$@"

python: /usr/bin/python2.7 Makefile
	cp "$<" "$@"
	sudo setcap $(SCRIPT_CAP_NEEDED)+ei "$@"

clean:
	rm -f python runooni

.PHONY: clean all test run
