#pragma once 

#include "iterator.h"
#include "memory.h"
#include "functional.h"
#include <functional>
#include "util.h"
#include "exceptdef.h"
#include <initializer_list>
#include <iterator>

namespace mystl
{
template <class T> struct node;
template <class T> struct base;

template <class T> struct base {
  typedef node<T>* node_ptr;
  typedef base<T>* base_ptr;

  base_ptr prev = nullptr;
  base_ptr next = nullptr;

  base() = default;

  base_ptr self() { return this; }
  node_ptr as_node() { return static_cast<node_ptr>(this); }

  void unlink() {
    prev = self();
    next = self();
  }
};

template <class T> struct node : public base<T> {
  typedef base<T>* base_ptr;
  typedef node<T>* node_ptr;
  T data;
  
  base_ptr as_base() {return static_cast<base_ptr>(this);}
  
  node() = default;
  node(const T& value) : data(value) {}
  node(T&& value) : data(mystl::move(value)) {}
  
  T& value() { return data; }
  const T& value() const { return data; }
};

template<class T> 
struct list_iterator
{
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T                               value_type;
    typedef ptrdiff_t                       difference_type;
    typedef T*                              pointer;
    typedef T&                              reference;

    typedef base<T>* base_ptr;
    typedef node<T>* node_ptr;
    typedef list_iterator<T> self;

    base_ptr base_ = nullptr;
    list_iterator() = default;

    list_iterator(base_ptr x) : base_(x) {}
    list_iterator(node_ptr x) : base_(x->as_base()) {}

    T& operator*() const { return base_->as_node()->value(); }
    T* operator->() const { return &(operator*()); }

    self& operator++() {
        base_ = base_->next;
        return *this;
    }

    self operator++(int) {
        self temp = *this;
        base_ = base_->next;
        return temp;
    }

    self& operator--() {
        base_ = base_->prev;
        return *this;
    }

    self operator--(int) {
        self temp = *this;
        base_ = base_->prev;
        return temp;
    }

    bool operator==(const self &rhs) const { return base_ == rhs.base_; }
    bool operator!=(const self &rhs) const { return base_ != rhs.base_; }
};

template<class T>
struct const_list_iterator
{
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef T                               value_type;
  typedef ptrdiff_t                       difference_type;
  typedef const T*                        pointer;
  typedef const T&                        reference;

  typedef base<T>* base_ptr;
  typedef node<T>* node_ptr;
  typedef const_list_iterator<T> self;

  base_ptr base_ = nullptr;

  const_list_iterator() = default;
  const_list_iterator(base_ptr x) : base_(x) {}
  const_list_iterator(node_ptr x) : base_(x->as_base()) {}
  const_list_iterator(const list_iterator<T> &rhs) : base_(rhs.base_) {}

  const T& operator*() const { return base_->as_node()->value(); }
  const T* operator->() const { return &(operator*()); }

  self& operator++() {
    base_ = base_->next;
    return *this;
  }

  self operator++(int) {
    self temp = *this;
    base_ = base_->next;
    return temp;
  }

  self& operator--() {
    base_ = base_->prev;
    return *this;
  }

  self operator--(int) {
    self temp = *this;
    base_ = base_->prev;
    return temp;
  }

  bool operator==(const self &rhs) const { return base_ == rhs.base_; }
  bool operator!=(const self &rhs) const { return base_ != rhs.base_; }
};



template <class T>
class list 
{
public:
    typedef base<T>*                    base_ptr;
    typedef node<T>*                    node_ptr;
    typedef const_list_iterator<T>      const_iterator;
    typedef list_iterator<T>            iterator;
    typedef size_t                      size_type;
    typedef T                           value_type;
    typedef T&                          reference;
    typedef const T&                    const_reference;
    typedef std::reverse_iterator<iterator>       reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
private:
    base_ptr base_ = nullptr;
    size_type sz = 0;
public:
    list();
    list(size_type size, const T& value); 
    list(size_type size) ;
    list(T* first, T* last);
    list(const list& t);
    list(list&& t);
    list(std::initializer_list<T> t);

