//=======================================================================================================================
//ACTIVE FTP SERVER Start-up Code for Assignment 1 (WinSock 2)

//This code gives parts of the answers away but this implementation is only IPv4-compliant. 
//Remember that the assignment requires a full IPv6-compliant FTP server that can communicate with a built-in FTP client either in Windows 10 or Ubuntu Linux.
//Firstly, you must change parts of this program to make it IPv6-compliant (replace all data structures that work only with IPv4).
//This step would require creating a makefile, as the IPv6-compliant functions require data structures that can be found only by linking with the appropritate library files. 
//The sample TCP server codes will help you accomplish this.

//OVERVIEW
//The connection is established by ignoring USER and PASS, but sending the appropriate 3 digit codes back
//only the active FTP mode connection is implemented (watch out for firewall issues - do not block your own FTP server!).

//The ftp LIST command is fully implemented, in a very naive way using redirection and a temporary file.
//The list may have duplications, extra lines etc, don't worry about these. You can fix it as an exercise, 
//but no need to do that for the assignment.
//In order to implement RETR you can use the LIST part as a startup.  RETR carries a filename, 
//so you need to replace the name when opening the file to send.


//=======================================================================================================================
#define _WIN32_WINNT 0x501

#include <stdio.h> 
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define WSVERS MAKEWORD(2,2)
WSADATA wsadata;

//********************************************************************
//MAIN
//********************************************************************
int main(int argc, char *argv[]) {
//********************************************************************
// INITIALIZATION
//********************************************************************
	     int err = WSAStartup(WSVERS, &wsadata);

		 if (err != 0) {
		      WSACleanup();
			  // Tell the user that we could not find a usable WinsockDLL
		      printf("WSAStartup failed with error: %d\n", err);
		      exit(1);
		 }
		 if(LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2){
		 	printf("Couldn't find useable version of the winsock DLL");
		 	WSACleanup();
		 	exit(27);
		 } else {
		 	printf("\n===============================\n");
		 	printf("     159.334 FTP Server");
		 	printf("\n===============================\n");
		 	printf("The TCP server was initiliazed with winsock 2.2\n");
		 }
		 struct sockaddr_storage localaddr,remoteaddr;  
		 struct sockaddr_storage local_data_addr_act;   

		 char clientHost[NI_MAXHOST];
		 char clientService[NI_MAXSERV];
		 char portNum[NI_MAXSERV];

		 SOCKET s,ns;
		 SOCKET ns_data, s_data_act;
		 char send_buffer[200],receive_buffer[200];
		
		 s = INVALID_SOCKET;
         ns_data=INVALID_SOCKET;

		 int active=0;
		 int n,bytes,addrlen;

		 struct addrinfo *result = NULL;
		 struct addrinfo hints;
		 int iResult;

		 memset(&hints,0,sizeof(addrinfo));
		 memset(&localaddr,0,sizeof(localaddr));//clean up the structure
		 memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
		 
//********************************************************************
//SOCKET
//********************************************************************
		 hints.ai_family = AF_INET6;
		 hints.ai_socktype = SOCK_STREAM;
		 hints.ai_protocol = IPPROTO_TCP;
		 hints.ai_flags = AI_PASSIVE;


		 //CONTROL CONNECTION:  port number = content of argv[1]
		 if (argc == 2) { 
		 	 sprintf(portNum, "%s", argv[1]);
			 iResult = getaddrinfo(NULL, portNum, &hints, &result);
			 printf("Connected on Port %s\n", portNum);
		 } else {
		 	 sprintf(portNum, "%s", "1234");
			 iResult = getaddrinfo(NULL, portNum, &hints, &result); // Set the default port
			 printf("Connected on Default Port 1234\n");
		 } 

		 if(iResult != 0) {
		 	printf("getaddrinfo failed: %d\n", iResult);
		 	WSACleanup();
		 	exit(27);
		 }


		 s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		 if (s == INVALID_SOCKET) {
			 printf("socket failed\n");
			 freeaddrinfo(result);
			 WSACleanup();
			 exit(27);
		 }
		 
		
		 
//********************************************************************
//BIND
//********************************************************************
		 iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);

		 if (iResult == SOCKET_ERROR) {
			 printf("Bind failed! %d\n", WSAGetLastError());
			 freeaddrinfo(result);
			 closesocket(s);
			 WSACleanup();
			 exit(0);
		 }

		 freeaddrinfo(result);
		 
//********************************************************************
//LISTEN
//********************************************************************
		 if(listen(s, SOMAXCONN) == SOCKET_ERROR){
		 	printf("Listen failed with error: %d\n", WSAGetLastError());
		 	closesocket(s);
		 	WSACleanup();
		 	exit(1);
		 }
		
