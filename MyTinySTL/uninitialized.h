#pragma once

#include "algobase.h"
#include "construct.h"
#include "iterator.h"
#include "type_traits.h"
#include "util.h"
#include <type_traits>

namespace mystl
{

    /*****************************************************************************************/
    // uninitialized_copy
    /*****************************************************************************************/
    template<class InputIter, class ForwardIter>
    ForwardIter unchecked_uninit_copy(InputIter first, InputIter last,
                                    ForwardIter result, std::false_type)
    {
        ForwardIter cur = result;
        try
        {
            for (; first != last; ++first, ++cur)
            {
                mystl::construct(&*cur, *first);
            }
        }
        catch (...)
        {
            for (; result != cur; ++result)
            {
                mystl::destroy(&*result);
            }
            throw;
        }
        return cur;
    }

    template<class InputIter, class ForwardIter>
    ForwardIter unchecked_uninit_copy(InputIter first, InputIter last,
                                    ForwardIter result, std::true_type)
    {
        return mystl::copy(first, last, result);
    }

    template<class InputIter, class ForwardIter>
    ForwardIter uninitialized_copy(InputIter first, InputIter last, ForwardIter result)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        return unchecked_uninit_copy(
            first, last, result,
            std::is_trivially_copy_assignable<value_type>{}
        );
    }

    /*****************************************************************************************/
    // uninitialized_copy_n
    /*****************************************************************************************/
    template<class InputIter, class Size, class ForwardIter>
    ForwardIter unchecked_uninit_copy_n(InputIter first, Size n,
                                        ForwardIter result, std::false_type)
    {
        ForwardIter cur = result;
        try
        {
            for (; n > 0; --n, ++first, ++cur)
            {
                mystl::construct(&*cur, *first);
            }
        }
        catch (...)
        {
            for (; result != cur; ++result)
            {
                mystl::destroy(&*result);
            }
            throw;
        }
        return cur;
    }

    template<class InputIter, class Size, class ForwardIter>
    ForwardIter unchecked_uninit_copy_n(InputIter first, Size n,
                                        ForwardIter result, std::true_type)
    {
        return mystl::copy_n(first, n, result).second;
    }

    template<class InputIter, class Size, class ForwardIter>
    ForwardIter uninitialized_copy_n(InputIter first, Size n, ForwardIter result)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        return unchecked_uninit_copy_n(
            first, n, result,
            std::is_trivially_copy_assignable<value_type>{}
        );
    }

    /*****************************************************************************************/
    // uninitialized_fill
    /*****************************************************************************************/
    template<class ForwardIter, class T>
    void unchecked_uninit_fill(ForwardIter first, ForwardIter last,
                            const T& value, std::false_type)
    {
        ForwardIter cur = first;
        try
        {
            for (; cur != last; ++cur)
            {
                mystl::construct(&*cur, value);
            }
        }
        catch (...)
        {
            for (; first != cur; ++first)
            {
                mystl::destroy(&*first);
            }
            throw;
        }
    }

    template<class ForwardIter, class T>
    void unchecked_uninit_fill(ForwardIter first, ForwardIter last,
                            const T& value, std::true_type)
    {
        mystl::fill(first, last, value);
    }

    template<class ForwardIter, class T>
    void uninitialized_fill(ForwardIter first, ForwardIter last, const T& value)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        unchecked_uninit_fill(
            first, last, value,
            std::is_trivially_copy_assignable<value_type>{}
        );
    }

    /*****************************************************************************************/
    // uninitialized_fill_n
    /*****************************************************************************************/
    template<class ForwardIter, class Size, class T>
    ForwardIter unchecked_uninit_fill_n(ForwardIter first, Size n,
                                        const T& value, std::false_type)
    {
        ForwardIter cur = first;
        try
        {
            for (; n > 0; --n, ++cur)
            {
                mystl::construct(&*cur, value);
            }
        }
        catch (...)
        {
            for (; first != cur; ++first)
            {
                mystl::destroy(&*first);
            }
            throw;
        }
        return cur;
    }

    template<class ForwardIter, class Size, class T>
    ForwardIter unchecked_uninit_fill_n(ForwardIter first, Size n,
                                        const T& value, std::true_type)
    {
        return mystl::fill_n(first, n, value);
    }

    template<class ForwardIter, class Size, class T>
    ForwardIter uninitialized_fill_n(ForwardIter first, Size n, const T& value)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        return unchecked_uninit_fill_n(
            first, n, value,
            std::is_trivially_copy_assignable<value_type>{}
        );
    }

    /*****************************************************************************************/
    // uninitialized_move
    /*****************************************************************************************/
    template<class InputIter, class ForwardIter>
    ForwardIter unchecked_uninit_move(InputIter first, InputIter last,
                                    ForwardIter result, std::false_type)
    {
        ForwardIter cur = result;
        try
        {
            for (; first != last; ++first, ++cur)
            {
                mystl::construct(&*cur, mystl::move(*first));
            }
        }
        catch (...)
        {
            for (; result != cur; ++result)
            {
                mystl::destroy(&*result);
            }
            throw;
        }
        return cur;
    }

    template<class InputIter, class ForwardIter>
    ForwardIter unchecked_uninit_move(InputIter first, InputIter last,
                                    ForwardIter result, std::true_type)
    {
        return mystl::move(first, last, result);
    }

    template<class InputIter, class ForwardIter>
    ForwardIter uninitialized_move(InputIter first, InputIter last, ForwardIter result)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        return unchecked_uninit_move(
            first, last, result,
            std::is_trivially_move_assignable<value_type>{}
        );
    }

    /*****************************************************************************************/
    // uninitialized_move_n
    /*****************************************************************************************/
    template<class InputIter, class Size, class ForwardIter>
    ForwardIter unchecked_uninit_move_n(InputIter first, Size n,
                                        ForwardIter result, std::false_type)
    {
        ForwardIter cur = result;
        try
        {
            for (; n > 0; --n, ++first, ++cur)
            {
                mystl::construct(&*cur, mystl::move(*first));
            }
        }
        catch (...)
        {
            for (; result != cur; ++result)
            {
                mystl::destroy(&*result);
            }
            throw;
        }
        return cur;
    }

    template<class InputIter, class Size, class ForwardIter>
    ForwardIter unchecked_uninit_move_n(InputIter first, Size n,
                                        ForwardIter result, std::true_type)
    {
        return mystl::move(first, first + n, result);
    }

    template<class InputIter, class Size, class ForwardIter>
    ForwardIter uninitialized_move_n(InputIter first, Size n, ForwardIter result)
    {
        using value_type = typename iterator_traits<ForwardIter>::value_type;
        return unchecked_uninit_move_n(
            first, n, result,
            std::is_trivially_move_assignable<value_type>{}
        );
    }

} // namespace mystl