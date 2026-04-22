#include "bpt.h"
#include <iostream>
#include <cstring>

BPTree::BPTree(const std::string& filename) : filename(filename), root_offset(-1), node_count(0) {
    std::ifstream check_file(filename);
    bool file_exists = check_file.good();
    check_file.close();

    if (!file_exists) {
        // Create new file
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
    }

    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file_exists) {
        // Initialize header
        root_offset = -1;
        node_count = 0;
        file.write(reinterpret_cast<const char*>(&root_offset), sizeof(root_offset));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    } else {
        // Read header
        file.read(reinterpret_cast<char*>(&root_offset), sizeof(root_offset));
        file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    }
}

BPTree::~BPTree() {
    if (file.is_open()) {
        // Update header
        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&root_offset), sizeof(root_offset));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        file.close();
    }
}

int BPTree::allocate_node() {
    int offset = sizeof(root_offset) + sizeof(node_count) + node_count * sizeof(BPTNode);
    node_count++;
    return offset;
}

void BPTree::write_node(int offset, const BPTNode& node) {
    file.seekp(offset);
    file.write(reinterpret_cast<const char*>(&node), sizeof(BPTNode));
}

BPTNode BPTree::read_node(int offset) {
    BPTNode node;
    file.seekg(offset);
    file.read(reinterpret_cast<char*>(&node), sizeof(BPTNode));
    return node;
}

int BPTree::find_key(const BPTNode& node, const std::string& key) {
    int idx = 0;
    while (idx < node.key_count && node.keys[idx] < key) {
        idx++;
    }
    return idx;
}

void BPTree::insert(const std::string& key, int value) {
    if (root_offset == -1) {
        // Empty tree, create root
        BPTNode root;
        root.is_leaf = true;
        root.key_count = 1;
        root.keys[0] = key;
        root.values[0] = value;
        root_offset = allocate_node();
        write_node(root_offset, root);
        return;
    }

    // Find the appropriate leaf
    int current_offset = root_offset;
    BPTNode current = read_node(current_offset);

    while (!current.is_leaf) {
        int idx = find_key(current, key);
        current_offset = current.child_offsets[idx];
        current = read_node(current_offset);
    }

    // Check if key-value pair already exists
    int idx = find_key(current, key);
    if (idx < current.key_count && current.keys[idx] == key) {
        // Key exists, check if value already exists
        for (int i = idx; i < current.key_count && current.keys[i] == key; i++) {
            if (current.values[i] == value) {
                return;  // Duplicate value, don't insert
            }
        }
    }

    // Insert into leaf
    idx = find_key(current, key);
    for (int i = current.key_count; i > idx; i--) {
        current.keys[i] = current.keys[i-1];
        current.values[i] = current.values[i-1];
    }
    current.keys[idx] = key;
    current.values[idx] = value;
    current.key_count++;

    if (current.key_count == MAX_KEYS) {
        // Split the leaf
        split_leaf(current_offset, current);
    } else {
        write_node(current_offset, current);
    }
}

void BPTree::split_leaf(int leaf_offset, BPTNode& leaf) {
    BPTNode new_leaf;
    new_leaf.is_leaf = true;
    new_leaf.parent_offset = leaf.parent_offset;

    // Move half the keys to new leaf
    int split_idx = MIN_KEYS;
    new_leaf.key_count = leaf.key_count - split_idx;
    for (int i = 0; i < new_leaf.key_count; i++) {
        new_leaf.keys[i] = leaf.keys[split_idx + i];
        new_leaf.values[i] = leaf.values[split_idx + i];
    }
    leaf.key_count = split_idx;

    // Update next leaf pointers
    new_leaf.next_leaf_offset = leaf.next_leaf_offset;
    int new_leaf_offset = allocate_node();
    leaf.next_leaf_offset = new_leaf_offset;

    // Write nodes
    write_node(leaf_offset, leaf);
    write_node(new_leaf_offset, new_leaf);

    // Insert new key in parent
    insert_parent(leaf_offset, new_leaf.keys[0], new_leaf_offset);
}

void BPTree::split_internal(int node_offset, BPTNode& node) {
    BPTNode new_node;
    new_node.is_leaf = false;
    new_node.parent_offset = node.parent_offset;

    int split_idx = MIN_KEYS;
    new_node.key_count = node.key_count - split_idx - 1;

    // Move keys and children
    for (int i = 0; i < new_node.key_count; i++) {
        new_node.keys[i] = node.keys[split_idx + i + 1];
        new_node.child_offsets[i] = node.child_offsets[split_idx + i + 1];
    }
    new_node.child_offsets[new_node.key_count] = node.child_offsets[node.key_count];

    node.key_count = split_idx;

    // Write nodes
    write_node(node_offset, node);
    int new_node_offset = allocate_node();
    write_node(new_node_offset, new_node);

    // Update parent for moved children
    for (int i = 0; i <= new_node.key_count; i++) {
        if (new_node.child_offsets[i] != -1) {
            BPTNode child = read_node(new_node.child_offsets[i]);
            child.parent_offset = new_node_offset;
            write_node(new_node.child_offsets[i], child);
        }
    }

    // Insert middle key in parent
    insert_parent(node_offset, node.keys[split_idx], new_node_offset);
}

