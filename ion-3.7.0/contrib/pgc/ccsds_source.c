/*
 * ccsds_source
 *
 * Get CCSDS packets from a UDP port and sends them as bundles
 * to a destination node.
 * inspired from bpsource
 * 
 * Pat Cappelaere, ASRC Federal
 */

#include <bp.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_TTL 300
#define BUFLEN 4096	    // Max length of Packets

int verbose = 0;      // for debugging

static int _running(int *newState) {
  int state = 1;

  if (newState) {
    state = *newState;
  }

  return state;
}

static ReqAttendant	*_attendant(ReqAttendant *newAttendant) {
	static ReqAttendant	*attendant = NULL;

	if (newAttendant)	{
		attendant = newAttendant;
	}

	return attendant;
}

static void	handleQuit() {
	int	stop = 0;

	oK(_running(&stop));
	ionPauseAttendant(_attendant(NULL));
}

int	main(int argc, char **argv) {
	char *destEid = argv[1];
  int udpPort   = atoi(argv[2]);

  if(argc == 4) {
    verbose = 1;
  }
  
  printf("ccsds_source %s %d verbose:%d", destEid, udpPort, verbose);

	int	ttl       = DEFAULT_TTL;

	Sdr	sdr;
  Object extent;
	Object bundleZco;
	ReqAttendant attendant;
	Object newBundle;

  struct sockaddr_in si_me, si_other;
	int s, recv_len;
  size_t slen = sizeof(si_other);
	char buf[BUFLEN];

  if (bp_attach() < 0) {
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (ionStartAttendant(&attendant)) {
		putErrmsg("Can't initialize blocking transmission.", NULL);
		return 0;
	}

	_attendant(&attendant);
	sdr = bp_get_sdr();

  // create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		putErrmsg("Can't create UDP socket.", NULL);
    return 0;
	}
	
	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(udpPort);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// bind socket to port
	if( bind(s, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
		putErrmsg("Can't bind to UDP port.", NULL);
    return 0;
	}

  // Get CCSDS packets from UDP and send them out
	isignal(SIGINT, handleQuit);
	while (_running(NULL)) {
      //try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
		  putErrmsg("Failed recvfrom.", NULL);
      return 0;
		}

    if(verbose) {
      printf("Received from UDP: %d", recv_len);
      for ( int i = 0; i < recv_len; i++ ) {
        printf("%02x ", buf[i]);
      }
      printf("\n");
    }

    CHKZERO(sdr_begin_xn(sdr));
		extent = sdr_malloc(sdr, recv_len);
		if (extent) {
			sdr_write(sdr, extent, buf, recv_len);
		}

		if (sdr_end_xn(sdr) < 0) {
			putErrmsg("No space for ZCO extent.", NULL);
			bp_detach();
			break;
		}

    bundleZco = ionCreateZco(ZcoSdrSource, extent, 0, recv_len, BP_STD_PRIORITY, 0, ZcoOutbound, &attendant);
		if (bundleZco == 0 || bundleZco == (Object) ERROR) {
			putErrmsg("Can't create ZCO extent.", NULL);
			bp_detach();
			break;
		}

		if (bp_send(NULL, destEid, NULL, ttl, BP_STD_PRIORITY, NoCustodyRequested, 0, 0, NULL, bundleZco, &newBundle) < 1) {
			putErrmsg("ccsds_source can't send ADU.", NULL);
      break;
		}
  }

  printf("Exiting gracefully...");
  ionStopAttendant(_attendant(NULL));
	bp_detach();
	return 0;
}