    list& operator=(const list& t) {
        if (this != &t) {
            list tmp(t);
            swap(tmp);
        }
        return *this;
    }
    
    list& operator=(list&& t) {
        if (this != &t) {
            clear();
            delete base_;

            base_ = t.base_;
            sz = t.sz;

            t.base_ = new base<T>;
            t.base_->unlink();
            t.sz = 0;
        }
        return *this;
    }

    list& operator=(std::initializer_list<T> t) {
        list tmp(t); swap(tmp);
        return *this;
    }

    ~list() {
        clear();
        delete base_;
        base_ = nullptr;
        sz = 0;
    }
public:
    iterator begin() { return iterator(base_->next); }
    const_iterator begin() const { return const_iterator(base_->next); }
    iterator end() { return iterator(base_); }
    const_iterator end() const { return const_iterator(base_); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    size_type size() const { return sz; }
    size_type max_size() const { return static_cast<size_type>(-1); }
    bool empty() const { return sz == 0; }

  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }
  reference back() { return *(--end()); }
  const_reference back() const { return *(--end()); }

    void assign(size_type count, const T &value);
    void assign(T* first, T* last);
    void assign(std::initializer_list<T> ilist);
    void push_back(const T& value);
    void push_front(const T& value);
    void pop_back();
    void pop_front();
    void resize(size_type count);
    void resize(size_type count, const T& value);
    void splice(const_iterator pos, list& other);
    void splice(const_iterator pos, list& other, const_iterator it);
    void splice(const_iterator pos, list& other, const_iterator first,
                const_iterator last);
    void remove(const T &value);
    template <class func> void remove_if(func f);
    void sort();
    template <class func> void sort(func f);
    void merge(list &other);
    template <class func> void merge(list &other, func f);
    void reverse();
    void clear();
    void swap(list &other);

    iterator insert(const_iterator pos, const T& value);
    iterator insert(const_iterator pos, size_type count, const T& value);
    iterator insert(const_iterator pos, T* first, T* last);
    iterator emplace(const_iterator pos, const T& value);
    iterator emplace_front(const T& value);
    iterator emplace_back(const T& value);
    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator last);
    iterator unique();
    template <class func> iterator unique(func f);

private:
    void linknx(base_ptr cur, base_ptr news);
    void linkpr(base_ptr cur, base_ptr news);
    void ulinks(base_ptr eras);
    template<class func>
    void merge_sort(iterator l, iterator r, size_type n, func& f);
};

template<class T>
list<T>::list() {
    base_ = new base<T>;
    base_->unlink(); 
    sz = 0;
};

template<class T>
list<T>::list(size_type size) : list(size, value_type()) {}

template<class T>
list<T>::list(size_type n, const value_type& value) : list(){
    while(n != 0) {
        push_back(value);
        -- n;
    }
}

template <class T>
list<T>::list(T* begins, T* ends) : list(){
    for(auto it = begins; it != ends; it ++) {
        push_back(*it);
    }
}

template<class T>
list<T>::list(const list& t) : list() {
    for (auto it = t.begin(); it != t.end(); ++it) {
        push_back(*it);
    }
}

template<class T>
list<T>::list(list&& t) : base_(t.base_), sz(t.sz) {
    t.base_ = new base<T>;
    t.base_->unlink();
    t.sz = 0;
}

template<class T>
list<T>::list(std::initializer_list<T> t) :list() {
    for(auto it = t.begin(); it != t.end(); it ++) {
        push_back(*it);
    }
}

template <class T>
void list<T>::linknx(base_ptr cur, base_ptr news) {
    news->next = cur->next;
    cur->next->prev = news;
    news->prev = cur;
    cur->next = news;
}

template<class T>
void list<T>::linkpr(base_ptr cur, base_ptr news) {
    news->prev = cur->prev;
    cur->prev->next = news;
    news->next = cur;
    cur->prev = news;
}

template<class T>
void list<T>::ulinks(base_ptr eras) {
    base_ptr cur = eras->prev;
    eras->next->prev = cur;
    cur->next = eras->next;
    delete eras->as_node();
}