//********************************************************************
//INFINITE LOOP
//********************************************************************
		 //====================================================================================
		 while (1) {//Start of MAIN LOOP
		 //====================================================================================
			 addrlen = sizeof(remoteaddr);
//********************************************************************
//NEW SOCKET newsocket = accept  //CONTROL CONNECTION
//********************************************************************
			 printf("\n------------------------------------------------------------------------\n");
			 printf("SERVER is waiting for an incoming connection request...");
			 printf("\n------------------------------------------------------------------------\n");
			 ns = INVALID_SOCKET;
			 ns = accept(s,(struct sockaddr *)(&remoteaddr),&addrlen); 
			 if (ns == INVALID_SOCKET ){
			 	printf("accept failed: %d\n", WSAGetLastError());
			 	closesocket(s);
			 	WSACleanup();
			 	exit(1);
			 } else {
			 	memset(clientHost, 0, sizeof(clientHost));
			 	memset(clientService, 0, sizeof(clientService));

			 	getnameinfo((struct sockaddr *)&remoteaddr, addrlen, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST);

				printf("\n============================================================================\n");
	 		 	printf("connected to [CLIENT's IP %s , port %s] through SERVER's port %s", clientHost, clientService, portNum);     
			 	printf("\n============================================================================\n");
			 }
//********************************************************************
//Respond with welcome message
//*******************************************************************
			 sprintf(send_buffer,"220 FTP Server ready. \r\n");
			 bytes = send(ns, send_buffer, strlen(send_buffer), 0);

			//********************************************************************
			//COMMUNICATION LOOP per CLIENT
			//********************************************************************

			 while (1) {
				 
				 n = 0;
				 //********************************************************************
			     //PROCESS message received from CLIENT
			     //********************************************************************
				 
				 while (1) {
//********************************************************************
//RECEIVE MESSAGE AND THEN FILTER IT
//********************************************************************
				     bytes = recv(ns, &receive_buffer[n], 1, 0);//receive byte by byte...

					 if ((bytes < 0) || (bytes == 0)) break;
					 if (receive_buffer[n] == '\n') { /*end on a LF*/
						 receive_buffer[n] = '\0';
						 break;
					 }
					 if (receive_buffer[n] != '\r') n++; /*Trim CRs*/
				 //=======================================================
				 } //End of PROCESS message received from CLIENT
				 //=======================================================
				 if ((bytes < 0) || (bytes == 0)) break;

				 printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);

//********************************************************************
//PROCESS COMMANDS/REQUEST FROM USER
//********************************************************************				 
				 if (strncmp(receive_buffer,"USER",4)==0)  {
					 printf("Logging in... \n");
					 sprintf(send_buffer,"331 Password required (anything will do really... :-) \r\n");
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"PASS",4)==0)  {
					 
					 sprintf(send_buffer,"230 Public login sucessful \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"SYST",4)==0)  {
					 printf("Information about the system \n");
					 sprintf(send_buffer,"215 Windows 64-bit\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"OPTS",4)==0)  {
					 printf("unrecognised command \n");
					 sprintf(send_buffer,"502 command not implemented\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"QUIT",4)==0)  {
					 printf("Quit \n");
					 sprintf(send_buffer,"221 Connection close by client\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
					 // closesocket(ns);
				 }
				 //---
				 if(strncmp(receive_buffer,"EPRT",4)==0) {
					 s_data_act = socket(AF_INET6, SOCK_STREAM, 0);

					 int port;
					 const char delim[] = "|";
					 char *token;
					 token = strtok(receive_buffer, delim);
					 for(int i = 0; i < 3; i++){
					 	token = strtok(NULL, delim);
					 }
					 sscanf(token, "%d", &port);
					 printf("port is %d\n", port);

					 
					 printf("===================================================\n");
					 printf("\n\tActive FTP mode, the client is listening... \n");
					 active=1;//flag for active connection
					 
					 printf("\tCLIENT's IP is %s\n", clientHost);  
					 printf("\tCLIENT's Port is %d\n",port);
					 printf("===================================================\n");

					 char *portStr;
					 sprintf(portStr, "%d", port);

					 iResult = getaddrinfo(clientHost, portStr, &hints, &result);
					 if(iResult != 0) {
					 	printf("getaddrinfo failed: %d\n", iResult);
					 	WSACleanup();
					 	exit(27);
					 }

					 if (connect(s_data_act, result->ai_addr, result->ai_addrlen) != 0){
						 sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 closesocket(s_data_act);
					 }
					 else {
						 sprintf(send_buffer, "200 EPRT Command successful\r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 printf("Connected to client\n");
					 }

				 }
				 //---				 
				 //technically, LIST is different than NLST,but we make them the same here
				 if ( (strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
					 //system("ls > tmp.txt");//change that to 'dir', so windows can understand
					 // system("dir > tmp.txt");
					 // FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
					 // //sprintf(send_buffer,"125 Transfering... \r\n");
					 // sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
					 // printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 // bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 // char temp_buffer[80];
					 // while (!feof(fin)){
						//  fgets(temp_buffer,78,fin);
						//  sprintf(send_buffer,"%s",temp_buffer);
						//  if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
						//  else send(s_data_act, send_buffer, strlen(send_buffer), 0);
					 // }
					 // fclose(fin);
					 // //sprintf(send_buffer,"250 File transfer completed... \r\n");
					 // sprintf(send_buffer,"226 File transfer complete. \r\n");
					 // printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 // bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 // if (active==0 )closesocket(ns_data);
					 // else closesocket(s_data_act);
						 
					 
					 //OPTIONAL, delete the temporary file
					 //system("del tmp.txt");
				 }
                 //---			    
			 //=================================================================================	 
			 }//End of COMMUNICATION LOOP per CLIENT
			 //=================================================================================
			 
//********************************************************************
//CLOSE SOCKET
//********************************************************************
			 closesocket(ns);
			 printf("DISCONNECTED from %s\n",clientService);
			 //sprintf(send_buffer, "221 Bye bye, server close the connection ... \r\n");
			 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 //bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			 
		 //====================================================================================
		 } //End of MAIN LOOP
		 //====================================================================================
		 closesocket(s);
		 WSACleanup();
		 printf("\nSERVER SHUTTING DOWN...\n");
		 exit(0);
}

