#pragma once
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
            std::vector<uint8_t> buffer;
            buffer.reserve(1024);
            while (true) {
                uint8_t tmp[512];
                ssize_t R = read(cl,tmp,sizeof(tmp));
                if(R <= 0) break;
                buffer.insert(buffer.end(),tmp,tmp+R);
                while (buffer.size() >= 15) {
                    uint8_t msg[15];
                    for (int i = 0; i < 15; ++i) msg[i] = buffer[i];
                    process_message(msg);
                    buffer.erase(buf.begin(),buffer.begin() + 15);
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
        if (cl<0) {  if (running) perror("accept"); break; }
            std::thread([cl, &re_latest, &re_stats](){
                char req[4096];
                ssize_t R = read(cl, req, sizeof(req)-1);
                if(R<=0){close(cl);return;}
                req[R] = 0;
                std::istringstream reqs(req);
                std::string method, path, ver;
                reqs >> method >> path >> ver;
                std::smatch M;
                std::string responce;
                if (method != "GET"){
                   response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
                   write(cl,responce.c_str(),responce.size());
                   close(cl);
                   return;
                }

                std::lock_guard<std::mutex> lk(_devices_mutex_);
                if (std::regex_match(path,M,re_latest)) {
                    int id = stoi(m[1].str());
                    if (devices.find((uint8_t)id) == devices.end() || devices[(uint8_t)id].count == 0) {
                        std::string body = "{\"error\": \"no data for device\"}";
                        std::ostringstream hdr;
                        hdr << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: " << body.size() << "\r\n\r\n";
                        responce = hdr.str()+body;
                        write(cl,responce.c_str(),responce.size());
                        close(cl);
                        return;
                    }
                    DeviceData &D = devices[(uint8_t)id];
                    int idx = (d.head - 1 + RING_SIZE) % RING_SIZE;
                    Sample S = d.buffer[idx];
                    std::ostringstream body;
                    body << "{\"device_id\": " << id << ", \"value\": " << std::fixed << std::setprecision(6) << s.value
                    << ", \"timestamp\": " << s.ts << "}";
                    std::string body_str = body.str();
                    std::ostringstream hdr;
                    hdr << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << body_s.size() << "\r\n\r\n";
                    response = hdr.str() + body_s;
                    write(cl,responce.c_str(),responce.size());
                    close(cl);
                    return;
                } else if (std::regex_match(path,M,re_stats)){
                    int id = stoi(M[1].str());
                    if (devices.find((uint8_t)id) == devices.end() || devices[(uint8_t)id].count == 0) {
                        std::string body = "{\"error\": \"no data for device\"}";
                        std::ostringstream hdr;
                        hdr << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: " << body.size() << "\r\n\r\n";
                        response = hdr.str() + body;
                        write(cl, response.c_str(), response.size());
                        close(cl);
                        return;
                    }
                    DeviceData &D = devices[(uint8_t)id];
                    double minv = {0}; maxv = {0}; sum = {0};
                    int cnt = d.count;
                    for (int i = 0; i < cnt; ++i) {
                        int idx = (D.head - 1 - i + RING_SIZE) % RING_SIZE;
                        double v = D.buffer[idx].value;
                        if (i == 0 || v < minv) minv = v;
                        if (i == 0 || v> maxv) maxv = v;
                        sum += v;
                    }
                    double avg = sum/cnt;
                    std::ostringstream body;
                    body << "{\"device_id\": " << id << ", \"min\": " << std::fixed << std::setprecision(6) << minv
                    << ", \"max\": " << maxv << ", \"avg\": " << avg << ", \"count\": " << cnt << "}";
                    std::string body_s = body.str();
                    std::ostringstream hdr;
                    hdr << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << body_s.size() << "\r\n\r\n";
                    response = hdr.str() + body_s;
                    write(cl, response.c_str(), response.size());
                    close(cl);
                    return;          
                } else {
                    std::string body = "{\"error\": \"not found\"}";
                    std::ostringstream hdr;
                    hdr << "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: " << body.size() << "\r\n\r\n";
                    response = hdr.str() + body;
                    write(cl, response.c_str(), response.size());
                    close(cl);
                    return;
                }
        }).detach();
    }
    close(SRV);
}