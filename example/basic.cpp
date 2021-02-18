//
// Created by xinyang on 2021/2/18.
//

#include <gcpp/gcpp.hpp>
#include <iostream>

struct A {
    gcpp::gc_ptr<A> p_a;

    ~A() {
        std::cout << "A destroy!" << std::endl;
    }
};

int main() {
    {
        auto a1 = gcpp::gc_new<A>();
        auto a2 = gcpp::gc_new<A>();
        a1->p_a = a2;
        a2->p_a = a1;
    }
    std::cout << "main stop!" << std::endl;
    // optional: 线程结束时会自动GC当前线程。
    gcpp::gc_collect();
    return 0;
}
