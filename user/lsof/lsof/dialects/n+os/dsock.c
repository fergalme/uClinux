/*
 * dsock.c - NEXTSTEP and OPENSTEP socket processing functions for lsof
 */


/*
 * Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Victor A. Abell
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright 1994 Purdue Research Foundation.\nAll rights reserved.\n";
static char *rcsid = "$Id: dsock.c,v 1.17 2005/08/08 19:54:03 abe Exp $";
#endif


#include "lsof.h"


/*
 * process_socket() - process socket
 */

void
process_socket(sa)
	KA_T sa;			/* socket address in kernel */
{
	struct domain d;
	char *ep;
	unsigned char *fa = (unsigned char *)NULL;
	int fam;
	int fp, lp;
	struct inpcb inp;
	unsigned char *la = (unsigned char *)NULL;
	struct mbuf mb;
	struct protosw p;
	struct rawcb raw;
	struct socket s;
	size_t sz;
	struct tcpcb t;
	struct unpcb uc, unp;
	struct sockaddr_un *ua = NULL;
	struct sockaddr_un un;

	(void) snpf(Lf->type, sizeof(Lf->type), "sock");
	Lf->inp_ty = 2;
/*
 * Read socket structure.
 */
	if (!sa) {
	    enter_nm("no socket address");
	    return;
	}
	if (kread((KA_T) sa, (char *) &s, sizeof(s))) {
	    (void) snpf(Namech,Namechl,"can't read socket struct from %#x",sa);
	    enter_nm(Namech);
	    return;
	}
	if (!s.so_type) {
	     enter_nm("no socket type");
	    return;
	}
/*
 * Read protocol switch and domain structures.
 */
	if (!s.so_proto
	||  kread((KA_T) s.so_proto, (char *) &p, sizeof(p))) {
	    (void) snpf(Namech, Namechl, "no protocol switch");
	    enter_nm(Namech);
	    return;
	}
	if (kread((KA_T) p.pr_domain, (char *) &d, sizeof(d))) {
	    (void) snpf(Namech, Namechl, "can't read domain struct from %s",
		print_kptr((KA_T)p.pr_domain, (char *)NULL, 0));
	    enter_nm(Namech);
	    return;
	}
/*
 * Save size information.
 */
	if (Fsize) {
	    if (Lf->access == 'r')
		Lf->sz = (SZOFFTYPE)s.so_rcv.sb_cc;
	    else if (Lf->access == 'w')
		Lf->sz = (SZOFFTYPE)s.so_snd.sb_cc;
	    else
		Lf->sz = (SZOFFTYPE)(s.so_rcv.sb_cc + s.so_snd.sb_cc);
	    Lf->sz_def = 1;
	} else
	    Lf->off_def = 1;

#if	defined(HASTCPTPIQ)
	Lf->lts.rq = (unsigned long)s.so_rcv.sb_cc;
	Lf->lts.sq = (unsigned long)s.so_snd.sb_cc;
	Lf->lts.rqs = Lf->lts.sqs = 1;
#endif	/* defined(HASTCPTPIQ) */

#if	defined(HASSOOPT)
	Lf->lts.ltm = (unsigned int)s.so_linger;
	Lf->lts.opt = (unsigned int)s.so_options;
	Lf->lts.pqlen = (unsigned int)s.so_q0len;
	Lf->lts.qlen = (unsigned int)s.so_qlen;
	Lf->lts.qlim = (unsigned int)s.so_qlimit;
	Lf->lts.rbsz = (unsigned long)s.so_rcv.sb_mbmax;
	Lf->lts.sbsz = (unsigned long)s.so_snd.sb_mbmax;
	Lf->lts.pqlens = Lf->lts.qlens = Lf->lts.qlims = Lf->lts.rbszs
		       = Lf->lts.sbszs = (unsigned char)1;
#endif	/* defined(HASSOOPT) */

#if	defined(HASSOSTATE)
	Lf->lts.ss = (unsigned int)s.so_state;
#endif	/* defined(HASSOSTATE) */

/*
 * Process socket by the associated domain family.
 */
	switch ((fam = d.dom_family)) {
/*
 * Process an Internet domain socket.
 */
	case AF_INET:
	    if (Fnet)
		Lf->sf |= SELNET;
	    (void) snpf(Lf->type, sizeof(Lf->type), "inet");
	    printiproto(p.pr_protocol);
	/*
	 * Read protocol control block.
	 */
	    if (!s.so_pcb) {
		(void) snpf(Namech, Namechl, "no PCB%s%s",
		    (s.so_state & SS_CANTSENDMORE) ? ", CANTSENDMORE" : "",
		    (s.so_state & SS_CANTRCVMORE)  ? ", CANTRCVMORE"  : "");
		enter_nm(Namech);
		return;
	    }
	    if (s.so_type == SOCK_RAW) {

	    /*
	     * Print raw socket information.
	     */
		if (kread((KA_T) s.so_pcb, (char *)&raw, sizeof(raw))
		||  (struct socket *)sa != raw.rcb_socket) {
		    (void) snpf(Namech, Namechl, "can't read rawcb at %s",
			print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
		    enter_nm(Namech);
		    return;
		}
		enter_dev_ch(print_kptr((KA_T)(raw.rcb_pcb ? raw.rcb_pcb
							   : s.so_pcb),
						    (char *)NULL, 0));
		if (raw.rcb_laddr.sa_family == AF_INET)
		    la = (unsigned char *)&raw.rcb_laddr.sa_data[2];
		else if (raw.rcb_laddr.sa_family)
		    printrawaddr(&raw.rcb_laddr);
		if (raw.rcb_faddr.sa_family == AF_INET)
		    fa = (unsigned char *)&raw.rcb_faddr.sa_data[2];
		else if (raw.rcb_faddr.sa_family) {
		    ep = endnm(&sz);
		    (void) snpf(ep, sz, "->");
		    printrawaddr(&raw.rcb_faddr);
		}
		if (fa || la)
		    (void) ent_inaddr(la, -1, fa, -1, AF_INET);
	    } else {

	    /*
	     * Print Internet socket information.
	     */
		if (kread((KA_T)s.so_pcb, (char *) &inp, sizeof(inp))
		||  (struct socket *)sa != inp.inp_socket) {
		    (void) snpf(Namech, Namechl, "can't read inpcb at %s",
			 print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
		    enter_nm(Namech);
		    return;
		}
		enter_dev_ch(print_kptr((KA_T)(inp.inp_ppcb ? inp.inp_ppcb
							    : s.so_pcb),
					 (char *)NULL, 0));
	    /*
	     * If the protocol is TCP, try to read the TCP protocol
	     * control block to record its state.
	     */
		if (p.pr_protocol == IPPROTO_TCP
		&&  inp.inp_ppcb
		&&  kread((KA_T)inp.inp_ppcb, (char *)&t, sizeof(t)) == 0) {
		    Lf->lts.type = 0;
		    Lf->lts.state.i = (int)t.t_state;

#if	defined(HASTCPOPT)
		    Lf->lts.mss = (unsigned long)t.t_maxseg;
		    Lf->lts.msss = (unsigned char)1;
		    Lf->lts.topt = (unsigned int)t.t_flags;
#endif	/* defined(HASTCPOPT) */

		}
	    /*
	     * Process the local and foreign addresses from the Internet
	     * control block.
	     */
		la = (unsigned char *)&inp.inp_laddr;
		lp = (int)ntohs(inp.inp_lport);
		if (inp.inp_faddr.s_addr != INADDR_ANY || inp.inp_fport != 0) {
		    fa = (unsigned char *)&inp.inp_faddr;
		    fp = (int)ntohs(inp.inp_fport);
		}
		if (fa || la)
		    (void) ent_inaddr(la, lp, fa, fp, AF_INET);
	    }
	    break;
/*
 * Process a Unix domain socket.
 */
	case AF_UNIX:
	    if (Funix)
		Lf->sf |= SELUNX;
	    (void) snpf(Lf->type, sizeof(Lf->type), "unix");
	/*
	 * Read Unix protocol control block and the Unix address structure.
	 */
	    enter_dev_ch(print_kptr(sa, (char *)NULL, 0));
	    if (kread((KA_T) s.so_pcb, (char *) &unp, sizeof(unp))) {
		(void) snpf(Namech, Namechl, "can't read unpcb at %s",
		    print_kptr((KA_T)s.so_pcb, (char *)NULL, 0));
		break;
	    }
	    if ((struct socket *)sa != unp.unp_socket) {
		(void) snpf(Namech, Namechl, "unp_socket (%s) mismatch",
		    print_kptr((KA_T)unp.unp_socket, (char *)NULL, 0));
		break;
	    }
	    if (unp.unp_addr) {
		if (kread((KA_T) unp.unp_addr, (char *) &mb, sizeof(mb))) {
		    (void) snpf(Namech, Namechl, "can't read unp_addr at %s",
			print_kptr((KA_T)unp.unp_addr, (char *)NULL, 0));
		    break;
		}
		ua = (struct sockaddr_un *)(((char *)&mb) + mb.m_off);
	    }
	    if (!ua) {
		ua = &un;
		(void) bzero((char *)ua, sizeof(un));
		ua->sun_family = AF_UNSPEC;
	    }
	/*
	 * Print information on Unix socket that has no address bound
	 * to it, although it may be connected to another Unix domain
	 * socket as a pipe.
	 */
	    if (ua->sun_family != AF_UNIX) {
		if (ua->sun_family == AF_UNSPEC) {
		    if (unp.unp_conn) {
			if (kread((KA_T)unp.unp_conn, (char *) &uc, sizeof(uc)))
			{
			    (void) snpf(Namech, Namechl,
				"can't read unp_conn at %s",
				print_kptr((KA_T)unp.unp_conn,(char *)NULL,0));
			} else {
			    (void) snpf(Namech, Namechl, "->%s",
				print_kptr((KA_T)uc.unp_socket,(char *)NULL,0));
			}
		    } else
			(void) snpf(Namech, Namechl, "->(none)");
		} else
		    (void) snpf(Namech, Namechl, "unknown sun_family (%d)",
			ua->sun_family);
		break;
	    }
	    if (ua->sun_path[0]) {
		if (mb.m_len >= sizeof(struct sockaddr_un))
		    mb.m_len = sizeof(struct sockaddr_un) - 1;
		*((char *)ua + mb.m_len) = '\0';
		if (Sfile && is_file_named(ua->sun_path, 0))
		    Lf->sf |= SELNM;
		if (!Namech[0])
		    (void) snpf(Namech, Namechl, "%s", ua->sun_path);
	    } else
		(void) snpf(Namech, Namechl, "no address");
	    break;
	default:
	    printunkaf(fam, 1);
	}
	if (Namech[0])
	    enter_nm(Namech);
}