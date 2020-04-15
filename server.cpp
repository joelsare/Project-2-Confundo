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

int CWND = 512;               //assignment requires server to have correct
int SS_THRESH = 10000;        //initial values for CWND, SS_THRESH and
int sequencenumber = 12345;   //sequence numbebr

std::string dirName;          //directory name
std::string destination;      //represents [dirname][connid][.file]
int sockfd;
struct sockaddr_in clientAddr;
struct timeval Timeout;      
int count = 0;                //responsible for setting connid correctly

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

//handles connections from clients
void *connection_handler(void *socket_desc)
{
  int sock = *(int*)socket_desc;
  std::ofstream fp;
  memcpy(&a, buf, sizeof(a));
  memcpy(&file, buf + sizeof(a), sizeof(file));

  uint16_t conn_id = a.connid;
  auto it = connections.find(conn_id);

  if (it == connections.end()) //new connection
  {
    if (a.S == 1 && a.F == 0 && a.A == 0) //syn flag set
    {
      a.awknum = a.sequencenum + 1;
      a.A = 1;
      a.connid = ++count;
			Connection c = (Connection) {.fp = nullptr, .client_seq_num = a.sequencenum+1, .server_seq_num = 4321, .needs_ack = true};
      a.sequencenum = 4321;
      connections[count] = c;
    }
  }
  else //known connection
  {
    if (a.S == 0 && a.F == 0 && a.A == 0) //no flags set (save to file)
    {
      a.A = 1;
      destination = dirName + std::to_string(count) + ".file";
      int len = strlen(file);
      if(len > 512) //this is for a weird bug, it was somehow writing more than 512 bytes
        len = 512;
      fp.open(destination,std::ofstream::app); //append everything to the end of the file
      for (int i = 0; i < len; i++) //i couldn't do fp << file because again it would somehow write more than 512 bytes and output would be messed up
      {
        fp << file[i];
      }
      fp.close();
      a.awknum = a.sequencenum + len;
      a.sequencenum = it->second.server_seq_num + 1;
    }
    else if (a.S == 0 && a.F == 1 && a.A == 0) //fin flag set (done saving file)
    {
      a.awknum = a.sequencenum + 1;
      a.sequencenum = it->second.server_seq_num + 1;
      a.A = 1;
    }
  }

  //copy header and file to buffer
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));

  //send buffer to client
  sendto(sockfd, buf, sizeof(buf), MSG_SEND, (const struct sockaddr *) &clientAddr, sizeof(clientAddr)); 

  close(sock);
  return 0;
}


int main(int argc, char *args[])
{
  const int MAX = 102400; //max sequencenum value allowed
  int port;
  int new1;
  int val;
  int bytes_read;
  struct sockaddr_in addr;
  Timeout.tv_sec = 5;
  Timeout.tv_usec = 0;

  //check port value
  std::stringstream p(args[1]);
  p >> port;
  if ((port <= 1023))
  {
    std::cerr << "ERROR: port must be above 1023" << std::endl;
    return 1;
  }

  //set up directory name
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

  int maxTime = 5000;      
  int curTime = 0;
  
  //waits for connection from client and creates new thread
  while((bytes_read = recvfrom(sockfd, buf, sizeof(buf), MSG_DONTWAIT, ( struct sockaddr *) &clientAddr, &clientAddrSize)))
  {
    if (bytes_read > 0)
    {
      bytes_read = 0;
      curTime = 0;
      if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &new1) == 0)
      {
        //puts("Could not create thread");
        pthread_join( thread_id , NULL);
      }
    }
    else
    {
      usleep(2000);
      curTime++;
      if(curTime == maxTime) //if client timeouts after 10 seconds, overwrite file with "ERROR"
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

