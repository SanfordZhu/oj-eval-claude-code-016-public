#include "bptree.h"
#include <iostream>

BPTree::BPTree(const std::string& filename) : filename(filename), root_pos(-1), node_count(0) {
    header_size = sizeof(root_pos) + sizeof(node_count);

    // Check if file exists
    std::ifstream check(filename);
    bool exists = check.good();
    check.close();

    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!exists) {
        // Create new file
        file.close();
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        // Write header
        root_pos = -1;
        node_count = 0;
        file.write(reinterpret_cast<const char*>(&root_pos), sizeof(root_pos));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    } else {
        // Read header
        file.read(reinterpret_cast<char*>(&root_pos), sizeof(root_pos));
        file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    }
}

BPTree::~BPTree() {
    if (file.is_open()) {
        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&root_pos), sizeof(root_pos));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        file.close();
    }
}

int BPTree::allocate_node() {
    int pos = header_size + node_count * sizeof(BPTNode);
    node_count++;
    return pos;
}

void BPTree::write_node(int pos, const BPTNode& node) {
    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&node), sizeof(BPTNode));
}

BPTNode BPTree::read_node(int pos) {
    BPTNode node;
    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&node), sizeof(BPTNode));
    return node;
}

int BPTree::find_key_index(const BPTNode& node, const std::string& key) {
    int idx = 0;
    while (idx < node.key_count && node.keys[idx] < key) {
        idx++;
    }
    return idx;
}

void BPTree::insert(const std::string& key, int value) {
    if (root_pos == -1) {
        // Empty tree - create root
        BPTNode root;
        root.is_leaf = true;
        root.key_count = 1;
        root.keys[0] = key;
        root.values[0] = value;
        root.parent = -1;
        root.next = -1;

        root_pos = allocate_node();
        write_node(root_pos, root);
        return;
    }

    // Find appropriate leaf
    int current_pos = root_pos;
    BPTNode current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Check if key-value pair already exists
    int idx = find_key_index(current, key);
    if (idx < current.key_count && current.keys[idx] == key) {
        // Key exists, check if value already exists
        for (int i = idx; i < current.key_count && current.keys[i] == key; i++) {
            if (current.values[i] == value) {
                return;  // Duplicate value
            }
        }
    }

    // Insert into leaf
    idx = find_key_index(current, key);
    for (int i = current.key_count; i > idx; i--) {
        current.keys[i] = current.keys[i-1];
        current.values[i] = current.values[i-1];
    }
    current.keys[idx] = key;
    current.values[idx] = value;
    current.key_count++;

    if (current.key_count == MAX_KEYS) {
        split_leaf(current_pos, current);
    } else {
        write_node(current_pos, current);
    }
}

void BPTree::split_leaf(int leaf_pos, BPTNode& leaf) {
    BPTNode new_leaf;
    new_leaf.is_leaf = true;
    new_leaf.parent = leaf.parent;

    // Split keys
    int split_idx = (leaf.key_count + 1) / 2;
    new_leaf.key_count = leaf.key_count - split_idx;

    for (int i = 0; i < new_leaf.key_count; i++) {
        new_leaf.keys[i] = leaf.keys[split_idx + i];
        new_leaf.values[i] = leaf.values[split_idx + i];
    }

    leaf.key_count = split_idx;

    // Update next pointers
    new_leaf.next = leaf.next;
    leaf.next = allocate_node();

    // Write nodes
    write_node(leaf_pos, leaf);
    write_node(leaf.next, new_leaf);

    // Insert in parent
    insert_in_parent(leaf_pos, new_leaf.keys[0], leaf.next);
}

