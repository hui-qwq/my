#pragma once

#include <cstddef>
#include <exception>
#include <initializer_list>
#include <stdexcept>

#include "allocator.h"
#include "construct.h"
#include "iterator.h"
#include "memory.h"
#include "util.h"
#include "exceptdef.h"
#include "algo.h"
#include "math.h"

namespace mystl {

template<class T>
class vector {
public:
    typedef mystl::allocator<T>                      allocator_type;
    typedef mystl::allocator<T>                      data_allocator;

    typedef T                                        value_type;
    typedef T*                                       pointer;
    typedef const T*                                 const_pointer;
    typedef T&                                       reference;
    typedef const T&                                 const_reference;
    typedef size_t                                   size_type;
    typedef ptrdiff_t                                difference_type;
    typedef T*                                       iterator;
    typedef const T*                                 const_iterator;
    typedef mystl::reverse_iterator<T*>              reverse_iterator;
    typedef mystl::reverse_iterator<const T*>        const_reverse_iterator;

private:
    iterator begin_ = nullptr;
    iterator end_ = nullptr;
    iterator cap_ = nullptr;

public:
    vector() { try_init(); }
    vector(size_t size) { initial(size, value_type()); }
    vector(size_t size, const_reference value) { initial(size, value); }
    vector(iterator begins, iterator ends) { initial_p(begins, ends); }
    vector(const vector<T>& tmp) { copy_initial(tmp); }
    vector(vector<T>&& tmp) noexcept{
        begin_ = tmp.begin_; end_ = tmp.end_; cap_ = tmp.cap_;
        tmp.begin_ = tmp.end_ = tmp.cap_ = nullptr; 
    }
    vector(std::initializer_list<T> list) { list_init(list); }

    vector& operator=(const vector<T>& tmp) { 
        if (this != &tmp) {
            vector tmp_vec(tmp);
            swap(tmp_vec);
        }
        return *this; 
    }
    vector& operator=(vector<T>&& tmp) noexcept { 
        if(&tmp != this) {
            clear();
            if (begin_) {
                data_allocator::deallocate(begin_, cap_ - begin_);
            }
            begin_ = tmp.begin_; end_ = tmp.end_; cap_ = tmp.cap_;
            tmp.begin_ = tmp.end_ = tmp.cap_ = nullptr; 
        }
        return *this;
    }
    vector& operator=(std::initializer_list<T> list) { 
        vector tmp(list);
        swap(tmp);
        return *this; 
    }



    reference operator[](size_t index) { return *(begin_ + index); }
    const_reference operator[](size_type index) const {
        return *(begin_ + index);
    }
    
    reference at(size_t index) {
        THROW_OUT_OF_RANGE_IF(index >= size(), "vector<T>::at() subscript out of range");
        return (*this)[index];
    }
    const_reference at(size_t index) const{
        THROW_OUT_OF_RANGE_IF(index >= size(), "vector<T>::at() subscript out of range");
        return (*this)[index];
    }
    
    ~vector() {
        clear();
        if (begin_) {
            data_allocator::deallocate(begin_, cap_ - begin_);
        }
    }

    iterator begin() { return begin_; }
    iterator end() { return end_; }
    const_iterator begin() const { return begin_; }
    const_iterator end() const { return end_; }
    reverse_iterator rbegin() noexcept {return reverse_iterator(end_);}
    reverse_iterator rend() noexcept {return reverse_iterator(begin_);}
    const_reverse_iterator rbegin() const noexcept {return const_reverse_iterator(end_);}
    const_reverse_iterator rend() const noexcept {return const_reverse_iterator(begin_);}

    reference front() { return *begin_; }
    const_reference front() const { return *begin_; }
    reference back() { return *(end_ - 1); }
    const_reference back() const {return *(end_-1);}

