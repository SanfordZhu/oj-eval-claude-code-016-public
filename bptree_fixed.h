#ifndef BPTREE_FIXED_H
#define BPTREE_FIXED_H

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

const int BPT_ORDER = 50;  // Reduced order for smaller nodes
const int MAX_KEYS = BPT_ORDER - 1;
const int MIN_KEYS = (BPT_ORDER - 1) / 2;
const int KEY_SIZE = 65;  // Max 64 bytes + null terminator

struct BPTNodeFixed {
    bool is_leaf;
    int key_count;
    int parent;
    int next;  // For leaf nodes

    // Fixed-size storage
    char keys[MAX_KEYS][KEY_SIZE];
    int values[MAX_KEYS];
    int children[MAX_KEYS + 1];

    BPTNodeFixed() : is_leaf(true), key_count(0), parent(-1), next(-1) {
        for (int i = 0; i <= MAX_KEYS; i++) {
            children[i] = -1;
            if (i < MAX_KEYS) {
                values[i] = 0;
                memset(keys[i], 0, KEY_SIZE);
            }
        }
    }
};

class BPTreeFixed {
private:
    std::string filename;
    std::fstream file;
    int root_pos;
    int node_count;

    int allocate_node();
    void write_node(int pos, const BPTNodeFixed& node);
    BPTNodeFixed read_node(int pos);
    void split_leaf(int leaf_pos, BPTNodeFixed& leaf);
    void split_internal(int int_pos, BPTNodeFixed& internal);
    void insert_in_parent(int left_pos, const std::string& key, int right_pos);
    int find_key_index(const BPTNodeFixed& node, const std::string& key);

public:
    BPTreeFixed(const std::string& filename);
    ~BPTreeFixed();

    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);
};

#endif