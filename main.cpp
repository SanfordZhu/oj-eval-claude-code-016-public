#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "bptree_fixed.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    BPTreeFixed tree("database.db");

    int n;
    std::cin >> n;

    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;

        if (command == "insert") {
            std::string index;
            int value;
            std::cin >> index >> value;
            tree.insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            std::cin >> index >> value;
            tree.remove(index, value);
        } else if (command == "find") {
            std::string index;
            std::cin >> index;
            std::vector<int> values = tree.find(index);

            if (values.empty()) {
                std::cout << "null";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
            }
            std::cout << "\n";
        }
    }

    return 0;
}