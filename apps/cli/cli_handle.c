/*
 *
  ***** BEGIN LICENSE BLOCK *****
 
  Copyright (C) 2009-2017 Olof Hagsand and Benny Holmgren

  This file is part of CLIXON.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Alternatively, the contents of this file may be used under the terms of
  the GNU General Public License Version 3 or later (the "GPL"),
  in which case the provisions of the GPL are applicable instead
  of those above. If you wish to allow use of your version of this file only
  under the terms of the GPL, and not to allow others to
  use your version of this file under the terms of Apache License version 2, 
  indicate your decision by deleting the provisions above and replace them with
  the  notice and other provisions required by the GPL. If you do not delete
  the provisions above, a recipient may use your version of this file under
  the terms of any one of the Apache License version 2 or the GPL.

  ***** END LICENSE BLOCK *****

 * 
 */

#ifdef HAVE_CONFIG_H
#include "clixon_config.h" /* generated by config & autoconf */
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <dirent.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <netinet/in.h>

/* cligen */
#include <cligen/cligen.h>

/* clicon */
#include <clixon/clixon.h>

#include "clixon_cli_api.h"
#include "cli_plugin.h"
#include "cli_handle.h"

#define CLICON_MAGIC 0x99aafabe

#define handle(h) (assert(clicon_handle_check(h)==0),(struct cli_handle *)(h))
#define cligen(h) (handle(h)->cl_cligen)

/*! CLI specific handle added to header CLICON handle
 * This file should only contain access functions for the _specific_
 * entries in the struct below.
 * @note The top part must be equivalent to struct clicon_handle in clixon_handle.c
 * @see struct clicon_handle, struct backend_handle
 */
struct cli_handle {
    int                      cl_magic;    /* magic (HDR)*/
    clicon_hash_t           *cl_copt;     /* clicon option list (HDR) */
    clicon_hash_t           *cl_data;     /* internal clicon data (HDR) */
    /* ------ end of common handle ------ */
    cligen_handle            cl_cligen;   /* cligen handle */

    cli_syntax_t *cl_stx;	           /* syntax structure */

};

/*
 * cli_handle_init
 * returns a clicon handle for other CLICON API calls
 */
clicon_handle
cli_handle_init(void)
{
    struct cli_handle *cl;
    cligen_handle      clih = NULL;
    clicon_handle      h = NULL;

    if ((cl = (struct cli_handle *)clicon_handle_init0(sizeof(struct cli_handle))) == NULL)
	return NULL;

    if ((clih = cligen_init()) == NULL){
	clicon_handle_exit((clicon_handle)cl);
	goto done;
    }
    cligen_userhandle_set(clih, cl);
    cl->cl_cligen = clih;
    h = (clicon_handle)cl;
  done:
    return h;
}

/*! Free clicon handle
 */
int
cli_handle_exit(clicon_handle h)
{
    cligen_handle ch = cligen(h);
    struct cli_handle *cl = handle(h);

    if (cl->cl_stx)
	free(cl->cl_stx);
    clicon_handle_exit(h); /* frees h and options */

    cligen_exit(ch);

    return 0;
}

/*----------------------------------------------------------
 * cli-specific handle access functions
 *----------------------------------------------------------*/

/*! Get current syntax-group */
cli_syntax_t *
cli_syntax(clicon_handle h)
{
    struct cli_handle *cl = handle(h);
    return cl->cl_stx;
}

/*! Set current syntax-group */
int
cli_syntax_set(clicon_handle h, 
	       cli_syntax_t *stx)
{
    struct cli_handle *cl = handle(h);
    cl->cl_stx = stx;
    return 0;
}

/*----------------------------------------------------------
 * cligen access functions
 *----------------------------------------------------------*/
cligen_handle
cli_cligen(clicon_handle h)
{
    return cligen(h);
}

/*
 * cli_interactive and clicon_eval
 */
int
cli_exiting(clicon_handle h)
{
    cligen_handle ch = cligen(h);

    return cligen_exiting(ch);
}
/*
 * cli_common.c: cli_quit
 * cli_interactive()
 */
int 
cli_set_exiting(clicon_handle h, int exiting)
{
    cligen_handle ch = cligen(h);

    return cligen_exiting_set(ch, exiting);
}

char
cli_comment(clicon_handle h)
{
    cligen_handle ch = cligen(h);

    return cligen_comment(ch);
}

char
cli_set_comment(clicon_handle h, char c)
{
    cligen_handle ch = cligen(h);

    return cligen_comment_set(ch, c);
}

char
cli_tree_add(clicon_handle h, char *tree, parse_tree pt)
{
    cligen_handle ch = cligen(h);

    return cligen_tree_add(ch, tree, pt);
}

char *
cli_tree_active(clicon_handle h)
{
    cligen_handle ch = cligen(h);

    return cligen_tree_active(ch);
}

int
cli_tree_active_set(clicon_handle h, char *treename)
{
    cligen_handle ch = cligen(h);

    return cligen_tree_active_set(ch, treename);
}

parse_tree *
cli_tree(clicon_handle h, char *name)
{
    cligen_handle ch = cligen(h);

    return cligen_tree(ch, name);
}

int
cli_parse_file(clicon_handle h,
	       FILE *f,
	       char *name, /* just for errs */
	       parse_tree *pt,
	       cvec *globals)
{
    cligen_handle ch = cligen(h);

    return cligen_parse_file(ch, f, name, pt, globals);
}

int
cli_susp_hook(clicon_handle h, cli_susphook_t *fn)
{
    cligen_handle ch = cligen(h);

    /* This assume first arg of fn can be treated as void* */
    return cligen_susp_hook(ch, (cligen_susp_cb_t*)fn); 
}

char *
cli_nomatch(clicon_handle h)
{
    cligen_handle ch = cligen(h);

    return cligen_nomatch(ch);
}

int
cli_prompt_set(clicon_handle h, char *prompt)
{
    cligen_handle ch = cligen(h);
    return cligen_prompt_set(ch, prompt);
}

int
cli_logsyntax_set(clicon_handle h, int status)
{
    cligen_handle ch = cligen(h);
    return cligen_logsyntax_set(ch, status);
}
