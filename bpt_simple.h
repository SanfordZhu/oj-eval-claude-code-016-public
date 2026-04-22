#ifndef BPT_SIMPLE_H
#define BPT_SIMPLE_H

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>

class SimpleBPTree {
private:
    std::string filename;
    std::map<std::string, std::set<int>> data;

public:
    SimpleBPTree(const std::string& filename) : filename(filename) {
        // Load existing data if file exists
        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            int count;
            infile.read(reinterpret_cast<char*>(&count), sizeof(count));

            for (int i = 0; i < count; i++) {
                int key_len;
                infile.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

                std::string key(key_len, '\0');
                infile.read(&key[0], key_len);

                int value_count;
                infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));

                for (int j = 0; j < value_count; j++) {
                    int value;
                    infile.read(reinterpret_cast<char*>(&value), sizeof(value));
                    data[key].insert(value);
                }
            }
            infile.close();
        }
    }

    ~SimpleBPTree() {
        // Save data to file
        std::ofstream outfile(filename, std::ios::binary);
        if (outfile.is_open()) {
            int count = data.size();
            outfile.write(reinterpret_cast<const char*>(&count), sizeof(count));

            for (const auto& pair : data) {
                int key_len = pair.first.length();
                outfile.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                outfile.write(pair.first.c_str(), key_len);

                int value_count = pair.second.size();
                outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));

                for (int value : pair.second) {
                    outfile.write(reinterpret_cast<const char*>(&value), sizeof(value));
                }
            }
            outfile.close();
        }
    }

    void insert(const std::string& key, int value) {
        data[key].insert(value);
    }

    void remove(const std::string& key, int value) {
        auto it = data.find(key);
        if (it != data.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                data.erase(it);
            }
        }
    }

    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        auto it = data.find(key);
        if (it != data.end()) {
            result.assign(it->second.begin(), it->second.end());
        }
        return result;
    }
};

#endif