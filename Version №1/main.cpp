#include "binary_message.h"
#include <iostream>
#include <thread>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    std::thread bin_thread(BynaryServer);
    std::thread http_thread(HTTP_server);
    
    std::cout << "Servers started. Press ENTER to stop...\n";
    std::string dummy;
    std::getline(std::cin, dummy);
    
    running = false;

    if (binary_listen_socket >= 0) {
        shutdown(binary_listen_socket, SHUT_RDWR);
        close(binary_listen_socket);
    }
    
    if (http_listen_socket >= 0) {
        shutdown(http_listen_socket, SHUT_RDWR);
        close(http_listen_socket);
    }
    
    bin_thread.join();
    http_thread.join();
    return 0;
}