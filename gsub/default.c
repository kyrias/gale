/* default.c -- default gsubrc implementation. */

#include "default.h"
#include "gale/all.h"
#include "gale/gsubrc.h"

#include <ctype.h>
#include <stdlib.h> /* TODO */

/* Print a string, shell-quoting any special characters. */
static void print_string(int style,struct gale_text string) {
	int i;
	for (i = 0; i < string.l; ++i)
		if (string.p[i] > 128 
		|| (!isalnum(string.p[i])
		&&  '.' != string.p[i]
		&&  ',' != string.p[i]
		&&  '-' != string.p[i]
		&&  '_' != string.p[i]
		&&  '/' != string.p[i]
		&&  '@' != string.p[i])) break;
	if (i < string.l) gale_print(stdout,0,G_("\""));
	gale_print(stdout,style,string);
	if (i < string.l) gale_print(stdout,0,G_("\""));
}

/* Print a user ID, with a default string (like "everyone") for NULL. */
static void print_id(struct gale_text var,struct gale_text dfl) {
	struct gale_text value = gale_var(var);
	int i = 1;

	if (0 == value.l) {
		gale_print(stdout,0,G_(" *"));
		gale_print(stdout,gale_print_bold,dfl);
		gale_print(stdout,0,G_("*"));
		return;
	}

	do {
                int at;
		gale_print(stdout,0,G_(" "));
                for (at = 0; at < value.l && '@' != value.p[at]; ++at) ;
		print_string(gale_print_bold,gale_text_left(value,at));
		print_string(0,gale_text_right(value,-at));

		value = gale_var(gale_text_concat(3,var,G_("_"),
			gale_text_from_number(++i,10,0)));
	} while (0 != value.l);
}

static int id_width(struct gale_text var,struct gale_text dfl) {
	struct gale_text value = gale_var(var);
	int i = 1,len = 0;
	if (0 == value.l) return 3 + dfl.l;

	do {
		len += 1 + value.l;
		value = gale_var(gale_text_concat(3,var,G_("_"),
			gale_text_from_number(++i,10,0)));
	} while (0 != value.l);
	return len;
}

/* The Wacky Quoting Proposal (WQP), as implemented:
   For any line of text, the Quote is /^([ >]*|\|[^>])/.  The Quote is repeated
   at the beginning of all wrap lines.  If the Quote is longer than the screen,
   then it is no longer considered the Quote. */
