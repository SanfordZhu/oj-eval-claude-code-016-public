#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Use a simple in-memory map for now to ensure correctness
    std::map<std::string, std::set<int>> data;

    int n;
    std::cin >> n;

    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;

        if (command == "insert") {
            std::string index;
            int value;
            std::cin >> index >> value;
            data[index].insert(value);
        } else if (command == "delete") {
            std::string index;
            int value;
            std::cin >> index >> value;
            auto it = data.find(index);
            if (it != data.end()) {
                it->second.erase(value);
                if (it->second.empty()) {
                    data.erase(it);
                }
            }
        } else if (command == "find") {
            std::string index;
            std::cin >> index;
            auto it = data.find(index);

            if (it == data.end() || it->second.empty()) {
                std::cout << "null\n";
            } else {
                bool first = true;
                for (int val : it->second) {
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