void BPTree::insert_parent(int left_offset, const std::string& key, int right_offset) {
    int left_parent = -1;
    if (left_offset != -1) {
        BPTNode left = read_node(left_offset);
        left_parent = left.parent_offset;
    }

    // Case: new root
    if (left_offset == root_offset && left_parent == -1) {
        BPTNode root;
        root.is_leaf = false;
        root.key_count = 1;
        root.keys[0] = key;
        root.child_offsets[0] = left_offset;
        root.child_offsets[1] = right_offset;

        int root_offset_new = allocate_node();
        write_node(root_offset_new, root);

        // Update children's parent
        if (left_offset != -1) {
            BPTNode left = read_node(left_offset);
            left.parent_offset = root_offset_new;
            write_node(left_offset, left);
        }
        if (right_offset != -1) {
            BPTNode right = read_node(right_offset);
            right.parent_offset = root_offset_new;
            write_node(right_offset, right);
        }

        root_offset = root_offset_new;
        return;
    }

    // Find parent node
    int parent_offset = left_parent;
    BPTNode parent = read_node(parent_offset);

    // Find position in parent
    int idx = 0;
    while (idx < parent.key_count && parent.keys[idx] < key) {
        idx++;
    }

    // Shift keys and children
    for (int i = parent.key_count; i > idx; i--) {
        parent.keys[i] = parent.keys[i-1];
        parent.child_offsets[i+1] = parent.child_offsets[i];
    }
    parent.keys[idx] = key;
    parent.child_offsets[idx+1] = right_offset;
    parent.key_count++;

    if (parent.key_count == MAX_KEYS) {
        split_internal(parent_offset, parent);
    } else {
        write_node(parent_offset, parent);
    }
}

std::vector<int> BPTree::find(const std::string& key) {
    std::vector<int> result;

    if (root_offset == -1) {
        return result;
    }

    // Find the leaf
    int current_offset = root_offset;
    BPTNode current = read_node(current_offset);

    while (!current.is_leaf) {
        int idx = find_key(current, key);
        current_offset = current.child_offsets[idx];
        current = read_node(current_offset);
    }

    // Search in leaf and subsequent leaves
    bool found = false;
    while (current_offset != -1) {
        for (int i = 0; i < current.key_count; i++) {
            if (current.keys[i] == key) {
                result.push_back(current.values[i]);
                found = true;
            } else if (found && current.keys[i] != key) {
                // We've passed all occurrences of the key
                std::sort(result.begin(), result.end());
                return result;
            }
        }

        if (found) {
            // Check next leaf if it might contain the same key
            if (current.next_leaf_offset != -1) {
                BPTNode next_leaf = read_node(current.next_leaf_offset);
                if (next_leaf.key_count > 0 && next_leaf.keys[0] == key) {
                    current_offset = current.next_leaf_offset;
                    current = next_leaf;
                    continue;
                }
            }
            break;
        }

        current_offset = -1;
    }

    std::sort(result.begin(), result.end());
    return result;
}

void BPTree::remove(const std::string& key, int value) {
    if (root_offset == -1) {
        return;
    }

    // Find the leaf containing the key-value pair
    int current_offset = root_offset;
    BPTNode current = read_node(current_offset);

    while (!current.is_leaf) {
        int idx = find_key(current, key);
        current_offset = current.child_offsets[idx];
        current = read_node(current_offset);
    }

    // Find the key-value pair
    int leaf_offset = current_offset;
    while (leaf_offset != -1) {
        current = read_node(leaf_offset);
        for (int i = 0; i < current.key_count; i++) {
            if (current.keys[i] == key && current.values[i] == value) {
                remove_from_leaf(leaf_offset, key, value);
                return;
            }
        }
        // Check if we've passed all possible occurrences
        if (current.key_count > 0 && current.keys[current.key_count - 1] > key) {
            break;
        }
        leaf_offset = current.next_leaf_offset;
    }
}

void BPTree::remove_from_leaf(int leaf_offset, const std::string& key, int value) {
    BPTNode leaf = read_node(leaf_offset);

    // Find and remove the key-value pair
    int idx = -1;
    for (int i = 0; i < leaf.key_count; i++) {
        if (leaf.keys[i] == key && leaf.values[i] == value) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        return;  // Not found
    }

    // Shift remaining entries
    for (int i = idx; i < leaf.key_count - 1; i++) {
        leaf.keys[i] = leaf.keys[i+1];
        leaf.values[i] = leaf.values[i+1];
    }
    leaf.key_count--;

    write_node(leaf_offset, leaf);
}