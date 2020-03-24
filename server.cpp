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
#include <fstream>
#include <sys/ioctl.h>
#include "header.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <map>

char h[12];
char file[512];
char buf[524] = {0};
struct header a;
int CWND = 512;
int SS_THRESH = 10000;
std::string dirName;
std::string destination;
int sockfd;
struct sockaddr_in clientAddr;
struct timeval Timeout;      
int count = 0;
fd_set Write, Err;

//Connection
typedef struct
{
	FILE* fp;
	uint32_t client_seq_num;
	uint32_t server_seq_num;
	bool needs_ack;
	bool is_closing;
} Connection;

std::map<uint16_t, Connection> connections;


void *connection_handler(void *socket_desc)
{
  int sock = *(int*)socket_desc;
  std::ofstream fp;
  memcpy(&a, buf, sizeof(a));
  memcpy(&file, buf + sizeof(a), sizeof(file));
  printf("File: %s\n", file);

  uint16_t conn_id = a.connid;
  auto it = connections.find(conn_id);

  if (it == connections.end())
  {
    if (a.S == 1 && a.F == 0 && a.A == 0)
    {
      a.awknum = a.sequencenum + 1;
      a.A = 1;
      a.connid = ++count;
      a.sequencenum = 4321;
      Connection c = (Connection) {.fp = nullptr, .client_seq_num = a.sequencenum, .server_seq_num = 4321, .needs_ack = true};
      connections[count] = c;
    }
  }
  else
  {
    if (a.S == 0 && a.F == 0 && a.A == 0)
    {
      a.A = 1;
      destination = dirName + std::to_string(count) + ".file";
      int len = strlen(file);
      if(len > 512)
        len = 512;
      fp.open(destination,std::ofstream::app);
      for (int i = 0; i < len; i++)
      {
        fp << file[i];
      }
      fp.close();
      it->second.server_seq_num = a.awknum;
      printf("Seq num: %d\n", a.sequencenum);
      a.awknum = a.sequencenum + len;
      a.sequencenum = 4322;
    }
    else if (a.S == 0 && a.F == 1 && a.A == 0)
    {
      a.awknum = a.sequencenum + 1;
      a.sequencenum = 4322;
      a.A = 1;
    }
  }
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));
  sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &clientAddr, sizeof(clientAddr)); 

  close(sock);
  return 0;
}


int main(int argc, char *args[])
{
  const int MAX = 102400;
  int port;
  int new1;
  int val;
  int bytes_read;
  struct sockaddr_in addr;
  Timeout.tv_sec = 5;
  Timeout.tv_usec = 0;

  std::stringstream p(args[1]);
  p >> port;
  if ((port <= 1023))
  {
    std::cerr << "ERROR: port must be above 1023" << std::endl;
    return 1;
  }

  dirName = args[2];
  dirName = dirName.substr(1) + "/";
  const char * dName = dirName.c_str();
  const int dir_err = mkdir(dName, 0777);

  // create a socket using UDP IP
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  fcntl(sockfd, F_SETFL, O_NONBLOCK);

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
  
  pthread_t thread_id;
  socklen_t clientAddrSize = sizeof(clientAddr);
  memset(buf, '\0', sizeof(buf));

  int maxTime = 10;      
  int curTime = 0;
  
  while((bytes_read = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, ( struct sockaddr *) &clientAddr, &clientAddrSize)))
  {
    if (bytes_read > 0)
    {
      bytes_read = 0;
      curTime = 0;
      if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &new1) < 0)
      {
        puts("Could not create thread");
        pthread_join( thread_id , NULL);
      }
    }
    else
    {
      usleep(1000000);
      curTime++;
      if(curTime == maxTime)
      {
        std::ofstream fp;
        destination = dirName + std::to_string(count) + ".file";
        fp.open(destination);
        fp << "ERROR";
        fp.close();
        curTime = 0;
      }
    }
    
  }
  close(new1);
  return 0;
}

/*
   void *connection_handler(void *socket_desc)
{
  int sock = *(int*)socket_desc;
  std::ofstream fp;
  memcpy(&a, buf, sizeof(a));
  memcpy(&file, buf + sizeof(a), sizeof(file));
  printf("File: %s\n", file);

  uint16_t conn_id = a.connid;
  auto it = connections.find(conn_id);

  if (a.S == 1 && a.F == 0 && a.A == 0)
  {
    a.awknum = a.sequencenum + 1;
    a.A = 1;
    a.connid = ++count;
    a.sequencenum = 4321;
  }
  else if (a.S == 0 && a.F == 0 && a.A == 0)
  {
    a.A = 1;
    destination = dirName + std::to_string(count) + ".file";
    int len = strlen(file);
    int localawknum = a.awknum;
    printf("local awk num: %d\n", localawknum);
    a.awknum = a.sequencenum + len;
    a.sequencenum = localawknum;
    if(len > 512)
      len = 512;
    fp.open(destination,std::ofstream::app);
    for (int i = 0; i < len; i++)
    {
      fp << file[i];
    }
    fp.close();
  }
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));
  sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &clientAddr, sizeof(clientAddr)); 

  close(sock);
  return 0;
}
*/
