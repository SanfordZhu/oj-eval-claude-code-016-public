#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <sstream>

class FileStorage {
private:
    std::map<std::string, std::set<int>> data;
    std::string filename;

    void loadFromFile() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (int i = 0; i < count; i++) {
            int keyLen;
            file.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));

            std::string key(keyLen, '\0');
            file.read(&key[0], keyLen);

            int valueCount;
            file.read(reinterpret_cast<char*>(&valueCount), sizeof(valueCount));

            for (int j = 0; j < valueCount; j++) {
                int value;
                file.read(reinterpret_cast<char*>(&value), sizeof(value));
                data[key].insert(value);
            }
        }
        file.close();
    }

    void saveToFile() {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        int count = data.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& pair : data) {
            int keyLen = pair.first.length();
            file.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
            file.write(pair.first.c_str(), keyLen);

            int valueCount = pair.second.size();
            file.write(reinterpret_cast<const char*>(&valueCount), sizeof(valueCount));

            for (int value : pair.second) {
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
        file.close();
    }

public:
    FileStorage(const std::string& fname) : filename(fname) {
        loadFromFile();
    }

    ~FileStorage() {
        saveToFile();
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

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage("database.db");

    int n;
    std::cin >> n;

    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;

        if (command == "insert") {
            std::string index;
            int value;
            std::cin >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            std::cin >> index >> value;
            storage.remove(index, value);
        } else if (command == "find") {
            std::string index;
            std::cin >> index;
            std::vector<int> values = storage.find(index);

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                bool first = true;
                for (int val : values) {
                    if (!first) std::cout << " ";
                    std::cout << val;
                    first = false;
                }
                std::cout << "\n";
            }
        }
    }

    return 0;
}