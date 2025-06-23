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
        std::vector<Key> keys;
        bool is_leaf;
        Node* parent;

        Node(bool leaf = false): is_leaf(leaf), parent(nullptr) {}
        virtual ~Node() = default;
    };

    struct InternalNode : public Node {
        std::vector<Node*> children;

        InternalNode() : Node(false) {}

        ~InternalNode() {
            for (auto child : children) {
                delete child;
            }
            children.clear();  // prevent dangling pointers
        }
    };

    struct LeafNode : public Node {
        std::vector<Value> values;
        LeafNode* next;
        LeafNode* prev;  // 💡 add prev for easier merge

        LeafNode() : Node(true), next(nullptr), prev(nullptr) {}
    };

    Node* root;
    LeafNode* leftmost_leaf;

    LeafNode* find_leaf(const Key& key);
    void insert_in_leaf(LeafNode* leaf, const Key& key, const Value& value);
    void insert_in_parent(Node* left, const Key& key, Node* right);
    Node* split_leaf(LeafNode* leaf);
    Node* split_internal(InternalNode* node);
    void remove_from_leaf(LeafNode* leaf, const Key& key, const Value& value);
    void merge_or_redistribute(Node* node);

    // 💡 New helper declarations
    int min_keys() const;
    int find_child_index(InternalNode* parent, Node* child);
    void merge_nodes(Node* left, Node* right, InternalNode* parent, int separator_idx);
    void redistribute_from_left(Node* node, Node* left_sibling, InternalNode* parent, int node_idx);
    void redistribute_from_right(Node* node, Node* right_sibling, InternalNode* parent, int node_idx);
    void remove_key_from_parent(InternalNode* parent, const Key& key);

public:
    BPlusTree();
    ~BPlusTree();

    void insert(const Key& key, const Value& value);
    void remove(const Key& key, const Value& value);
    std::vector<Value> search(const Key& key);
    std::vector<Value> range_search(const Key& start_key, const Key& end_key);
    bool empty() const { return root == nullptr; }
};
