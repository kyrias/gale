#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <limits.h> /* NetBSD (at least) requires this order. */

#include "gale/all.h"

#include "connect.h"
#include "attach.h"
#include "subscr.h"
#include "server.h"
#include "directed.h"

#include "oop.h"

int server_port;

static void *on_error_message(oop_source *src,struct gale_message *msg,void *user) {
	subscr_transmit(src,msg,NULL);
	return OOP_CONTINUE;
}

static void *on_incoming(oop_source *source,int fd,oop_event ev,void *user) {
	struct sockaddr_in sin;
	struct connect *conn;
	struct gale_link *link;

	int len = sizeof(sin);
	int one = 1;
	int newfd = accept(fd,(struct sockaddr *) &sin,&len);
	if (newfd < 0) {
		if (errno != ECONNRESET)
			gale_alert(GALE_WARNING,"accept",errno);
		return OOP_CONTINUE;
	}
	gale_dprintf(2,"[%d] new connection from %s\n",
	             newfd,inet_ntoa(sin.sin_addr));
	setsockopt(newfd,SOL_SOCKET,SO_KEEPALIVE,
	           (SETSOCKOPT_ARG_4_T) &one,sizeof(one));

	link = new_link(source);
	link_set_fd(link,newfd);
	conn = new_connect(source,link,G_("-"));

	return OOP_CONTINUE;
}

static struct gale_message *link_filter(struct gale_message *msg,void *x) {
	struct gale_message *rewrite = new_message();
	struct gale_text cat = null_text;
	int do_transmit = 0;

	rewrite->data = msg->data;
	while (gale_text_token(msg->cat,':',&cat)) {
		struct gale_text base;
		int orig_flag;
		int flag = !is_directed(cat,&orig_flag,&base,NULL) && orig_flag;
		base = category_escape(base,flag);
		do_transmit |= flag;
		rewrite->cat = gale_text_concat(3,rewrite->cat,G_(":"),base);
	}

	if (!do_transmit) {
		gale_dprintf(5,"*** no positive categories; message dropped\n");
		return NULL;
	}

	/* strip leading colon */
	if (rewrite->cat.l > 0) rewrite->cat = gale_text_right(rewrite->cat,-1);
		gale_dprintf(5,"*** rewrote categories to \"%s\"\n",
		gale_text_to_local(rewrite->cat));
	return rewrite;
}

static void add_links(oop_source *source) {
	struct gale_text str,link = null_text,in,out;

	str = gale_var(G_("GALE_LINKS")); if (!str.l) return;
	gale_text_token(str,';',&link);

	in = gale_var(G_("GALE_LINKS_INCOMING"));
	out = gale_var(G_("GALE_LINKS_OUTGOING"));

	if (!in.l) in = G_("+");
	if (!out.l) out = G_("+");

	(void) new_attach(source,link,link_filter,NULL,in,out);

	if (gale_text_token(str,';',&link))
		gale_alert(GALE_WARNING,"extra GALE_LINKS ignored",0);
}

static void usage(void) {
	fprintf(stderr,
	"%s\n"
	"usage: galed [-h] [-p port]\n"
	"flags: -h       Display this message\n"
	"       -p       Set the port to listen on (default %d)\n"
	,GALE_BANNER,server_port);
	exit(1);
}

static void make_listener(oop_source *source,int port) {
	struct sockaddr_in sin;
	int one = 1,sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (sock < 0) {
		gale_alert(GALE_ERROR,"socket",errno);
		return;
	}
	fcntl(sock,F_SETFD,1);
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,
	               (SETSOCKOPT_ARG_4_T) &one,sizeof(one)))
		gale_alert(GALE_ERROR,"setsockopt",errno);
	if (bind(sock,(struct sockaddr *)&sin,sizeof(sin))) {
		gale_alert(GALE_ERROR,"bind",errno);
		close(sock);
		return;
	}
	if (listen(sock,20)) {
		gale_alert(GALE_ERROR,"listen",errno);
		close(sock);
		return;
	}

	source->on_fd(source,sock,OOP_READ,on_incoming,NULL);
}

int main(int argc,char *argv[]) {
	int opt;
	oop_source_sys *sys;

	gale_init("galed",argc,argv);
	sys = oop_sys_new();
	gale_init_signals(oop_sys_source(sys));

	srand48(time(NULL) ^ getpid());

	server_port = gale_port;
	while ((opt = getopt(argc,argv,"hdDp:")) != EOF) switch (opt) {
	case 'd': ++gale_global->debug_level; break;
	case 'D': gale_global->debug_level += 5; break;
	case 'p': server_port = atoi(optarg); break;
	case 'h':
	case '?': usage();
	}

	add_links(oop_sys_source(sys));

	if (optind != argc) usage();

	gale_dprintf(0,"starting gale server\n");
	openlog(argv[0],LOG_PID,LOG_LOCAL5);

	make_listener(oop_sys_source(sys),server_port);

	gale_dprintf(1,"now listening, entering main loop\n");
	gale_daemon(oop_sys_source(sys),0);
	gale_on_error_message(oop_sys_source(sys),on_error_message,NULL);
	oop_sys_run(sys);
	return 0;
}
