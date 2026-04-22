#include "bptree_final.h"
#include <iostream>

BPTreeFinal::BPTreeFinal(const std::string& filename) : filename(filename), root_pos(-1), node_count(0) {
    // Check if file exists
    std::ifstream check(filename);
    bool exists = check.good();
    check.close();

    file.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

    if (!exists) {
        // Create new file
        file.close();
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

        // Write header
        int header[2] = {-1, 0};
        file.write(reinterpret_cast<const char*>(header), sizeof(header));
    } else {
        // Read header
        file.seekg(0);
        file.read(reinterpret_cast<char*>(&root_pos), sizeof(root_pos));
        file.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    }
}

BPTreeFinal::~BPTreeFinal() {
    if (file.is_open()) {
        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&root_pos), sizeof(root_pos));
        file.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
        file.close();
    }
}

int BPTreeFinal::allocate_node() {
    int pos = sizeof(int) * 2 + node_count * sizeof(BPTNodeFinal);
    node_count++;
    return pos;
}

void BPTreeFinal::write_node(int pos, const BPTNodeFinal& node) {
    file.seekp(pos);
    file.write(reinterpret_cast<const char*>(&node), sizeof(BPTNodeFinal));
}

BPTNodeFinal BPTreeFinal::read_node(int pos) {
    BPTNodeFinal node;
    file.seekg(pos);
    file.read(reinterpret_cast<char*>(&node), sizeof(BPTNodeFinal));
    return node;
}

