//
// Created by xinyang on 2021/2/18.
//

#ifndef GCPP_GCPP_HPP
#define GCPP_GCPP_HPP

#include <stack>
#include <vector>
#include <memory>

namespace gcpp {

    /**
     * @brief 进行一次垃圾回收，当前线程结束时会自动运行
     */
    void gc_collect();

    /**
     * @brief 用于管理gc_ptr的依赖关系
     */
    class gc_dependency {
        friend void gc_collect();

    public:
        /// 析构函数
        ~gc_dependency();

        /// 默认构造
        gc_dependency();

        /// 拷贝构造
        gc_dependency(const gc_dependency &other);

        /// 移动构造
        gc_dependency(gc_dependency &&other) noexcept;

        /// 拷贝复制
        gc_dependency &operator=(const gc_dependency &other);

        /// 移动复制
        gc_dependency &operator=(gc_dependency &&other) noexcept;

        /// 手动添加子依赖
        void add_child(gc_dependency *child);

    protected:
        /**
         * @brief 回收当前gc_ptr管理的内存，由gc_ptr<T>提供实现
         */
        virtual void collect() = 0;

    private:
        /**
         * @brief 初始化，将当前对象添加到gc列表中，并自动注册依赖。在构造函数中调用
         */
        void init();

        /**
         * @brief 反初始化，将当前对象从gc列表中标记为待删除。在析构函数中调用
         */
        void uninit();

        /**
         * @brief 从当前对象开始，遍历依赖图，并标记可达性。
         */
        void check();

    private:
        enum {
            UNKNOWN,        /// 可达性未知
            ROOT,           /// 根节点
            REACHABLE,      /// 可达
            UNREACHABLE,    /// 不可达
        } reachability = UNKNOWN;               /// 当前对象的可达性
        std::vector<gc_dependency *> children;  /// 当前对象的子依赖
        gc_dependency **p_list_item;            /// 当前对象在列表中的指针
    };

    /**
     * @brief GC指针
     * @tparam T 该GC指针管理的对象类型
     */
    template<class T>
    class gc_ptr : public gc_dependency, private std::shared_ptr<T> {
    public:
        gc_ptr() = default;

        using std::shared_ptr<T>::operator bool;
        using std::shared_ptr<T>::operator*;
        using std::shared_ptr<T>::operator->;
        using std::shared_ptr<T>::get;

        bool operator==(std::nullptr_t null) { return !std::shared_ptr<T>::operator bool(); }

    protected:
        /**
         * @brief 回收当前gc_ptr管理的内存，由gc_ptr<T>提供实现
         */
        void collect() override {
            std::shared_ptr<T>::reset();
        }

        using std::shared_ptr<T>::reset;
    };

    /// 自动注册依赖时用到的栈
    inline thread_local std::stack<gc_dependency *> gc_parent;

    /**
     * @brief 创建一个被gc_ptr管理的对象，并自动注册依赖。
     * @tparam T 被管理对象的类型
     * @tparam Ts 构造被管理对象时的参数类型
     * @param args 构造被管理对象时的参数
     * @return 管理一个对象的gc_ptr
     */
    template<class T, class ...Ts>
    gc_ptr<T> gc_new(Ts &&...args) {
        struct wrapper_gc_ptr : public gc_ptr<T> {
            using gc_ptr<T>::reset;
        };

        wrapper_gc_ptr ptr;
        gc_parent.push(&ptr);
        ptr.reset(new T(std::forward<Ts>(args)...));
        gc_parent.pop();
        return ptr;
    }
}

#endif //GCPP_GCPP_HPP