void default_gsubrc(void) {
	char *buf, *quotebuf;
	struct gale_text timecode,text;
	struct gale_text presence = gale_var(G_("GALE_TEXT_NOTICE_PRESENCE"));
	struct gale_text answer = gale_var(G_("GALE_TEXT_ANSWER_RECEIPT"));
	struct gale_text from_name = gale_var(G_("GALE_TEXT_MESSAGE_SENDER"));
	int i,len,buflen,bufloaded = 0,termwid = gale_columns(stdout);

	if (termwid < 2) termwid = 80; /* Don't crash */

	/* Get the time */
	if (0 == (timecode = gale_var(G_("GALE_TIME_ID_TIME"))).l) {
		char tstr[80];
		time_t when = time(NULL);
		strftime(tstr,sizeof(tstr),"%Y-%m-%d %H:%M:%S",localtime(&when));
		timecode = gale_text_from(NULL,tstr,-1);
	}

	if (0 != gale_var(G_("GALE_DATA_SECURITY_ENCRYPTION")).l) {
		gale_alert(GALE_WARNING,gale_text_concat(3,
			G_("cannot decrypt message to \""),
			gale_var(G_("GALE_TO")),
			G_("\"")),0);
		return;
	}

	if (0 != (text = gale_var(G_("GALE_TEXT_QUESTION_KEY"))).l)
		return; /* ignore AKD request */

	/* Format return receipts and presence notices specially */
	if (0 != answer.l || 0 != presence.l) {
		gale_print(stdout,
		gale_print_bold | gale_print_clobber_left,G_("* "));
		gale_print(stdout,0,timecode);

		if (answer.l) gale_print(stdout,0,G_(" received:"));
		if (presence.l) {
			gale_print(stdout,0,G_(" "));
			print_string(0,presence);
		}

		print_id(G_("GALE_FROM"),G_("unverified"));
		if (from_name.l) {
			gale_print(stdout,0,G_(" ("));
			gale_print(stdout,0,from_name);
			gale_print(stdout,0,G_(")"));
		}
		gale_print(stdout,gale_print_clobber_right,G_(""));
		gale_print(stdout,0,G_("\n"));
		fflush(stdout);
		return;
	}

	gale_print(stdout,gale_print_clobber_left,G_("-"));
	for (len = 0; len < termwid - 3; ++len) gale_print(stdout,0,G_("-"));
	gale_print(stdout,gale_print_clobber_right,G_("-"));
	gale_print(stdout,0,G_("\n"));

	/* Print the header */

	gale_print(stdout,0,G_("To:"));
	print_id(G_("GALE_TO"),G_("unknown"));
	text = gale_var(G_("GALE_TEXT_MESSAGE_RECIPIENT"));
	if (text.l) {
		gale_print(stdout,0,G_(" ("));
		gale_print(stdout,0,text);
		gale_print(stdout,0,G_(")"));
	}

	i = 1;
	text = gale_var(G_("GALE_TEXT_MESSAGE_KEYWORD"));
	while (text.l) {
		gale_print(stdout,0,G_(" /"));
		print_string(gale_print_bold,text);
		text = gale_var(gale_text_concat(2,
			G_("GALE_TEXT_MESSAGE_KEYWORD_"),
			gale_text_from_number(++i,10,0)));
	}

	if (gale_var(G_("GALE_TEXT_QUESTION_RECEIPT")).l)
		gale_print(stdout,gale_print_clobber_right,G_(" [rcpt]"));

	gale_print(stdout,gale_print_clobber_right,G_("\n"));

	/* Print the message body. */
	buflen = termwid; /* can't be longer than this */
	buf = gale_malloc(buflen);
	quotebuf = gale_malloc(buflen);

	while (1) {
		int quotelen = 0, quotecol = 0;
		char curchar;
		/* Read more data in order to process a line of input: */
		bufloaded += fread(buf + bufloaded, 1, buflen - bufloaded, stdin);
		if (!bufloaded && feof(stdin)) goto done_proper;

		/* Find the Quote. */
		while (1) {
			if (quotelen == bufloaded || quotecol >= termwid) {
				/* This Quote is too long - give up and format as regular text. */
				quotelen = quotecol = 0;
				goto end_quote;
			}
			curchar = buf[quotelen];
			if (('\n' != curchar && isspace(curchar)) || '>' == curchar) {
				quotecol = gale_column(quotecol, curchar); ++quotelen;
			} else if ('|' == curchar) {
				++quotecol; ++quotelen;
				for (; quotelen < bufloaded && '>' != buf[quotelen]; ++quotelen)
					quotecol = gale_column(quotecol, buf[quotelen]);
			} else
				goto end_quote;
		}
		end_quote:

		/* Process rest of the line. */
		while (1) {
			/* Produce a line of output. */
			int pos = quotelen; /* current position */
			int col = quotecol; /* current screen column */
			int prevend = pos; /* end of previous word */
			int curstart = pos; /* start of current word */

			/* Advance past end of first word. */
			for (; pos < bufloaded && !isspace(buf[pos]); ++pos) {
				col = gale_column(col, buf[pos]);
				if (col >= termwid) {
					/* Extra long word!  Output it verbatim. */
					gale_print(stdout, gale_print_clobber_right, 
						 gale_text_from(gale_global->enc_console, buf, quotelen));
					do {
						for (; pos < bufloaded && !isspace(buf[pos]); ++pos)
							col = gale_column(col, buf[pos]);
						gale_print(stdout, 0, 
						           gale_text_from(gale_global->enc_console, buf + quotelen, pos - quotelen));

						/* Read more, if necessary. */
						if (pos == bufloaded) {
							pos = bufloaded = quotelen;
							bufloaded += fread(buf + bufloaded, 1, buflen - bufloaded, stdin);
							if (pos == bufloaded && feof(stdin)) goto done_premie;
						}
					} while (!isspace(buf[pos]));
					
					/* Skip whitespace. */
					for (; '\n' != buf[pos] && isspace(buf[pos]);)
						if (++pos == bufloaded) {
							pos = bufloaded = quotelen;
							bufloaded += fread(buf + bufloaded, 1, buflen - bufloaded, stdin);
							if (pos == bufloaded && feof(stdin)) goto done_premie;
						}
					gale_print(stdout,0,G_("\n"));
					if ('\n' == buf[pos]) {
						++pos; 
						memmove(buf, buf + pos, bufloaded - pos);
						bufloaded -= pos;
						goto end_line;
					} else {
						memmove(buf + quotelen, buf + pos, bufloaded - pos);
						bufloaded -= pos - quotelen;
						goto end_out_line;
					}
				}
			}

			/* Process remaining words.*/
			while (1) {
				prevend = pos;

				/* Have we reached premature EOF? */
				if (pos == bufloaded) {
					gale_print(stdout, 0, gale_text_from(gale_global->enc_console, buf, prevend));
					goto done_premie;
				}

				/* Advance past whitespace. */
				for (; pos < bufloaded && isspace(buf[pos]); ++pos) {
					col = gale_column(col, buf[pos]);
					if ('\n' == buf[pos] || col >= termwid) {
						/* Wrap line! */
						gale_print(stdout, 0, gale_text_from(gale_global->enc_console, buf, prevend));
						gale_print(stdout,0,G_("\n"));
						/* Skip any more whitespace. */
						for (; '\n' != buf[pos] && isspace(buf[pos]);)
							if (++pos == bufloaded) {
								pos = bufloaded = quotelen;
								bufloaded += fread(buf + bufloaded,1,buflen - bufloaded,stdin);
								if (pos == bufloaded && feof(stdin)) goto done_premie;
							}
						if ('\n' == buf[pos]) {
							++pos; 
							memmove(buf, buf + pos, bufloaded - pos);
							bufloaded -= pos;
							goto end_line;
						} else {
							memmove(buf + quotelen, buf + pos, bufloaded - pos);
							bufloaded -= pos - quotelen;
							goto end_out_line;
						}
					}
				}

				/* Have we reached premature EOF? */
				if (pos == bufloaded) {
					gale_print(stdout, 0, gale_text_from(gale_global->enc_console, buf, prevend));
					goto done_premie;
				}

				/* Process next word. */
				curstart = pos;
				while (pos < bufloaded && !isspace(buf[pos])) {
					col = gale_column(col, buf[pos++]);
					if (col >= termwid) {
						/* Wrap line! */
						gale_print(stdout, 0, gale_text_from(gale_global->enc_console, buf, prevend));
						gale_print(stdout,0,G_("\n"));
						memmove(buf + quotelen, buf + curstart, bufloaded - curstart);
						bufloaded -= curstart - quotelen;
						goto end_out_line;
					}
				}
			}

			end_out_line:
			bufloaded += fread(buf + bufloaded, 1, buflen - bufloaded, stdin);
			if (!bufloaded && feof(stdin)) goto done_proper;
		}
		end_line: ;
	}
	gale_print(stdout, gale_print_clobber_right, 
	           gale_text_from(gale_global->enc_console, buf, buflen));
	done_premie:
	/* We must have got here via premature EOF, so add a newline. */
	gale_print(stdout,0,G_("\n"));
	done_proper:

	/* Print the signature information. */
	{
		struct gale_text from_name = 
			gale_var(G_("GALE_TEXT_MESSAGE_SENDER"));
		int len = 0;

		if (0 == from_name.l)
			len += id_width(G_("GALE_FROM"),G_("anonymous"));
		else
			len += id_width(G_("GALE_FROM"),G_("unverified")) 
			    +  from_name.l + 3;

		while (len++ < termwid - 24) gale_print(stdout,0,G_(" "));
		gale_print(stdout,0,G_("--"));
		if (0 == from_name.l)
			print_id(G_("GALE_FROM"),G_("anonymous"));
		else {
			print_id(G_("GALE_FROM"),G_("unverified"));
			gale_print(stdout,0,G_(" ("));
			gale_print(stdout,0,from_name);
			gale_print(stdout,0,G_(")"));
		}

		gale_print(stdout,0,G_(" at "));
		gale_print(stdout,gale_print_clobber_right,
			gale_text_right(timecode,-5));
		gale_print(stdout,0,G_(" --"));
		gale_print(stdout,gale_print_clobber_right,G_("\n"));
	}

	fflush(stdout);
}
