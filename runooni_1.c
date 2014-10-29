/**
 * Option 1: wrapper script with $cap+p set on file
 *
 * Upside: nothing runs as root.
 * Downside: child interpreter (e.g. python) needs $cap+ei set on file.
 *
 * Depends: libcap-dev
 */
#include <sys/capability.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: check that normal user cannot bypass paths, e.g. via chroot/fakeroot/fakechroot

/**
 * Capabilities that the script needs. This wrapper program
 * needs capabilities to be set as newcaps+p.
 */
static const cap_value_t newcaps[2] = { SCRIPT_CAP_NEEDED };

/**
 * Path to the copy of the interpreter that has newcaps+ei set.
 */
static const char* cap_interpreter = SCRIPT_CAP_INTERPRETER;

/**
 * Args to run the interpreter+script with.
 *
 * It should ignore environment variables, and anything else the user may do
 * to gain newcaps on arbitrary code. E.g. python should be run with -Es.
 */
static const char* const script_run[3] = { SCRIPT_CAP_RUN };

// see http://unix.stackexchange.com/questions/128394/passing-capabilities-through-exec
int main(int argc, char **argv) {
	// set caps
	cap_t caps = cap_get_proc();
	if (caps == NULL) {
		perror("cap_get_proc");
		exit(-1);
	}
#ifdef DEBUG
	printf("Capabilities: %s\n", cap_to_text(caps, NULL));
#endif
	if (cap_set_flag(caps, CAP_INHERITABLE, 2, newcaps, CAP_SET) == -1) {
		perror("cap_set_flag");
		goto cap_fail;
	}
	if (cap_set_proc(caps) == -1) {
		perror("cap_set_proc");
		goto cap_fail;
	}
#ifdef DEBUG
	printf("Capabilities: %s\n", cap_to_text(caps, NULL));
#endif
	cap_free(caps);

	// set argv
	char** const new_argv = malloc(sizeof(char *) * (argc+3));
	if (new_argv == NULL) {
		perror("new_argv malloc");
		exit(-1);
	}
	memcpy(new_argv, script_run, sizeof(char *) * 3);
	memcpy(new_argv+3, argv+1, sizeof(char *) * (argc-1));
	new_argv[argc+2] = NULL;

#ifdef DEBUG
	for (int i=0; i<argc+3; i++) {
		printf("New args: %s\n", new_argv[i]);
	}
#endif

	if (execv(cap_interpreter, new_argv) == -1) {
		perror("interpret-script execv");
		exit(-1);
	}

cap_fail:
	cap_free(caps);
	exit(-1);
}
