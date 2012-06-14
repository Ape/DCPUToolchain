/**

	File:		main.c

	Project:	DCPU-16 Tools
	Component:	Debugger

	Authors:	José Manuel Díez

	Description:	Main entry point.

**/

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <libgen.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <argtable2.h>
#include <bstring.h>
#include <iio.h>
#include <dcpu.h>
#include <dbgaux.h>
#include <dbglua.h>
#include <lexfix.h>
#include <parser.h>
#include <lexer.h>
#include <dcpuhook.h>
#include <version.h>
#include <debug.h>
#include "sdp.h"

#define MAX_ARGUMENTS 10
#define MAX_ARGUMENT_LENGTH 50

bstring path;
vm_t* vm;
#ifdef FEATURE_SDP
pthread_t sdp_thread;
#endif
struct dbg_state lstate;

extern int dbg_yyparse(void* scanner);

void ddbg_sigint(int sig)
{
	vm->halted = true;
	printf("(interrupt)\n");
#ifdef WIN32
	signal(SIGINT, ddbg_sigint);
#endif
}

void get_command(char* command_buffer, int max)
{
	printf("> ");
	fgets(command_buffer, max, stdin);
}

int main(int argc, char** argv)
{
	char* buf;
	yyscan_t scanner;
	int nerrors;

	// Define arguments.
	struct arg_lit* show_help = arg_lit0("h", "help", "Show this help.");
	struct arg_str* command_arg = arg_str0("c", NULL, "<command>", "Run a single debugger command and exit with the result.");
	struct arg_file* symbols_file = arg_file0("s", "symbols", "<file>", "The file to load symbols from.");
	struct arg_file* input_file = arg_file0(NULL, NULL, "<file>", "The file to initially load.");
	struct arg_lit* little_endian_mode = arg_lit0(NULL, "little-endian", "Use little endian serialization (for compatibility with older versions).");
	struct arg_lit* verbose = arg_litn("v", NULL, 0, LEVEL_EVERYTHING - LEVEL_DEFAULT, "Increase verbosity.");
	struct arg_lit* quiet = arg_litn("q", NULL,  0, LEVEL_DEFAULT - LEVEL_SILENT, "Decrease verbosity.");
	struct arg_end* end = arg_end(20);
	void* argtable[] = { show_help, command_arg, symbols_file, input_file, little_endian_mode, verbose, quiet, end };

	// Parse arguments.
	nerrors = arg_parse(argc, argv, argtable);

	version_print(bautofree(bfromcstr("Debugger")));
	if (nerrors != 0 || show_help->count != 0)
	{
		if (show_help->count != 0)
			arg_print_errors(stdout, end, "Debugger");

		printf("syntax:\n    dtdb");
		arg_print_syntax(stderr, argtable, "\n");
		printf("options:\n");
		arg_print_glossary(stderr, argtable, "	  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	// Set verbosity level.
	debug_setlevel(LEVEL_DEFAULT + verbose->count - quiet->count);

	// Set global path variable.
	osutil_setarg0(bautofree(bfromcstr(argv[0])));

	// Set endianness.
	isetmode(little_endian_mode->count == 0 ? IMODE_BIG : IMODE_LITTLE);

	// Register signal handler.
	signal(SIGINT, ddbg_sigint);

	// Initialize debugger.
	ddbg_init();

	// Load file if filename is specified.
	if (input_file->count > 0)
	{
		ddbg_attach(bfromcstr("clock"));
		ddbg_attach(bfromcstr("keyboard"));
		ddbg_attach(bfromcstr("lem1802"));
		ddbg_load(bfromcstr(input_file->filename[0]));
		ddbg_flash_vm();
	}
	if (symbols_file->count > 0)
		ddbg_load_symbols(bfromcstr(symbols_file->filename[0]));

	// If the user specified a command, execute only that
	// command and then continue.
	if (command_arg->count > 0)
	{
		ddbg_return_code = 1; // Default is failure.
		dbg_yylex_init(&scanner);
		dbg_yy_scan_string(command_arg->sval[0], scanner);
		dbg_yyparse(scanner);
		dbg_yylex_destroy(scanner);
		return ddbg_return_code;
	}

	// Create SDP thread if supported.
#ifdef FEATURE_SDP
	pthread_create(&sdp_thread, NULL, (void*)ddbg_sdp_thread, vm);
#endif

	// We always want to show the debugger start message.
	debug_setlevel(LEVEL_VERBOSE + verbose->count - quiet->count);
	version_print(bautofree(bfromcstr("Debugger")));
	debug_setlevel(LEVEL_DEFAULT + verbose->count - quiet->count);

	for (;;)
	{
		buf = readline("> ");
		add_history(buf);

		dbg_yylex_init(&scanner);
		dbg_yy_scan_string(buf, scanner);
		dbg_yyparse(scanner);
		dbg_yylex_destroy(scanner);
	}

	free(buf);

	return 0;
}
