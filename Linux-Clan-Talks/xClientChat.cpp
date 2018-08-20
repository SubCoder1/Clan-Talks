#include <string>
#include <iostream>
#include <errno.h>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unordered_map>

#define TRUE   1
#define FALSE  0
#define MAX_CLIENTS 30

class CLAN_TALKS{
    fd_set origin;
    int client_sock[MAX_CLIENTS] = {0};
    char buff[1024];
    int server_sock, new_socket, opt, PORT;
    sockaddr_in address;
    int listen_val, addrlen, sd, max_sd, bind_val;
    std::string welcome_msg = "		___________________Clan-Talks(v1.0)__________________\r\n";
	  std::string rule0 = "			      Type & Enter '#' to Exit\n";
	  std::string rule1 = "	         Type $[yourname] to let others know who you are B)\n\n";
    std::unordered_map <std::string, std::string> client_info;
	
    public:
        CLAN_TALKS() : PORT(12300), opt(TRUE) { }

        void create_sock(){         //Create SOCKET
            std::cout << "Creating SOCKET...\n";
            server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(server_sock == 0){
                std::cerr << "Can't create a socket :: Quitting\n";
                exit(EXIT_FAILURE);
            } std::cout << "SOCKET Created.\n";
        }

         void setsocketoption(){     //Sets SOCKET option to handle multiple connections(optional)
            std::cout << "Setting SOCKET to receive multiple connections...\n";
            if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0){
                std::cerr << "setsockopt failure :: Quitting\n";
                exit(EXIT_FAILURE);
            }
        }

        void bind_socket(){
            std::cout << "BINDING IP & PORT to the SOCKET created...\n";
            address.sin_family = AF_INET;
            address.sin_port = htons(PORT);
            address.sin_addr.s_addr = INADDR_ANY;
            bind_val = bind(server_sock, (sockaddr*)&address, sizeof(address));
            if(bind_val < 0){ std::cerr << "Bind Failed :: Quitting\n"; exit(EXIT_FAILURE); }
            std::cout << "BINDING successful.\n";
        }

        void listen_mode(){
            std::cout << "Waiting for Connections...\n";
            listen_val = listen(server_sock, SOMAXCONN);
            if(listen_val < 0){
                std::cerr << "Listen error :: Quitting\n";
                exit(EXIT_FAILURE);
            }
            std::cout << "Connected :)\n";
        }

        void send_rec_handler(){

            addrlen = sizeof(address);
            bool status = true;

            while(status){
                FD_ZERO(&origin);
                FD_SET(server_sock, &origin);
                max_sd = server_sock;
                //Add child socket to set
                for (auto i = 0 ; i < MAX_CLIENTS ; i++) {
                    //socket descriptor
                    sd = client_sock[i];
                    //if valid socket descriptor then add to read list
                    if(sd > 0)
                        FD_SET(sd , &origin);

                    //highest file descriptor number, need it for the select function
                    if(sd > max_sd)
                        max_sd = sd;
                }
                auto client_activity = select(max_sd+1, &origin, nullptr, nullptr, nullptr);
                //If something happened on the server_socket ,
                //then its an incoming connection
                if (FD_ISSET(server_sock, &origin)) {
                    new_socket = accept(server_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    std::cout << "New connection :: SOCKET ID > " << new_socket << " IP > " << inet_ntoa(address.sin_addr);
                    std::cout << " PORT > " << ntohs(address.sin_port) << std::endl;
                    send(new_socket, welcome_msg.c_str(), welcome_msg.size()+1, 0);
                    send(new_socket, rule0.c_str(), rule0.size()+1, 0);
                    send(new_socket, rule1.c_str(), rule1.size()+1, 0);
                    for (auto i = 0; i < MAX_CLIENTS; i++){
                    //if position is empty
                        if(client_sock[i] == 0){
                            client_sock[i] = new_socket;
                            break;
                        }
                    }
                }
                  //incoming msg
                    int send_val, rec_val;
                    std::string t;
                    for(auto i = 0; i < MAX_CLIENTS; i++){
                        sd = client_sock[i];
                        std::stringstream ss;
                        if(FD_ISSET(sd, &origin)){
                            rec_val = read(sd, buff, 1024);
                            getpeername(sd, (sockaddr*)&address, (socklen_t*)&addrlen);
                            if(buff[0] == '#' || rec_val <= 0){
                                ss << client_info[inet_ntoa(address.sin_addr)] << " has left the clan chat.\n";
                                t = ss.str();
                                echo(sd, t);
                                close(sd);
                                client_sock[i] = 0;
                                ss.clear();
                            }
                            else if(buff[0] == '$'){
                                buff[rec_val-1] = '\0';
                                client_info[inet_ntoa(address.sin_addr)] = buff;
                                ss << client_info[inet_ntoa(address.sin_addr)] << " joined the clan chat.\n";
                                t = ss.str();
                                echo(sd, t);
                                ss.clear();
                            }
                            else {
                                buff[rec_val] = '\0';
                                ss << client_info[inet_ntoa(address.sin_addr)]  << "> " << buff;
                                t = ss.str();
                                echo(sd, t);
                                ss.clear();
                            }
                        }
                    }
            }
        }

        void echo(int sd, std::string t){
            int temp;
            for(int i = 0; i < MAX_CLIENTS; i++){
                temp = client_sock[i];
                if(temp != sd && temp != server_sock){
                   send(temp, t.c_str(), t.size()+1, 0);
                }
            }
        }


};
int main(int argc, char *argv[]){
    CLAN_TALKS engine;

    engine.create_sock();

    engine.setsocketoption();

    engine.bind_socket();

    engine.listen_mode();

    engine.send_rec_handler();

	std::cout << "Clients Disconnected\n\n";
	std::cout << "Terminating......\n";
    system("pause");
}
