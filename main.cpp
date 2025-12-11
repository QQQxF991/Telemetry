#include "bynary_message.сpp"

int main(int argc, char const *argv[]) {
    std::thread bin_thread(BynaryServer);
    std::thread http_thread(HTTP_server);
    std::cout << "Press ENTER to stop...\n";
    std::string Dum;
    std::getline(std::cin, Dum);
    
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int s2 = socket(AF_INET, SOCK_STREAM, 0);

    if (s >= 0){
        sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(BINARY_PORT); addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(s, (sockaddr*)&addr, sizeof(addr)); close(s);
    }
    
    if (s2 >= 0) {
        sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(HTTP_PORT); addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(s2, (sockaddr*)&addr, sizeof(addr)); close(s2);
    }
    
    bin_thread.join();
    http_thread.join();
    return 0;
}
