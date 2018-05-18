#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <asm/byteorder.h>
#include <assert.h>
#include <signal.h>
#include <sys/queue.h>
#include <errno.h>

#include <mos_api.h>
#include "cpu.h"

/* Maximum CPU cores */
#define MAX_CORES 		16
/* Number of TCP flags to monitor */
#define NUM_FLAG 		6
/* Default path to mOS configuration file */
#define MOS_CONFIG_FILE		"config/mos.conf"
/*----------------------------------------------------------------------------*/
/* Global variables */

struct connection {
	int sock;                      /* socket ID */
	struct sockaddr_in addrs[2];   /* Address of a client and a serer */
	int cli_state;                 /* TCP state of the client */
	int svr_state;                 /* TCP state of the server */
	TAILQ_ENTRY(connection) link;  /* link to next context in this core */
};

int g_max_cores;                              /* Number of CPU cores to be used */
mctx_t g_mctx[MAX_CORES];                     /* mOS context */
TAILQ_HEAD(, connection) g_sockq[MAX_CORES];  /* connection queue */
/*----------------------------------------------------------------------------*/
/* Signal handler */
static void
sigint_handler(int signum)
{
	int i;

	/* Terminate the program if any interrupt happens */
	for (i = 0; i < g_max_cores; i++)
		mtcp_destroy_context(g_mctx[i]);
}
/*----------------------------------------------------------------------------*/
/* Find connection structure by socket ID */
static inline struct connection *
find_connection(int cpu, int sock)
{
	struct connection *c;

	TAILQ_FOREACH(c, &g_sockq[cpu], link)
		if (c->sock == sock)
			return c;

	return NULL;
}
/*----------------------------------------------------------------------------*/
/* Create connection structure for new connection */

/*----------------------------------------------------------------------------*/
/* Convert state value (integer) to string (char array) */
const char *
strstate(int state)
{
	switch (state) {
#define CASE(s) case TCP_##s: return #s
		CASE(CLOSED);
		CASE(LISTEN);
		CASE(SYN_SENT);
		CASE(SYN_RCVD);
		CASE(ESTABLISHED);
		CASE(FIN_WAIT_1);
		CASE(FIN_WAIT_2);
		CASE(CLOSE_WAIT);
		CASE(CLOSING);
		CASE(LAST_ACK);
		CASE(TIME_WAIT);
		default:
		return "-";
	}
}
/*----------------------------------------------------------------------------*/
/* Print ongoing connection information based on connection structure */
/*static void
cb_printstat(mctx_t mctx, int sock, int side,
				  uint64_t events, filter_arg_t *arg)
{
	int i;
	struct connection *c;
//	struct timeval tv_1sec = {
//		.tv_sec = 1,
//		.tv_usec = 0
//	};

	printf("Proto CPU "
		   "Client Address        Client State "
		   "Server Address        Server State\n");
	for (i = 0; i < g_max_cores; i++)
		TAILQ_FOREACH(c, &g_sockq[i], link) {
		//	int space;

			printf("%-5s %-3d ", "tcp", i);
			space = printf("%s:", inet_ntoa(c->addrs[MOS_SIDE_CLI].sin_addr));
			printf("%*d %-12s ",
					space - 21,
					ntohs(c->addrs[MOS_SIDE_CLI].sin_port),
					strstate(c->cli_state));
			space = printf("%s:", inet_ntoa(c->addrs[MOS_SIDE_SVR].sin_addr));
			printf("%*d %-12s\n",
					space - 21,
					ntohs(c->addrs[MOS_SIDE_SVR].sin_port),
					strstate(c->svr_state));
		}


//	if (mtcp_settimer(mctx, sock, &tv_1sec, cb_printstat)) {
//		fprintf(stderr, "Failed to register print timer\n");
//		exit(-1);
//	}

	return;
}*/
/*----------------------------------------------------------------------------*/
/* Register required callbacks */

static void
Change_eth_addr(mctx_t mctx, int msock, int side,
        uint64_t events, filter_arg_t *arg)

{
    /* this function is called at the first SYN */
    struct pkt_info p;


    mtcp_getlastpkt(mctx, msock, side, &p);
   /* printf("In callback: eth src:%x:%x:%x:%x:%x:%x  ",p.ethh->h_source[0],p.ethh->h_source[1],p.ethh->h_source[2],p.ethh->h_source[3],p.ethh->h_source[4],p.ethh->h_source[5]);
    printf("In callback: eth dst:%x:%x:%x:%x:%x:%x  ",p.ethh->h_dest[0],p.ethh->h_dest[1],p.ethh->h_dest[2],p.ethh->h_dest[3],p.ethh->h_dest[4],p.ethh->h_dest[5]);

    printf("received pkt!  ");
    printf("side: %d\n",side);*/

    //printf("dst_mac: 0%x:%x:%x:%x:%x:%x\n",p.ethh->h_dest[0],p.ethh->h_dest[1],p.ethh->h_dest[2],p.ethh->h_dest[3],p.ethh->h_dest[4],p.ethh->h_dest[5]);

    if(side == MOS_SIDE_CLI){
        //printf("from client!\n");
        char mac_dst_str[6]={0x3c, 0xfd, 0xfe, 0x06, 0x07, 0x82};
       // char mac_src_str[6]={0x3c, 0xfd, 0xfe, 0x06, 0x09, 0x62};
        mtcp_setlastpkt(mctx, msock, side, 0,
                        (uint8_t*)mac_dst_str, 6, MOS_ETH_HDR | MOS_OVERWRITE);
       // mtcp_setlastpkt(mctx, msock, side, 6,
       //                 (uint8_t*)mac_src_str, 6, MOS_ETH_HDR | MOS_OVERWRITE);
    }else if(side == MOS_SIDE_SVR){
        //printf("from server!!!!!!!\n");
        char mac_dst_str[6]={0x3c, 0xfd, 0xfe, 0x06, 0x08, 0x00};
       // char mac_src_str[6]={0x3c, 0xfd, 0xfe, 0x06, 0x09, 0x60};
        mtcp_setlastpkt(mctx, msock, side, 0,
                        (uint8_t*)mac_dst_str, 6, MOS_ETH_HDR | MOS_OVERWRITE);
      //  mtcp_setlastpkt(mctx, msock, side, 6,
      //                  (uint8_t*)mac_src_str, 6, MOS_ETH_HDR | MOS_OVERWRITE);
    }

    //printf("After modification......\n");
    //printf("dst_mac: 0%x:%x:%x:%x:%x:%x\n",p.ethh->h_dest[0],p.ethh->h_dest[1],p.ethh->h_dest[2],p.ethh->h_dest[3],p.ethh->h_dest[4],p.ethh->h_dest[5]);

}