int BPTreeFinal::find_key_index(const BPTNodeFinal& node, const std::string& key) {
    int left = 0, right = node.key_count;
    while (left < right) {
        int mid = (left + right) / 2;
        if (std::string(node.keys[mid]) < key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return left;
}

void BPTreeFinal::insert(const std::string& key, int value) {
    if (root_pos == -1) {
        // Empty tree - create root
        BPTNodeFinal root;
        root.is_leaf = true;
        root.key_count = 1;
        strncpy(root.keys[0], key.c_str(), KEY_SIZE - 1);
        root.keys[0][KEY_SIZE - 1] = '\0';
        root.values[0] = value;
        root.parent = -1;
        root.next = -1;

        root_pos = allocate_node();
        write_node(root_pos, root);
        return;
    }

    // Find appropriate leaf
    int current_pos = root_pos;
    BPTNodeFinal current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Check if key-value pair already exists
    int idx = find_key_index(current, key);
    if (idx < current.key_count && std::string(current.keys[idx]) == key) {
        // Key exists, check if value already exists
        for (int i = idx; i < current.key_count && std::string(current.keys[i]) == key; i++) {
            if (current.values[i] == value) {
                return;  // Duplicate value
            }
        }
    }

    // Insert into leaf
    idx = find_key_index(current, key);
    for (int i = current.key_count; i > idx; i--) {
        strncpy(current.keys[i], current.keys[i-1], KEY_SIZE);
        current.values[i] = current.values[i-1];
    }
    strncpy(current.keys[idx], key.c_str(), KEY_SIZE - 1);
    current.keys[idx][KEY_SIZE - 1] = '\0';
    current.values[idx] = value;
    current.key_count++;

    if (current.key_count == MAX_KEYS) {
        split_leaf(current_pos, current);
    } else {
        write_node(current_pos, current);
    }
}

void BPTreeFinal::split_leaf(int leaf_pos, BPTNodeFinal& leaf) {
    BPTNodeFinal new_leaf;
    new_leaf.is_leaf = true;
    new_leaf.parent = leaf.parent;

    // Split keys - use better split point
    int split_idx = leaf.key_count / 2;
    new_leaf.key_count = leaf.key_count - split_idx;

    for (int i = 0; i < new_leaf.key_count; i++) {
        strncpy(new_leaf.keys[i], leaf.keys[split_idx + i], KEY_SIZE);
        new_leaf.values[i] = leaf.values[split_idx + i];
    }

    leaf.key_count = split_idx;

    // Update next pointers
    new_leaf.next = leaf.next;
    int new_leaf_pos = allocate_node();
    leaf.next = new_leaf_pos;

    // Write nodes
    write_node(leaf_pos, leaf);
    write_node(new_leaf_pos, new_leaf);

    // Insert in parent
    insert_in_parent(leaf_pos, new_leaf.keys[0], new_leaf_pos);
}

void BPTreeFinal::split_internal(int int_pos, BPTNodeFinal& internal) {
    BPTNodeFinal new_internal;
    new_internal.is_leaf = false;
    new_internal.parent = internal.parent;

    int split_idx = internal.key_count / 2;
    new_internal.key_count = internal.key_count - split_idx - 1;

    // Move keys and children
    for (int i = 0; i < new_internal.key_count; i++) {
        strncpy(new_internal.keys[i], internal.keys[split_idx + i + 1], KEY_SIZE);
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
            BPTNodeFinal child = read_node(new_internal.children[i]);
            child.parent = new_pos;
            write_node(new_internal.children[i], child);
        }
    }

    // Insert middle key in parent
    insert_in_parent(int_pos, internal.keys[split_idx], new_pos);
}

void BPTreeFinal::insert_in_parent(int left_pos, const std::string& key, int right_pos) {
    if (left_pos == root_pos) {
        // Create new root
        BPTNodeFinal root;
        root.is_leaf = false;
        root.key_count = 1;
        strncpy(root.keys[0], key.c_str(), KEY_SIZE - 1);
        root.keys[0][KEY_SIZE - 1] = '\0';
        root.children[0] = left_pos;
        root.children[1] = right_pos;
        root.parent = -1;

        int new_root_pos = allocate_node();
        write_node(new_root_pos, root);

        // Update children's parent
        BPTNodeFinal left = read_node(left_pos);
        left.parent = new_root_pos;
        write_node(left_pos, left);

        BPTNodeFinal right = read_node(right_pos);
        right.parent = new_root_pos;
        write_node(right_pos, right);

        root_pos = new_root_pos;
        return;
    }

    // Find parent
    BPTNodeFinal left = read_node(left_pos);
    int parent_pos = left.parent;
    BPTNodeFinal parent = read_node(parent_pos);

    // Find position in parent
    int idx = find_key_index(parent, key);

    // Shift keys and children
    for (int i = parent.key_count; i > idx; i--) {
        strncpy(parent.keys[i], parent.keys[i-1], KEY_SIZE);
        parent.children[i+1] = parent.children[i];
    }
    strncpy(parent.keys[idx], key.c_str(), KEY_SIZE - 1);
    parent.keys[idx][KEY_SIZE - 1] = '\0';
    parent.children[idx+1] = right_pos;
    parent.key_count++;

    if (parent.key_count == MAX_KEYS) {
        split_internal(parent_pos, parent);
    } else {
        write_node(parent_pos, parent);
    }
}

std::vector<int> BPTreeFinal::find(const std::string& key) {
    std::vector<int> result;

    if (root_pos == -1) {
        return result;
    }

    // Find leaf
    int current_pos = root_pos;
    BPTNodeFinal current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Collect all values for the key
    int leaf_pos = current_pos;
    while (leaf_pos != -1) {
        current = read_node(leaf_pos);

        // Check each key in this leaf
        for (int i = 0; i < current.key_count; i++) {
            int cmp = std::string(current.keys[i]).compare(key);
            if (cmp == 0) {
                result.push_back(current.values[i]);
            } else if (cmp > 0) {
                // Keys are sorted, so we can stop if we pass the key
                std::sort(result.begin(), result.end());
                return result;
            }
        }

        // Check if we should continue to next leaf
        if (current.key_count > 0) {
            int last_cmp = std::string(current.keys[current.key_count - 1]).compare(key);
            if (last_cmp < 0) {
                // Last key is still less than our key, continue
                leaf_pos = current.next;
            } else {
                // We've passed where our key would be
                break;
            }
        } else {
            break;
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

void BPTreeFinal::remove(const std::string& key, int value) {
    if (root_pos == -1) {
        return;
    }

    // Find leaf
    int current_pos = root_pos;
    BPTNodeFinal current = read_node(current_pos);

    while (!current.is_leaf) {
        int idx = find_key_index(current, key);
        current_pos = current.children[idx];
        current = read_node(current_pos);
    }

    // Find key-value pair
    int leaf_pos = current_pos;
    while (leaf_pos != -1) {
        current = read_node(leaf_pos);

        // Check each key in this leaf
        for (int i = 0; i < current.key_count; i++) {
            int cmp = std::string(current.keys[i]).compare(key);
            if (cmp == 0 && current.values[i] == value) {
                // Found it! Remove from leaf
                for (int j = i; j < current.key_count - 1; j++) {
                    strncpy(current.keys[j], current.keys[j+1], KEY_SIZE);
                    current.values[j] = current.values[j+1];
                }
                current.key_count--;
                write_node(leaf_pos, current);
                return;
            } else if (cmp > 0) {
                // Keys are sorted, so we can stop if we pass the key
                return;
            }
        }

        // Check if we should continue to next leaf
        if (current.key_count > 0) {
            int last_cmp = std::string(current.keys[current.key_count - 1]).compare(key);
            if (last_cmp < 0) {
                // Last key is still less than our key, continue
                leaf_pos = current.next;
            } else {
                // We've passed where our key would be
                break;
            }
        } else {
            break;
        }
    }
}