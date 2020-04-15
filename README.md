## Joel Sare
## 35125075

##header.h
I used bit fields for header.h to ensure each field was as specified in the assignment description. Each header that goes with each packet is 12 bytes. They are unsigned ints because with this project, there's no way any of the values can be negative.

##server.cpp
The server handles connections with p_threads. This is very different than what I did last project where I used fork() but was later told I should not do this. Learning p_threads was a little challenging but I got through it. The high level design of server works as: wait for connection, if it has synced flag set, create new connid for it and give it a sequence number of 4321. If it has no flags set, append payload to destination file. If fin flag is set, simply increase awknum and sequencenum by 1 and send back to client. If client times out after 10 seconds, overwrite file with "ERROR". I measured this timeout by using usleep().

##client.cpp
The client uses a header struct named 'a' for every time it sends data to server. Every time the client sends something to server, it checks the errno to see if it should retransmit (to ensure reliable delivery). Also, every time client is waiting for a response and 10 seconds go by with nothing, it aborts the connection. Because all recvfrom() have 0.5s timeout, I increment the "timer" by 1 each while loop iteration and when it gets to 20 (which means 10 seconds has passed) it aborts the connection.

When sending the file in chunks, I used the sample code provided on GitHub. I don't know how else I would do this, so thank you very much for providing this sample for us :).

After the file is successfully sent, it sends a FIN packet to server and waits for ACK FIN from server. If no ACK FIN is sent from server for 2 seconds the client closes the connection (I asked about this on Piazza, question@114). And after the client receives the ACK FIN packet, it sends an ACK packet and closes the connection and file pointer.

##Problems & solutions
The biggest problem I ran into was finding out I cannot use fork() with this project midway through completing it. I solved this problem by reading about p_thread online and implementing it for my server.

##Additional libraries
<sys/types.h> <sys/socket.h> <netinet/in.h> <arpa/inet.h> <string.h> <stdio.h> <errno.h> <unistd.h> <iostream> <sstream> <netdb.h> <sys/types.h> <sys/socket.h>  <arpa/inet.h>  <netinet/in.h> "header.h" <fstream> "queue" <sys/time.h>

##Additional online tutorials & code examples
Threads:
https://www.tutorialspoint.com/cplusplus/cpp_multithreading.htm
https://stackoverflow.com/questions/21405204/multithread-server-client-implementation-in-c
https://gist.github.com/oleksiiBobko/43d33b3c25c03bcc9b2b

Files:
http://www.cplusplus.com/doc/tutorial/files/