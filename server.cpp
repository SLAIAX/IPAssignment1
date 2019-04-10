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

#define USE_IPV6 true

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h> //required by getaddrinfo() and special constants

#define WSVERS MAKEWORD(2,0)
WSADATA wsadata;


#define DEFAULT_PORT "1234" 
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

	printf("\n\n<<<TCP (CROSS-PLATFORM, IPv6-ready) SERVER, by nhreyes>>>\n");  
    if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        exit(1);
    }
    else{              
        printf("\nThe Winsock 2.2 dll was initialised.\n");
    }
		 // struct sockaddr_storage localaddr,remoteaddr;  //ipv4 only
		 struct sockaddr_storage local_data_addr_act;   //ipv4 only
		 struct sockaddr_storage clientAddress;
		 char clientHost[NI_MAXHOST]; 
		 char clientService[NI_MAXSERV];
		 SOCKET s,ns;
		 SOCKET ns_data, s_data_act;
		 char send_buffer[200],receive_buffer[200];
		 char portNum[NI_MAXSERV];
		
         ns_data=INVALID_SOCKET;

		 int active=0;
		 int n,bytes,addrlen;
		 
		 printf("\n===============================\n");
		 printf("     159.334 FTP Server");
		 printf("\n===============================\n");
	
		 
		 // memset(&localaddr,0,sizeof(localaddr));//clean up the structure
		 // memset(&remoteaddr,0,sizeof(remoteaddr));//clean up the structure
		 
//********************************************************************
//SOCKET
//********************************************************************
		struct addrinfo *result = NULL;
		struct addrinfo hints;
		// struct addrinfo *ptr = NULL;
		int iResult;

		//********************************************************************
// STEP#0 - Specify server address information and socket properties
//********************************************************************
	 
//ZeroMemory(&hints, sizeof (hints)); //alternatively, for Windows only
memset(&hints, 0, sizeof(struct addrinfo));

if(USE_IPV6){
   hints.ai_family = AF_INET6;  
}	 else { //IPV4
   hints.ai_family = AF_INET;
}

//hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM;
hints.ai_protocol = IPPROTO_TCP;
hints.ai_flags = AI_PASSIVE; // For wildcard IP address 
                             //setting the AI_PASSIVE flag indicates the caller intends to use 
									           //the returned socket address structure in a call to the bind function. 

// Resolve the local address and port to be used by the server
if(argc==2){	 
	 iResult = getaddrinfo(NULL, argv[1], &hints, &result); //converts human-readable text strings representing hostnames or IP addresses 
	                                                        //into a dynamically allocated linked list of struct addrinfo structures
																			                    //IPV4 & IPV6-compliant
	 sprintf(portNum,"%s", argv[1]);
	 printf("\nargv[1] = %s\n", argv[1]); 	

} else {
   iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); //converts human-readable text strings representing hostnames or IP addresses 
	                                                             //into a dynamically allocated linked list of struct addrinfo structures
																				                       //IPV4 & IPV6-compliant
	 sprintf(portNum,"%s", DEFAULT_PORT);
	 printf("\nUsing DEFAULT_PORT = %s\n", portNum); 
}

if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);

#if defined _WIN32
    WSACleanup();
#endif    
    return 1;
}	 

//********************************************************************
// STEP#1 - Create welcome SOCKET
//********************************************************************


#if defined __unix__ || defined __APPLE__
  s = -1;
#elif defined _WIN32
  s = INVALID_SOCKET;
#endif


// Create a SOCKET for the server to listen for client connections

s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

#if defined __unix__ || defined __APPLE__
  if (s < 0) {
      printf("socket failed\n");
      freeaddrinfo(result);
  }
#elif defined _WIN32
  //check for errors in socket allocation
  if (s == INVALID_SOCKET) {
      printf("Error at socket(): %d\n", WSAGetLastError());
      freeaddrinfo(result);
      WSACleanup();
      exit(1);//return 1;
  }
#endif
		 
		 
//********************************************************************
//BIND
//********************************************************************


// bind the TCP welcome socket to the local address of the machine and port number
   iResult = bind( s, result->ai_addr, (int)result->ai_addrlen);
    
     
