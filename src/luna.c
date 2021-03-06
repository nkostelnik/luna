
//
// luna.c
//
// Copyright (c) 2011 TJ Holowaychuk <tj@vision-media.ca>
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "linenoise.h"
#include "luna.h"
#include "lexer.h"
#include "parser.h"
#include "errors.h"
#include "prettyprint.h"

// --ast

static int ast = 0;

// -

static int stdio = 0;

/*
 * Output usage information.
 */

static void
usage() {
  fprintf(stderr, 
    "\n  Usage: luna [options] [file]"
    "\n"
    "\n  Options:"
    "\n"
    "\n    -a, --ast       output ast to stdout"
    "\n    -h, --help      output help information"
    "\n    -V, --version   output luna version"
    "\n    -               read from stdin"
    "\n"
    "\n  Examples:"
    "\n"
    "\n    $ luna - < some.luna"
    "\n    $ luna some.luna"
    "\n    $ luna some"
    "\n    $ luna"
    "\n"
    "\n"
    );
  exit(1);
}

/*
 * Output luna version.
 */

static void
version() {
  printf("%s\n", LUNA_VERSION);
  exit(0);
}

/*
 * Line-noise REPL.
 */

static void
repl() {
  char *line;
  while(line = linenoise("luna> ")) {
    if ('\0' != line[0]) {
      printf("%s\n", line);
      linenoiseHistoryAdd(line);
    }
    free(line);
  }
  exit(0);
}

/*
 * Parse arguments.
 */

const char **
parse_args(int *argc, const char **argv) {
  const char *arg, **args = argv;

  for (int i = 0, len = *argc; i < len; ++i) {
    arg = args[i];
    if (0 == strcmp(arg, "-h") || 0 == strcmp(arg, "--help"))
      usage();
    else if (0 == strcmp(arg, "-V") || 0 == strcmp(arg, "--version"))
      version();
    else if (0 == strcmp(arg, "-a") || 0 == strcmp(arg, "--ast")) {
      ast = 1;
      --*argc;
      ++argv;
    } else if (0 == strcmp(arg, "-")) {
      stdio = 1;
      --*argc;
      ++argv;
    } else if ('-' == arg[0]) {
      fprintf(stderr, "unknown flag %s\n", arg);
      exit(1);
    }
  }

  return argv;
}

/*
 * Parse arguments and scan from stdin (for now).
 */

int
main(int argc, const char **argv){
  int appended_ext = 0;
  const char *path, *orig;
  FILE *stream;

  // parse arguments
  argv = parse_args(&argc, argv);

  // REPL
  if (!stdio && 1 == argc) repl();

  // stdin
  if (stdio) {
    path = "stdin";
    stream = stdin;
  } else {
    orig = path = argv[1];
    read:
    stream = fopen(path, "r");
    if (!stream) {
      // try with .luna extension
      if (!appended_ext) {
        appended_ext = 1;
        char buf[256];
        snprintf(buf, 256, "%s.luna", path);
        path = buf;
        goto read;
      }
      fprintf(stderr, "error reading %s:\n\n  %s\n\n", orig, strerror(errno));
      exit(1);
    }    
  }

  // parse the input
  luna_lexer_t lex;
  luna_lexer_init(&lex, stream, path);
  luna_parser_t parser;
  luna_parser_init(&parser, &lex);
  luna_block_node_t *root;

  // oh noes!
  if (!(root = luna_parse(&parser))) {
    luna_report_error(&parser);
    exit(1);
  }

  // --ast
  if (ast) {
    luna_prettyprint((luna_node_t *) root);
    exit(0);
  }

  return 0;
}