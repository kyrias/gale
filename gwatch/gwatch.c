#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "gale/all.h"

struct gale_client *client;

#ifndef NDEBUG
char **restart_argv;
#endif

const char **subs;
struct gale_message **pings;
int count_subs = 0,count_pings = 0;

const char *tty,*receipt = NULL,*gwatchrc = "gwatchrc";

int max_num = 0;
int max_secs = 0;

enum { m_login, m_logout, m_ping };

void *gale_malloc(int size) { return malloc(size); }
void gale_free(void *ptr) { free(ptr); }

void watch_cat(const char *cat) {
	subs[count_subs++] = cat;
}

void watch_ping(const char *cat,const char *id) {
	struct gale_message *msg;
	if (!receipt) {
		const char *host = getenv("HOST");
		char *tmp = gale_malloc(strlen(host) + 20);
		sprintf(tmp,"%s.%d",host,(int) getpid());
		receipt = gale_idtocat("receipt",getenv("GALE_ID"),tmp);
		gale_free(tmp);
		watch_cat(receipt);
	}

	msg = new_message();
	msg->category = gale_strdup(cat);
	msg->data = gale_malloc(30 + strlen(receipt));
	sprintf(msg->data,"Receipt-To: %s\r\n\r\n",receipt);
	msg->data_size = strlen(msg->data);

	if (id) encrypt_message(id,msg);

	pings[count_pings++] = msg;
}

void watch_id(const char *id) {
	watch_ping(gale_idtocat("user",id,"ping"),id);
	watch_cat(gale_idtocat("notice",id,""));
}

void watch_domain(const char *id) {
	char *tmp = gale_malloc(strlen(id) + 30);
	sprintf(tmp,"notice/%s",id);
	watch_cat(tmp);
}

void read_file(const char *fn) {
	FILE *fp;
	int num;
	const char *file;

	file = dir_search(fn,1,dot_gale,sys_dir,NULL);
	if (!file) {
		gale_alert(GALE_WARNING,fn,ENOENT);
		return;
	}

	fp = fopen(file,"r");
	if (!fp) {
		gale_alert(GALE_WARNING,file,errno);
		return;
	}

	do {
		char ch,var[40],value[256];
		while (fscanf(fp," #%*[^\n]%c",&ch) == 1) ;
		num = fscanf(fp,"%39s %255[^\n]",var,value);
		if (num != 2) continue;
		if (!strcmp(var,"category"))
			watch_cat(gale_strdup(value));
		else if (!strcmp(var,"ping"))
			watch_ping(value,NULL);
		else if (!strcmp(var,"id"))
			watch_id(value);
		else if (!strcmp(var,"domain"))
			watch_domain(value);
		else
			gale_alert(GALE_WARNING,var,EINVAL);
	} while (num == 2);

	fclose(fp);
}

void usage(void) {
	fprintf(stderr,
		"%s\n"
		"usage: gwatch [flags] cat\n"
		"flags: -n          Do not fork (default if -m, -t, or stdout redirected)\n"
		"       -k          Do not kill other gwatch processes\n"
		"       -K          Kill other gwatch processes and terminate\n"
		"       -r          Do not retry server connection\n"
		"       -i id       Watch user \"id\"\n"
		"       -d domain   Watch domain \"domain\"\n"
		"       -p cat      Send a \"ping\" to the given category\n"
		"       -m num      Exit after num responses received\n"
		"       -s count    Exit after count seconds pass\n"
		"       -w file     Use config file (default \"spylist\")\n"
		"       -f rc       Use script file (default \"gwatchrc\")\n"
		,GALE_BANNER);
	exit(1);
}

void open_client(void) {
	char *spec;
	int i,len = 0;

	for (i = 0; i < count_subs; ++i) len += 1 + strlen(subs[i]);
	spec = gale_malloc(len);

	strcpy(spec,subs[0]);
	len = strlen(spec);
	for (i = 1; i < count_subs; ++i) {
		spec[len++] = ':';
		strcpy(spec + len,subs[i]);
		len += strlen(subs[i]);
	}

	client = gale_open(spec,count_pings,262144);
	gale_free(spec);
}

void send_pings(void) {
	int i;

	for (i = 0; i < count_pings; ++i) {
		link_put(client->link,pings[i]);
		release_message(pings[i]);
	}
}

