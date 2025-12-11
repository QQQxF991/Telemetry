#include "bynary_message.hpp"

void Server() {
    int SRV = socket(AF_INET,SOCK_STREAM,0);
    if (SRV < 0) {perror("Socket"); return;}
    int opt = 1; 
    setsocport = (srv,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


    
}



int main(int argc, char const *argv[]) {
    
    return 0;
}
