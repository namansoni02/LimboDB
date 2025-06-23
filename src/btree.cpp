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

// template<typename Key, typename Value>
// void BPlusTree<Key, Value>::insert_in_leaf(LeafNode* leaf, const Key& key, const Value& value)