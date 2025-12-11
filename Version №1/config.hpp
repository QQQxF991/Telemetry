#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>

class Config {
private:
    std::unordered_map<std::string, std::string> params;
    
public:
    Config() = default;
    
    bool parse_args(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.substr(0, 2) == "--") {
                size_t eq_pos = arg.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = arg.substr(2, eq_pos - 2);
                    std::string value = arg.substr(eq_pos + 1);
                    params[key] = value;
                } else {
                    
                    params[arg.substr(2)] = "true";
                }
            } else if (arg[0] == '-') {
                
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    params[arg.substr(1)] = argv[++i];
                } else {
                    params[arg.substr(1)] = "true";
                }
            }
        }
        return true;
    }

    std::string get_string(const std::string& key, const std::string& default_val = "") const {
        auto it = params.find(key);
        return it != params.end() ? it->second : default_val;
    }
    
    int get_int(const std::string& key, int default_val = 0) const {
        auto it = params.find(key);
        if (it != params.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return default_val;
            }
        }
        return default_val;
    }
    
    bool get_bool(const std::string& key, bool default_val = false) const {
        auto it = params.find(key);
        if (it != params.end()) {
            std::string val = it->second;
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            return val == "true" || val == "1" || val == "yes";
        }
        return default_val;
    }
    
    double get_double(const std::string& key, double default_val = 0.0) const {
        auto it = params.find(key);
        if (it != params.end()) {
            try {
                return std::stod(it->second);
            } catch (...) {
                return default_val;
            }
        }
        return default_val;
    }
    
    bool has(const std::string& key) const {
        return params.find(key) != params.end();
    }
    
    void set(const std::string& key, const std::string& value) {
        params[key] = value;
    }
};