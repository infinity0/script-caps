#!/usr/bin/python -Es
"""
Option 2: run script as root, then adds caps and then drops root.

Upside: does not require $cap+ei.
Downside: requires something to run as root.

Depends: python-prctl

TODO: need to figure out a safe way to run as root (e.g. forbid env(1),
path injections). Several options are:

- suid wrapper script
- sudo/sudoers to restrict
- (done) embedded cython https://github.com/cython/cython/tree/master/Demos/embed

"""
import os

if os.geteuid() == 0:
	import prctl

	#print os.getresuid()

	prctl.securebits.keep_caps = True
	prctl.cap_permitted.net_admin = True
	prctl.cap_permitted.net_raw = True

	os.setgid(os.getgid())
	os.setuid(os.getuid())

	#print os.getresuid()

	prctl.cap_effective.net_admin = True
	prctl.cap_effective.net_raw = True

#import socket
#socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
#import sys
#print sys.path

execfile("/usr/bin/ooniprobe", {})
