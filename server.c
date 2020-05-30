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
  char response[1024];

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
    bzero(response, 1024);
    
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
      error("ERROR on accept");
    
    bzero(buffer,1024);
    n = read(newsockfd,buffer,1023); //Read is a block function. It will read at most 255 bytes
    if (n < 0) error("ERROR reading from socket");
      printf("Request Message:\n%s\n",buffer);
    
    char *method = strtok(buffer,"\r\n");
    strtok(method, " ");
    char *fileName = strtok(NULL," /");
    char *httpV = strtok(NULL," ");
    char *statusCode;
    char *reason;
    int fileSize;
    char *type;

    printf("method = %s\n", method);
    printf("fileName = %s\n", fileName);
    printf("httpV = %s\n", httpV);

    FILE *fp = fopen(fileName, "rb");

    if(fp == NULL){
      fclose(fp);
      statusCode = "404";
      reason = "Not Found";
      sprintf(response, "%s %s %s\r\n" , httpV, statusCode, reason);
      printf("response : %s\n", response);
      n = write(newsockfd, response, strlen(response));
      if (n < 0) error("ERROR writing to socket");
      continue;
    }
    else{
      printf("/n/n111111");      
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
      else {
        fclose(fp);
        statusCode = "406";
        reason = "Not Acceptable";
        sprintf(response, "%s %s %s\r\n" , httpV, statusCode, reason);
        printf("response : %s\n", response);
        n = write(newsockfd, response, strlen(response));
        if (n < 0) error("ERROR writing to socket");
        continue;
      }
      statusCode = "200";
      reason = "OK";

      fseek(fp, 0, SEEK_END);   
      fileSize = ftell(fp);  
      fseek(fp, 0, SEEK_SET);
      
      char *file = (char *)malloc(sizeof(char) * fileSize); // content에 메모리 할당
      bzero(file, fileSize);
    /*
    fread()함수는 입력 file에서 sizeof(char) 길이의 file_size항목까지 읽고,
    지정된 content(buffer)에 저장합니다.
    */
      fread(file, sizeof(char), fileSize, fp);
      fclose(fp);

      sprintf(response, "%s %s %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n" , httpV, statusCode, reason,
      fileSize, type);
      
      printf("response : %s", response);
      printf("file : %s", file);
      n = write(newsockfd, response, strlen(response));
      if (n < 0) error("ERROR writing to socket");
      n = write(newsockfd, file, fileSize);
      if (n < 0) error("ERROR writing to socket");
      free(file); // malloc으로 할당된 기억장치를 해제시킴.
    }
  }

  close(sockfd);
  close(newsockfd);
  
  return 0; 
}