static void
RegisterCallbacks(mctx_t mctx, int sock, event_t ev_new_syn)
{
	/*struct timeval tv_1sec = {
		.tv_sec = 1,
		.tv_usec = 0
	};*/

	/* Register callbacks */
    if (mtcp_register_callback(mctx, sock,
                               MOS_ON_PKT_IN,
                               MOS_HK_SND,
                               Change_eth_addr) == -1){
        fprintf(stderr, "Failed to register cb_st_chg()\n");
        exit(-1); /* no point in proceeding if callback registration fails */
    }



	/* CPU 0 is in charge of printing stats */
	/*if (mctx->cpu == 0 &&
		mtcp_settimer(mctx, sock, &tv_1sec, cb_printstat)) {
		fprintf(stderr, "Failed to register print timer\n");
		exit(-1);
	}*/
}
/*----------------------------------------------------------------------------*/
/* Open monitoring socket and ready it for monitoring */
static void
InitMonitor(mctx_t mctx, event_t ev_new_syn)
{
	int sock;

	/* Initialize internal memory structures */
	TAILQ_INIT(&g_sockq[mctx->cpu]);

	/* create socket and set it as nonblocking */
	if ((sock = mtcp_socket(mctx, AF_INET,
						 MOS_SOCK_MONITOR_STREAM, 0)) < 0) {
		fprintf(stderr, "Failed to create monitor listening socket!\n");
		exit(-1); /* no point in proceeding if we don't have a listening socket */
	}

	/* Disable socket buffer */
	int optval = 0;
	if (mtcp_setsockopt(mctx, sock, SOL_MONSOCKET, MOS_CLIBUF,
							   &optval, sizeof(optval)) == -1) {
		fprintf(stderr, "Could not disable CLIBUF!\n");
	}
	if (mtcp_setsockopt(mctx, sock, SOL_MONSOCKET, MOS_SVRBUF,
							   &optval, sizeof(optval)) == -1) {
		fprintf(stderr, "Could not disable SVRBUF!\n");
	}

	RegisterCallbacks(mctx, sock, ev_new_syn);
}
/*----------------------------------------------------------------------------*/
int 
main(int argc, char **argv)
{
	int i, opt;
	event_t ev_new_syn;             /* New SYN UDE */
	char *fname = MOS_CONFIG_FILE;  /* path to the default mos config file */
	struct mtcp_conf mcfg;          /* mOS configuration */

	/* get the total # of cpu cores */
	g_max_cores = GetNumCPUs();       

	/* Parse command line arguments */
	while ((opt = getopt(argc, argv, "c:f:")) != -1) {
		switch (opt) {
		case 'f':
			fname = optarg;
			break;
		case 'c':
			if (atoi(optarg) > g_max_cores) {
				printf("Available number of CPU cores is %d\n", g_max_cores);
				return -1;
			}
			g_max_cores = atoi(optarg);
			break;
		default:
			printf("Usage: %s [-f mos_config_file] [-c #_of_cpu]\n", argv[0]);
			return 0;
		}
	}

	/* parse mos configuration file */
	if (mtcp_init(fname)) {
		fprintf(stderr, "Failed to initialize mtcp.\n");
		exit(EXIT_FAILURE);
	}

	/* set the core limit */
	mtcp_getconf(&mcfg);
	mcfg.num_cores = g_max_cores;
	mtcp_setconf(&mcfg);
	printf("gmax in main: %d\n",g_max_cores);

	/* Register signal handler */
	mtcp_register_signal(SIGINT, sigint_handler);

	for (i = 0; i < g_max_cores; i++) {
		/* Run mOS for each CPU core */
		if (!(g_mctx[i] = mtcp_create_context(i))) {
			fprintf(stderr, "Failed to craete mtcp context.\n");
			return -1;
		}

		/* init monitor */
		InitMonitor(g_mctx[i], ev_new_syn);
	}

	/* wait until mOS finishes */
	for (i = 0; i < g_max_cores; i++)
		mtcp_app_join(g_mctx[i]);

	mtcp_destroy();
	return 0;
}
/*----------------------------------------------------------------------------*/
