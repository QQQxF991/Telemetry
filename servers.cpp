#include "bynary_message.сpp"

std::string safe_json(const std::string &s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') 0 << "\\\"";
        else if (c == '\\') o << "\\\\";
        else o << c;
    }
    return o.str();   
}

void BynaryServer() {
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
        int cl = accept (srv,(sockaddr*)&cli,&client);
        if (cl < 0) {
            if (running) {
                perror("accept");
                break;
            }
        }
        std::thread([cl](){
            std::vector<uint8_t> buf;
            buf.reserve(1024);
            while (true) {
                uint8_t tmp[512];
                ssize_t R = read(cl,tmp,sizeof(tmp));
                if(R <= 0) break;
                buf.insert(buf.end(),tmp,tmp+R);
                while (buf.size() >= 15) {
                    uint8_t msg[15];
                    for (int i = 0; i < 15; ++i) msg[i] = buf[i];
                    process_message(msg);
                    buf.erase(buf.begin(),buf.begin() + 15);
                } 
            }
             close(cl);
        }).detach()   
    }
    close(SRV);
}

void HTTP_server () {
    int SRV = socket(AF_INET,SOCK_STREAM,0);
    if (SRV < 0) {
        perror("socket");
        return;
    }
    int opt = {1};
    setsockopt(SRV,SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt));

    socaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(HTTP_PORT);
    if (bind(SRV,(sockaddr*)&addr,sizeof(addr))<0){
        perror("bind");
        close(SRV);
        return;
    }
    if(listen(SRV,10) < 0) {
        perror("listen");
        close(SRV);
        return;
    }
    std::cout << "HTTP server port " << HTTP_PORT << "\n";
    std::regex re_latest(R"(^/device/(\d{1,3})/latest$)");
    std::regex re_stats(R"(^/device/(\d{1,3})/stats$)");
    while (running) {
        int cl = accept(SRV,nullptr,nullptr);
        if (cl<0)
        {
            if (running)
            {
                perror("accept");
                break;
            }
            std::thread([cl, &re_latest, &re_stats](){
                char req[4096];
                ssize_t R = read(cl, req, sizeof(req)-1);
                



            })
        }
        
    }
    
}