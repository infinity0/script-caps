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

CAP_NEEDED = SCRIPT_CAP_NEEDED.split(",")

if os.geteuid() == 0:
	import prctl

	prctl.securebits.keep_caps = True
	for cap in CAP_NEEDED:
		setattr(prctl.cap_permitted, cap, True)

	os.setgid(os.getgid())
	os.setuid(os.getuid())

	got_root = False
	try:
		os.setuid(0)
		got_root = True
	except OSError:
		pass
	if got_root:
		raise ValueError("regained root after dropping it??")

	for cap in CAP_NEEDED:
		setattr(prctl.cap_effective, cap, True)

SCRIPT_CAP_TEST

execfile(SCRIPT_CAP_SCRIPT, {})