template<class T>
void list<T>::assign(T* begins, T* ends) {
    clear();
    for(auto it = begins; it != ends; it ++) {
        push_back(*it);
    }
}

template <class T>
void list<T>::assign(size_type count, const T& value) {
    clear();
    while(count != 0) {
        push_back(value);
        -- count;
    }
}

template<class T>
void list<T>::assign(std::initializer_list<T> ilist) {
    clear();
    for(auto it = ilist.begin(); it != ilist.end(); it ++) {
        push_back(*it);
    }
}

template<class T> 
void list<T>::push_front(const T& value) {
    node_ptr news = new node<T>(value);
    linknx(base_, news->as_base());
    ++ sz;
}

template<class T> 
void list<T>::push_back(const T &value) {
    node_ptr news = new node<T>(value);
    linkpr(base_, news->as_base());
    ++ sz;
}

template<class T>
void list<T>::pop_front() {
    if(sz == 0) throw("no element can be poped");       
    ulinks(base_->next);
    -- sz;
}

template<class T> 
void list<T>::pop_back() {
    if(sz == 0) throw("no element can be poped");       
    ulinks(base_->prev);
    -- sz;
}

template<class T>
void list<T>::resize(size_type count) {
    resize(count, value_type());
}

template<class T>
void list<T>::resize(size_type count, const T &value) {
    while(sz > count) pop_back();
    while(sz < count) push_back(value);
}

template<class T>
void list<T>::splice(const_iterator pos, list<T> &other) {
    if(this == &other || other.empty()) return ;

    base_ptr cur = pos.base_;
    base_ptr fst = other.base_->next;
    base_ptr lst = other.base_->prev;

    fst->prev = cur->prev;
    cur->prev->next = fst;
    lst->next = cur;
    cur->prev = lst;

    sz += other.sz;
    other.sz = 0;
    other.base_->unlink();
}

template<class T>
void list<T>::splice(const_iterator pos, list<T> &other, const_iterator first, const_iterator last) {
    if(first == last) return ;
    if (this == &other) {
        for (auto it = first; it != last; ++it) {
            if (it.base_ == pos.base_) return;
        }
    } else if (other.empty()) {
        return;
    }

    size_type n = 0;
    for (auto it = first; it != last; ++it) {
        ++n;
    }

    base_ptr cur = pos.base_;
    base_ptr fst = first.base_;
    base_ptr lst = last.base_->prev;

    fst->prev->next = lst->next;
    lst->next->prev = fst->prev;
    
    fst->prev = cur->prev;
    cur->prev->next = fst;
    lst->next = cur;
    cur->prev = lst;

    if(this != &other) {
        sz += n;
        other.sz -= n;
    }
}

template<class T>
void list<T>::splice(const_iterator pos, list<T>& other, const_iterator it) {
    if (it == other.end()) return;
    if (pos.base_ == it.base_ || pos.base_ == it.base_->next) return;

    base_ptr cur = pos.base_;
    base_ptr node = it.base_;

    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->prev = cur->prev;
    cur->prev->next = node;
    node->next = cur;
    cur->prev = node;

    if (this != &other) {
        ++sz;
        --other.sz;
    }
}

template<class T>
void list<T>::remove(const T &value) {
    auto it = begin();
    while(it != end()) {
        if(*it == value) it = erase(it);
        else ++ it;
    }
}

template<class T> 
template<class func>
void list<T>::remove_if(func f) {
    auto it = begin();
    while(it != end()) {
        if(f(*it)) it = erase(it);
        else ++ it;
    }
}

template<class T>
void list<T>::merge(list<T>& t) {
    merge(t, [](const T& a, const T& b) { return a < b; });
}

template<class T>
template<class func>
void list<T>::merge(list<T>& t, func f) {
    if(this == &t || t.empty()) return ;
    
    auto it1 = begin();
    auto it2 = t.begin();

    while(it1 != end() && it2 != t.end()) {
        if(f(*it2, *it1)) {
            auto nx = it2; ++ nx;
            splice(it1, t, it2);
            it2 = nx;
        }else {
            ++ it1;
        }
    }

    while(it2 != t.end()) {
        auto nx = it2; ++nx;
        splice(it1, t, it2);
        it2 = nx;
    }
}

