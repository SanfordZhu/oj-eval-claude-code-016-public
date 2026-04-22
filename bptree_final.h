#ifndef BPTREE_FINAL_H
#define BPTREE_FINAL_H

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <set>

const int BPT_ORDER = 64;  // Optimal order
const int MAX_KEYS = BPT_ORDER - 1;
const int MIN_KEYS = (BPT_ORDER - 1) / 2;
const int KEY_SIZE = 65;

struct BPTNodeFinal {
    bool is_leaf;
    int key_count;
    int parent;
    int next;

    char keys[MAX_KEYS][KEY_SIZE];
    int values[MAX_KEYS];
    int children[MAX_KEYS + 1];

    BPTNodeFinal() : is_leaf(true), key_count(0), parent(-1), next(-1) {
        for (int i = 0; i <= MAX_KEYS; i++) {
            children[i] = -1;
            if (i < MAX_KEYS) {
                values[i] = 0;
                memset(keys[i], 0, KEY_SIZE);
            }
        }
    }
};

class BPTreeFinal {
private:
    std::string filename;
    std::fstream file;
    int root_pos;
    int node_count;

    int allocate_node();
    void write_node(int pos, const BPTNodeFinal& node);
    BPTNodeFinal read_node(int pos);
    void split_leaf(int leaf_pos, BPTNodeFinal& leaf);
    void split_internal(int int_pos, BPTNodeFinal& internal);
    void insert_in_parent(int left_pos, const std::string& key, int right_pos);
    int find_key_index(const BPTNodeFinal& node, const std::string& key);

public:
    BPTreeFinal(const std::string& filename);
    ~BPTreeFinal();

    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);
};

#endif