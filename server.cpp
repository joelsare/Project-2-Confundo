#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sstream> 
#include <iostream>
#include "header.h"

int main(int argc, char *args[])
{
  const int MAX = 102400;
  int CWND = 512;
  int SS_THRESH = 10000;
  int counter = 0;
  int port;
  int new1;
  int val;
  int bytes_read;
  char buf[524] = {0};
  char h[12];
  char file[512];
  struct sockaddr_in addr;

  std::stringstream p(args[1]);
  p >> port;
  if ((port <= 1023))
  {
    std::cerr << "ERROR: port must be above 1023" << std::endl;
    return 1;
  }

  // create a socket using UDP IP
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);     // short, network byte order
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
  {
    perror("bind");
    return 2;
  }

  while(1)
  {
    struct header a;

    memset(buf, '\0', sizeof(buf));      
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

    //new1 = accept(sockfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
    //char ipstr[INET_ADDRSTRLEN] = {'\0'};
    //inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
    bytes_read = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, ( struct sockaddr *) &clientAddr, &clientAddrSize); 
    if (bytes_read > 0)
    {
      val = fork();
      if (val == -1)
      {
        close(new1);
        continue;
      }
      else if (val > 0)
      {
        close(new1);
        counter++;
        continue;
      }
      else if (val == 0)
      {
        memcpy(&a, buf, sizeof(a));
        memcpy(file, buf + sizeof(h), sizeof(file));
        
        if (a.S == 1 && a.F == 0 && a.A == 0)
        {
          a.awknum = a.sequencenum + 1;
          a.A = 1;
          a.connid = counter;
          a.sequencenum = 4321;
        }
        else
        {
          
        }
        memcpy(buf, &a, sizeof(a));
        memcpy(buf + sizeof(a), file, sizeof(file));

        sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &clientAddr, clientAddrSize); 

        close(new1);
        counter++;
        break;
      }
    }
  }
  close(new1);
  return 0;
}
