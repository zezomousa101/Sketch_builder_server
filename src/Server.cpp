
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include "server/Server.h"
#include "server/ServerException.h"
#include "server/RequestHandler.h"
#include "server/Command.h"

std::string Server::serverPort;
int Server::sockfd, Server::newsockfd, Server::portno;
socklen_t Server::clilen;
char Server::buffer[256];
struct sockaddr_in Server::serv_addr, Server::cli_addr;
int Server::n;
bool Server::listening = true;

void Server::setPort(std::string serverPort){
    Server::serverPort = serverPort;
}
std::string Server::getPort(){
    return Server::serverPort;
}

void Server::connect(){
    Server::sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (Server::sockfd < 0) 
       throw ServerException("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    Server::portno = atoi(Server::serverPort.c_str());
    Server::serv_addr.sin_family = AF_INET;
    Server::serv_addr.sin_addr.s_addr = INADDR_ANY;
    Server::serv_addr.sin_port = htons(portno);

    while (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr<<("Searching on empty port ...")<<std::endl;
        // sleep(3);
        Server::serv_addr.sin_port = htons(++portno);
    }
    std::cout << "System is ready port "<< portno <<" :" <<std::endl;    
}

void Server::start(){
    listen(sockfd, 10);
    while(listening){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) 
            throw ServerException("ERROR on accept");

        // bzero(buffer,256);
        Server::n = recv(newsockfd,buffer,255, 0);

        if (n < 0) 
            throw ServerException("ERROR reading from socket");
        printf("Recieved Command: %s\n",buffer);

        //there was n- 1 here
        int response = handlRequest(std::string(buffer));
        
        std::string sres = "200";
        if(response > 0)
            sres = "400";
        else if (response < 0)
            sres = "404";
        Server::sendMessageToClient(std::string(sres));
        if (Server::n < 0) 
            throw ServerException("ERROR writing to socket");
        close(Server::newsockfd);
    }
    
    shutdown(Server::newsockfd, SHUT_RDWR);
    close(Server::sockfd);
    std::cout << "Socket is closed!" << std::endl;
}

void Server::disconnect(){
    Server::listening = false;
}

void Server::sendMessageToClient(std::string message){
    Server::n = send(newsockfd, message.c_str(),18, 0);
}

int Server::handlRequest(std::string request){
    RequestHandler *requestHandler = new RequestHandler();
    requestHandler->setRequest(request);
    Command *command = requestHandler->getCommand();
    if(command == NULL)
        return -1;
    std::cerr << "command not null"<<std::endl;
    int res = command->doCommand();
    
    delete command;
    delete requestHandler;
    return res;
}