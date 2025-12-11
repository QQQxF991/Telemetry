#include "bynary_message.hpp"

void Server() {
    int SRV = socket(AF_INET,SOCK_STREAM,0);
    if (SRV < 0) {perror("Socket"); return;}
    int opt = 1; 
    setsocport = (srv,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(BINARY_PORT);
    if (bind(SRV,(sockaddr*)&addr,sizeof(addr)) < 0) {
        perror("bind");
        close(SRV);
        return;   
    }
    if (listen(SRV,5) < 0) {
        perror("listen");
        close(SRV); 
        return;
    }
    std::cout << "Server listening port : " << BINARY_PORT << "\n";
    while (running) {
        sockaddr_in cli{};
        socklen_t client = sizeof(cli);
        int cl = accept (srv,(sockaddr*)&cli,sizeof(clilen)) < 0)
    }
    
    
    


}



int main(int argc, char const *argv[]) {
    
    return 0;
}
