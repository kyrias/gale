/* misc.h -- stuff used in gale that doesn't have to do with gale /per se/. */

#ifndef GALE_MISC_H
#define GALE_MISC_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "gale/types.h"

/* -- terminal functions --------------------------------------------------- */

#define gale_print_bold 1
#define gale_print_clobber_left 2
#define gale_print_clobber_right 4

/* Safely output a string to the console. */
void gale_print(FILE *fp,int attr,struct gale_text);
/* Beep. */
void gale_beep(FILE *fp);
/* Get the terminal width. */
int gale_columns(FILE *fp);

/* -- process management --------------------------------------------------- */

/* Restart ourselves -- re-exec() the program with the same argc and argv.
   Called automatically on SIGUSR1. */
void gale_restart(void);
/* Run a subprogram, with the name given by "prog" (will search PATH), and
   the given argv (NULL-terminated).  Returns the pid of the child, or -1 if
   an error happenned.  If "in" is non-NULL, a pipe will be established to
   the process' standard input and the fd (open for writing) returned in "in";
   similarly for "out", standard out, open for reading.  The last argument is
   a function to call (with the argument list) if the exec failed, to provide
   default functionality; if NULL, it will report an error and exit the sub-
   process instead. */
pid_t gale_exec(const char *prog,char * const *argv,int *in,int *out,
                void (*)(char * const *));
/* Wait for the subprogram to exit and return its return code.  You should call
   this to avoid zombies. */
int gale_wait(pid_t pid);

/* Look for other processes of the same type in the given 'class' and kill them
   (if do_kill is 1); register ourselves (with temp files) to get killed in
   a similar manner if appropriate. */
void gale_kill(struct gale_text class,int do_kill);
/* Register a cleanup function.  This will get called, if at all possible, when
   the program exists, including most signals. */
void gale_cleanup(void (*)(void));
/* Perform all the cleanup functions "early". */
void gale_do_cleanup();
/* Watch the given file descriptor; if it stops returning 1 to isatty(),
   raise SIGHUP. */
void gale_watch_tty(int fd);

/* -- memory management ---------------------------------------------------- */

extern const struct gale_data null_data;

/* These are currently defined in terms of the Boehm GC. */
void *gale_malloc(size_t size);
void *gale_malloc_atomic(size_t size); /* memory cannot contain pointers */
void gale_free(void *);
void gale_finalizer(void *,void (*)(void *,void *),void *);

/* Handy macros. */
#define gale_create(x) ((x) = gale_malloc(sizeof(*(x))))
#define gale_create_array(x,size) ((x) = gale_malloc(sizeof(*(x)) * (size)))

/* Duplicate memory, strings, counted strings, etc. */
void *gale_memdup(const void *,int);
char *gale_strdup(const char *);
char *gale_strndup(const char *,int);

/* Compare memory blocks. */
int gale_data_compare(struct gale_data,struct gale_data);

/* Not really safe. */
void *gale_realloc(void *,size_t);

/* -- text buffer manipulation --------------------------------------------- */

extern const struct gale_text null_text;

#define G_(x) (_gale_text_literal(L##x))
struct gale_text _gale_text_literal(const wchar_t *);
struct gale_text gale_text_concat(int count,...);

struct gale_text gale_text_left(struct gale_text,int);
struct gale_text gale_text_right(struct gale_text,int);
int gale_text_token(struct gale_text string,wch sep,struct gale_text *token);
int gale_text_compare(struct gale_text,struct gale_text);

int gale_text_to_number(struct gale_text);
struct gale_text gale_text_from_number(int n,int base,int pad);

typedef struct gale_text gale_text_from(const char *,int len);
typedef char *gale_text_to(struct gale_text);

gale_text_from gale_text_from_local,gale_text_from_latin1,gale_text_from_utf8;
gale_text_to gale_text_to_local,gale_text_to_latin1,gale_text_to_utf8;

/* -- time functions ------------------------------------------------------- */

struct timeval;

struct gale_time gale_time_zero(void);
struct gale_time gale_time_now(void);
struct gale_time gale_time_forever(void);
struct gale_time gale_time_seconds(int);

int gale_time_compare(struct gale_time,struct gale_time);
struct gale_time gale_time_diff(struct gale_time,struct gale_time);

void gale_time_to(struct timeval *,struct gale_time);
void gale_time_from(struct gale_time *,struct timeval *);

/* -- fragment/group management -------------------------------------------- */

struct gale_group gale_group_empty(void);
void gale_group_add(struct gale_group *,struct gale_fragment);
void gale_group_append(struct gale_group *,struct gale_group);

struct gale_group gale_group_find(struct gale_group,struct gale_text name);
int gale_group_remove(struct gale_group *,struct gale_text name);
void gale_group_replace(struct gale_group *,struct gale_fragment);

int gale_group_null(struct gale_group);
struct gale_fragment gale_group_first(struct gale_group);
struct gale_group gale_group_rest(struct gale_group);
void gale_group_prefix(struct gale_group *,struct gale_group tail);

/* -- data interchange conversion ------------------------------------------ */

int gale_unpack_copy(struct gale_data *,void *,size_t);
int gale_unpack_compare(struct gale_data *,const void *,size_t);
void gale_pack_copy(struct gale_data *,const void *,size_t);
#define gale_copy_size(s) (s)

int gale_unpack_skip(struct gale_data *);
void gale_pack_skip(struct gale_data *,size_t);
#define gale_skip_size(sz) ((sz) + gale_u32_size())

