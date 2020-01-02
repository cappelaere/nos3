#!/usr/bin/env python
#
# Proxy to receive CCSDS packets from UDP
# Packets could be CMDS or TLM depending on port being used
#
# Pat Cappelaere, ASRC Federal
#
import sys
import os
import socket
import binascii

from time import sleep

# Receive port where the CFS TO_Lab app sends the telemetry packets
udpTLMPort = 1235

# Receive port where the CFS CI_Lab app receives the cmd packets
udpCMDPort = 1234

def receiveTLM(udpPort):
  print "Starting UDP Proxy at port:", udpPort

  # Init udp socket
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  sock.bind(('', udpPort))

  # Wait for UDP messages
  while True:
    try:
      # Receive message
      datagram, host = sock.recvfrom(4096) # buffer size is 1024 bytes

      # Ignore datagram if it is not long enough (doesnt contain tlm header?)
      if len(datagram) < 6:
        continue

      # Read host address
      hostIpAddress = host[0]
      print "got dgram from:", hostIpAddress, 'len:', len(datagram)
      print binascii.hexlify(datagram)

    except:
      print 'Ignore socket error'
      sleep(1)

if __name__ == '__main__':
  udpPort = sys.argv[1]
  receiveTLM(udpPort)
