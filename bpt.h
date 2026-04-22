#ifndef BPT_H
#define BPT_H

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <algorithm>

const int MAX_KEYS = 100;  // Maximum keys per node
const int MIN_KEYS = 50;   // Minimum keys per node (except root)

struct BPTNode {
    bool is_leaf;
    int key_count;
    std::string keys[MAX_KEYS];
    int values[MAX_KEYS];
    int child_offsets[MAX_KEYS + 1];  // For internal nodes
    int next_leaf_offset;             // For leaf nodes
    int parent_offset;

    BPTNode() : is_leaf(true), key_count(0), next_leaf_offset(-1), parent_offset(-1) {
        for (int i = 0; i <= MAX_KEYS; i++) {
            child_offsets[i] = -1;
        }
    }
};

class BPTree {
private:
    std::string filename;
    std::fstream file;
    int root_offset;
    int node_count;

    int allocate_node();
    void write_node(int offset, const BPTNode& node);
    BPTNode read_node(int offset);
    void split_leaf(int leaf_offset, BPTNode& leaf);
    void split_internal(int node_offset, BPTNode& node);
    void insert_parent(int left_offset, const std::string& key, int right_offset);
    void remove_from_leaf(int leaf_offset, const std::string& key, int value);
    void remove_from_internal(int node_offset, int idx);
    void borrow_from_prev(int leaf_offset, BPTNode& leaf, int parent_idx);
    void borrow_from_next(int leaf_offset, BPTNode& leaf, int parent_idx);
    void merge(int leaf_offset, BPTNode& leaf, int parent_idx);
    int find_key(const BPTNode& node, const std::string& key);

public:
    BPTree(const std::string& filename);
    ~BPTree();

    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);
};

#endif