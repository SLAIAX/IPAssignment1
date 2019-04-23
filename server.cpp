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

#define IPV6 true


	#include <stdio.h> 
	#include <unistd.h>
	#include <string.h>
	#include <stdlib.h>
	#include <iostream>
#if defined __unix__ || defined __APPLE__
  	#include <errno.h>
  	#include <sys/types.h>
  	#include <sys/socket.h>
  	#include <arpa/inet.h>
  	#include <netdb.h> //used by getnameinfo()
#elif defined _WIN32
	#define _WIN32_WINNT 0x501
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define WSVERS MAKEWORD(2,2)

	WSADATA wsadata;
#endif

//********************************************************************
//MAIN
//********************************************************************
int main(int argc, char *argv[]) {
//********************************************************************
// INITIALIZATION
//********************************************************************
	#if defined _WIN32
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
	#elif defined __unix__ || defined __APPLE__
		 printf("\n===============================\n");
		 printf("     159.334 FTP Server");
		 printf("\n===============================\n");
		 printf("The TCP server was initiliazed\n");
	#endif
 		 struct sockaddr_storage localaddr,remoteaddr;    

		 char clientHost[NI_MAXHOST];
		 char clientService[NI_MAXSERV];
		 char portNum[NI_MAXSERV];

		 bool isAuth = false;

		 char serverRoot[100];
		 getcwd(serverRoot, sizeof(serverRoot));

		 char username[80];
		 char password[80];

	#if defined __unix__ || defined __APPLE__
  		 int s, ns;
  		 int ns_data, s_data_act;
  	#elif defined _WIN32
		 SOCKET s, ns;
		 SOCKET ns_data, s_data_act;
	#endif

		 char send_buffer[200],receive_buffer[200];

		 char mode = 'I';
		
         ns_data=-1;

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
		 if(IPV6){	
		 	hints.ai_family = AF_INET6;
		 } else {
		 	hints.ai_family = AF_INET;
		 }
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
		 	#if defined _WIN32
			    WSACleanup();
			#endif    
			    return 1;
		 }	
		

		 #if defined __unix__ || defined __APPLE__
		  	s = -1;
		 #elif defined _WIN32
		  	s = INVALID_SOCKET;
		 #endif

		 s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		#if defined __unix__ || defined __APPLE__
		  	if (s < 0) {
		      printf("socket failed\n");
		      freeaddrinfo(result);
		  	}
		#elif defined _WIN32
		 	if (s == INVALID_SOCKET) {
				printf("socket failed\n");
				freeaddrinfo(result);
				WSACleanup();
				exit(27);
		 	}
		#endif 
		 
		
		 
