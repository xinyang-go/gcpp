//
// Created by xinyang on 2021/2/18.
//

#include <gcpp/gcpp.hpp>
#include <list>
#include <mutex>

namespace gcpp {

    /**
     * @brief 自动垃圾回收辅助类
     */
    class gc_auto_collect {
    public:
        ~gc_auto_collect();
    };

    static thread_local std::list<gc_dependency *> gc_list; /// 当前线程中创建的gc_ptr的列表
    static thread_local gc_auto_collect auto_collect;       /// 自动垃圾回收辅助
    static std::recursive_mutex gc_mtx;                     /// 多线程锁

    void gc_collect() {
        /* GC时加锁，避免多个线程同时GC */
        std::unique_lock lock(gc_mtx);
        /* 初始化依赖图，除ROOT外，其余节点可达性设为未知，同时删除无效列表项 */
        for (auto p_list = gc_list.begin(); p_list != gc_list.end();) {
            if ((*p_list) == nullptr) {
                p_list = gc_list.erase(p_list);
            } else {
                if ((*p_list)->reachability != gc_dependency::ROOT) {
                    (*p_list)->reachability = gc_dependency::UNKNOWN;
                }
                p_list++;
            }
        }
        /* 遍历依赖图，确定可达性 */
        for (auto p_list = gc_list.begin(); p_list != gc_list.end(); p_list++) {
            if ((*p_list)->reachability == gc_dependency::ROOT) {
                (*p_list)->check();
            }
        }
        /* 根据可达性，回收不可达节点，同时删除无效列表项 */
        for (auto p_list = gc_list.begin(); p_list != gc_list.end();) {
            if ((*p_list) == nullptr) {
                p_list = gc_list.erase(p_list);
            } else {
                if ((*p_list)->reachability == gc_dependency::UNKNOWN) {
                    (*p_list)->reachability = gc_dependency::UNREACHABLE;
                    (*p_list)->collect();
                }
                p_list++;
            }
        }
    }

    // 析构时反初始化
    gc_dependency::~gc_dependency() {
        uninit();
    }

    // 构造时初始化
    gc_dependency::gc_dependency() {
        init();
    }

    // 构造时初始化，同时拷贝依赖关系
    gc_dependency::gc_dependency(const gc_dependency &other) : children(other.children) {
        init();
    }

    // 构造时初始化，同时移动依赖关系
    gc_dependency::gc_dependency(gc_dependency &&other) noexcept: children(std::move(other.children)) {
        init();
    }

    // 复制时拷贝依赖关系
    gc_dependency &gc_dependency::operator=(const gc_dependency &other) {
        if (this == &other) return *this;
        children = other.children;
        return *this;
    }

    // 复制时移动依赖关系
    gc_dependency &gc_dependency::operator=(gc_dependency &&other) noexcept {
        if (this == &other) return *this;
        children = std::move(other.children);
        return *this;
    }

    void gc_dependency::add_child(gc_dependency *child) {
        children.emplace_back(child);
    }

    void gc_dependency::init() {
        {
            /* 加锁，并将当前对象添加到列表中 */
            std::unique_lock lock(gc_mtx);
            gc_list.emplace_back(this);
            p_list_item = &gc_list.back();
        }
        /* 如果有父级依赖则设置依赖，否则当前节点为ROOT */
        if (!gc_parent.empty()) {
            gc_parent.top()->add_child(this);
        } else {
            reachability = ROOT;
        }
    }

    void gc_dependency::uninit() {
        /* 加锁并将列表中的对应项设为待删除 */
        std::unique_lock lock(gc_mtx);
        *p_list_item = nullptr;
    }

    void gc_dependency::check() {
        /* 遍历子依赖，如果可达性未知则将可达性设为可达，同时递归遍历 */
        for (auto c: children) {
            if (c->reachability == UNKNOWN) {
                c->reachability = REACHABLE;
                c->check();
            }
        }
    }

    // 析构时自动GC
    gc_auto_collect::~gc_auto_collect() {
        gc_collect();
    }
}
