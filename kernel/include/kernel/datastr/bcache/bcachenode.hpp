#pragma once

template <typename K, typename V>
struct Node{
    V data;
    K key;
    Node* prev;
    Node* next;
	Node() {prev = nullptr; next = nullptr;}
    Node(const K& key, const V& data)
    {
        this->data=data;
        this->key=key;
        prev=nullptr;
        next=nullptr;
    }
};