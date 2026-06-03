# Week 1：内存管理

## 一、RAII

**一句话**：把资源释放绑定到析构函数，让编译器保证资源不泄漏。

**没有 RAII 的问题：**
```cpp
void foo() {
    FILE* f = fopen("a.txt", "r");
    if (some_error) return;  // fclose 没执行，泄漏
    fclose(f);
}
```

**有 RAII：**
```cpp
void foo() {
    std::ifstream f("a.txt");
    if (some_error) return;  // 没问题，f 析构时自动关文件
}
```

工作中最常见的 RAII：`std::lock_guard`——进入作用域自动加锁，退出自动解锁。

---

## 二、自己实现一个 RAII 类

要求：管理裸指针，构造时获取资源，析构时释放，禁止拷贝，允许移动。

```cpp
class Resource {
public:
    explicit Resource(int* ptr) : ptr_(ptr) {}

    ~Resource() {
        delete ptr_;
    }

    // 禁止拷贝——两个对象不能管同一块内存，否则 double free
    Resource(const Resource&)            = delete;
    Resource& operator=(const Resource&) = delete;

    // 允许移动——转移所有权，原对象置空
    Resource(Resource&& other) noexcept
        : ptr_(other.ptr_) {
        other.ptr_ = nullptr;   // 关键：防止 other 析构时 delete 已转移的指针
    }

    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            delete ptr_;            // 先释放自己持有的
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    int* get() const { return ptr_; }

private:
    int* ptr_;
};
```

**几个必须能解释的点：**

| 点 | 原因 |
|---|------|
| 拷贝构造 `= delete` | 两个对象析构时会 `delete` 同一个指针，undefined behavior |
| 移动后 `other.ptr_ = nullptr` | 防止 `other` 析构时对已转移的指针再次 `delete` |
| 移动函数加 `noexcept` | `std::vector` 扩容时只有 `noexcept` 的移动构造才会被使用，否则退化为拷贝 |
| 移动赋值先 `delete ptr_` | 自己原来持有的资源要先释放，否则泄漏 |

**Rule of Five**：如果你自定义了析构函数，就必须同时考虑拷贝构造、拷贝赋值、移动构造、移动赋值这四个——五个函数要一起管。

---

## 三、智能指针选型

**默认用 `unique_ptr`，有共享所有权需求才用 `shared_ptr`。**

```
这个资源有几个所有者？
├── 一个  →  unique_ptr   （零开销，等于裸指针）
└── 多个  →  shared_ptr   （有控制块开销，引用计数是原子操作）
               └── 出现循环引用？→ 反向持有改成 weak_ptr
```

### unique_ptr
```cpp
auto p = std::make_unique<int>(42);
// 出作用域自动 delete，不能拷贝，可以移动
auto p2 = std::move(p);  // p 变成 nullptr
```

### shared_ptr
```cpp
auto a = std::make_shared<int>(42);
auto b = a;              // 引用计数变 2
// a 和 b 都析构后才 delete
```

### weak_ptr — 不增加引用计数，用来打破循环
```cpp
std::weak_ptr<int> w = a;  // 引用计数还是 2，不变
if (auto sp = w.lock()) {  // 使用前必须 lock()，检查对象是否还活着
    *sp = 100;
}
```

---

## 四、循环引用问题

### 有问题的版本
```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;  // 问题在这
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->prev = a;
// a 离开作用域：引用计数 2→1（b->prev 还拿着），不析构
// b 离开作用域：引用计数 2→1（a->next 还拿着），不析构
// 两个对象永远不会被释放
```

### 修复：反向链接用 weak_ptr
```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node>   prev;  // 不增加引用计数
};
// a 离开作用域：引用计数 1→0，析构，同时释放 a->next 对 b 的持有
// b 引用计数随之变 0，析构
```

---

## 五、面试常考问答

**Q：`shared_ptr` 线程安全吗？**
引用计数的增减是原子操作，是线程安全的。但指针指向的对象本身不是线程安全的，并发读写对象需要自己加锁。

**Q：`make_shared` 和 `new` 有什么区别？**
`make_shared` 把对象和控制块（引用计数）分配在一块连续内存里，只有一次 `malloc`；`shared_ptr<T>(new T)` 有两次 `malloc`。优先用 `make_shared`。

**Q：`unique_ptr` 怎么传给函数？**
- 需要转移所有权：`std::move` 传值
- 只是用一下：传裸指针 `ptr.get()` 或传引用 `*ptr`，不转移所有权
