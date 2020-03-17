#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "header.h"

using namespace std; 

int main(int argc, char *args[])
{
  int CWND = 512;
  int SS_THRESH = 10000;
  struct header a;
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  char * hostName = args[1];

  stringstream p(args[2]);
  int port;
  p >> port;

  if ((port <= 1023))
  {
    std::cerr << "ERROR: invalid arguments" << std::endl;
    return 1;
  }

  if (gethostbyname(hostName) == NULL)
  {
    std::cerr << "ERROR: invalid arguments" << std::endl;
    return 1;
  }

  char * fileName = args[3];
  char file[512] = "";
  strcpy(file, fileName);

  FILE * fp;
  fp = fopen(file, "r");

  // create a socket using UDP IP
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // struct sockaddr_in addr;
  // addr.sin_family = AF_INET;
  // addr.sin_port = htons(40001);     // short, network byte order
  // addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
  // if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
  //   perror("bind");
  //   return 1;
  // }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr(hostName);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1)
  {
    perror("getsockname");
    return 3;
  }

  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
  {
    cerr << "Error";
    return 1;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));

  //set up header
  a.sequencenum = 12345;
  a.awknum = 0;
  a.connid = 0;
  a.notused = 0;
  a.A = 0;
  a.S = 1;
  a.F = 0;

  // send/receive data to/from connection
  char buf[524] = {0};

  unsigned int n, len;

  memset(buf, '\0', sizeof(buf));

  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));
  sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
  printf("SEND %d %d %d %d %d ", a.sequencenum, a.awknum, a.connid, CWND, SS_THRESH);
      if(a.A == 1) printf("ACK ");
      if(a.S == 1) printf("SYN ");
      if(a.F == 1) printf("FIN");
      printf("\n");
      

  while (1)
  {
    int bytes_read = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, (struct sockaddr *) &serverAddr, &len); 
    if (bytes_read > 0)
    {
      memcpy(&a, buf, sizeof(a));
      printf("RECV %d %d %d %d %d ", a.sequencenum, a.awknum, a.connid, CWND, SS_THRESH);
      if(a.A == 1) printf("ACK ");
      if(a.S == 1) printf("SYN ");
      if(a.F == 1) printf("FIN");
      printf("\n");
      break;
    }
  }
  int sequencenum = a.sequencenum;
  a.sequencenum = a.awknum;
  a.awknum = sequencenum + 1;
  a.S = 0;

  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));  
  sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)); 

  printf("SEND %d %d %d %d %d ", a.sequencenum, a.awknum, a.connid, CWND, SS_THRESH);
  if(a.A == 1) printf("ACK ");
  if(a.S == 1) printf("SYN ");
  if(a.F == 1) printf("FIN");
  printf("\n");

  if (strlen(file) > 0)
  {
    printf("%lu\n", strlen(file));
  }
  
  close(sockfd);
  return 0;
}
