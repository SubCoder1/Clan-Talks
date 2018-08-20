#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <WinSock2.h>
#include <unordered_map>
#pragma comment (lib, "ws2_32.lib")
#define DEFAULT_LEN 4096

class SERVER {
	int PORT, flag = -1, nClientAddrLen;
	WORD ver;
	fd_set origin, copy;
	sockaddr_in addr_info;
	SOCKET server_sock;
	std::string welcome_msg = "		___________________Clan-Talks(v1.0)__________________\r\n";
	std::string rule0 = "			      Type & Enter '#' to Exit\n";
	std::string rule1 = "	         Type $[yourname] to let others know who you are B)\n\n";
	std::unordered_map <std::string, std::string> client_info;

	public:
		SERVER() : ver( MAKEWORD(2,2) ), PORT(12300), server_sock(INVALID_SOCKET) { }
		
		void Init_winsock(){	//Initializes WINSOCK
			WSADATA wsData;
			int wsOK = WSAStartup(ver, &wsData);
			if (wsOK != 0) { 
				std::cerr << "WSAStartup failed :: Quitting..\n"; 
				exit(0); 
			}
		}

		void create_sock() {
			//Create SERVER SOCKET
			int opt = TRUE;
			server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (server_sock == INVALID_SOCKET) { 
				std::cerr << "Error at Socket Creation :: Quitting..\n"; 
			    exit(0); 
			}
			//set master socket to allow multiple connections , 
			//this is just a good habit, it will work without this 
			if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
				sizeof(opt)) < 0)
			{
				perror("setsockopt");
				exit(EXIT_FAILURE);
			}
		}

		int bind_socket() {					//Binds the SOCKET to IP & PORT
			addr_info.sin_family = AF_INET;
			addr_info.sin_port = htons(PORT);
			addr_info.sin_addr.S_un.S_addr = INADDR_ANY;
			
			int bind_res = bind(server_sock, (sockaddr*)&addr_info, sizeof(addr_info));
			if (bind_res < 0) { 
				std::cerr << "Binding failed :: Quitting...\n"; 
				exit(0); 
			}
			return 1;
		}

		int socket_listen() {
			int listen_outcome = listen(server_sock, SOMAXCONN);
			if (listen_outcome == SOCKET_ERROR) {
				std::cerr << "Listen Socket Error :: Quitting..\n";
				closesocket(server_sock);
				WSACleanup();
				exit(0);
			}
			return 1;
		}

		void send_rec_handler() {
			flag = -1;
			FD_ZERO(&origin);				//Make Sure the created file-descriptor set is empty
			// Add our first socket that we're interested in interacting with; the listening socket!
		    // It's important that this socket is added for our server or else we won't 'hear' incoming
		    // connections
			FD_SET(server_sock, &origin);

			bool status = true;
			while (status) {
				// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
				// are accepting inbound connection requests OR messages.
				copy = origin;
				//Check which Client wants to interact
				int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
				// Loop through all the current connections / potential connect
				for (int i = 0; i < socketCount; i++) {
					SOCKET collector = copy.fd_array[i];

					if (collector == server_sock) { //Just an Interaction..
						// Accept a new connection
						nClientAddrLen = sizeof(addr_info);
						SOCKET newConnection = accept(server_sock, (sockaddr*)&addr_info, &nClientAddrLen);

						printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , newConnection , inet_ntoa(addr_info.sin_addr) , ntohs(addr_info.sin_port));
						// add into origin set
						FD_SET(newConnection, &origin);
						// send the initial msg
						send(newConnection, welcome_msg.c_str(), welcome_msg.size() + 1, 0);
						send(newConnection, rule0.c_str(), rule0.size() + 1, 0);
						send(newConnection, rule1.c_str(), rule1.size() + 1, 0);

					} 
					else {	// Its a incoming message..
						IN_ADDR clientIn;
						nClientAddrLen = sizeof(addr_info);
						getpeername(collector, (LPSOCKADDR)&addr_info, &nClientAddrLen);
						memcpy(&clientIn, &addr_info.sin_addr.s_addr, 4);
						std::string client_ip = inet_ntoa(clientIn);
						flag = 0;
						char buff[4096];
						ZeroMemory(buff, DEFAULT_LEN);
						int bytes_rec = recv(collector, buff, DEFAULT_LEN, 0);
						if (bytes_rec <= 0) { status = false; break; }
						else {
							if (buff[0] == '#') {
								//drop that client
									flag = 1;
									FD_CLR(collector, &origin);
									closesocket(collector);
							}
							else if (buff[0] == '$') {
								flag = 2;
								buff[bytes_rec-1] = '\0';
								client_info[client_ip] = buff;
							}
							buff[bytes_rec] = '\0';
							echo(collector, flag, buff, client_ip);
						}
					}
				}
			}
			shutdown_socket();
		}
		// Send message to other clients, and definiately NOT the listening socket

		void echo(SOCKET collector, int flag, char buff[], std::string client_ip) {
			for (unsigned int i = 0; i < origin.fd_count; i++) {
				SOCKET outSock = origin.fd_array[i];
				std::string strOut;
				if (outSock != server_sock && outSock != collector) {
					std::ostringstream ss;
					if (flag == 1) {
						ss << client_info[client_ip] << " left the Clan.\n";
					}
					else if (flag == 2) {
						ss << client_info[client_ip] << " Joined the clan.\n\n";
					}
					else {
						ss << client_info[client_ip] << "> " << buff << "\n\n";
					}
					strOut = ss.str();
					send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					ss.clear();
				}
			}
		}

		void shutdown_socket() {
			FD_CLR(server_sock, &origin);
			closesocket(server_sock);

			std::string goodbye_msg = "Server is shutting down. GoodBye\r\n";
			while (origin.fd_count > 0) {
				SOCKET temp2 = origin.fd_array[0];
				send(temp2, goodbye_msg.c_str(), goodbye_msg.size() + 1, 0);
				FD_CLR(temp2, &origin);
				closesocket(temp2);
			}
			WSACleanup();
		}
};

int main(){
	std::cout << "-----STARTING CLAN-CHAT ENGINE-----\n\n";
	SERVER starter;

	std::cout << "Initializing WINSOCK...\n";
	starter.Init_winsock();
	std::cout << "WINSOCK Initialized.\n";

	std::cout << "Creating SOCKET...\n";
	starter.create_sock();
	std::cout << "SOCKET Created.\n";
	
	std::cout << "Binding IP & PORT...\n";
	starter.bind_socket();
	std::cout << "Successfully Binded.\n";

	std::cout << "Waiting for Connections...\n";
	starter.socket_listen();
	std::cout << "Connected :)\n";

	std::cout << "Let's have a Conversation!!\n";
	starter.send_rec_handler();
	
	std::cout << "Clients Disconnected\n\n";
	std::cout << "Terminating......\n";
	system("pause");
}
