#ifndef BPTREE_H
#define BPTREE_H

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

const int BPT_ORDER = 200;  // B+ tree order
const int MAX_KEYS = BPT_ORDER - 1;
const int MIN_KEYS = (BPT_ORDER - 1) / 2;

struct BPTNode {
    bool is_leaf;
    int key_count;
    int parent;
    int next;  // For leaf nodes (next leaf)

    // In a real implementation, these would be dynamically sized
    // For simplicity, we'll use fixed arrays
    std::string keys[MAX_KEYS];
    int values[MAX_KEYS];  // For leaf nodes
    int children[MAX_KEYS + 1];  // For internal nodes

    BPTNode() : is_leaf(true), key_count(0), parent(-1), next(-1) {
        for (int i = 0; i <= MAX_KEYS; i++) {
            children[i] = -1;
            if (i < MAX_KEYS) {
                values[i] = 0;
            }
        }
    }
};

class BPTree {
private:
    std::string filename;
    std::fstream file;
    int root_pos;
    int node_count;
    int header_size;

    int allocate_node();
    void write_node(int pos, const BPTNode& node);
    BPTNode read_node(int pos);
    void split_leaf(int leaf_pos, BPTNode& leaf);
    void split_internal(int int_pos, BPTNode& internal);
    void insert_in_parent(int left_pos, const std::string& key, int right_pos);
    void remove_from_leaf(int leaf_pos, const std::string& key, int value);
    void merge_nodes(int pos, BPTNode& node, int parent_pos, int parent_idx);
    void borrow_from_prev(int pos, BPTNode& node, int parent_pos, int parent_idx);
    void borrow_from_next(int pos, BPTNode& node, int parent_pos, int parent_idx);
    int find_key_index(const BPTNode& node, const std::string& key);

public:
    BPTree(const std::string& filename);
    ~BPTree();

    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);
};

#endif