int gale_unpack_rle(struct gale_data *,void *,size_t);
void gale_pack_rle(struct gale_data *,const void *,size_t);
#define gale_rle_size(s) (((s)+127)/128+(s))

int gale_unpack_u32(struct gale_data *,u32 *);
void gale_pack_u32(struct gale_data *,u32);
#define gale_u32_size() (sizeof(u32))

int gale_unpack_wch(struct gale_data *,wch *);
void gale_pack_wch(struct gale_data *,wch);
#define gale_wch_size() (sizeof(u16))

/* ANSI; deprecated! */
int gale_unpack_str(struct gale_data *,const char **);
void gale_pack_str(struct gale_data *,const char *);
#define gale_str_size(t) (strlen(t) + 1)

int gale_unpack_text(struct gale_data *,/*owned*/ struct gale_text *);
void gale_pack_text(struct gale_data *,struct gale_text);
#define gale_text_size(t) (gale_text_len_size(t) + gale_u32_size())

int gale_unpack_text_len(struct gale_data *,size_t len,
                         /*in,out*/ struct gale_text *);
void gale_pack_text_len(struct gale_data *,struct gale_text);
#define gale_text_len_size(t) ((t).l * gale_wch_size())

int gale_unpack_time(struct gale_data *,struct gale_time *);
void gale_pack_time(struct gale_data *,struct gale_time);
#define gale_time_size() (sizeof(u32) * 4)

int gale_unpack_fragment(struct gale_data *,struct gale_fragment *);
void gale_pack_fragment(struct gale_data *,struct gale_fragment);
size_t gale_fragment_size(struct gale_fragment);

int gale_unpack_group(struct gale_data *,struct gale_group *);
void gale_pack_group(struct gale_data *,struct gale_group);
size_t gale_group_size(struct gale_group);

/* -- directory management stuff ------------------------------------------- */

/* preinitialized pathnames, set by gale_init. 
   dot_gale  -> ~/.gale
   home_dir  -> ~
   sys_dir   -> etc/gale */
extern struct gale_text dot_gale,home_dir,sys_dir;

/* (Attempt to) create a directory if it does not exist, with the specified
   name and mode. */
void make_dir(struct gale_text path,int mode);

/* Append a subdirectory (creating it with the specified mode if necessary) */
struct gale_text sub_dir(struct gale_text path,struct gale_text sub,int mode);
/* Find the parent of a directory. */
struct gale_text up_dir(struct gale_text path);

/* Construct a filename in the given directory.  Makes sure the filename
   contains no "../" (for safety) and concatenates the directory with the
   path information. */
struct gale_text dir_file(struct gale_text path,struct gale_text file);

/* Search for a file in several directories.  Takes the filename, a flag (cwd)
   indicating whether the current directory should be searched as well (1 for
   yes, 0 for no), and a list of directories (end with NULL).  Will return the
   full filename (as from dir_file) if it finds such a file in any of the
   specified directories, null_text otherwise. */
struct gale_text dir_search(struct gale_text name,int cwd,struct gale_text,...);

/* -- error reporting ------------------------------------------------------ */
/* XXX - these should use gale_text, not char* */

/* Types of error severity. */
enum { GALE_NOTICE, GALE_WARNING, GALE_ERROR };

/* Error handler function. */
typedef void gale_error_f(int severity,char *msg);

/* The prefix to prepend to error names -- by default the program name */
extern const char *gale_error_prefix;

/* The error handler to use -- set up to a default by gale_init, but you can
   change this for custom error processing. */
extern gale_error_f *gale_error_handler;

/* Standard error handlers. */
gale_error_f gale_error_syslog,gale_error_stderr;

/* Report an error.  If GALE_ERROR, the program will terminate, otherwise it
   will continue. */
void gale_alert(int severity,const char *,int err);

/* -- debugging support ---------------------------------------------------- */

extern int gale_debug;    /* debugging level (starts out zero) */

/* Debugging printf.  Will only output if gale_debug > level. */
void gale_dprintf(int level,const char *fmt,...);
void gale_diprintf(int level,int indent,const char *fmt,...);

/* -- useful things for servers -------------------------------------------- */

struct gale_connect;

/* Create a connect object, which represents a connection in progress.  This
   accepts a server specification (like GALE_SERVER), which contains a list
   of host[:port] entries delimited by commas.  It will attempt a connection
   to all such hosts simultaneously.  (The first one to connect "wins".)
   This call will not block; it just starts the process rolling and creates
   the object. */
struct gale_connect *make_connect(struct gale_text serv);

/* Call this before calling select().  Pass in a pointer to an fd_set,
   initialized with any other file descriptors you want to check for writing
   (possibly none -- FD_ZERO it).  You could use several connect objects
   with the same select loop. */
void connect_select(struct gale_connect *,fd_set *wfd);

/* Then call select(), passing that fd_set as the fourth argument. */

/* Then (if it succeeds) call select_connect(), giving the same fd_set and
   the connect object.  This call will return 0 if nothing has connected, but
   you should keep trying (go through the loop again, back to connect_select),
   -1 if no connection could be made (the object has been destroyed; do 
   whatever you do to handle errors), or the file descriptor if a connection 
   has succeeded (the object has been destroyed; you don't need now). */
int select_connect(fd_set *wfd,struct gale_connect *);

/* This destroys the connect object with any pending connections. */
void abort_connect(struct gale_connect *);

/* Daemonize (go into the background).  If keep_tty is true (1), don't detach
   from the tty (gsub does this), otherwise do (like most daemons). */
void gale_daemon(int keep_tty);

#endif