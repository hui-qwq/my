#include <iostream>
#include <string>
#include "../MyTinySTL/allocator.h"
#include "../MyTinySTL/construct.h"

class Test
{
public:
    std::string name;

    Test() : name("default")
    {
        std::cout << "default ctor\n";
    }

    Test(const std::string& s) : name(s)
    {
        std::cout << "value ctor: " << name << "\n";
    }

    Test(const Test& other) : name(other.name)
    {
        std::cout << "copy ctor: " << name << "\n";
    }

    Test(Test&& other) noexcept : name(std::move(other.name))
    {
        std::cout << "move ctor: " << name << "\n";
    }

    ~Test()
    {
        std::cout << "dtor: " << name << "\n";
    }
};

void test_single()
{
    std::cout << "==== test_single ====\n";

    Test* p = mystl::allocator<Test>::allocate();
    mystl::allocator<Test>::construct(p, "hello");

    std::cout << "object name = " << p->name << "\n";

    mystl::allocator<Test>::destroy(p);
    mystl::allocator<Test>::deallocate(p);
}

void test_array()
{
    std::cout << "==== test_array ====\n";

    Test* p = mystl::allocator<Test>::allocate(3);

    mystl::allocator<Test>::construct(p, "A");
    mystl::allocator<Test>::construct(p + 1, "B");
    mystl::allocator<Test>::construct(p + 2, "C");

    std::cout << p[0].name << " " << p[1].name << " " << p[2].name << "\n";

    mystl::allocator<Test>::destroy(p);
    mystl::allocator<Test>::destroy(p + 1);
    mystl::allocator<Test>::destroy(p + 2);

    mystl::allocator<Test>::deallocate(p, 3);
}

void test_copy_construct()
{
    std::cout << "==== test_copy_construct ====\n";

    Test t("copy_source");

    Test* p = mystl::allocator<Test>::allocate();
    mystl::allocator<Test>::construct(p, t);

    std::cout << "object name = " << p->name << "\n";

    mystl::allocator<Test>::destroy(p);
    mystl::allocator<Test>::deallocate(p);
}

void test_move_construct()
{
    std::cout << "==== test_move_construct ====\n";

    Test t("move_source");

    Test* p = mystl::allocator<Test>::allocate();
    mystl::allocator<Test>::construct(p, std::move(t));

    std::cout << "object name = " << p->name << "\n";

    mystl::allocator<Test>::destroy(p);
    mystl::allocator<Test>::deallocate(p);
}

void test_construct_only()
{
    std::cout << "==== test_construct_only ====\n";

    void* raw = ::operator new(sizeof(Test));
    Test* p = static_cast<Test*>(raw);

    mystl::construct(p, "construct_only");
    std::cout << "object name = " << p->name << "\n";

    mystl::destroy(p);
    ::operator delete(p);
}

int main()
{
    test_single();
    std::cout << "\n";

    test_array();
    std::cout << "\n";

    test_copy_construct();
    std::cout << "\n";

    test_move_construct();
    std::cout << "\n";

    test_construct_only();
    std::cout << "\n";

    return 0;
}