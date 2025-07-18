#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>

using namespace std;

template<typename Key, typename Value>
class BPlusTree {
private:
    static const int ORDER = 4;

    struct Node {
        vector<Key> keys;
        bool is_leaf;
        Node* parent;

        Node(bool leaf = false): is_leaf(leaf), parent(nullptr){}
        virtual ~Node() = default;
    };

    struct InternalNode: public Node {
        vector<Node*> children;

        InternalNode() : Node(false){}

        ~InternalNode() {
            for(auto child : children) {
                delete child;
            }
        }
    };

    struct LeafNode: public Node {
        vector<Value> values;
        LeafNode* next;

        LeafNode() : Node(true), next(nullptr) {}
    };

    Node* root;
    LeafNode* leftmost_leaf;

    LeafNode* find_leaf(const Key& key);
    void insert_in_leaf(LeafNode* leaf, const Key& key, const Value& value);
    void insert_in_parent(Node* left, const Key& key, Node* right);
    Node* split_leaf(LeafNode* leaf);
    Node* split_internal(InternalNode* Node);
    void remove_from_leaf(LeafNode* leaf, const Key& key, const Value& value);
    void merge_or_redistribute(Node* node);

public:
    BPlusTree();
    ~BPlusTree();

    void insert(const Key& key, const Value& value);
    void remove(const Key& key, const Value& value);
    vector<Value> search(const Key& key);
    vector<Value> range_search(const Key& start_key, const Key& end_key);
    bool empty() const {return root == nullptr; }   
};