    bool empty() const noexcept { return begin_ == end_; }
    size_type max_size() const noexcept {return size_type(-1)/sizeof(T); }
    size_type size() const noexcept { return static_cast<size_type>(end_ - begin_); }
    size_type capacity() const noexcept { return static_cast<size_type>(cap_ - begin_); }

    pointer data() noexcept { return begin_; }
    const_pointer data() const noexcept { return begin_; }
    
public:
    void assign(iterator begins, iterator ends);
    void assign(size_t size, value_type value);
    void resize(size_t size, value_type value);
    void resize(size_t size);
    void reserve(size_type n);
    void push_back(const T&);
    void swap(vector& other) noexcept;
    void shrink_to_fit();
    void pop_back();
    void reverse();
    void clear();

    iterator emplace(iterator pos, const value_type value);
    iterator emplace_back(const value_type value);
    iterator insert(iterator pos, const value_type value);
    iterator insert(iterator pos, size_t cnt, const value_type value);
    iterator insert(iterator pos, iterator begins, iterator ends);
    iterator erase(iterator pos);
    iterator erase(iterator begins, iterator ends);

private:
    void try_init();
    void initial(size_t size, const value_type& value);
    void initial_p(iterator begins, iterator ends);
    void copy_initial(const vector<T>& tmp);
    void list_init(std::initializer_list<T> list);
};

template<class T> 
void vector<T>::assign(iterator begins, iterator ends) {
    vector<T> tmp(begins, ends);
    swap(tmp);
}

template<class T>
void vector<T>::assign(size_t size, value_type value) {
    vector<T> tmp(size, value);
    swap(tmp);
}

template <class T>
void vector<T>::resize(size_t new_size) {
    resize(new_size, value_type());
}
template <class T>
void vector<T>::resize(size_t new_size, value_type value) {
    if(new_size > size()) {
        if(new_size > capacity()) reserve(new_size);
        while(size() < new_size) {
            data_allocator::construct(end_, value);
            ++ end_;
        }
    } else {
        while(size() > new_size) {
            -- end_;
            data_allocator::destroy(end_);
        }
    }
}

template <class T>
void vector<T>::reserve(size_type n) {
    if (capacity() >= n) return;

    pointer new_begin = data_allocator::allocate(n);
    pointer new_end = new_begin;

    try {
        new_end = mystl::uninitialized_move(begin_, end_, new_begin);
    } catch(...) {
        while(new_end != new_begin) {
            -- new_end;
            data_allocator::destroy(new_end);
        }
        data_allocator::deallocate(new_begin, n);
        throw;
    }
    while (end_ != begin_) {
        --end_;
        data_allocator::destroy(end_);
    }
    if (begin_) {
        data_allocator::deallocate(begin_, cap_ - begin_);
    }

    begin_ = new_begin;
    end_ = new_end;
    cap_ = new_begin + n;
}

template <class T>
void vector<T>::push_back(const T& value) {
    if (end_ == cap_) {
        reserve(capacity() == 0 ? 1 : capacity() * 2);
    }
    data_allocator::construct(end_, value);
    ++end_;
}

template <class T>
void vector<T>::swap(vector<T>& other) noexcept {
    if (this != &other) {
        mystl::swap(begin_, other.begin_);
        mystl::swap(end_, other.end_);
        mystl::swap(cap_, other.cap_);
    }
}

template<class T>
void vector<T>::shrink_to_fit() {
    if (end_ == cap_) return;

    size_type old_size = size();

    if (old_size == 0) {
        if (begin_) {
            data_allocator::deallocate(begin_, cap_ - begin_);
        }
        begin_ = end_ = cap_ = nullptr;
        return;
    }

    pointer new_begin = data_allocator::allocate(old_size);
    pointer new_end = new_begin;

    try {
        new_end = mystl::uninitialized_move(begin_, end_, new_begin);
    } catch (...) {
        while (new_end != new_begin) {
            --new_end;
            data_allocator::destroy(new_end);
        }
        data_allocator::deallocate(new_begin, old_size);
        throw;
    }

    while (end_ != begin_) {
        --end_;
        data_allocator::destroy(end_);
    }
    data_allocator::deallocate(begin_, cap_ - begin_);

    begin_ = new_begin;
    end_ = new_end;
    cap_ = new_begin + old_size;
}

template <class T>
void vector<T>::pop_back() {
    if(!empty()) {
        -- end_;
        data_allocator::destroy(end_);
    }
}

template<class T>
void vector<T>::reverse() {
    if(size() <= 1) return ;
    size_type i = 0, j = size() - 1;
    while(i < j) {
        std::swap((*this)[i], (*this)[j]);
        i ++, j --;
    }
}

template <class T>
void vector<T>::clear() {
    while(end_ != begin_) {
        -- end_;
        data_allocator::destroy(end_);
    }
}



template <class T> 
auto vector<T>::emplace_back(const value_type value) -> iterator {
    return insert(end_, value);
}

template <class T> 
auto vector<T>::emplace(iterator pos, const value_type value) -> iterator {
    return insert(pos, value);
}

template <class T>
auto vector<T>::insert(iterator pos, const value_type value) -> iterator {

    if(pos > end_ || pos < begin_) {
        throw std::out_of_range("out of range");
    }

    size_type idx = pos - begin_;

    if (end_ == cap_) {
        reserve(capacity() == 0 ? 1 : capacity() * 2);
    }
    pos = begin_ + idx;

    if (pos == end_) {
        push_back(value);
        return end_ - 1;
    } else {
        data_allocator::construct(end_, *(end_ - 1));
        ++end_;
        mystl::move_backward(pos, end_ - 1, end_);
        (*this)[idx] = value; 
        return pos;
    }
}

template<class T>
auto vector<T>::insert(iterator pos, size_t cnt, const value_type value) -> iterator {
    if(pos > end_ || pos < begin_) {
        throw std::out_of_range("out of range");
    }
    if(cnt == 0) return pos;

    size_type idx = pos - begin_;

    if(size() + cnt > capacity()) {
        size_type lg2 = log2(size() + cnt) + 1;
        reserve(1ll << lg2);
    }

    pos = begin_ + idx;

    size_type cnt_cp = cnt;
    size_type rollback_cnt = 0;
    size_type cnt_cp2 = cnt, id = idx;
    if(pos == end_) {
        while(cnt_cp --) push_back(value);
    } else {
        auto old_end = end_;
        try {
            while(cnt_cp --) {
                data_allocator::construct(end_, *(end_-1));
                end_ ++;
                rollback_cnt ++;
            }
        } catch(...) {
            while(rollback_cnt > 0) {
                rollback_cnt --;
                end_ --;
                data_allocator::destroy(end_);
            }
            throw;
        }
        mystl::move_backward(pos, old_end, end_);
        while(cnt_cp2 --) {
            (*this)[id] = value; 
            id ++;
        }
    }
    return begin_ + idx;
}

template<class T>
auto vector<T>::insert(iterator pos, iterator begins, iterator ends) -> iterator {
    if (pos < begin_ || pos > end_) {
        throw std::out_of_range("out of range");
    }

    vector<T> tmp(begins, ends);
    if (tmp.empty()) return pos;

    size_type idx = pos - begin_;
    size_type cnt = tmp.size();

    if (size() + cnt > capacity()) {
        size_type lg2 = log2(size() + cnt) + 1;
        reserve(1ll << lg2);
    }

    pos = begin_ + idx;

    size_type cnt_cp = cnt;
    size_type rollback_cnt = 0;
    size_type id = idx;

    if (pos == end_) {
        for (size_type i = 0; i < tmp.size(); ++i) {
            push_back(tmp[i]);
        }
    } else {
        auto old_end = end_;
        try {
            while (cnt_cp--) {
                data_allocator::construct(end_, *(end_ - 1));
                ++end_;
                ++rollback_cnt;
            }
        } catch (...) {
            while (rollback_cnt > 0) {
                --rollback_cnt;
                --end_;
                data_allocator::destroy(end_);
            }
            throw;
        }

        mystl::move_backward(pos, old_end, end_);

        for (size_type i = 0; i < tmp.size(); ++i, ++id) {
            (*this)[id] = tmp[i];
        }
    }

    return begin_ + idx;
}

template <class T>
auto vector<T>::erase(iterator pos) -> iterator {
    if (pos + 1 != end_) {
        mystl::move(pos + 1, end_, pos);
    }
    --end_;
    data_allocator::destroy(end_);
    return pos;
}

template<class T>
auto vector<T>::erase(iterator begins, iterator ends) -> iterator{
    if(begins == ends) return begins;

    iterator new_end = mystl::move(ends, end_, begins);
    while(end_ != new_end) {
        -- end_;
        data_allocator::destroy(end_);
    }
    return begins;

}



// 初始化的完善
template <class T>
void vector<T>::try_init() {
    try {
        begin_ = data_allocator::allocate(16);
        end_ = begin_;
        cap_ = begin_ + 16;
    } catch (...) {
        begin_ = nullptr;
        end_ = nullptr;
        cap_ = nullptr;
        throw;
    }
}

template <class T>
void vector<T>::initial(size_t size, const value_type& value) {
    begin_ = data_allocator::allocate(size + 16);
    end_ = begin_;
    cap_ = begin_ + size + 16;

    try {
        for (size_type i = 0; i < size; ++i) {
            data_allocator::construct(end_, value);
            ++end_;
        }
    } catch (...) {
        while (end_ != begin_) {
            --end_;
            data_allocator::destroy(end_);
        }
        data_allocator::deallocate(begin_, cap_ - begin_);
        begin_ = end_ = cap_ = nullptr;
        throw;
    }
}

template<class T>
void vector<T>::initial_p(iterator begins, iterator ends) {
    size_t n = ends - begins;
    begin_ = data_allocator::allocate(n ? n : 16);
    end_ = begin_;
    cap_ = begin_ + (n ? n : 16);

    try {
        for(auto it = begins; it != ends; it ++, end_ ++) {
            data_allocator::construct(end_, *it);
        }
    } catch(...) {
        while(end_ != begin_) {
            -- end_;
            data_allocator::destroy(end_);
        }

        data_allocator::deallocate(begin_, cap_ - begin_);
        begin_ = end_ = cap_ = nullptr;
        throw;
    }
}

template <class T>
void vector<T>::copy_initial(const vector<T>& tmp) {
    size_t size = tmp.size();
    begin_ = data_allocator::allocate(size ? size : 16);
    end_ = begin_;
    cap_ = begin_ + (size ? size : 16);

    try {
        for(auto it = tmp.begin(); it != tmp.end(); it ++, end_ ++) {
            data_allocator::construct(end_, *it);
        }
    } catch(...) {
        while(end_ != begin_) {
            -- end_;
            data_allocator::destroy(end_);
        }
        data_allocator::deallocate(begin_, cap_ - begin_);
        begin_ = end_ = cap_ = nullptr;
        throw;
    }
}

template <class T>
void vector<T>::list_init(std::initializer_list<T> list) {
    size_t size = list.size();
    begin_ = data_allocator::allocate(size+16);
    end_ = begin_;
    cap_ = begin_ + size + 16;

    try {
        for(auto it = list.begin(); it != list.end(); it ++, end_ ++) {
            data_allocator::construct(end_, *it);
        }
    } catch(...) {
        while(end_ != begin_) {
            -- end_;
            data_allocator::destroy(end_);
        }
        data_allocator::deallocate(begin_, cap_ - begin_);
        begin_ = end_ = cap_ = nullptr;
        throw;
    }
}

}