//********************************************************************
//BIND
//********************************************************************
		 iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);

		#if defined __unix__ || defined __APPLE__
		 if (iResult == -1) {
		      printf( "\nbind failed\n"); 
		      freeaddrinfo(result);
		      close(s);//close socket
		#elif defined _WIN32
		 if (iResult == SOCKET_ERROR) {
			 printf("Bind failed! %d\n", WSAGetLastError());
			 freeaddrinfo(result);
			 closesocket(s);
			 WSACleanup();
		#endif       
	      return 1;
	    }

		freeaddrinfo(result);
		 
//********************************************************************
//LISTEN
//********************************************************************
		#if defined __unix__ || defined __APPLE__
		    if (listen( s, 5) == -1) {
		#elif defined _WIN32
			if(listen(s, SOMAXCONN) == SOCKET_ERROR){
		#endif

		#if defined __unix__ || defined __APPLE__
		      printf( "\nListen failed\n"); 
		      close(s); 
		#elif defined _WIN32	 	
			  printf("Listen failed with error: %d\n", WSAGetLastError());
			  closesocket(s);
			  WSACleanup();
		#endif   

      		  exit(1);

		    } else {
				printf("\n<<<SERVER>>> is listening at PORT: %s\n", portNum);
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

		#if defined __unix__ || defined __APPLE__
		       ns = -1;
		#elif defined _WIN32
		       ns = INVALID_SOCKET;
		#endif

		#if defined __unix__ || defined __APPLE__     
		      ns = accept(s,(struct sockaddr *)(&remoteaddr),(socklen_t*)&addrlen); //IPV4 & IPV6-compliant
		#elif defined _WIN32 	 
			  ns = accept(s,(struct sockaddr *)(&remoteaddr),&addrlen); 
		#endif

		#if defined __unix__ || defined __APPLE__
			 if (ns == -1) {
			     printf("\naccept failed\n");
			     close(s);
			     return 1;
			 }
		#elif defined _WIN32	 	 
			 if (ns == INVALID_SOCKET ){
			 	printf("accept failed: %d\n", WSAGetLastError());
			 	closesocket(s);
			 	WSACleanup();
			 	exit(1);
			 }
		#endif	  
			 else {
			 	memset(clientHost, 0, sizeof(clientHost));
			 	memset(clientService, 0, sizeof(clientService));

			 	getnameinfo((struct sockaddr *)&remoteaddr, addrlen, clientHost, sizeof(clientHost), clientService, sizeof(clientService), NI_NUMERICHOST);

				printf("\n============================================================================\n");
	 		 	printf("connected to [CLIENT's IP %s , port %s] through SERVER's port %s", clientHost, clientService, portNum);     
			 	printf("\n============================================================================\n");

			 	chdir(serverRoot); // Resets the working directory for a new connection to the server root
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

				 //printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);

//********************************************************************
//PROCESS COMMANDS/REQUEST FROM USER
//********************************************************************				 
				 if (strncmp(receive_buffer,"USER",4)==0)  {
					 printf("Logging in... \n");
					 // get username
					 sscanf(receive_buffer, "USER %s", username);
					 sprintf(send_buffer,"331 Password required \r\n");
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"PASS",4)==0)  {
					 // get password
					 sscanf(receive_buffer, "PASS %s", password);
					 if((strcmp(username, "napoleon") == 0) && (strcmp(password, "334") == 0)){
					 	sprintf(send_buffer,"230 Public login sucessful\r\n");
					 	isAuth = true;
					 } else {
					 	sprintf(send_buffer,"230 Public login sucessful\r\n");
					 	isAuth = false;
					 }
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"SYST",4)==0)  {
					 printf("Information about the system \n");
					 #if defined __unix__ || defined __APPLE__
					 sprintf(send_buffer,"215 Unix\r\n");
					 #elif defined _WIN32
					 sprintf(send_buffer,"215 Windows 64-bit\r\n");
					 #endif
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"OPTS",4)==0)  {
					 sprintf(send_buffer,"550 unrecognized command\r\n");
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"QUIT",4)==0)  {
					 printf("Quit \n");
					 sprintf(send_buffer,"221 Connection close by client\r\n");
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes < 0) break;
				 }
				 //---
				 if(strncmp(receive_buffer,"EPRT",4)==0) {
				 	if(IPV6){
					 	s_data_act = socket(AF_INET6, SOCK_STREAM, 0);
					} else {
						s_data_act = socket(AF_INET, SOCK_STREAM, 0);
					}

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

					 char portStr[20]; 
					 sprintf(portStr, "%d", port);

					 iResult = getaddrinfo(clientHost, portStr, &hints, &result);
					 if(iResult != 0) {
					 	printf("getaddrinfo failed: %d\n", iResult);
						#if defined __unix__ || defined __APPLE__
							//test
						#elif defined _WIN32 
					 		WSACleanup();
						#endif
					 	exit(27);
					 }

					 if (connect(s_data_act, result->ai_addr, result->ai_addrlen) != 0){
						 sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						#if defined __unix__ || defined __APPLE__
					    		close(s_data_act);//close listening socket
						#elif defined _WIN32 
							closesocket(s_data_act);
							WSACleanup();
						#endif
					 }
					 else {
						 sprintf(send_buffer, "200 EPRT Command successful\r\n");
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 printf("Connected to client\n");
					 }
				 }
				 //---				 
				 //technically, LIST is different than NLST,but we make them the same here
				 if ( (strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
					 #if defined __unix__ || __APPLE__
					 system("ls > tmp.txt");//change that to 'dir', so windows can understand
					 #elif defined _WIN32
					 system("dir > tmp.txt");
					 #endif
					 FILE *fin;
					 if( access( "tmp.txt", F_OK ) != -1 ) {
					 	// file exists
					 	fin=fopen("tmp.txt","r");//open tmp.txt file
					 } else {
					 	sprintf(send_buffer,"510 Directory command failed \r\n");
					  	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						#if defined __unix__ || defined __APPLE__
					  		if (active==0 )close(ns_data);
					 		else close(s_data_act);
						#elif defined _WIN32 
					  		if (active==0 )closesocket(ns_data);
					 		else closesocket(s_data_act);
						#endif
					  	break;
					 }
					 sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 char temp_buffer[80];
					 while (!feof(fin)){
						 fgets(temp_buffer,78,fin);
						 sprintf(send_buffer,"%s",temp_buffer);
						 if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
						 else send(s_data_act, send_buffer, strlen(send_buffer), 0);
					 }
					 fclose(fin);
					 sprintf(send_buffer,"226 File transfer complete. \r\n");
					 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 #if defined __unix__ || defined __APPLE__
					  		if (active==0 )close(ns_data);
					 		else close(s_data_act);
						#elif defined _WIN32 
					  		if (active==0 )closesocket(ns_data);
					 		else closesocket(s_data_act);
						#endif
					 //OPTIONAL, delete the temporary file
					 system("del tmp.txt");
				 }

                 //---	
                 if ( (strncmp(receive_buffer,"RETR",4)==0))   {
					 char filename[80];
					  FILE *fin;
					 // must get filename from the reply
					 sscanf(receive_buffer, "RETR %s", filename);
					 if( access( filename, F_OK ) != -1 ) {
   					 	// file exists
						 if(mode == 'I'){
						 	fin=fopen(filename,"rb");
						 	sprintf(send_buffer, "150 Opening Binary mode data connection\r\n");
						 	//printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 } else {
						 	fin=fopen(filename,"r");
						 	sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
						 	//printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
						 }
						 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 } else {
    					sprintf(send_buffer,"550 File transfer failed. File does not exist in directory. \r\n");
					  	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					  	#if defined __unix__ || defined __APPLE__
					  		if (active==0 )close(ns_data);
					 		else close(s_data_act);
						#elif defined _WIN32 
					  		if (active==0 )closesocket(ns_data);
					 		else closesocket(s_data_act);
						#endif
					  	break;
					 }
					 // TEST CODE

					 	int fileSize;
					 	fseek(fin, 0, SEEK_END);
					 	fileSize = ftell(fin);
					 	fseek(fin, 0, SEEK_SET);

					 	char temp_buffer[fileSize];

    					fread(temp_buffer, 1, sizeof(temp_buffer), fin);
    					if (active==0) send(ns_data, temp_buffer, sizeof(temp_buffer), 0);
    					else send(s_data_act, temp_buffer, sizeof(temp_buffer), 0);

						fclose(fin);
					  	sprintf(send_buffer,"226 File transfer complete. \r\n");
					  	//printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					  	bytes = send(ns, send_buffer, strlen(send_buffer), 0);

						#if defined __unix__ || defined __APPLE__
					  		if (active==0 )close(ns_data);
					 		else close(s_data_act);
						#elif defined _WIN32 
					  		if (active==0 )closesocket(ns_data);
					 		else closesocket(s_data_act);
						#endif
//break?
				 }		 
				 // ---
				 if ( (strncmp(receive_buffer,"TYPE",4)==0))   {  			 	
				 	sscanf(receive_buffer, "TYPE %c", &mode);
				 	if(mode == 'A'){
				 		sprintf(send_buffer,"200 Type set to A\r\n");
				 	} else {
				 		sprintf(send_buffer,"200 Type set to I\r\n");
				 	}
				 	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					if (bytes < 0) break;
				 } 
				 // ---
				 if ( (strncmp(receive_buffer,"CWD",3)==0)) {  
				 	char directory[100];
				 	sscanf(receive_buffer, "CWD %s", directory);
				 	char current[100];
				 	getcwd(current, sizeof(current));
				 	char newdir[100];
				 	// Check dir exists
				 	if(chdir(directory) == 0){
				 		// Check that they are allowed in this directory if not, send back to previous and send error
				 		getcwd(newdir, sizeof(newdir));
				 		if(strncmp(serverRoot, newdir, strlen(serverRoot)) == 0){
				 			if(strstr(directory, "secret") != NULL){
				 				// if the secret folder is in the directory
						 		if(!isAuth){
						 			chdir(current);
						 			sprintf(send_buffer, "530 Unauthorized directory.\r\n");
						 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
									if (bytes < 0) break;
									continue;
						 		}
				 			}
				 			getcwd(directory, sizeof(directory));
				 			sprintf(send_buffer,"200 Directory changed to %s \r\n", directory);
						 	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
							if (bytes < 0) break; 
				 		} else {
				 			// not allowed here, go back
				 			chdir(current);
				 			sprintf(send_buffer, "530 Unauthorized directory.\r\n");
				 			bytes = send(ns, send_buffer, strlen(send_buffer), 0);
							if (bytes < 0) break;
							continue;
				 		}
				 	} else { 
				 		sprintf(send_buffer,"510 No directory found at %s \r\n", directory);
				 		bytes = send(ns, send_buffer, strlen(send_buffer), 0);
						if (bytes < 0) break;
				 	}
				 }
				 if ( (strncmp(receive_buffer,"XPWD",4)==0) || (strncmp(receive_buffer,"PWD",3)==0))   {
				 	char directory[100]; 
				 	getcwd(directory, sizeof(directory));
				 	sprintf(send_buffer,"200 Working Directory is %s\r\n", directory);
				 	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
				 	if (bytes < 0) break;
				 } 
			 //=================================================================================	 
			 }//End of COMMUNICATION LOOP per CLIENT
			 //=================================================================================
			 
//********************************************************************
//CLOSE SOCKET
//********************************************************************
		#if defined __unix__ || defined __APPLE__
      		close(ns);
		#elif defined _WIN32
		    int iResult = shutdown(ns, SD_SEND);
		    if (iResult == SOCKET_ERROR) {
		        printf("shutdown failed with error: %d\n", WSAGetLastError());
		        closesocket(ns);
		        WSACleanup();
		        exit(1);
		    } 
		#endif 
			 printf("DISCONNECTED from %s\n",clientHost);
			 //sprintf(send_buffer, "221 Bye bye, server close the connection ... \r\n");
			 //printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
			 //bytes = send(ns, send_buffer, strlen(send_buffer), 0);
			 
		 //====================================================================================
		 } //End of MAIN LOOP
		 //====================================================================================
	#if defined __unix__ || defined __APPLE__
    		close(s);//close listening socket
	#elif defined _WIN32 
		closesocket(s);
		WSACleanup();
	#endif	
		printf("\nSERVER SHUTTING DOWN...\n");
		return 0;
}

