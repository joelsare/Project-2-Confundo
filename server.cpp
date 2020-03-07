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

int main(int argc, char *args[])
{
  int counter = 0;
  int port;
  int new1;
  int val;
  int bytes_read;
  char buf[20] = {0};
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
        printf("Client #%d : %s\n", counter, buf); 
        sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &clientAddr, clientAddrSize); 

        counter++;
        close(new1);
        break;
      }
    }
  }
  close(new1);
  return 0;
}
