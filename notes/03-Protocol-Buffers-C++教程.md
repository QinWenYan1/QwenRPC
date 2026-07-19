# 📘 Protocol Buffers C++ 生成代码（C++ Generated Code）

> 来源说明：[Protocol Buffers 官方文档中文站 — C++ 生成代码指南](https://protobuf.com.cn/reference/cpp/cpp-generated/) | 本节涵盖：`protoc` 为 C++ 生成什么代码——消息类、各类字段访问器、oneof/map/枚举、Service 与 Stub

---

## 🧠 核心概念总览（严格按原文顺序）
> 📎 返回：[QwenRPC 仓库首页](../README.md)

- [*知识点1: 教程目标与问题领域*](#id1)
- [*知识点2: 编译 `.proto` 生成 C++ 代码*](#id2)
- [*知识点3: package 与 C++ 命名空间*](#id3)
- [*知识点4: 生成的消息类*](#id4)
- [*知识点5: 嵌套消息类型*](#id5)
- [*知识点6: 字段访问器总览与存在性（presence）*](#id6)
- [*知识点7: 单值标量/字符串/枚举字段访问器*](#id7)
- [*知识点8: 单值消息字段访问器与内存所有权*](#id8)
- [*知识点9: 重复字段访问器*](#id9)
- [*知识点10: oneof 字段*](#id10)
- [*知识点11: 映射（map）字段*](#id11)
- [*知识点12: 枚举辅助函数与开放枚举陷阱*](#id12)
- [*知识点13: 服务（Services）——RPC 项目核心*](#id13)
- [*知识点14: 省略内容速查指引*](#id14)

---

<a id="id1"></a>
## ✅ 知识点1: 教程目标与问题领域


**教程目标**
- 在 `.proto` 文件中定义消息格式
- 使用 Protocol Buffer 编译器 `protoc`
- 使用 C++ Protocol Buffer API 编写和读取消息

- **问题领域：为什么需要 Protobuf？**

  - 对于一个地址簿程序，从文件读写人们的联系方式（姓名、ID、邮箱、电话）。在 Protobuf 出现之前，常见的序列化方法各有问题：

    - **直接写内存结构的二进制**：脆弱，要求接收方使用完全相同的内存布局、字节序，难以扩展格式
    - **把数据编码成单个字符串**：例如 `"12:3:-23:67"`。简单灵活，但需要手写编码/解析代码，解析有运行时开销，只适合非常简单的情况
    - **XML**：人类可读、多语言绑定好，适合跨应用共享数据。但体积大、编解码性能开销高、遍历 DOM 比访问简单字段复杂

- **Protobuf 的优势**：
  - 开发者编写 `.proto` 文件描述数据结构
  - `protoc` 自动生成类，自动完成高效的二进制编码和解析
  - 生成的类提供字段的 getter 和 setter，并负责消息的整体读写
  - 格式可随时间扩展，新代码仍能读取旧格式编码的数据


> ⚠️ **关键区分**：本教程是 C++ 入门示例，不是 C++ Protobuf 的完整参考。
> 💡 **理解技巧**：可以把 `.proto` 当成「数据类的设计图」，`protoc` 就是按图纸自动建造 C++ 类的工厂。
> 📋 **术语提醒**：`序列化（serialization）` 指把内存对象变成可存储/传输的字节流；`反序列化（deserialization）` 是逆过程。

---

<a id="id2"></a>
## ✅ 知识点2: 编译 `.proto` 生成 C++ 代码

**一条 `protoc` 命令，把 `.proto` 文件变成 `.pb.h` + `.pb.cc` 两个 C++ 文件。**

- **核心机制：**

  1. **生成开关**
      - 调用编译器时通过 `--cpp_out=` 标志生成 C++ 输出
      - 参数是你希望编译器写入 C++ 输出的目录

  2. **输出文件规则**
      - 编译器为**每个**输入的 `.proto` 文件创建一个头文件和一个实现文件
      - 扩展名 `.proto` 分别替换为 `.pb.h`（头文件）或 `.pb.cc`（实现文件）
      - `--proto_path=`（或 `-I`）指定的源路径，替换为 `--cpp_out=` 指定的输出路径

- **示例/实践**
  ```bash
  protoc --proto_path=src --cpp_out=build/gen src/foo.proto src/bar/baz.proto
  ```

- 编译器读取 `src/foo.proto` 和 `src/bar/baz.proto`，生成四个输出文件：
  - `build/gen/foo.pb.h`、`build/gen/foo.pb.cc`
  - `build/gen/bar/baz.pb.h`、`build/gen/bar/baz.pb.cc`

> ⚠️ **关键区分**：`-I`/`--proto_path` 是 `.proto` 的**源目录**；`--cpp_out` 才是 C++ 代码的**输出目录**
> ⚠️ **警告**：编译器会自动创建 `build/gen/bar`，但**不会**创建 `build` 或 `build/gen`——输出目录必须事先建好
> 💡 **理解技巧**：`.pb.h`/`.pb.cc` 就是普通 C++ 文件，编译项目时把它们一起编译、链接 protobuf 库即可


---

<a id="id3"></a>
## ✅ 知识点3: package 与 C++ 命名空间

**`.proto` 里的 `package` 声明，在 C++ 中直接变成嵌套的 `namespace`**

- 如果 `.proto` 文件包含 `package` 声明，该文件的**所有内容**都会放进对应的 C++ 命名空间中

- **示例/实践**
  ```proto
  package foo.bar;
  ```

- 文件中的所有声明都将驻留在 `foo::bar` 命名空间中

> 💡 **理解技巧**：`package` 的主要作用是防止不同项目之间的命名冲突——和 C++ 引入命名空间的动机一模一样
> 📋 **术语提醒**：`package` 在 C++ 中映射为 `namespace`，在 Java 中映射为 `package`

---

<a id="id4"></a>
## ✅ 知识点4: 生成的消息类

**如何生成消息类...**

- **每个 `message` 生成一个公开继承 `google::protobuf::Message` 的具体类——直接用就好，但千万不要去继承它**

- **类骨架：**
  ```proto
  message Foo {}
  ```

  - 编译器生成名为 `Foo` 的类，**公开派生自 `google::protobuf::Message`**
  - 它是**具体类**：没有未实现的纯虚方法
  - **你不应该创建自己的 `Foo` 子类**——许多生成的调用做了去虚拟化（devirtualization）优化，你重写的虚方法可能会被忽略

- **`Message` 接口定义了允许您检查、操作、读取或写入整个消息的方法：**

  - `bool ParseFromString(::absl::string_view data)`：从序列化二进制字符串（`线格式(wire format)`）解析消息
  - `bool SerializeToString(string* output) const`：把消息序列化为二进制字符串
  - `string DebugString()`：返回 `proto` 的 `text_format` 可读表示，**只应用于调试**

- **生成类自己定义的方法：**

  - `Foo()` / `~Foo()`：默认构造与析构
  - `Foo(const Foo& other)` / `Foo(Foo&& other)`：拷贝构造 / 移动构造
  - `operator=`：拷贝赋值 / 移动赋值
  - `void Swap(Foo* other)`：与另一个消息交换内容
  - `unknown_fields()` / `mutable_unknown_fields()`：访问解析此消息时遇到的`未知字段集(UnknownFieldSet)`

- **静态方法：**

  - `static const Descriptor* descriptor()`：类型`描述符(Descriptor)`——包含字段及类型信息，可配合`反射(reflection)`以编程方式检查字段
  - `static const Foo& default_instance()`：常量单例，等同于新构造的 `Foo`（所有单值字段未设置、所有重复字段为空）

- **优化选项（了解即可）：**

  - `option optimize_for = CODE_SIZE;`：只重写最小方法集，其余依赖基于反射的实现——**代码显著变小，但性能降低**
  - `option optimize_for = LITE_RUNTIME;`：实现 `MessageLite` 子集接口（不支持描述符/反射），只需链接小得多的 `libprotobuf-lite.so`，适合手机等资源受限系统


> ⚠️ **关键区分**：拷贝构造和赋值运算符是**深拷贝**——每个消息对象拥有自己的数据副本，改副本不影响原件，也不会双重释放。
> ⚠️ **警告**：不要继承生成的消息类来"加功能"——请改用组合或自由函数。
> 💡 **理解技巧**：`DebugString()` 是调试好朋友：`std::cout << foo.DebugString();` 直接打印整个消息内容。
> 📋 **术语提醒**：`线格式（wire format）` 指 Protobuf 的二进制编码格式；`MessageLite` 是裁剪版消息接口。

---

<a id="id5"></a>
## ✅ 知识点5: 嵌套消息类型

**嵌套定义的 message 生成 `Foo_Bar` 类，再用 `typedef` 伪装成 `Foo::Bar`——但前向声明时只能用真名。**

**示例/实践**
```proto
message Foo {
  message Bar {}
}
```

- 编译器生成**两个类**：`Foo` 和 `Foo_Bar`
- 并在 `Foo` 内部生成一个 typedef：

```cpp
typedef Foo_Bar Bar;
```

- 因此可以**把它当作嵌套类使用**：`Foo::Bar`
- **但 C++ 不允许前向声明嵌套类型**——想在另一个文件中前向声明 `Bar`，必须使用真名 `Foo_Bar`

**注意点**
> ⚠️ **关键区分**：日常写代码用 `Foo::Bar`；需要前向声明时必须写 `Foo_Bar`。
> 💡 **理解技巧**：`typedef` 就是起别名——`Foo::Bar` 是"艺名"，`Foo_Bar` 是"身份证名"。
> 📋 **术语提醒**：`前向声明（forward declaration）` 是不给出完整定义、只声明某个类存在的写法，用于减少头文件依赖。

---

<a id="id6"></a>
## ✅ 知识点6: 字段访问器总览与存在性（presence）

**所有字段访问器都按同一套命名规律生成；字段有没有 `has_` 方法，由"存在性"决定。**

**命名与常量：**

- 访问器采用**小写蛇形命名**：`has_foo()`、`clear_foo()`、`set_foo(...)`
- 每个字段还生成一个字段编号常量：`k` + 驼峰字段名 + `FieldNumber`
  - 例如 `optional int32 foo_bar = 5;` → `static const int kFooBarFieldNumber = 5;`

**存在性（presence）——有没有 `has_` 的分水岭：**

- **显式存在（explicit presence）**：生成 `has_foo()`，能区分"未设置"和"设置成了默认值"
  - 包括：消息字段、oneof 成员、proto2 的字段、proto3 中标记 `optional` 的字段
- **隐式存在（implicit presence）**：**没有** `has_foo()`，未设置时 getter 直接返回零值
  - 包括：proto3 的普通标量字段

**保留关键字：**

- 字段名若撞上 C++ 保留字，生成的名字后加下划线
  - 例如 `string false = 1;` → `false_()`、`set_false_()`、`clear_false_()`

**引用/指针失效规则（重要）：**

- 返回 const 引用的访问器：**对消息做任何修改访问后**，之前拿到的引用可能失效
- 返回指针的访问器：规则更严——**对消息做任何访问（哪怕只读）后**，指针都可能失效，不要长期保存

**注意点**
> ⚠️ **关键区分**：`has_` 只存在于显式存在字段——proto3 普通 `int32` 字段没有 `has_`，proto3 `optional int32` 才有。
> ⚠️ **警告**：经典 bug——先把 `mutable_xxx()` 返回的指针存起来，接着修改消息的其他字段，然后继续用旧指针。访问器的指针/引用请**即用即取**。
> 💡 **理解技巧**：presence 回答的问题是"这个字段**被设过吗**？"，而不是"这个字段**等于几**？"。
> 📋 **术语提醒**：`显式存在（explicit presence）` 指 Protobuf 会跟踪字段是否被显式设置过；`隐式存在（implicit presence）` 则不跟踪。

---

<a id="id7"></a>
## ✅ 知识点7: 单值标量/字符串/枚举字段访问器

**数值、字符串、枚举三类单值字段的访问器高度同构——`has_`/get/`set_`/`clear_` 四件套，字符串额外多 `mutable_`。**

**数值字段**（`int32 foo = 1;`；其余数值类型与 `bool` 同理，C++ 类型按`标量值类型表`替换）：

```cpp
bool has_foo() const;         // 仅显式存在：字段已设置返回 true
int32_t foo() const;          // 当前值；未设置返回默认值
void set_foo(int32_t value);  // 设置后 has_foo() 为 true，foo() 返回 value
void clear_foo();             // 清除后 has_foo() 为 false，foo() 返回默认值
```

**字符串/字节字段**（`string foo = 1;`）比数值多几个：

```cpp
bool has_foo() const;             // 仅显式存在
const string& foo() const;
void set_foo(...);                // 多重重载，接受 string_view / const string& / string&& / const char* 等
string* mutable_foo();            // 返回可修改的内部字符串指针；未设置时自动初始化为空串
void clear_foo();
void set_allocated_foo(string*);  // 所有权语义见知识点8
string* release_foo();            // 所有权语义见知识点8
```

**枚举字段**（`Bar bar = 1;`）与数值字段完全同构，额外注意：

- **调试模式下**（未定义 `NDEBUG`），`set_bar()` 传入未定义的枚举值会**中止进程**

**隐式存在版本的差别**：没有 `has_foo()`，未设置时返回零值（0 / 空字符串 / false / 枚举首值），其余方法相同。

**注意点**
> ⚠️ **关键区分**：`set_foo()` 之后 `has_foo()` 一定为 `true`；`clear_foo()` 之后一定为 `false`——这对显式存在字段是铁律。
> 💡 **理解技巧**：`mutable_foo()` = "把内部字符串的指针给我，我要直接改"——注意即使字段没设过，调用它也算设置了字段（`has_foo()` 变 `true`）。
> 📋 **术语提醒**：`标量值类型表（scalar value type table）` 是 .proto 类型到 C++ 类型的官方对照表（如 `int32`→`int32_t`、`string`→`std::string`）。

---

<a id="id8"></a>
## ✅ 知识点8: 单值消息字段访问器与内存所有权

**消息字段永远是显式存在；`mutable_` 会自动分配子消息，而 `set_allocated_`/`release_` 是一对方向相反的所有权转移操作。**

给定 `message Bar {}` 和字段 `Bar bar = 1;`，生成：

```cpp
bool has_bar() const;
const Bar& bar() const;
Bar* mutable_bar();
void clear_bar();
void set_allocated_bar(Bar* value);
Bar* release_bar();
```

**逐个语义：**

1. `has_bar()`：字段已设置返回 `true`
2. `bar()`：返回当前值；**未设置时返回一个字段全未设置的 `Bar`**（可能是 `Bar::default_instance()`）
3. `mutable_bar()`：**字段未设置时自动分配一个新 `Bar`**，返回可修改指针；调用后 `has_bar()` 为 `true`，`bar()` 返回同一实例的引用
4. `clear_bar()`：清除字段；`has_bar()` 变 `false`
5. `set_allocated_bar(Bar* value)`：把堆上的 `Bar` 对象塞进字段并释放旧值；**消息接管所有权**，可随时 delete 它（原指针随即失效）；传 `NULL` 等价于 `clear_bar()`
6. `release_bar()`：**消息放弃所有权**并返回指针；之后 `has_bar()` 为 `false`，**调用者负责 delete**

**示例/实践**
```cpp
Foo foo;

// 用法一（最常用）：mutable_ 自动分配，foo 拥有子消息
foo.mutable_bar()->set_xxx(1);

// 用法二：set_allocated_ 移交所有权——之后不要再碰这个指针
Bar* heap_bar = new Bar();
heap_bar->set_xxx(1);
foo.set_allocated_bar(heap_bar);   // foo 接管，禁止再 delete heap_bar

// 用法三：release_ 取回所有权——之后必须自己 delete
Bar* stolen = foo.release_bar();
// ... 使用 stolen ...
delete stolen;                      // 忘了 delete 就是内存泄漏
```

**注意点**
> ⚠️ **关键区分**：`mutable_` 拿到的指针**归消息所有**，千万别 delete；`release_` 拿到的指针**归你所有**，必须 delete。
> ⚠️ **警告**：`set_allocated_` 传入的指针**必须是堆对象**（new 出来的），传栈对象地址会灾难性崩溃。
> 💡 **理解技巧**：90% 场景用 `mutable_` 就够了；只有跨消息"搬移"子对象时才需要 `set_allocated_`/`release_`。
> 🔄 **知识关联**：RPC 框架填响应时就是 `response->mutable_result()->set_xxx(...)` 这种模式（见 notes/01）。
> 📋 **术语提醒**：`所有权（ownership）` 指"谁负责 delete 这个对象"。

---

<a id="id9"></a>
## ✅ 知识点9: 重复字段访问器

**`repeated` 就是动态数组：`_size` 查长度、索引访问、`add_` 追加、`clear_` 清空——但没有整体 `set_`。**

**通用方法**（以 `repeated int32 foo = 1;` 为例）：

```cpp
int foo_size() const;                    // 元素数量
int32_t foo(int index) const;            // 读第 index 个元素（越界 = 未定义行为）
void set_foo(int index, int32_t value);  // 改第 index 个元素
void add_foo(int32_t value);             // 尾部追加
void clear_foo();                        // 清空全部，foo_size() 归零
const RepeatedField<int32_t>& foo() const;  // 底层容器（只读）
RepeatedField<int32_t>* mutable_foo();      // 底层容器（可修改）
```

**按元素类型的差异：**

- **数值/枚举**：底层容器是 `RepeatedField<T>`；`add_foo(value)` 直接传值
- **字符串/字节**：底层容器是 `RepeatedPtrField<string>`；`set_foo(index, ...)` 与 `add_foo(...)` 有多种重载（`string_view` / `const string&` / `string&&` / `const char*` 等）；另有 `string* mutable_foo(int index)` 和无参 `string* add_foo()`（追加空串、返回指针再填）
- **消息**：底层容器是 `RepeatedPtrField<Bar>`；`add_bar()` 返回**新元素的可修改指针**（追加后接着填字段），`mutable_bar(int index)` 按索引取可修改指针
- 两种容器都提供**类似 STL 的迭代器**，可直接范围 for

**示例/实践**
```cpp
Foo foo;

// 追加元素
foo.add_score(95);            // 标量直接传值
Bar* b = foo.add_bars();      // 消息先拿新元素指针
b->set_xxx(1);                // 再填它的字段

// 遍历（两种方式）
for (int i = 0; i < foo.bars_size(); ++i) {
  const Bar& b = foo.bars(i);
}
for (const Bar& b : foo.bars()) {   // 范围 for，更简洁
  // ...
}
```

**注意点**
> ⚠️ **关键区分**：repeated 字段**没有**整体替换的 `set_foo(列表)`——想整体替换用 `clear_foo()` + 逐个 `add_foo()`，或拿 `mutable_foo()` 容器指针操作。
> ⚠️ **警告**：索引越界（超出 `[0, foo_size())`）是**未定义行为**，不会抛异常、不会自动扩容。
> 💡 **理解技巧**：`add_bars()` 之于消息类型，相当于 `push_back()` 后再返回新元素的指针。
> 📋 **术语提醒**：`RepeatedField`（标量用）与 `RepeatedPtrField`（字符串/消息用）是 Protobuf C++ 运行时的两种重复字段容器。

---

<a id="id10"></a>
## ✅ 知识点10: oneof 字段

**oneof 里同时最多只有一个成员被设置——给新成员赋值，会自动清掉旧成员。**

给定：

```proto
oneof example_name {
  int32 foo_int = 4;
  string foo_string = 9;
}
```

**生成的 case 枚举与判别方法：**

```cpp
enum ExampleNameCase {
  kFooInt = 4,              // 枚举值 = 字段编号
  kFooString = 9,
  EXAMPLE_NAME_NOT_SET = 0  // 一个都没设置
};

ExampleNameCase example_name_case() const;  // 查询当前哪个成员被设置
void clear_example_name();                  // 清空整个 oneof
```

**成员访问器的关键差异：**

- 每个成员的访问器与普通字段相同（`has_foo_int()`、`foo_int()`、`set_foo_int()`、`clear_foo_int()`……），但是——
- 调用 `set_` 或 `mutable_` 时，**如果 oneof 中其他成员正被设置，会先自动清除它**，再设置新成员，并把 oneof 情况（case）切到新成员
- 调用某成员自己的 `clear_` 只清自己；若它正是当前成员，oneof 回到 `EXAMPLE_NAME_NOT_SET`

**示例/实践**
```cpp
switch (msg.example_name_case()) {
  case MyMsg::kFooInt:
    std::cout << "int: " << msg.foo_int();
    break;
  case MyMsg::kFooString:
    std::cout << "string: " << msg.foo_string();
    break;
  case MyMsg::EXAMPLE_NAME_NOT_SET:
    std::cout << "什么都没设置";
    break;
}
```

**注意点**
> ⚠️ **关键区分**：`set_foo_int()` 之后再 `set_foo_string()`，`foo_int` 就**没了**——不是覆盖，是清除。
> ⚠️ **警告**：对消息类型成员调用 `mutable_` 同样会清掉其他成员——操作顺序要小心。
> 💡 **理解技巧**：oneof 就是"带标签的 union"——一块存储、多种解释，标签就是 `xxx_case()`。
> 📋 **术语提醒**：`oneof 情况（case）` 枚举的值就是字段编号，所以 `kFooInt = 4`。

---

<a id="id11"></a>
## ✅ 知识点11: 映射（map）字段

**map 字段生成 `google::protobuf::Map` 容器——接口是 `std::map`/`std::unordered_map` 常用方法的子集，手感很像。**

给定 `map<int32, int32> weight = 1;`，生成：

```cpp
const google::protobuf::Map<int32, int32>& weight() const;  // 只读
google::protobuf::Map<int32, int32>* mutable_weight();      // 可修改
```

**核心用法：**

- 支持迭代器（`begin`/`end`）、`size()`/`empty()` 等常见接口
- **添加数据最简单的方式**就是常规 map 语法：

```cpp
(*msg.mutable_weight())[my_key] = my_value;
```

- `insert(const value_type&)` 会**隐式深拷贝** value_type 实例；插入新值**最高效**的方式是 `operator[]`：`map[new_key] = new_mapped;`

**与标准 map 互转（注意：都是深拷贝）：**

```cpp
// Map → std::map
std::map<int32, int32> standard_map(msg.weight().begin(), msg.weight().end());

// std::map → Map
google::protobuf::Map<int32, int32> weight(standard_map.begin(), standard_map.end());
```

**注意点**
> ⚠️ **警告**：与标准 map 互转会**深拷贝整个表**，大表慎用。
> 💡 **理解技巧**：读用 `weight()`，写先 `mutable_weight()` 再 `operator[]`——和 `std::map` 的手感基本一致。
> 📋 **术语提醒**：`google::protobuf::Map` 的元素类型是 `MapPair<Key, T>`，遍历时用 `kv.first` / `kv.second`。

---

<a id="id12"></a>
## ✅ 知识点12: 枚举辅助函数与开放枚举陷阱

**每个枚举额外生成 `IsValid`/`Name`/`Parse` 等助手函数；proto3 枚举是"开放"的，switch 不带 default 可能出事故。**

给定：

```proto
enum Foo {
  VALUE_A = 0;
  VALUE_B = 5;
  VALUE_C = 1234;
}
```

**生成的辅助函数：**

- `const EnumDescriptor* Foo_descriptor()`：枚举`描述符(Descriptor)`，含定义了哪些值的信息
- `bool Foo_IsValid(int value)`：数值是否匹配某个定义值（上例中 0、5、1234 返回 `true`）
- `const string& Foo_Name(int value)`：数值 → 名字（`Foo_Name(5)` 返回 `"VALUE_B"`）
- `bool Foo_Parse(::absl::string_view name, Foo* value)`：名字 → 数值（`Foo_Parse("VALUE_C", &v)` 后 `v == 1234`）
- `Foo_MIN` / `Foo_MAX`：枚举的最小/最大有效值常量

**proto2 vs proto3 的关键差异：**

- **proto2（封闭枚举）**：把整数强转为枚举值时，该整数**必须**是有效值，否则行为未定义；给枚举字段设置无效值可能断言失败；解析出的无效枚举值会被当作**未知字段**。强转前请用 `Foo_IsValid()` 检查
- **proto3/editions（开放枚举）**：任何 `int32` 范围内的整数都可安全强转；解析出的未知枚举值会被**保留**，并通过枚举字段访问器原样返回

**switch 陷阱：**

- 对开放枚举写 `switch` 时，**即使列出所有已知值也覆盖不完**——未知值照样可能存在
- 没有 `default` 分支可能导致意外行为，包括**数据损坏和运行时崩溃**
- **始终添加 `default` case，或在 switch 外显式调用 `Foo_IsValid()` 处理未知值**

**注意点**
> ⚠️ **关键区分**：proto2 枚举"封闭"（无效值被拒绝/转未知字段），proto3 枚举"开放"（无效值照单全收）——跨版本迁移代码时最容易踩。
> 💡 **理解技巧**：`Foo_Name()` 打日志特别好用：`std::cout << Foo_Name(msg.type());` 直接打印枚举名而不是数字。
> 📋 **术语提醒**：`开放枚举（open enum）` 指允许持有定义之外数值的枚举；`封闭枚举（closed enum）` 则只允许定义值。

---

<a id="id13"></a>
## ✅ 知识点13: 服务（Services）——RPC 项目核心

**`protoc` 能为 `service` 生成 Service 基类（服务端用）和 Stub 类（客户端用）；但网络传输它不包——`RpcChannel` 和 `RpcController` 要你自己实现。这正是 QwenRPC 要干的事。**

**前置开关：**

```proto
option cc_generic_services = true;
```

- 该选项**默认为 `false`**（通用服务已被官方标记弃用）——不写这行，protoc 不会生成任何服务代码
- 官方建议正经 RPC 系统应提供 protoc `插件(plugin)` 生成定制代码；但学习/自研框架时，通用服务生成的类恰好够用

**接口（服务端基类）：**

给定服务定义：

```proto
service Foo {
  rpc Bar(FooRequest) returns(FooResponse);
}
```

生成名为 `Foo` 的类（`Service` 接口的子类），每个 rpc 方法生成一个**虚方法**：

```cpp
virtual void Bar(RpcController* controller, const FooRequest* request,
                 FooResponse* response, Closure* done);
```

- 是虚方法但**不是纯虚**：默认实现只是调用 `controller->SetFailed()`（报"方法未实现"），然后调用 `done` 回调
- **实现自己的服务 = 继承生成的类并重写需要的方法**
- 编译器还自动实现 `Service` 接口的方法：
  - `GetDescriptor`：返回服务的 `ServiceDescriptor`
  - `CallMethod`：根据方法描述符判断在调哪个方法，把请求/响应向下转型后直接分发到对应虚方法
  - `GetRequestPrototype` / `GetResponsePrototype`：返回给定方法的请求/响应默认实例

**存根（客户端代理）：**

- 同时生成 `Foo_Stub`（经 typedef 也可写作 `Foo::Stub`），是 `Foo` 的子类：

```cpp
Foo_Stub(RpcChannel* channel);  // 构造一个在给定通道上发送请求的 stub
RpcChannel* channel();          // 返回此 stub 的通道
```

- Stub 实现的每个服务方法**只是通道的包装器**——调用其中一个方法只是调用 `channel->CallMethod()`
- 另有带 `ChannelOwnership` 参数的构造函数：传 `Service::STUB_OWNS_CHANNEL` 表示 Stub 析构时连带 delete 通道

**关键事实：**

- **Protocol Buffer 库不包含 RPC 实现**——它只包含把生成的服务类连接到任意 RPC 实现所需的全部工具
- 你需要自己实现 `RpcChannel`（怎么把调用发到网络上）和 `RpcController`（超时、错误等调用控制），详见 `service.h` 的文档

**注意点**
> ⚠️ **关键区分**：`Service` 在提供端"真正干活"，`Stub` 在调用端"假装是本地函数"——两者是同一份 proto 生成的两面，接口签名一致。
> 💡 **理解技巧**：Stub 的所有方法最后都汇到同一个出口 `channel_->CallMethod()`——网络细节全藏在 Channel 里。
> 🔄 **知识关联**：notes/01 的 RPC 框架就是这个模式——服务端 `class UserService : public fixbug::UserServiceRpc` 重写虚方法；客户端 `fixbug::UserServiceRpc_Stub stub(new MprpcChannel())`，其中 `MprpcChannel` 就是自实现的 `RpcChannel`。
> 📋 **术语提醒**：`RpcController` 控制单次调用的状态（错误、超时）；`Closure` 是调用完成时的回调；`存根（Stub）` 一词源于"树桩"——看着像完整的树（函数），其实只是一小截本地代理。

---

<a id="id14"></a>
## ✅ 知识点14: 省略内容速查指引

**以下内容学习阶段很少用到，本笔记不展开——知道"有这么个东西、什么时候需要、去哪查"即可。**

| 被省略内容 | 是什么 | 什么时候才需要 |
|---|---|---|
| **Any** | 任意消息类型的"盒子"：`PackFrom` 打包、`UnpackTo` 解包、`Is` 判型 | 字段要装"运行时才确定类型"的消息（如错误详情） |
| **扩展（Extensions）** | proto2 的 `extensions`/`extend`：不改原消息定义也能加字段（`HasExtension`/`SetExtension`/`GetExtension` 等） | 维护 proto2 老系统；proto3 已移除该特性 |
| **Arena 分配** | C++ 专属内存池：批量分配、一次性释放 | 短期创建海量消息、性能分析发现内存分配是瓶颈时 |
| **插件插入点** | 给 protoc 插件预留的代码插入位置（`includes`/`class_scope` 等） | 你自己写代码生成器插件时 |
| **Cord** | `bytes` 字段改用 `absl::Cord` 存储（`[ctype=CORD]`） | 超大字节串场景；且是 v23+ 才有的特性 |
| **Abseil flag 支持** | 消息/枚举可直接用作 `ABSL_FLAG` 的类型 | 使用 Abseil 命令行标志库时 |
| **string_view API** | edition 2023 的 `features.(pb.cpp).string_type = VIEW` | 使用 editions 的新项目 |
| **map 解析未知值** | proto2/proto3/editions 对 map 条目中未知字段、未知枚举值的不同处理 | 跨版本解析 map 出现"数据莫名消失/变形"时排查 |

- 以上内容的权威说明都在本笔记来源页面：[C++ 生成代码指南](https://protobuf.com.cn/reference/cpp/cpp-generated/)

**注意点**
> ⚠️ **关键区分**：本项目 Docker 镜像是 **Protobuf 3.12.4**——不支持 editions，Cord、string_view VIEW 等特性**用不了**，在官方文档里看到相关章节直接跳过即可。
> 💡 **理解技巧**：这张表的价值在于"排除干扰"——官方文档很长，先知道哪些可以暂时不看，阅读速度能快一倍。
> 🔄 **知识关联**：日后真用到 Any、Arena 等，回到来源页面按对应小节查阅即可。

---

## 🔑 核心要点总结

1. **三步走**：写 `.proto` → `protoc --proto_path=… --cpp_out=…` 生成 `.pb.h`/`.pb.cc` → 与项目一起编译并链接 protobuf 库。
2. **结构映射**：`package` → `namespace`；`message` → 继承 `Message` 的具体类（**不要子类化**）；嵌套 message → `Foo_Bar` + typedef。
3. **presence 决定 `has_`**：显式存在（消息字段、oneof 成员、proto3 `optional`）有 `has_`；隐式存在（proto3 普通标量）没有。
4. **访问器规律**：单值字段 `has_`/get/`set_`/`clear_`；字符串加 `mutable_`；消息字段再加 `set_allocated_`/`release_`（所有权转移）；重复字段 `_size`/索引/`add_`/`clear_`；oneof 用 `xxx_case()` 判别；map 用 `mutable_xxx()` + `operator[]`。
5. **枚举**：`Foo_IsValid`/`Foo_Name`/`Foo_Parse` 三助手；proto3 开放枚举的 switch 必带 `default`。
6. **服务**：`option cc_generic_services = true` → 生成 Service 基类 + Stub；**库不含 RPC 实现**，`RpcChannel`/`RpcController` 要自己写——这正是 QwenRPC 的核心工作。

**最小上手路径**（新手照着走一遍就会了）：

1. `.proto` 里写 `message` 和字段编号
2. `protoc --cpp_out=. xxx.proto` 生成代码
3. 填数据：`set_xxx()` / `add_xxx()` / `mutable_xxx()`
4. 发送或存盘前：`SerializeToString(&bytes)`
5. 收到后：`ParseFromString(bytes)`
6. 读数据：`xxx()` getter；显式存在字段先 `has_xxx()` 判断

---

## 📌 考试速记版

**访问器速查表**（字段类型 × 生成方法）：

| 字段类型 | 生成方法 | 备注 |
|---|---|---|
| 单值数值/枚举 | `has_` get `set_` `clear_` | 隐式存在时无 `has_`；枚举 debug 模式校验值 |
| 单值字符串 | 四件套 + `mutable_` `set_allocated_` `release_` | `mutable_` 自动初始化为空串 |
| 单值消息 | `has_` get `mutable_` `clear_` `set_allocated_` `release_` | `mutable_` 自动分配；永远显式存在 |
| 重复数值/枚举 | `_size` 索引 get `set_(i,v)` `add_` `clear_` | 底层 `RepeatedField` |
| 重复字符串/消息 | 同上 + `mutable_(i)`、`add_()` 返回指针 | 底层 `RepeatedPtrField` |
| oneof 成员 | 普通访问器 + `xxx_case()` `clear_xxx()` | 设新成员自动清旧成员 |
| map | `xxx()` + `mutable_xxx()` 返回 Map | `operator[]` 插入最高效 |

**易混淆概念对比**：

- `mutable_` vs `set_allocated_` vs `release_`：借来改（所有权还在消息） vs 交出去（消息接管） vs 要回来（自己负责 delete）
- `RepeatedField` vs `RepeatedPtrField`：标量值容器 vs 字符串/消息容器
- 显式存在 vs 隐式存在：有 `has_`、能区分"没设"和"设成默认值" vs 无 `has_`、未设返回零值
- `Service` vs `Stub`：服务端基类（继承重写、真正干活） vs 客户端代理（转发到 Channel）

**记忆口诀**：单值四件套 has/get/set/clear，字符串多 mutable，消息再管所有权；重复 size/add/索引取，oneof 设新先清旧，map 就爱方括号；服务开关别忘开，Channel 自己来实现。
