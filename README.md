# 200行代码实现C++多线程垃圾回收

垃圾回收（garbage collection，简称GC）和C++标准库的智能指针的最大区别就在于GC可以处理**循环引用**的情况。这里实现了一个最简单的GC应用。

## 编译

需要使用支持C++17的编译器进行编译。

## usage

```c++
#include <gcpp/gcpp.hpp>
struct A {
    gcpp::gc_ptr<A> p_a;
};
int main() {
    {
        auto a1 = gcpp::gc_new<A>();
        auto a2 = gcpp::gc_new<A>();
        a1->p_a = a2;
        a2->p_a = a1;
    }
    // optional: 线程结束时会自动GC当前线程。
    gcpp::gc_collect();
    return 0;
}
```

上述例子是一个典型的循环引用的场景，如果将gc_ptr替换为标准库中的shared_ptr则无法正确回收内存。

## 实现

实现GC的关键在于建立依赖图。通常来说，程序中的变量有三种类型：全局（静态）变量、局部（栈）变量、动态（堆）变量。其中全局变量和局部变量都是编译器自动处理的，换言之，这两种变量在依赖图中都为根节点。而动态变量由于需要程序进行管理，所以总会有一个管理者，故动态变量在依赖图中不是根节点。

为了自动建立依赖关系，减少不必要的麻烦，程序中提供接口函数```gcpp::gc_new<T>()```，凡是通过这个函数创建出来的对象，都会自动处理依赖关系。对于特殊情况，无法自动处理依赖时，可以使用```add_child```函数手动添加依赖。

当依赖图成功建立之后，在GC时，只需要从根节点开始遍历整张依赖图，如果有节点没有被遍历到，则GC会将其回收。

为了实现上述功能，每次GC的时候，会遍历三次依赖图，故GC的时间复杂度为O(3n)。

## 线程安全

多线程间的线程安全使用thread_local变量处理，每个线程仅管理本线程中创建的gc_ptr对象。由于不同线程间的gc_ptr对象可能存在交叉依赖关系，所以在GC以及其他必要的位置进行了加锁处理，避免线程间的冲突。

## 局限

* 自动处理依赖对于STL容器可能失效。当STL容器是一个被gc_ptr管理的堆上容器时，如果通过emplace等函数创建新的gc_ptr元素，则新创建出来gc_ptr不会被计入依赖。原因在于STL容器内部使用到的堆上内存没有被gc_ptr管理。
* gc_ptr不支持管理数组。
* gc_ptr不支持自定义析构函数。
* gc_ptr复制时会同时复制依赖关系，开销比较大。