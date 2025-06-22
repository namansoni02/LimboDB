#include "../include/btree.h"
#include<algorithm>
#include<iostream>

template<typename Key, typename Value>
BPlusTree<Key, Value>::BPlusTree() : root(nullptr), leftmost_leaf(nullptr) {}

template<typename Key, typename Value>
BPlusTree<Key, Value> :: ~BPlusTree() {
    delete root;
}

template<typename Key, typename Value>
typename BPlusTree<Key, Value>::LeafNode* BPlusTree<Key, Value> ::find_leaf(const Key& key){
    if(!root) return nullptr;

    Node* current = root;
    while(!current->is_leaf){
        InternalNode* internal = static_cast<InternalNode*>(current);
        int i = 0;
        while(i < current->keys.size() && key >= current->keys[i]) {
            i++;
        }
        current = internal->children[i];
    }
    return static_cast<LeafNode*>(current);
}

template<typename Key, typename Value>
void BPlusTree<Key, Value>::insert(const Key& key, const Value& value) {
    if(!root) {
        root = new LeafNode();
        leftmost_leaf = static_cast<LeafNode*>(root);
    }

    LeafNode* leaf = find_leaf(key);
    if(!leaf) {
        leaf = static_cast<LeafNode*>(root);
    }

    insert_in_leaf(leaf, key, value);
}

template<typename Key, typename Value>
void BPlusTree<Key, Value>::insert_in_leaf(LeafNode* leaf, const Key& key, const Value& value){
    auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    int pos = it - leaf->keys.begin();

    //check if key already exists
    if(it != leaf->keys.end() && *it == key) {
        leaf->values[pos] = value; //update value if key exists
        return;
    }

    //Insert the key and value
    leaf->keys.insert(it, key);
    leaf->values.insert(leaf->values.begin() + pos, value);

    // Check if leaf needs to be split
    if(lead->keys.size() >= ORDER) {
        node* new_node = split_leaf(leaf);
        Key split_key = new_node->keys[0];
        // Insert the new key into the parent
        insert_in_parent(leaf, split_key, new_node);
    }
}

template<typename Key, typename Value>
typename BPlusTree<Key,Value>::Node* BPlusTree<Key, Value>::split_leaf(LeafNode* leaf){
    LeafNode* new_leaf = new LeafNode();
    int mid = ORDER / 2;

    //Move half of the keys and values to the new leaf
    new_leaf->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    new_leaf->values.assign(leaf->values.begin() + mid, leaf->values.end());

    // Update the original lead
    leaf->keys.resize(mid);
    leaf->values.resize(mid);

    // Update next pointers
    new_leaf->next = leaf->next;
    leaf->next = new_leaf;

    return new_leaf;
}

template<typename Key, typename Value>
typename BPlusTree<Key, Value>::Node* BPlusTree<Key, Value>::split_internal(InternalNode* node) {
    InternalNode* new_internal = new InternalNode();
    int mid = ORDER / 2;

    // Move keys and children
    new_internal->keys.assign(node->keys.begin() + mid, node->keys.end());
    new_internal->children.assign(node->children.begin() + mid, node->children.end());

    // Update the parent pointers
    for(auto child : new_internal->children) {
        child->parent = new_internal;
    }

    // Get the middle key to promote
    Key split_key = node->keys[mid];

    // Update the original internal node
    node->keys.resize(mid);
    node->children.resize(mid + 1); // +1 because internal nodes have one more

    return new_internal;
}

template<typename Key, typename Value>
void BPlusTree<Key, Value>::insert_in_parent(Node* left, const Key& key, Node* right) {
    if(left == root){
        // Create a new root
        InternalNode* new_root = new InternalNode();
        new_root->keys.push_back(key);
        new_root->children.push_back(left);
        new_root->children.push_back(right);
        left->parent = new_root;
        right->parent = new_root;
        root = new_root;
        return;
    }

    InternalNode* parent = static_cast<InternalNode*>(left->parent);
    right->parent = parent;

    // Find the position to insert the new key
    auto it = lower_bound(parent->keys.begin(), parent->keys.end(), key);
    int pos = it - parent->keys.begin();

    parent->keys.insert(it, key);
    parent->children.insert(parent->children.begin() + pos + 1, right);

    // Check if the parent needs to be split
    if(parent->keys.size() >= ORDER){
        Node* new_internal = split_internal(parent);
        Key promote_key = parent->keys.back();
        parent->keys.pop_back(); // Remove the promoted key from the parent
        insert_in_parent(parent, promote_key, new_internal);
    }
}