template<class T>
void list<T>::reverse() {
    base_ptr cur = base_;
    do {
        base_ptr tmp = cur->next;
        cur->next = cur->prev;
        cur->prev = tmp;
        cur = tmp;
    }while(cur != base_);
}

template<class T>
void list<T>::clear() {
    while(!empty()) {
        pop_front();
    }
}

template<class T> 
void list<T>::swap(list<T> &t) {
    base_ptr tmp = base_;
    base_ = t.base_;
    t.base_ = tmp;

    size_type tmp_sz = sz;
    sz = t.sz;
    t.sz = tmp_sz;
}

template <class T>
auto list<T>::insert(const_iterator pos, const T& value) -> iterator {
    node_ptr news = new node<T>(value);
    linkpr(pos.base_, news->as_base());
    ++sz;
    return iterator(news);
}

template <class T>
auto list<T>::insert(const_iterator pos, size_type cnt, const T& value) -> iterator {
    if (cnt == 0) return iterator(pos.base_);

    iterator ret = end();
    bool first_inserted = false;
    while (cnt--) {
        iterator cur = insert(pos, value);
        if (!first_inserted) {
            ret = cur;
            first_inserted = true;
        }
    }
    return ret;
}

template<class T>
auto list<T>::insert(const_iterator pos, T* first, T* last) -> iterator {
    if (first == last) return iterator(pos.base_);

    iterator ret = end();
    bool first_inserted = false;
    for (auto it = first; it != last; ++it) {
        iterator cur = insert(pos, *it);
        if (!first_inserted) {
            ret = cur;
            first_inserted = true;
        }
    }
    return ret;
}

template<class T>
auto list<T>::emplace(const_iterator pos, const T &value) ->iterator{
    return insert(pos, value);
}

template<class T>
auto list<T>::emplace_back(const T &value) ->iterator {
    return insert(end(), value);
}

template<class T>
auto list<T>::emplace_front(const T &value) ->iterator {
    return insert(begin(), value);
}


template <class T>
auto list<T>::erase(const_iterator pos) -> iterator {
    if (pos == end())
        throw("invalid place");

    base_ptr nx = pos.base_->next;
    ulinks(pos.base_);
    --sz;
    return iterator(nx);
}

template<class T>
auto list<T>::erase(const_iterator begins, const_iterator ends) -> iterator {
    iterator ret(begins.base_);
    while (const_iterator(ret) != ends) {
        ret = erase(const_iterator(ret));
    }
    return ret;
}

template<class T>
auto list<T>::unique() -> iterator {
    iterator cur = begin();
    iterator nx = cur;
    ++ nx;
    while(nx != end()) {
        if((*cur) == *nx) nx = erase(nx);
        else {
            cur = nx; ++ nx;
        }
    }
    return end();
}

template<class T>
template<class func>
auto list<T>::unique(func f) -> iterator {
    iterator cur = begin();
    iterator nx = cur; ++ nx;
    while(nx != end()) {
        if(f(*cur, *nx)) nx = erase(nx);
    else { cur = nx, ++ nx; }
    }
    return end();
}

template<class T>
void list<T>::sort() {
    sort([](const T& a, const T& b) { return a < b; });
}

template<class T>
template<class func>
void list<T>::sort(func f) {
    merge_sort(begin(), end(), sz, f);
}

template<class T>
template<class func>
void list<T>::merge_sort(iterator l, iterator r, size_type n, func& f) {
    if (n <= 1) return;

    auto mid = l;
    for (size_type i = 0; i < n / 2; ++i) ++mid;

    merge_sort(l, mid, n / 2, f);
    merge_sort(mid, r, n - n / 2, f);

    auto left = l;
    auto right = mid;
    auto border = mid;

    while (left != border && right != r) {
        if (!f(*right, *left)) {
            ++left;
        } else {
            auto tmp = right;
            ++right;
            if (tmp == border) {
                border = right;
            }
            splice(left, *this, tmp);
        }
    }


}
}