//if error is detected, then clean-up
#if defined __unix__ || defined __APPLE__
   if (iResult == -1) {
      printf( "\nbind failed\n"); 
      freeaddrinfo(result);
      close(s);//close socket
#elif defined _WIN32
    if (iResult == SOCKET_ERROR) {
      printf("bind failed with error: %d\n", WSAGetLastError());
      freeaddrinfo(result);
      closesocket(s);
      WSACleanup();
#endif       
      return 1;
    }
	 
	 freeaddrinfo(result); //free the memory allocated by the getaddrinfo 
	                       //function for the server's address, as it is 
	                       //no longer needed

		 
//********************************************************************
//LISTEN
//********************************************************************
#if defined __unix__ || defined __APPLE__
    if (listen( s, 5) == -1) {
#elif defined _WIN32
    if (listen( s, SOMAXCONN ) == SOCKET_ERROR ) {
#endif

	


#if defined __unix__ || defined __APPLE__
      printf( "\nListen failed\n"); 
      close(s); 
#elif defined _WIN32
      printf( "Listen failed with error: %d\n", WSAGetLastError() );
      closesocket(s);
      WSACleanup(); 
#endif   

      exit(1);

   } else {
		  printf("\n<<<SERVER>>> is listening at PORT: %s\n", portNum);
	}
		

		 
 //====================================================================================
//Start of MAIN LOOP
 //====================================================================================
	while (1) {
		addrlen = sizeof(clientAddress);
//********************************************************************
//NEW SOCKET newsocket = accept  //CONTROL CONNECTION
//********************************************************************
	ns = INVALID_SOCKET;
	printf("\n------------------------------------------------------------------------\n");
	printf("SERVER is waiting for an incoming connection request...");
	printf("\n------------------------------------------------------------------------\n");
	ns = accept(s,(struct sockaddr *)(&clientAddress),&addrlen);
	if (ns == INVALID_SOCKET) {
     	printf("accept failed: %d\n", WSAGetLastError());
     	closesocket(s);
    	 WSACleanup();
    	 return 1;
    } else {
		printf("\nA <<<CLIENT>>> has been accepted.\n");
	    
	    //strcpy(clientHost,inet_ntoa(clientAddress.sin_addr)); //IPV4
	    //sprintf(clientService,"%d",ntohs(clientAddress.sin_port)); //IPV4
	    //---

	    DWORD returnValue;
	    memset(clientHost, 0, sizeof(clientHost));
	    memset(clientService, 0, sizeof(clientService));

	    returnValue = getnameinfo((struct sockaddr *)&clientAddress, addrlen,
	                    clientHost, sizeof(clientHost),
	                    clientService, sizeof(clientService),
	                    NI_NUMERICHOST);
	    if(returnValue != 0){
	       printf("\nError detected: getnameinfo() failed with error#%d\n",WSAGetLastError());
	       exit(1);
	    } else{
	       printf("\nConnected to <<<Client>>> with IP address:%s, at Port:%s\n",clientHost, clientService);
	    }
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

					 if ((bytes ==  SOCKET_ERROR) || (bytes == 0)) break; // may require SOCKET_ERROR CHECK
					 if (receive_buffer[n] == '\n') { /*end on a LF*/
						 receive_buffer[n] = '\0';
						 break;
					 }
					 if (receive_buffer[n] != '\r') n++; /*Trim CRs*/
				 //=======================================================
				 } //End of PROCESS message received from CLIENT
				 //=======================================================
				 if ((bytes ==  SOCKET_ERROR) || (bytes == 0)) break; // may require SOCKET_ERROR CHECK

				 printf("<< DEBUG INFO. >>: the message from the CLIENT reads: '%s\\r\\n' \n", receive_buffer);

//********************************************************************
//PROCESS COMMANDS/REQUEST FROM USER
//********************************************************************				 
				 if (strncmp(receive_buffer,"USER",4)==0)  {
					 printf("Logging in... \n");
					 sprintf(send_buffer,"331 Password required (anything will do really... :-) \r\n");
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes ==  SOCKET_ERROR) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"PASS",4)==0)  {
					 
					 sprintf(send_buffer,"230 Public login sucessful \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes ==  SOCKET_ERROR) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"SYST",4)==0)  {
					 printf("Information about the system \n");
					 sprintf(send_buffer,"215 Windows 64-bit\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes ==  SOCKET_ERROR) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"OPTS",4)==0)  {
					 printf("unrecognised command \n");
					 sprintf(send_buffer,"502 command not implemented\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes ==  SOCKET_ERROR) break;
				 }
				 //---
				 if (strncmp(receive_buffer,"QUIT",4)==0)  {
					 printf("Quit \n");
					 sprintf(send_buffer,"221 Connection close by client\r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (bytes ==  SOCKET_ERROR) break;
					 // closesocket(ns);
				 }
				 //---
				 // if(strncmp(receive_buffer,"PORT",4)==0) {
					//  s_data_act = socket(AF_INET, SOCK_STREAM, 0);
					//  //local variables
					//  //unsigned char act_port[2];
					//  int act_port[2];
					//  int act_ip[4], port_dec;
					//  char ip_decimal[40];
					//  printf("===================================================\n");
					//  printf("\n\tActive FTP mode, the client is listening... \n");
					//  active=1;//flag for active connection
					//  //int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
					//  //		&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
					//  //     (int*)&act_port[0],(int*)&act_port[1]);
					 
					//  int scannedItems = sscanf(receive_buffer, "PORT %d,%d,%d,%d,%d,%d",
					// 		&act_ip[0],&act_ip[1],&act_ip[2],&act_ip[3],
					//       &act_port[0],&act_port[1]);
					 
					//  if(scannedItems < 6) {
		   //       	    sprintf(send_buffer,"501 Syntax error in arguments \r\n");
					// 	printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					// 	bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					//     //if (bytes < 0) break;
			  //           break;
		   //           }
					 
					//  local_data_addr_act.sin_family=AF_INET;//local_data_addr_act  //ipv4 only
					//  sprintf(ip_decimal, "%d.%d.%d.%d", act_ip[0], act_ip[1], act_ip[2],act_ip[3]);
					//  printf("\tCLIENT's IP is %s\n",ip_decimal);  //IPv4 format
					//  local_data_addr_act.sin_addr.s_addr=inet_addr(ip_decimal);  //ipv4 only
					//  port_dec=act_port[0];
					//  port_dec=port_dec << 8;
					//  port_dec=port_dec+act_port[1];
					//  printf("\tCLIENT's Port is %d\n",port_dec);
					//  printf("===================================================\n");
					//  local_data_addr_act.sin_port=htons(port_dec); //ipv4 only
					//  if (connect(s_data_act, (struct sockaddr *)&local_data_addr_act, (int) sizeof(struct sockaddr)) != 0){
					// 	 printf("trying connection in %s %d\n",inet_ntoa(local_data_addr_act.sin_addr),ntohs(local_data_addr_act.sin_port));
					// 	 sprintf(send_buffer, "425 Something is wrong, can't start active connection... \r\n");
					// 	 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					// 	 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					// 	 closesocket(s_data_act);
					//  }
					//  else {
					// 	 sprintf(send_buffer, "200 PORT Command successful\r\n");
					// 	 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					// 	 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					// 	 printf("Connected to client\n");
					//  }

				 // }
				 //---				 
				 //technically, LIST is different than NLST,but we make them the same here
				 if ( (strncmp(receive_buffer,"LIST",4)==0) || (strncmp(receive_buffer,"NLST",4)==0))   {
					 //system("ls > tmp.txt");//change that to 'dir', so windows can understand
					 system("dir > tmp.txt");
					 FILE *fin=fopen("tmp.txt","r");//open tmp.txt file
					 //sprintf(send_buffer,"125 Transfering... \r\n");
					 sprintf(send_buffer,"150 Opening ASCII mode data connection... \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 char temp_buffer[80];
					 while (!feof(fin)){
						 fgets(temp_buffer,78,fin);
						 sprintf(send_buffer,"%s",temp_buffer);
						 if (active==0) send(ns_data, send_buffer, strlen(send_buffer), 0);
						 else send(s_data_act, send_buffer, strlen(send_buffer), 0);
					 }
					 fclose(fin);
					 //sprintf(send_buffer,"250 File transfer completed... \r\n");
					 sprintf(send_buffer,"226 File transfer complete. \r\n");
					 printf("<< DEBUG INFO. >>: REPLY sent to CLIENT: %s\n", send_buffer);
					 bytes = send(ns, send_buffer, strlen(send_buffer), 0);
					 if (active==0 )closesocket(ns_data);
					 else closesocket(s_data_act);
						 
					 
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
			 int iResult = shutdown(ns, SD_SEND);
      if (iResult == SOCKET_ERROR) {
         printf("shutdown failed with error: %d\n", WSAGetLastError());
         closesocket(ns);
         WSACleanup();
         exit(1);
      } 
      printf("\nDisconnected from <<<CLIENT>>> with IP address:%s, Port:%s\n",clientHost, clientService);
		  printf("=============================================");
//***********************************************************************
      		closesocket(ns);   
			 
		 //====================================================================================
		 } //End of MAIN LOOP
		 //====================================================================================
		 closesocket(s);
		 WSACleanup(); /* call WSACleanup when done using the Winsock dll */
		 printf("\nSERVER SHUTTING DOWN...\n");
		 exit(0);
}

