#pragma once
#include "bcachenode.hpp"
#include "../hashmap/hashmap.hpp"
#include "../pair.hpp"
// From this
// https://medium.com/@ngneha090/lru-least-recently-used-cache-with-implementation-2487e8316443
template<typename K, typename V>
class LRUCache {
	public:
		HashMap<K,Pair<V,Node<K*, V*>*>>mp;
		int cap;
		Node<K*, V*>* head;
		Node<K*, V*>* tail;
		LRUCache(int capacity) {
			cap=capacity;
			head=new Node<K*, V*>();
			tail=new Node<K*, V*>();
			head->next=tail;
			tail->prev=head;
		}
		
		V* get(K& key) {
			Pair<V, Node<K*, V*>*> pair;
			if(!mp.get(key, pair)) //not present in cache
			{
				return nullptr;
			}
			else
			{
				Node<K*, V*>* temp=pair.second;
				remove(temp);
				pair.second=insert(&pair.first, &key);
				return &pair.first;
			}
		}
		
		void put(K& key, V&& value) {
			Pair<V, Node<K*, V*>*> pair;
			if(!mp.get(key, pair))
			{
				if(mp.size()<cap)
				{
					mp.put(key, {move(value), insert(&value, &key)});
				}
				else
				{
					K k=tail->prev->key;
					remove(tail->prev);
					mp.remove(k);
					mp.put(key, {move(value), insert(&value, &key)});
				}
			}
			else
			{
				Node<K*, V*>* temp=pair.second;
				remove(temp);
				mp.put(key, {move(value), insert(&value, &key)});
			}
			
		}
		void remove(Node<K*, V*>* temp)
		{
			temp->prev->next=temp->next;
			temp->next->prev=temp->prev;
			temp->prev=NULL;
			temp->next=NULL;
			delete(temp);
		}
		Node<K*, V*>* insert(V* val, K* key)
		{
			Node<K*, V*>* newnode=new Node(key, val);
			Node<K*, V*>* temp=head->next;
			head->next=newnode;
			newnode->next=temp;
			newnode->prev=head;
			temp->prev=newnode;
			return newnode;
		}
};