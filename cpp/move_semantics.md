# 移动语义

## 为什么比拷贝快

拷贝是复制数据，移动是转移所有权（只复制指针，不复制数据本身）。

```cpp
std::string a = "hello world";

std::string b = a;             // 拷贝：分配新内存，把字符逐个复制过去
std::string c = std::move(a);  // 移动：把 a 内部的指针直接给 c，a 置空
```

`string` 内部大概长这样：
```
拷贝前：a = { ptr -> [hello world], size=11 }
移动后：c = { ptr -> [hello world], size=11 }
        a = { ptr -> nullptr, size=0 }   ← a 被掏空了
```

移动的代价是固定的（几个指针赋值），与数据大小无关。拷贝的代价随数据量线性增长。

---

## 什么时候触发移动

编译器在对象是**右值**（临时值、即将销毁的值）时自动选移动构造，而不是拷贝构造。

```cpp
std::string make() {
    return std::string("hello");  // 返回临时对象，触发移动
}

std::string s = make();           // 移动构造，不是拷贝
```

**`std::move` 做的事**：把一个左值强制转成右值引用，告诉编译器"这个对象我不要了，可以移动"。它本身不移动任何东西，只是一个类型转换。

```cpp
std::string a = "hello";
std::string b = std::move(a);  // a 被转成右值，触发移动构造
// 此后 a 处于"有效但不确定"的状态，不能再用 a 的值
```

---

## 左值和右值

| | 左值 lvalue | 右值 rvalue |
|--|--|--|
| 特征 | 有名字，有地址，可以取地址 | 临时对象，表达式结果，没有持久地址 |
| 例子 | `int x = 5;` 中的 `x` | `5`、`x + 1`、函数返回的临时值 |
| 引用类型 | `T&` | `T&&` |

```cpp
int x = 5;
int& lref  = x;       // 左值引用，正常
int&& rref = 5;       // 右值引用，绑定到临时值
int&& rref2 = x;      // 错误，x 是左值
int&& rref3 = std::move(x);  // 可以，强制转换
```

---

## std::move vs std::forward

| | `std::move` | `std::forward` |
|--|--|--|
| 作用 | 无条件转成右值引用 | 保持原来的值类别（左值传左值，右值传右值）|
| 用途 | 明确放弃对象所有权 | 模板函数中转发参数，不改变其左/右值属性 |

`std::forward` 主要用在模板的**完美转发**场景：

```cpp
template<typename T>
void wrapper(T&& arg) {
    // 如果 arg 传进来是左值，就以左值转发
    // 如果 arg 传进来是右值，就以右值转发
    real_function(std::forward<T>(arg));
}
```

不用 `std::forward`，直接写 `real_function(arg)` 的问题：模板函数内部 `arg` 有了名字，变成了左值，右值信息丢失，永远走拷贝路径。

---

## 移动后的对象能用吗

能用，但值不确定。标准保证移动后对象处于"有效但未指定"状态，可以安全析构和重新赋值，但不能依赖它的值。

```cpp
std::string a = "hello";
std::string b = std::move(a);

// a 现在可能是空字符串，也可能是其他状态
// 可以这样用：
a = "world";   // 重新赋值，完全没问题
a.clear();     // 可以
std::cout << a;  // 能编译，但值不确定，不要依赖它
```

---

## 什么时候用 std::move

1. 把资源从一个对象转给另一个，原对象不再需要
2. 往容器里放大对象，避免拷贝

```cpp
std::vector<std::string> v;
std::string s = "a long string...";

v.push_back(s);             // 拷贝，s 还可以用
v.push_back(std::move(s));  // 移动，s 被掏空，更快
```

3. 函数返回局部变量时**不要手动加 `std::move`**，编译器会自动做 NRVO（具名返回值优化）：

```cpp
std::string make() {
    std::string result = "hello";
    return result;          // 正确，编译器自动优化
    return std::move(result); // 反而可能阻止 NRVO，不要这样写
}
```

---

## 面试常问

**Q：右值引用变量本身是左值还是右值？**
是左值。有名字的东西都是左值。所以才需要 `std::forward` 来保持原始的值类别。

```cpp
void foo(std::string&& s) {
    // s 在这里有名字，是左值
    bar(s);             // 传给 bar 的是左值
    bar(std::move(s));  // 才能传右值
}
```

**Q：哪些类型移动和拷贝一样快？**
内置类型（`int`、`double`、裸指针）没有移动语义，移动等于拷贝。移动优化只对持有堆内存的类型有意义（`string`、`vector`、`unique_ptr` 等）。
