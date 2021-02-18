//
// Created by xinyang on 2021/2/18.
//

#include <gcpp/gcpp.hpp>
#include <iostream>
#include <thread>

struct A {
    gcpp::gc_ptr<A> p_a1, p_a2;

    ~A() {
        std::cout << "A destroy!" << std::endl;
    }
};

int main() {
    {
        auto a1 = gcpp::gc_new<A>();
        auto a2 = gcpp::gc_new<A>();
        // a1一个指向自己，一个指向a2
        a1->p_a1 = a1;
        a1->p_a2 = a2;
        // a2一个指向自己，一个指向a1
        a2->p_a1 = a2;
        a2->p_a2 = a1;

        // 这里使用引用，使得不同线程间的gc_ptr产生依赖关系
        std::thread([&]() {
            // a3,a4和其他线程有依赖关系，故当前线程结束时，GC不会回收a3和a4，当主函数结束时才会回收。
            auto a3 = gcpp::gc_new<A>();
            auto a4 = gcpp::gc_new<A>();
            a1->p_a2 = a3;
            a3->p_a1 = a2;
            a3->p_a2 = a4;
            a2->p_a2 = a4;
            a4->p_a1 = a1;
            a4->p_a2 = a2;

            // a5和其他线程没有依赖关系，故当当前线程结束时，GC会回收a5管理的对象
            auto a5 = gcpp::gc_new<A>();
            a5->p_a1 = a5;
            a5->p_a2 = a3;

            std::cout << "thread stop!" << std::endl;
            // optional: 线程结束时会自动GC当前线程。
            gcpp::gc_collect();
        }).join();
    }
    std::cout << "main stop!" << std::endl;
    // optional: 线程结束时会自动GC当前线程。
    gcpp::gc_collect();
    return 0;
}

