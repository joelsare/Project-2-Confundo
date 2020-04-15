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
#include <fstream>
#include "queue"
#include <sys/time.h>

using namespace std; 

int CWND;
int SS_THRESH;

struct Connection {

	FILE* fd;
	uint16_t id;
	uint32_t client_seq_num;
	uint32_t server_seq_num;
	uint32_t cwnd;
	uint32_t ssthresh;
}c;

//updates CWND and SS_THRESH
void congestion_mode() {
	if (CWND < SS_THRESH) {
		// slow start
	  CWND += 512;
	}
	else {
		// congestion avoidance
		CWND += (512 * 512) / CWND;
	}
}

//prints struct header.h in format specified in assignment
void printHeader(string command ,header a, int C, int SS, int needsDup)
{
  printf("%s %d %d %d %d %d ", command.c_str(), a.sequencenum, a.awknum, a.connid, C, SS);
  if(a.A == 1) printf("ACK ");
  if(a.S == 1) printf("SYN ");
  if(a.F == 1) printf("FIN ");
  if(needsDup == 1) printf("DUP");
  printf("\n");
}

int main(int argc, char *args[])
{
  int bytes_read;
  const int maxTime = 20;       //max time before client aborts connection (represents 10 seconds)
  int curTime = 0;              //represents how long client has been waiting with no response from server
  std::queue<header> c_window;  //holds header.h objects
  std::queue<char*> words;      //holds "words" (words sent to server)
  int recv_bytes;
  const int MAX = 102400;       //max sequence number allowed
  Connection c;
  CWND = 512;
  SS_THRESH = 10000;
  struct header a;              //packet header
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 500000;     //ack timeout

  char * hostName = args[1];

  stringstream p(args[2]);
  int port;
  p >> port;
  
  //validate port
  if ((port <= 1023))
  {
    std::cerr << "ERROR: invalid arguments" << std::endl;
    return 1;
  }

  //validate hostname
  if (gethostbyname(hostName) == NULL)
  {
    std::cerr << "ERROR: invalid arguments" << std::endl;
    return 1;
  }

  //read filename from command line, copy into char array
  char * fileName = args[3];
  char file[512] = "";
  strcpy(file, fileName);

  //open file
  c.fd = fopen(args[3], "r");

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

  //i dont know what any of this is
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

  //set ack timeout (0.5 seconds)
  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
  {
    cerr << "Error";
    return 1;
  }

  //i dont know what this is
  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));

  //set up header 'a'
  //get syn packet ready
  a.sequencenum = 12345;
  a.awknum = 0;
  a.connid = 0;
  a.notused = 0;
  a.A = 0;
  a.S = 1;
  a.F = 0;

  // send/receive data to/from connection
  char buf[524] = {0};

  unsigned int len;

  //clear buffer
  memset(buf, '\0', sizeof(buf));

  //copy syn packet to buffer
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));

  //send syn packet to server
  sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
  printHeader("SEND", a, CWND, SS_THRESH, 0);

  //wait for response
  while ((bytes_read = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, &len)))
  {
    if (bytes_read > 0)
    {
      bytes_read = 0;
      memcpy(&a, buf, sizeof(a));
      printHeader("RECV", a, CWND, SS_THRESH, 0);
      break;
    }
    else
    {
      curTime++;
      if(curTime == maxTime) //timeout after 10 seconds
      {
        close(sockfd);
        exit(1);
      }
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) //resend if timeout occurs
    {
      sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
      printHeader("SEND", a, CWND, SS_THRESH, 1);
    }
  }
  //get ack packet ready
  int sequencenum;
  sequencenum = a.sequencenum;
  a.sequencenum = a.awknum;
  a.awknum = sequencenum + 1;
  a.S = 0;

  //copy ack packet to buffer
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));  

  //send ack packet to server
  sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
  printHeader("SEND", a, CWND, SS_THRESH, 0);

  //clear buffer
  memset(file, '\0', sizeof(file));

  //getting packet ready to send file to server
  a.A = 0;
  a.awknum = 0;
  curTime = 0;
  int c_win_size;
  bool sending_data = true;
  timeval receive_time;

  while(1)
  {
    c_win_size = 0;

    // utilize the entire congestion window
    while(c_win_size < CWND)
    {
      int read_bytes = fread(file, sizeof(char), 512, c.fd);
      if (read_bytes < 0) {
        perror("reading payload file");
      }
      if (read_bytes == 0) {
        sending_data = false;
        break; // No payload to send
      }
      if (read_bytes > 0)
      {
        //get packet ready
        a.awknum = 0;
        a.A = 0;
        a.F = 0;
        a.S = 0;
        c_window.push(a); //pushing header
        words.push(file); //pushing words from file

        //copy header and words to buffer
        memcpy(buf, &a, sizeof(a));
        memcpy(buf + sizeof(a), file, sizeof(file));  

        //IMPORTANT to clear file after copying to buffer
        memset(file, '\0', sizeof(file));
        c_win_size += read_bytes;

        //send buffer to server
        printHeader("SEND", a, CWND, SS_THRESH, 0);
        if (sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
        {
          perror("sending payload packet to server");
        }

        //increase sequencenum
        a.sequencenum += read_bytes;
        a.sequencenum %= (MAX+1);
      }
    }
    curTime = 0;
    // Wait for ACK
    while(!c_window.empty())
    {
      do { // Check for ACK received, retransmit
        recv_bytes = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, &len);
        if (recv_bytes > 0)
        {
          gettimeofday(&receive_time, NULL);
          break;
        }
        else
        {
          curTime++;
          if(curTime == maxTime) //timeout after 10 seconds
          {
            close(sockfd);
            exit(1);
          }
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) //resend if timeout occurs
        {
          // change SS_THRESH and CWND after timeout
          SS_THRESH = CWND / 2;
          CWND = 512;

          //resend buffer
          sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
          printHeader("SEND", a, CWND, SS_THRESH, 1);
        }
      } while(errno == EAGAIN || errno == EWOULDBLOCK);
      
      //copy data sent from server
      memcpy(&a, buf, sizeof(a));

      if (a.A == 1) //recieved ack from server
      {
        recv_bytes = 0;
        c_window.pop();
        words.pop();
        printHeader("RECV", a, CWND, SS_THRESH, 0);
        a.sequencenum = a.awknum;

        //update CWND & SS_THRESH
        congestion_mode();
      }
      else //wrong header format
     {
        //copy header and words to buffer
        memcpy(buf, &c_window.front(), sizeof(a));
        memcpy(buf + sizeof(a), words.front(), sizeof(file));

        //send buffer to server
        sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
        printHeader("SEND", c_window.front(), CWND, SS_THRESH, 1);
      }
      c.server_seq_num = a.sequencenum;
      c.client_seq_num %= (MAX+1);
  }
  if (!sending_data)
  {
    break;
  }
}
  //clear buffer
  memset(buf, '\0', sizeof(buf));

  //getting fin packet ready
  a.awknum = 0;
  a.F = 1;
  a.A = 0;

  //copy fin packet to buffer
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));  

  //send fin packet to server
  sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
  printHeader("SEND", a, CWND, SS_THRESH, 0);
  curTime = 0;

  //recieve ack fin from server
  while (1)
  {
    int bytes_read = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, &len); 
    if (bytes_read > 0)
    {
      bytes_read = 0;
      memcpy(&a, buf, sizeof(a));
      a.F = 1;
      printHeader("RECV", a, CWND, SS_THRESH, 0);
      break;
    }
    else
    {
      curTime++;
      if(curTime == 4) //respond with ack for incoming fins for 2 seconds
      {
        close(sockfd);
        exit(1);
      }
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) //resend if timeout occurs
    {
      sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
      printHeader("SEND", a, CWND, SS_THRESH, 1);
    }
  }

  //getting ack packet ready
  curTime = 0;
  a.F = 0;
  sequencenum = a.sequencenum;
  a.sequencenum = a.awknum;
  a.sequencenum %= (MAX+1);
  a.awknum = sequencenum + 1;

  //copy ack packet to buffer
  memcpy(buf, &a, sizeof(a));
  memcpy(buf + sizeof(a), file, sizeof(file));  

  //send ack packet to server, dont check for response because file is successfully sent
  sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); 
  printHeader("SEND", a, CWND, SS_THRESH, 0);

  //done sending file so close FILE pointer and socket
  fclose (c.fd);
  close(sockfd);
  return 0;
}