/*
 * ccsds_sink
 *
 * Receives CCSDS packets over DTN and send them to a UDP port
 * 
 * inspired from bpsink
 * 
 * Pat Cappelaere, ASRC Federal
 */

#include <bp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DEFAULT_TTL 300
#define BUFLEN 4096	     // Max length of Packets

int verbose = 0;		// debug 

typedef struct {
	BpSAP	sap;
	int	running;
} BptestState;

static BptestState	*_bptestState(BptestState *newState) {
	void		*value;
	BptestState	*state;

	if (newState) {			/*	Add task variable.	*/
		value = (void *) (newState);
		state = (BptestState *) sm_TaskVar(&value);
	}
	else {				      /*	Retrieve task variable.	*/ 
		state = (BptestState *) sm_TaskVar(NULL);
	}

	return state;
}

static void	handleQuit() {
	BptestState	*state;

	isignal(SIGINT, handleQuit);
	PUTS("BP reception interrupted.");
	state = _bptestState(NULL);
	bp_interrupt(state->sap);
	state->running = 0;
}

int	main(int argc, char **argv) {
  char *ownEid  = argv[1];
  int udpPort   = atoi(argv[2]);

  if(argc==4) verbose = 1;
  
  printf("ccsds_sink %s %d verbose:%d", ownEid, udpPort, verbose);

  static char	*deliveryTypes[] =	{
    "Payload delivered.",
    "Reception timed out.",
    "Reception interrupted.",
    "Endpoint stopped."
  };

	BptestState	state = { NULL, 1 };
	Sdr	sdr;
	BpDelivery	dlv;
	int	contentLength;
	ZcoReader	reader;
	int	len;
	char content[BUFLEN];
  
  struct sockaddr_in si_me, si_other;
	int s;
	size_t slen = sizeof(si_other);
  
  // Create a UDP socket
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		putErrmsg("Can't create UDP Socket.", NULL);
		return 0;
  }

  // zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(udpPort);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Bind socket to port
	if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
	  putErrmsg("Can't bind to UDP port.", NULL);
		return 0;	
  }

  if (bp_attach() < 0) {
		putErrmsg("Can't attach to BP.", NULL);
		return 0;
	}

	if (bp_open(ownEid, &state.sap) < 0) {
		putErrmsg("Can't open own endpoint.", ownEid);
		return 0;
	}

	oK(_bptestState(&state));
	sdr = bp_get_sdr();
	isignal(SIGINT, handleQuit);

	while (state.running) {
    if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0) {
			putErrmsg("bpsink bundle reception failed.", NULL);
			state.running = 0;
			continue;
		}

		PUTMEMO("ION event", deliveryTypes[dlv.result - 1]);
		if (dlv.result == BpReceptionInterrupted) {
			continue;
		}

		if (dlv.result == BpEndpointStopped) {
			state.running = 0;
			continue;
		}

		if (dlv.result == BpPayloadPresent) {
			CHKZERO(sdr_begin_xn(sdr));
			contentLength = zco_source_data_length(sdr, dlv.adu);
			sdr_exit_xn(sdr);

			// isprintf(line, sizeof line, "\tpayload length is %d.", contentLength);
			// PUTS(line);
			
      if (contentLength < sizeof content) {
				zco_start_receiving(dlv.adu, &reader);
				CHKZERO(sdr_begin_xn(sdr));
				len = zco_receive_source(sdr, &reader, contentLength, content);
				if (sdr_end_xn(sdr) < 0 || len < 0) {
					putErrmsg("Can't handle delivery.", NULL);
					state.running = 0;
					continue;
				}

        if(verbose) {
          printf("Received from DTN: %d", contentLength);
          for ( int i = 0; i < contentLength; i++ ) {
            printf("%02x ", content[i]);
          }
          printf("\n");
        }

        //now reply the client with the same data
		    if (sendto(s, content, len, 0, (struct sockaddr*)&si_other, slen) == -1) {
					putErrmsg("Error sending to UDP.", NULL);
		    }
			}
		}
		bp_release_delivery(&dlv, 1);
  }

  bp_close(state.sap);
	writeErrmsgMemos();
	PUTS("Stopping ccsds_sink.");
	bp_detach();
	return 0;
}