void BPTree::split_internal(int int_pos, BPTNode& internal) {
    BPTNode new_internal;
    new_internal.is_leaf = false;
    new_internal.parent = internal.parent;

    int split_idx = internal.key_count / 2;
    new_internal.key_count = internal.key_count - split_idx - 1;

    // Move keys and children
    for (int i = 0; i < new_internal.key_count; i++) {
        new_internal.keys[i] = internal.keys[split_idx + i + 1];
        new_internal.children[i] = internal.children[split_idx + i + 1];
    }
    new_internal.children[new_internal.key_count] = internal.children[internal.key_count];

    internal.key_count = split_idx;

    // Write nodes
    write_node(int_pos, internal);
    int new_pos = allocate_node();
    write_node(new_pos, new_internal);

    // Update parent for moved children
    for (int i = 0; i <= new_internal.key_count; i++) {
        if (new_internal.children[i] != -1) {
            BPTNode child = read_node(new_internal.children[i]);
            child.parent = new_pos;
            write_node(new_internal.children[i], child);
        }
    }

    // Insert middle key in parent
    insert_in_parent(int_pos, internal.keys[split_idx], new_pos);
}

void BPTree::insert_in_parent(int left_pos, const std::string& key, int right_pos) {
    if (left_pos == root_pos) {
        // Create new root
        BPTNode root;
        root.is_leaf = false;
        root.key_count = 1;
        root.keys[0] = key;
        root.children[0] = left_pos;
        root.children[1] = right_pos;
        root.parent = -1;

        int new_root_pos = allocate_node();
        write_node(new_root_pos, root);

        // Update children's parent
        BPTNode left = read_node(left_pos);
        left.parent = new_root_pos;
        write_node(left_pos, left);

        BPTNode right = read_node(right_pos);
        right.parent = new_root_pos;
        write_node(right_pos, right);

        root_pos = new_root_pos;
        return;
    }

    // Find parent
    BPTNode left = read_node(left_pos);
    int parent_pos = left.parent;
    BPTNode parent = read_node(parent_pos);

    // Find position in parent
    int idx = find_key_index(parent, key);

    // Shift keys and children
    for (int i = parent.key_count; i > idx; i--) {
        parent.keys[i] = parent.keys[i-1];
        parent.children[i+1] = parent.children[i];
    }
    parent.keys[idx] = key;
    parent.children[idx+1] = right_pos;
    parent.key_count++;

    if (parent.key_count == MAX_KEYS) {
        split_internal(parent_pos, parent);
    } else {
        write_node(parent_pos, parent);
    }
}

std::vector<int> BPTree::find(const std::string& key) {
    std::vector<int> result;

    if (root_pos == -1) {
        return result;
    }

    // Find leaf
    int current_pos = root_pos;
    BPTNode current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Search in leaf and subsequent leaves
    bool found = false;
    while (current_pos != -1) {
        for (int i = 0; i < current.key_count; i++) {
            if (current.keys[i] == key) {
                result.push_back(current.values[i]);
                found = true;
            } else if (found && current.keys[i] != key) {
                std::sort(result.begin(), result.end());
                return result;
            }
        }

        if (found) {
            // Check next leaf
            if (current.next != -1) {
                BPTNode next = read_node(current.next);
                if (next.key_count > 0 && next.keys[0] == key) {
                    current_pos = current.next;
                    current = next;
                    continue;
                }
            }
            break;
        }

        current_pos = -1;
    }

    std::sort(result.begin(), result.end());
    return result;
}

void BPTree::remove(const std::string& key, int value) {
    if (root_pos == -1) {
        return;
    }

    // Find leaf
    int current_pos = root_pos;
    BPTNode current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Find key-value pair
    int leaf_pos = current_pos;
    while (leaf_pos != -1) {
        current = read_node(leaf_pos);
        for (int i = 0; i < current.key_count; i++) {
            if (current.keys[i] == key && current.values[i] == value) {
                remove_from_leaf(leaf_pos, key, value);
                return;
            }
        }
        if (current.key_count > 0 && current.keys[current.key_count - 1] > key) {
            break;
        }
        leaf_pos = current.next;
    }
}

void BPTree::remove_from_leaf(int leaf_pos, const std::string& key, int value) {
    BPTNode leaf = read_node(leaf_pos);

    // Find and remove
    int idx = -1;
    for (int i = 0; i < leaf.key_count; i++) {
        if (leaf.keys[i] == key && leaf.values[i] == value) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        return;
    }

    // Shift
    for (int i = idx; i < leaf.key_count - 1; i++) {
        leaf.keys[i] = leaf.keys[i+1];
        leaf.values[i] = leaf.values[i+1];
    }
    leaf.key_count--;

    write_node(leaf_pos, leaf);
}