void incoming(int type,const char *id,const char *agent,const char *from,
              int seq) 
{
	char *flag;
	(void) seq; (void) id; (void) agent;
	switch (type) {
	case m_login: flag = "<login>"; break;
	case m_logout: flag = "<logout>"; break;
	case m_ping: flag = "<ping>"; break;
	}
	printf("%s %s\r\n",flag,from);
	fflush(stdout);
}

void process_message(struct gale_message *msg) {
	int type,len = strlen(msg->category);
	char *next,*key,*data,*end;
	char *id_sign = NULL,*id_encrypt = NULL,*decrypt = NULL;
	char *agent = NULL,*from = NULL;
	int sequence = -1,first = 1;

	if (len >= 7 && !strcmp(msg->category + len - 7,"/logout"))
		type = m_logout;
	else if (len >= 6 && !strcmp(msg->category + len - 6,"/login"))
		type = m_login;
	else
		type = m_ping;

	next = msg->data;
	end = next + msg->data_size;
	while (parse_header(&next,&key,&data,end)) {
		if (first && !strcasecmp(key,"Encryption")) {
			decrypt = gale_malloc(end - next + DECRYPTION_PADDING);
			id_encrypt = decrypt_data(data,next,end,decrypt,&end);
			if (!id_encrypt) goto error;
			next = decrypt;
			continue;
		}
		if (first && !strcasecmp(key,"Signature"))
			id_sign = verify_data(data,next,end);
		first = 0;
		if (!strcasecmp(key,"Agent")) agent = data;
		if (!strcasecmp(key,"From")) from = data;
		if (!strcasecmp(key,"Sequence")) sequence = atoi(data);
	}

#ifndef NDEBUG
	if (!strcmp(msg->category,"debug/restart") && 
	    id_sign && !strcmp(id_sign,"egnor@ofb.net")) {
		gale_alert(GALE_NOTICE,"Restarting from debug/restart.",0);
		execvp(restart_argv[0],restart_argv);
		gale_alert(GALE_WARNING,restart_argv[0],errno);
	}
#endif

	incoming(type,id_sign,agent,from,sequence);

error:
	if (id_sign) gale_free(id_sign);
	if (id_encrypt) gale_free(id_encrypt);
	if (decrypt) gale_free(decrypt);
}

int main(int argc,char *argv[]) {
	int arg,do_fork = 0,do_kill = 0,do_retry = 1;

	if ((tty = ttyname(1))) {
		char *tmp = strrchr(tty,'/');
		if (tmp) tty = tmp + 1;
		do_fork = do_kill = 1;
	}

	gale_init("gwatch");

	subs = gale_malloc(sizeof(char*) * argc);
	pings = gale_malloc(sizeof(struct gale_message *) * argc);

	while ((arg = getopt(argc,argv,"hnkKi:d:p:m:s:w:f:")) != EOF) 
	switch (arg) {
	case 'n': do_fork = 0; break;
	case 'k': do_kill = 0; break;
	case 'K': if (tty) gale_kill(tty,1); return 0;
	case 'r': do_retry = 0; break;
	case 'i': watch_id(optarg); break;
	case 'd': watch_domain(optarg); break;
	case 'p': watch_ping(optarg,NULL); break;
	case 'm': max_num = atoi(optarg); do_fork = 0; break;
	case 's': max_secs = atoi(optarg); do_fork = 0; break;
	case 'w': read_file(optarg);
	case 'f': gwatchrc = optarg;
	case 'h':
	case '?': usage();
	}

	if (optind != argc) {
		if (optind != argc - 1) usage();
		watch_cat(argv[optind]);
	}

	if (count_subs == 0) read_file("spylist");
	if (count_subs == 0) {
		gale_alert(GALE_WARNING,"Nothing specified to watch.",0);
		usage();
	}

#ifndef NDEBUG
	restart_argv = argv;
	watch_cat("debug/");
#endif

	open_client();
	if (!do_retry && gale_error(client))
		gale_alert(GALE_ERROR,"Could not connect to server.",0);

	if (do_fork) {
		gale_daemon(1);
		gale_kill(tty,do_kill);
	}

	send_pings();
	do {
		while (!gale_send(client) && !gale_next(client)) {
			struct gale_message *msg;
			if (tty && !isatty(1)) return 0;
			if ((msg = link_get(client->link))) {
				process_message(msg);
				release_message(msg);
			}
		}
		if (do_retry) gale_retry(client);
	} while (do_retry);

	gale_close(client);
	return 0;
}