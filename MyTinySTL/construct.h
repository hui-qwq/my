#pragma once

// 这个头文件包含两个函数 construct，destroy
// construct : 负责对象的构造
// destroy   : 负责对象的析构

#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

#include "type_traits.h"
#include "iterator.h"
#include "util.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unused parameter
#endif // _MSC_VER

namespace mystl
{
  template<class T> 
  void construct(T* ptr) {
    ::new((void*)ptr) T();
  }

  template<class T1, class T2> 
  void construct(T1* ptr, const T2& value) {
    ::new((void*)ptr) T1(value);
  }

  template<class T1, class... Args>
  void construct(T1* ptr, Args&&... args) {
    ::new((void*)ptr) T1(std::forward<Args>(args)...);
  }

  template<class T> 
  void destroy_one(T*, std::true_type) {}

  template<class T>
  void destroy_one(T* ptr, std::false_type) {
    if(ptr != nullptr) ptr->~T();
  }

  template<class T>
  void destroy(T* ptr) {
    destroy_one(ptr, std::is_trivially_destructible<T>{});
  }

  template<class ForwardIter>
  void destroy_cat(ForwardIter first, ForwardIter last, std::true_type) {}

  template<class ForwardIter>
  void destroy_cat(ForwardIter first, ForwardIter last, std::false_type) {
    for(; first != last; ++ first) {
      destroy(&*first);
    }
  }

  template<class ForwardIter>
  void destroy(ForwardIter first, ForwardIter last) {
    using ValueType = typename std::iterator_traits<ForwardIter>::value_type;
    destroy_cat(first, last, std::is_trivially_destructible<ValueType>{});
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER


