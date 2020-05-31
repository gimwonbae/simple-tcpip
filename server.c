/* 
   A simple server in the internet domain using TCP
   Usage:./server port (E.g. ./server 10000 )
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd; //descriptors rturn from socket and accept system calls
  int portno; // port number
  socklen_t clilen;
  
  char buffer[1024];
  char response[1024]; //response 메세지

  /*sockaddr_in: Structure Containing an Internet Address*/
  struct sockaddr_in serv_addr, cli_addr;
  
  int n;
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  
  /*Create a new socket
    AF_INET: Address Domain is Internet 
    SOCK_STREAM: Socket Type is STREAM Socket */
  sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) 
    error("ERROR opening socket");
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]); //atoi converts from String to Integer
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //for the server the IP address is always the address that the server is running on
  serv_addr.sin_port = htons(portno); //convert from host to network byte order
  
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
    error("ERROR on binding");
  
  listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5
  
  clilen = sizeof(cli_addr);
  /*accept function: 
    1) Block until a new connection is established
    2) the new socket descriptor will be used for subsequent communication with the newly connected client.
  */
  while(1){
    bzero(response, 1024); //response message를 초기화
    
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
      error("ERROR on accept");
    
    bzero(buffer,1024); // request message 초기화

    n = read(newsockfd,buffer,1023); //Read is a block function. It will read at most 1023 bytes
    if (n < 0) error("ERROR reading from socket");
      printf("Request Message:\n%s\n",buffer); //읽어온 request message를 서버에 출력
    
    char *method = strtok(buffer,"\r\n"); 
    strtok(method, " ");  //request message에서 method token을 parsing
    char *fileName = strtok(NULL," /"); //request message에서 file name을 parsing
    char *httpV = strtok(NULL," "); //request message에서 http version을 parsing
    char *statusCode; 
    char *reason;
    int fileSize;
    char *type;

    FILE *fp = fopen(fileName, "rb"); //fileNmae 에 해당하는 file을 open (성공시: file 구조체 포인터, 실패시 : NULL)

    if(fp == NULL){ //file이 없을경우
      fclose(fp);
      statusCode = "404";
      reason = "Not Found";  
      sprintf(response, "%s %s %s\r\n\r\n" , httpV, statusCode, reason); //response에 HTTP-Version SP Status-Code SP Reason-Phrase CRLF로 이루어진 status-line를 담는다. 
      n = write(newsockfd, response, strlen(response)); //client에 response를 보낸다.
      if (n < 0) error("ERROR writing to socket");
      close(newsockfd); //client와 연결된 sockfd를 닫고
      continue; // 루프문의 처음으로 돌아간다.
    }
    else{ //file이 있을 경우, fileName에 따른 response message의 content-type을 결정해준다.
      if(strstr(fileName,"html") != NULL){
        type = "text/html";
      }
      else if(strstr(fileName,"gif") != NULL){
        type = "image/gif";
      }
      else if(strstr(fileName,"jpeg") != NULL){
        type = "image/jpeg";
      }
      else if(strstr(fileName,"mp3") != NULL){
        type = "audio/mp3";
      }
      else if(strstr(fileName,"pdf") != NULL){
        type = "application/pdf";
      }
      else { //html, gif, jpeg, mp3, pdf외의 file일 경우, not accptable의 status-line을 response로 보낸다.
        fclose(fp);
        statusCode = "406";
        reason = "Not Acceptable";
        sprintf(response, "%s %s %s\r\n" , httpV, statusCode, reason);
        n = write(newsockfd, response, strlen(response));
        if (n < 0) error("ERROR writing to socket");
        close(newsockfd);
        continue; //루프의 처음으로 돌아가준다.
      }
      //file이 있고, html,gif,jpeg,mp3,pdf에 해당할 경우
      statusCode = "200";
      reason = "OK"; //202 OK

      fseek(fp, 0, SEEK_END); //file 포인터를 파일의 끝으로 옮겨  
      fileSize = ftell(fp);  //size를 알아내고,
      fseek(fp, 0, SEEK_SET); //다시 포인터를 처음으로 돌려준다.
      
      char *file = (char *)malloc(sizeof(char) * fileSize); 
      bzero(file, fileSize);
    
      fread(file, sizeof(char), fileSize, fp); //데이터를 읽어 file 변수에 저장한다.
      fclose(fp);

      sprintf(response, "%s %s %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n" , httpV, statusCode, reason,
      fileSize, type); //status-line, contetnt-length header,content-type header로 이루어진 response 메세지를 만든다.
      
      n = write(newsockfd, response, strlen(response)); //response를 클라이언트에 보낸다.
      if (n < 0) error("ERROR writing to socket");
      n = write(newsockfd, file, fileSize); //file 버퍼를 클라이언트에 보낸다.
      if (n < 0) error("ERROR writing to socket");
      free(file); 
      close(newsockfd);
    }
  }
  close(sockfd);
  
  return 0; 
}
