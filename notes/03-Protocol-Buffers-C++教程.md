# 📘 Protocol Buffers C++ 教程（C++ Tutorial）

> 来源说明：[Protocol Buffers 官方文档中文站 — C++ 基础教程](https://protobuf.com.cn/getting-started/cpptutorial/) | 本节涵盖：用 C++ 定义 `.proto`、编译生成类、使用 Protobuf API 读写消息、兼容规则与性能优化

---

## 🧠 核心概念总览（严格按原文顺序）
> 📎 返回：[KopiRPC 仓库首页](../README.md)

- [*知识点1: 教程目标与问题领域*](#id1)
- [*知识点3: 定义 `.proto` 协议格式*](#id3)
- [*知识点4: 字段基数：singular 与 repeated*](#id4)
- [*知识点5: 编译 `.proto` 文件*](#id5)
- [*知识点6: 生成类的字段访问器（singular）*](#id6)
- [*知识点7: 生成类的字段访问器（repeated）*](#id7)
- [*知识点8: 枚举与嵌套类*](#id8)
- [*知识点9: 标准消息方法*](#id9)
- [*知识点10: 序列化与反序列化 API*](#id10)
- [*知识点11: 写入消息的完整流程*](#id11)
- [*知识点12: 读取消息的完整流程*](#id12)
- [*知识点13: 扩展 Protocol Buffer 的兼容规则*](#id13)
- [*知识点14: 优化技巧与高级用法*](#id14)
- [*知识点15: 定义服务接口（service 与 rpc）*](#id15)
- [*知识点16: RpcChannel——框架要实现的网络插头*](#id16)

---

<a id="id1"></a>
## ✅ 知识点1: 教程目标与问题领域


**教程目标**
- 在 `.proto` 文件中定义消息格式；
- 使用 Protocol Buffer 编译器 `protoc`；
- 使用 C++ Protocol Buffer API 编写和读取消息。

- **问题领域：为什么需要 Protobuf？**

  - 对于一个地址簿程序，从文件读写人们的联系方式（姓名、ID、邮箱、电话）。在 Protobuf 出现之前，常见的序列化方法各有问题：

    - **直接写内存结构的二进制**：脆弱，要求接收方使用完全相同的内存布局、字节序，难以扩展格式。
    - **把数据编码成单个字符串**：例如 `"12:3:-23:67"`。简单灵活，但需要手写编码/解析代码，解析有运行时开销，只适合非常简单的情况。
    - **XML**：人类可读、多语言绑定好，适合跨应用共享数据。但体积大、编解码性能开销高、遍历 DOM 比访问简单字段复杂。

- **Protobuf 的优势**：
  - 开发者编写 `.proto` 文件描述数据结构；
  - `protoc` 自动生成类，自动完成高效的二进制编码和解析；
  - 生成的类提供字段的 getter 和 setter，并负责消息的整体读写；
  - 格式可随时间扩展，新代码仍能读取旧格式编码的数据。


> ⚠️ **关键区分**：本教程是 C++ 入门示例，不是 C++ Protobuf 的完整参考。
> 💡 **理解技巧**：可以把 `.proto` 当成「数据类的设计图」，`protoc` 就是按图纸自动建造 C++ 类的工厂。
> 📋 **术语提醒**：`序列化（serialization）` 指把内存对象变成可存储/传输的字节流；`反序列化（deserialization）` 是逆过程。


---

<a id="id3"></a>
## ✅ 知识点3: 定义 `.proto` 协议格式

**地址簿应用从 `addressbook.proto` 文件开始**

- `.proto` 文件为每个要序列化的数据结构定义一个 `message`，并为消息中的每个字段指定名称和类型。

- **示例/实践**
  ```proto
  edition = "2023";

  package tutorial;

  message Person {
    string name = 1;
    int32 id = 2;
    string email = 3;

    enum PhoneType {
      PHONE_TYPE_UNSPECIFIED = 0;
      PHONE_TYPE_MOBILE = 1;
      PHONE_TYPE_HOME = 2;
      PHONE_TYPE_WORK = 3;
    }

    message PhoneNumber {
      string number = 1;
      PhoneType type = 2;
    }

    repeated PhoneNumber phones = 4;
  }

  message AddressBook {
    repeated Person people = 1;
  }
  ```

- **核心机制：**

  1. **`edition` 声明**
      - 文件以 `edition = "2023"` 开头
      - `Editions` 取代了旧的 `syntax = "proto2"` 和 `syntax = "proto3"`，提供更灵活的语言演进方式

  2. **`package` 声明**
      - 用于防止不同项目之间的命名冲突
      - 在 C++ 中，生成的类将置于与包名匹配的命名空间中（本例为 `tutorial`）

  3. **消息定义 `message`**
      - 是一组类型化字段的聚合。
      - 可用字段类型包括 `bool`、`int32`、`float`、`double`、`string` 等
      - 消息类型可作为其他消息的字段类型：`Person` 包含 `PhoneNumber`，`AddressBook` 包含 `Person`
      - 消息类型可以嵌套定义：`PhoneNumber` 定义在 `Person` 内部

  4. **枚举类型 `enum`**
      - 使字段值为预定义列表之一
      - 第一个枚举值必须为 0（如 `PHONE_TYPE_UNSPECIFIED = 0`）

  5. **字段编号**
      - 每个字段后的 `= 1`、`= 2` 等是字段在二进制编码中的唯一编号
      - **字段编号 1–15 比更高编号少占用一个字节**，适合作为常用或重复元素的优化
      - 16 及以上用于不常用元素
  6. **类型继承**
    - 所有的消息类型都继承自`Message`类
      ![alt text](images/3.png)

> ⚠️ **关键区分**：字段编号不是赋值，而是二进制格式中的「字段身份证」
> 💡 **理解技巧**：把 1–15 号字段想象成「短途火车票」，更便宜（省字节）；16 以上是「长途票」，稍贵
> 📋 **术语提醒**：`package` 在 C++ 中对应 `namespace`，在 Java 中对应 `package`

---

<a id="id4"></a>
## ✅ 知识点4: 字段基数：singular 与 repeated

**字段基数 `基数（cardinality）` 描述一个字段可以出现多少次。**

- 在 `.proto` 中，主要有两种基数：
  - **singular**（单数字段）

    - **默认行为**：没有特殊标记的普通字段就是 singular。
    - 字段可能已设置，也可能未设置。
    - 未设置时使用类型特定的**默认值**：
      - 数字类型：0
      - 字符串：空字符串
      - 布尔值：false
      - 枚举：第一个定义的枚举值（必须为 0）
    - **不能显式把字段标记为 `singular`**，它只是对非重复字段的描述性说法。
      ```cpp
        tutorial::Person person;
        person.set_name("张三");   // name 设置了
        // id 和 email 根本没碰 —— 完全没问题，不会报错
        // name 输出: 张三（设过，返回实际值）
        // id 输出: 0  （没设 → int32 默认值）
      ```
  - **repeated**（可重复字段）

    - 字段可以重复任意次数，包括零次
    - 重复值的顺序保持不变
    - 可将其视为**动态大小的数组**

- `required` 已废弃
  - 旧版本 Protobuf 支持 `required` 关键字，但实践证明它很脆弱。
  - **现代 Protobuf（包括 Editions）不再支持 `required`**。
  - Editions 中虽有功能可启用类似行为，但仅用于向后兼容。

**注意点**
> ⚠️ **关键区分**：`singular` 不是关键字，不能写在 `.proto` 里；`repeated` 是必须显式写的关键字。
> 💡 **理解技巧**：`repeated` 就是「这个字段可以出现 0 次、1 次或很多次」。
> 📋 **术语提醒**：`默认值（default value）` 是字段未被设置时 Protobuf 自动返回的值。

---

<a id="id5"></a>
## ✅ 知识点5: 编译 `.proto` 文件

**有了 `.proto` 文件后，需要使用 `protoc` 编译器生成 C++ 类**

- 流程：
  - **阶段1: 安装 protoc**
    - 下一状态：执行编译命令

  - **阶段2: 执行编译命令**
    - 事件：运行 `protoc`
    - 动作：指定源目录、目标目录和 `.proto` 文件路径
    - 下一状态：生成 C++ 源文件

    - **编译命令**
      ```bash
      protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/addressbook.proto
      ```
    - 参数说明：
      - `-I=$SRC_DIR`：指定源目录（`.proto` 文件所在位置），不提供则使用当前目录
      - `--cpp_out=$DST_DIR`：指定生成 C++ 代码的输出目录
      - `$SRC_DIR/addressbook.proto`：要编译的 `.proto` 文件路径

- 由于需要 C++ 类，所以使用 `--cpp_out`。其他语言有类似选项，如 `--java_out`、`--python_out`
- **编译后会在目标目录生成两个文件**：
    - `addressbook.pb.h`：声明生成类的头文件
    - `addressbook.pb.cc`：生成类的实现文件

> ⚠️ **关键区分**：`-I` 是 include path，不是输出目录；`--cpp_out` 才是输出目录
> 💡 **理解技巧**：`.pb.h` 和 `.pb.cc` 就是普通的 C++ 头文件和源文件，编译你的程序时一起编译即可
> 📋 **术语提醒**：`protoc` 是 Protocol Buffers 编译器的命令行工具

---

<a id="id6"></a>
## ✅ 知识点6: 生成类的字段访问器（singular）

**`.proto` 中定义的每个消息对应生成一个 C++ 类，编译器为每个字段生成访问器方法**

- 以 `Person` 中的 `name`、`id`、`email` 三个 singular 字段为例，生成的访问器如下：

  ```cpp
  // name
  bool has_name() const;        // Only for explicit presence
  void clear_name();
  const ::std::string& name() const;
  void set_name(const ::std::string& value);
  ::std::string* mutable_name();

  // id
  bool has_id() const;
  void clear_id();
  int32_t id() const;
  void set_id(int32_t value);

  // email
  bool has_email() const;
  void clear_email();
  const ::std::string& email() const;
  void set_email(const ::std::string& value);
  ::std::string* mutable_email();
  ```

- **访问器规则：**

  1. **getter**
      - 名称与字段的小写名称相同，如 `name()`、`id()`
      - 返回字段当前值；未设置时返回默认值

  2. **setter**
      - 以 `set_` 开头，如 `set_name(...)`、`set_id(...)`

  3. **`has_` 方法**
      - 仅对具有**显式存在跟踪**的 singular 字段提供
      - 字段已设置时返回 `true`
> ⚠️ **关键区分**：`has_` 只存在于「显式存在（explicit presence）」字段；隐式存在的 singular 字段没有 `has_`

  4. **`clear_` 方法**
      - 每个字段都有，将字段重置为默认状态

  5. **字符串字段的 `mutable_` getter**
      - 如 `mutable_name()`、`mutable_email()`
      - **返回字符串指针，可直接修改其内容**
      - 即使字段尚未设置，调用 `mutable_email()` 也会自动初始化为空字符串

> 💡 **理解技巧**：`mutable_` 就是「给我一个指向内部字符串的指针，我要直接改它」
> 📋 **术语提醒**：`显式存在（explicit presence）` 指 Protobuf 会跟踪字段是否被显式设置过

---

<a id="id7"></a>
## ✅ 知识点7: 生成类的字段访问器（repeated）

**重复字段 `repeated` 的访问器与 `singular` 字段不同，因为它表示一个动态数组**

- 以 `Person::phones` 为例，生成的访问器如下：

  ```cpp
  int phones_size() const;
  void clear_phones();
  const ::google::protobuf::RepeatedPtrField< ::tutorial::Person_PhoneNumber >& phones() const;
  ::google::protobuf::RepeatedPtrField< ::tutorial::Person_PhoneNumber >* mutable_phones();
  const ::tutorial::Person_PhoneNumber& phones(int index) const;
  ::tutorial::Person_PhoneNumber* mutable_phones(int index);
  ::tutorial::Person_PhoneNumber* add_phones();
  ```

- **repeated 字段访问器规则：**
  1. **`_size()`**：返回重复字段的长度，即元素数量
  2. **`clear_()`**：清空整个列表
  3. **`phones()`**：返回整个重复字段的只读引用
  4. **`mutable_phones()`**：返回整个列表的可修改指针
  5. **按索引访问**：
      - `phones(int index)`：只读访问第 `index` 个元素。
      - `mutable_phones(int index)`：可修改访问第 `index` 个元素
  6. **`add_()`**：
      - 添加一个新元素并返回其指针
      - 返回后可继续编辑该新元素
      - 重复标量类型也有 `add_` 方法，可直接传入新值
> ⚠️ **关键区分**：repeated 字段用 `add_` 新增元素，用索引访问已有元素，不能用 `set_phones(...)`

- **关键区别：**
  - `repeated` 字段**没有 `set_` 方法**，因为不能一次性替换整个列表（需要用 `clear` + `add` 或 `mutable_`）

> 💡 **理解技巧**：把 `repeated PhoneNumber phones` 想象成 `std::vector<PhoneNumber>`，`add_phones()` 就像 `push_back()`
> 📋 **术语提醒**：`RepeatedPtrField` 是 Protobuf C++ 运行时提供的重复消息字段容器类

---

<a id="id8"></a>
## ✅ 知识点8: 枚举与嵌套类

**`.proto` 中的枚举和嵌套消息会被编译器生成对应的 C++ 枚举和嵌套类。**

- **枚举**

  - `.proto` 中的 `enum PhoneType` 生成 C++ 枚举 `Person::PhoneType`
  - 枚举值名称前会加上类型前缀，如：
    - `Person::PHONE_TYPE_MOBILE`
    - `Person::PHONE_TYPE_HOME`
    - `Person::PHONE_TYPE_WORK`

- **嵌套类**

  - `.proto` 中嵌套定义的 `message PhoneNumber` 生成嵌套类 `Person::PhoneNumber`
  - 实际生成的类名是 `Person_PhoneNumber`
  - Protobuf 在 `Person` 内部定义了 typedef，因此可以当作 `Person::PhoneNumber` 使用

- **前向声明的限制**

  - 不能在 C++ 中前向声明嵌套类型 `Person::PhoneNumber`
  - 但可以前向声明实际的类名 `Person_PhoneNumber`

- **示例/实践**

  - protoc 生成的代码（概念示意）——**不是真嵌套类，而是「压平 + typedef」**：

    ```cpp
    // 真实生成的类：全局类，名字用下划线压平
    class Person_PhoneNumber : public google::protobuf::Message { ... };

    // Person 类内部只放了一行 typedef（别名）
    class Person : public google::protobuf::Message {
    public:
      typedef Person_PhoneNumber PhoneNumber;
      ...
    };
    ```

  - 日常使用：两个名字**完全等价**，推荐用别名（可读性好）：

    ```cpp
    tutorial::Person::PhoneNumber phone1;   // 别名
    tutorial::Person_PhoneNumber phone2;    // 真名，与上行是同一个类
    Person::PhoneNumber* p = person.add_phones();  // 教程写法走的就是别名
    ```

  - 前向声明场景：头文件里只想「预告」类名、不想 `#include` 整个 `.pb.h` 时，**只能用真名**：

    ```cpp
    namespace tutorial {
    class Person_PhoneNumber;      // ✅ 合法：真名是独立的顶层类
    // class Person::PhoneNumber;  // ❌ 编译错误：别名写在 Person 类体内，
                                  //    不看 Person 完整定义就无法引用
    }
    void PrintPhone(const tutorial::Person_PhoneNumber* phone);
    ```

> ⚠️ **关键区分**：日常写代码用 `Person::PhoneNumber`，前向声明时要用 `Person_PhoneNumber`。
> 💡 **理解技巧**：`typedef` 就是给 `Person_PhoneNumber` 起了一个别名 `Person::PhoneNumber`，方便使用。
> 💡 **设计原因**：protoc 故意「压平 + typedef」而非生成真嵌套类，正是为了让前向声明成为可能——真嵌套类无论如何都无法前向声明。
> 📋 **术语提醒**：`前向声明（forward declaration）` 是在不给出完整定义的情况下声明一个类存在。

---

<a id="id9"></a>
## ✅ 知识点9: 标准消息方法

**每个消息类还包含检查或操作整个消息的方法，这些方法实现了 `Message` 接口。**

- 常用标准方法：
  - `bool IsInitialized() const;`
    - 检查所有必填字段是否已设置
  - `string DebugString() const;`
    - 返回消息的可读表示，对调试特别有用
  - `void CopyFrom(const Person& from);`
    - 用给定消息的值覆盖当前消息
  - `void Clear();`
    - 将所有元素清除回空状态

- 这些方法以及 I/O 方法实现了所有 C++ Protocol Buffer 类共享的 `Message` 接口


> ⚠️ **关键区分**：`Clear()` 清除整个消息的所有字段；`clear_xxx()` 只清除单个字段。
> 💡 **理解技巧**：`DebugString()` 是你调试时的好朋友，可以像 `std::cout << person.DebugString()` 这样打印消息内容。
> 📋 **术语提醒**：`Message` 是 Protobuf C++ 运行时中所有生成消息类的公共基类接口。

---

<a id="id10"></a>
## ✅ 知识点10: 序列化与反序列化 API

**每个 Protobuf 类都提供使用二进制格式写入和读取消息的方法。**

常用方法：

- `bool SerializeToString(string* output) const;`
  - 序列化消息并将字节存储在给定字符串中。
  - 字节是二进制而非文本，`string` 仅作为方便的容器。

- `bool ParseFromString(const string& data);`
  - 从给定字符串解析消息。

- `bool SerializeToOstream(ostream* output) const;`
  - 将消息写入给定的 C++ `ostream`。

- `bool ParseFromIstream(istream* input);`
  - 从给定的 C++ `istream` 解析消息。

- 这些方法只是部分解析和序列化选项，完整列表可参阅 `Message` API 参考


> ⚠️ **关键区分**：`SerializeToString` 生成的不是文本字符串，而是二进制字节流；只是借用了 `std::string` 作为容器。
> 💡 **理解技巧**：`SerializeToOstream` 适合写文件；`SerializeToString` 适合在内存中传递。
> 🔄 **知识关联**：这些 API 是知识点 11、12 中写入/读取程序的核心。

---

<a id="id11"></a>
## ✅ 知识点11: 写入消息的完整流程

**地址簿程序首先将个人详细信息写入地址簿文件：读取旧文件 → 添加新人 → 写回文件**

- FSM 风格流程：

  - **阶段1: 初始化库**
    - 事件：程序启动
    - 动作：调用 `GOOGLE_PROTOBUF_VERIFY_VERSION`
    - 下一状态：打开输入文件

  - **阶段2: 读取已有地址簿**
    - 事件：打开文件
    - 动作：如果文件不存在则创建新文件；否则用 `ParseFromIstream` 解析
    - 下一状态：添加新联系人

  - **阶段3: 添加新联系人**
    - 事件：用户输入信息
    - 动作：用 `add_people()` 创建 `Person`，填充字段
    - 下一状态：写回文件

  - **阶段4: 写回文件**
    - 事件：准备保存
    - 动作：用 `SerializeToOstream` 写入文件
    - 下一状态：清理或退出

  - **阶段5: 清理（可选）**
    - 事件：程序即将退出
    - 动作：调用 `ShutdownProtobufLibrary()`
    - 下一状态：结束

- **示例/实践**
  ```cpp
  #include <iostream>
  #include <fstream>
  #include <string>
  #include "addressbook.pb.h"
  using namespace std;


  int main(int argc, char* argv[]) {
    // 验证链接的库版本与编译头文件版本兼容
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc != 2) {
      cerr << "Usage:  " << argv[0] << " ADDRESS_BOOK_FILE" << endl;
      return -1;
    }

    tutorial::AddressBook address_book;

    {
      // 读取已有地址簿
      fstream input(argv[1], ios::in | ios::binary);
      if (!input) {
        cout << argv[1] << ": File not found.  Creating a new file." << endl;
      } else if (!address_book.ParseFromIstream(&input)) {
        cerr << "Failed to parse address book." << endl;
        return -1;
      }
    }

    // 添加一个地址
    // 根据用户输入填充 Person 消息
    PromptForAddress(*address_book.add_people()); 

    {
      // 将新地址簿写回磁盘
      fstream output(argv[1], ios::out | ios::trunc | ios::binary);
      if (!address_book.SerializeToOstream(&output)) {
        cerr << "Failed to write address book." << endl;
        return -1;
      }
    }

    // 可选：删除 libprotobuf 分配的所有全局对象
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
  }
  ```

- **关键说明：**

  - **`GOOGLE_PROTOBUF_VERIFY_VERSION`**：
  - 使用 C++ Protobuf 库前执行此宏是好习惯。
  - 验证没有意外链接到与编译所用头文件版本不兼容的库版本。
  - 如果检测到版本不匹配，程序将中止。
  - 每个 `.pb.cc` 文件在启动时都会自动调用此宏。

  - **`ShutdownProtobufLibrary()`**：
    - 删除 Protobuf 库分配的所有全局对象。
    - 大多数程序不需要，因为进程退出时操作系统会回收内存。
    - 但如果使用需要释放每个对象的内存泄漏检查器，或编写可能由单个进程多次加载和卸载的库，可能需要强制清理。


> ⚠️ **关键区分**：`GOOGLE_PROTOBUF_VERIFY_VERSION` 不是严格必要，但强烈建议；`ShutdownProtobufLibrary()` 绝大多数情况下不需要。
> 💡 **理解技巧**：`add_people()` 返回的是 `Person*` 指针，可以直接用 `*add_people()` 解引用后传给 `PromptForAddress`。
> 📋 **术语提醒**：`Arena` 相关知识见知识点 14。

---

<a id="id12"></a>
## ✅ 知识点12: 读取消息的完整流程

**读取程序打开地址簿文件，解析所有联系人并打印信息。**

- FSM 风格流程：

  - **阶段1: 初始化库**
    - 事件：程序启动
    - 动作：调用 `GOOGLE_PROTOBUF_VERIFY_VERSION`
    - 下一状态：打开输入文件

  - **阶段2: 解析地址簿**
    - 事件：文件已打开
    - 动作：用 `ParseFromIstream` 将文件内容解析到 `AddressBook`
    - 下一状态：遍历打印

  - **阶段3: 遍历并打印**
    - 事件：解析成功
    - 动作：遍历 `people()`，再遍历每个 `Person` 的 `phones()`
    - 下一状态：清理或退出

- **示例/实践**
  ```cpp
  #include <iostream>
  #include <fstream>
  #include <string>
  #include "addressbook.pb.h"
  using namespace std;

  int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc != 2) {
      cerr << "Usage:  " << argv[0] << " ADDRESS_BOOK_FILE" << endl;
      return -1;
    }

    tutorial::AddressBook address_book;

    {
      // 读取地址簿
      fstream input(argv[1], ios::in | ios::binary);
      if (!address_book.ParseFromIstream(&input)) {
        cerr << "Failed to parse address book." << endl;
        return -1;
      }
    }

    ListPeople(address_book); // 遍历 AddressBook 中所有人并打印信息

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
  }
  ```
- 读取时用 `ParseFromIstream`；写入时用 `SerializeToOstream`
> 💡 **理解技巧**：遍历 repeated 字段就像遍历 `std::vector`，用范围 for 循环即可
> 📋 **术语提醒**：`has_email()` 检查的是显式存在；如果 `email` 是隐式存在字段，则没有 `has_email()`

---

<a id="id13"></a>
## ✅ 知识点13: 扩展 Protocol Buffer 的兼容规则

**发布使用 Protobuf 的代码后，可能需要改进 `.proto` 定义。要保持向后兼容和向前兼容，需遵守字段编号规则**

- **必须遵守的规则**
  1. **不得更改任何现有字段的字段编号**
      - 字段编号是二进制格式中的身份标识，一旦确定就不能改。

  2. **可以删除 singular 或 repeated 字段**
      - 删除后，旧代码读取新消息时该字段取默认值；
      - 旧代码发送的旧消息中新代码看不到该字段。

  3. **可以添加新的 singular 或 repeated 字段**
      - 但必须使用**新的字段编号**；
      - 「新编号」指该 Protocol Buffer 中从未使用过的编号，包括已被删除的字段。

- **兼容效果**
  - 旧代码读取新消息：忽略新字段。
  - 旧代码读取被删除字段：取默认值（singular）或空列表（repeated）。
  - 新代码读取旧消息：缺失的新字段使用默认值。
  - 但新字段不会出现在旧消息中，因此使用前需要检查默认值来确认其存在。


> ⚠️ **关键区分**：删除字段后其编号也不能复用，否则会导致旧数据被错误解析。
> 💡 **理解技巧**：字段编号就像数据库列 ID，可以删列、加列，但不能改已有列的 ID，也不能复用被删列的 ID。
> 🔄 **知识关联**：与 `notes/02-Protocol-Buffers-概览.md` 知识点 6 的兼容规则一致。

---

<a id="id14"></a>
## ✅ 知识点14: 优化技巧与高级用法

**C++ Protobuf 库已经过严格优化，但正确使用还能进一步提升性能；同时 Protobuf 也提供更高级的反射能力。**

- **优化技巧**

  1. **使用 Arena 进行内存分配**
      - 在短期操作中创建大量消息时，系统内存分配器可能成为瓶颈。
      - Arena 以低开销执行多次分配，并一次性释放所有分配。
      - 适合消息密集型应用。

        ```cpp
        google::protobuf::Arena arena;
        tutorial::Person* person = google::protobuf::Arena::Create<tutorial::Person>(&arena);
        // ... populate person ...
        ```

      - Arena 对象销毁时，其上分配的所有消息都会被释放。

  2. **尽可能重用非 Arena 消息对象**
      - 消息会保留为重用分配的内存，即使被清除后也是如此。
      - 连续处理多个相同类型和相似结构的消息时，重用相同消息对象可减轻内存分配器负担。
      - 但对象会随时间膨胀，应通过 `SpaceUsed` 方法监控大小，过大时删除。

  3. **注意重用 Arena 消息的风险**
      - 重用 Arena 消息可能导致无限制内存增长。
      - 重用堆消息更安全。
      - 即使使用堆消息，也可能遇到字段高水位线问题：两个字段分别在不同消息中达到最大长度时，重用一个消息会让两个字段都保留最大内存。

  4. **使用 TCMalloc**
      - 系统内存分配器可能未针对多线程分配大量小对象优化，可尝试 Google 的 TCMalloc。

  5. **`string` 数据用 `bytes` 类型表示**
      - `string`字段装的是**人读的文本**，承诺内容是合法 UTF-8 文本，序列化时会**扫描校验 UTF-8**
      - `bytes` 装的是**机器读的字节**，不承诺任何格式，**零校验直通**
        - **bytes 为什么更快**：数据被序列化后为二进制数据本就不是合法 UTF-8，塞进 `string` 字段意味着每次序列化/解析都对整个 payload 白扫一遍（O(n)）；RPC 高频路径上每次调用都省一遍扫描。
      - **业务层需要前者`string`的保证，框架层需要后者`bytes`的中立**：
        - RPC 框架的职责是"把这块数据从 A 搬到 B"，不是"搬运一段文本"，用 bytes 是在类型系统层面强制这个抽象边界
      - C++ 的 `std::string` 本质是**字节容器**（可装 `\0` 和任意二进制），所以 protoc 编译后 `bytes` 就是 `std::string`；`string`/`bytes` 的区分是给**契约读者、运行时校验器、其他语言的生成代码**看的，不是给 C++ 类型系统看的

- **高级用法：反射 Reflection**
  - Protobuf 消息类提供**反射**能力。
  - 反射允许迭代消息的字段并操作其值，而无需针对特定消息类型编写代码。
  - 实用用途：将 Protobuf 消息与 XML 或 JSON 等其他编码相互转换。
  - 更高级用途：查找同一类型两个消息之间的差异，或开发「协议消息的正则表达式」。
  - 反射通过 `Message::Reflection` 接口提供。


> ⚠️ **关键区分**：Arena 适合「大量短生命周期消息」；重用对象适合「连续处理同构消息」。
> ⚠️ **关键区分**：二进制放进 `string` 字段，跨语言时会被当文本解码导致**数据损坏**（Java `String` vs `ByteString`）——不只是性能问题。
> 💡 **理解技巧**：Arena 就像一块临时画布，画完一整批画后一起扔掉，不用一幅一幅单独清理。
> 📋 **术语提醒**：`反射（reflection）` 指程序在运行时检查自身数据结构的能力。

---

<a id="id15"></a>
## ✅ 知识点15: 定义服务接口（service 与 rpc）

**Protobuf 还能定义服务接口（service）——这是 RPC 的入口**

- **语法**：`service` 把一组 `rpc` 方法定义成一份「接口合同」：

  ```proto
  syntax = "proto3";
  package fixbug;

  option cc_generic_services = true;   // 关键开关：让 protoc 生成服务代码

  message ResultCode {                // 响应中嵌套的错误码
    int32 errcode = 1;
    bytes errmsg  = 2;
  }
  message LoginRequest {
    bytes name = 1;
    bytes pwd  = 2;
  }
  message LoginResponse {
    ResultCode result = 1;
    bool success      = 2;
  }
  message RegisterRequest {
    int32 id   = 1;
    bytes name = 2;
    bytes pwd  = 3;
  }
  message RegisterResponse {
    ResultCode result = 1;
    bool success      = 2;
  }

  // 在 proto 中注册服务及其方法
  service UserServiceRpc {
    rpc Login (LoginRequest) returns (LoginResponse);
    rpc Register (RegisterRequest) returns (RegisterResponse);
  }
  ```

- **语法规则：**
  1. `rpc 方法名 (请求消息) returns (响应消息);`
  2. **请求和响应必须都是 message**，不能是 `int32` 等标量——所以每个 rpc 方法都要配套定义一对消息
  3. 一个 `service` 可包含任意多个 `rpc` 方法

- **关键开关 `option cc_generic_services = true;`**：
  - protobuf 3.x 的 C++ 生成器**默认不生成服务代码**，必须显式打开（本项目镜像是 3.12.4，需要它）
  - 更新的 protobuf（v22+，含 Editions）已移除该选项，官方推荐改用 gRPC 插件生成服务代码

- **编译生成两个类——同一服务的两面：**

  | 生成的类 | 给谁用 | 干什么 |
  |---|---|---|
  | `UserServiceRpc`（Service 基类） | 服务端 Callee | **继承它并重写虚方法**，填真正的业务逻辑 |
  | `UserServiceRpc_Stub`（桩类） | 客户端 Caller | 构造时传入 `RpcChannel`，**像调本地函数一样发远程调用** |

  - 虚方法四参数签名：`void Login(RpcController*, const LoginRequest*, LoginResponse*, Closure* done);`
  - **protobuf 库本身不含网络实现**：`RpcChannel` / `RpcController` 是抽象接口，需要框架自己实现
- 类型的继承：
  - **所有服务类都继承自 `service` 基类**
  ![alt text](images/4.png)
    - `Login`：proto 里每行 rpc 生成一个虚函数——服务端继承后重写它，填业务逻辑。
    - `GetDescriptor()`：返回服务的元数据（服务名/方法名）——框架靠它在运行时不认识你的类也能注册和分发调用。
    - **就这两个角色：虚函数填业务，`GetDescriptor` 给框架当地图**
  - **`UserServiceRpc_Stub` 就是客户端那一半——同一个 `service` 生成的两个类，服务端用基类，客户端用 Stub**：
    - 它也继承 `UserServiceRpc`：
      - 所以方法签名和服务端一模一样，`stub.Login(...)` 调起来就是个普通本地函数
    - **构造 `stub` 类时传入一个 `RpcChannel`**：
      - `UserServiceRpc_Stub(RpcChannel* channel);`
    - **函数体只有一行——通过 `channel` 转发调用**：
      ```cpp
        void UserServiceRpc_Stub::Login(controller, request, response, done) {
        channel->CallMethod(descriptor()->method(0),  // 告诉 channel：调的是第 0 个方法
                            controller, request, response, done);
      }
      ```
    - 顺带看点妙的：
      - `descriptor()->method(0)` 就是 `GetDescriptor()` 那份元数据
      - `Stub` 靠它告诉 `channel`"我调的是哪个方法"

> ⚠️ **关键区分**：`message` 定义「数据长什么样」，`service` 定义「能调什么方法」——proto 文件的两种顶级定义。
> ⚠️ **坑提醒**：忘了 `option cc_generic_services = true;`，生成的代码里就没有 Service/Stub 类。
> 💡 **理解技巧**：`service` 就是一组 rpc 方法的「合同」；Stub 在调用端"假装是函数"，Service 在提供端"真正干活"。

---

<a id="id16"></a>
## ✅ 知识点16: `RpcChannel` ——框架要实现的网络插头

**`UserServiceRpc_Stub` 的每个方法都只是转发给 `RpcChannel`**

- **而 `RpcChannel` 是 protobuf 只定义、不实现的抽象接口——网络传输由框架自己写**

- **是什么**：`google::protobuf::RpcChannel` 抽象接口，代表「客户端通往服务端的通信通道」。核心方法只有一个：

  ```cpp
  virtual void CallMethod(const MethodDescriptor* method,
                          RpcController* controller,
                          const Message* request,   // 序列化前的请求
                          Message* response,        // 待填充的响应
                          Closure* done) = 0;
  ```

- **为什么只有接口没有实现**：protobuf 不知道你想用什么网络库（TCP？HTTP？muduo？）——它只规定「插口形状」，电流（网络传输）由框架自己接。

- **漏斗设计**：Stub 的**每个** rpc 方法体都汇成同一行 `channel_->CallMethod(...)`——所有调用流经同一个漏斗，网络逻辑**只需写一次**，而不是每个 rpc 方法写一遍。

- **调用链全貌**：

  ```text
  stub.Login(...)                      // 业务方调用，像本地函数
    └→ channel_->CallMethod(...)       // Stub 只负责转发（protoc 生成）
         └→ RpcChannel::CallMethod   // 框架手写：真正的网络收发
  ```

- **框架实现 `CallMethod` 的六步**：
  1. 从 `method` 描述符取出服务名 + 方法名
  2. 序列化 `request`
  3. 拼 RPC 头部（服务名、方法名、参数长度）
  4. 查 ZooKeeper 拿到服务端 IP:Port
  5. 建 TCP 连接并发送
  6. 接收响应 → 反序列化进 `response` → `done->Run()`

- **姊妹接口 `RpcController`**：单次调用的「控制面板」——报错 `SetFailed`、取消等，框架同样可自行实现。

> ⚠️ **关键区分**：`Stub` 是 protoc **生成**的（只负责转发），`RpcChannel` 是框架**手写**的（负责网络）——客户端代码一半白拿、一半自己造
> 💡 **理解技巧**：`RpcChannel` 就像电源插座的标准——protobuf 只规定插口形状，框架（`RpcChannel`）才是插上去的电器
> 📋 **术语提醒**：`通道（channel）` 在 RPC 语境中泛指「一次调用从客户端到服务端的传输通路」

---

## 🔑 核心要点总结

1. **Protobuf C++ 开发三步走**：写 `.proto` → `protoc --cpp_out` 生成 `.pb.h`/`.pb.cc` → 在 C++ 代码中构建/序列化/反序列化。
2. **字段基数**：`singular` 是默认单数字段；`repeated` 是动态数组，用 `add_` 添加元素。
3. **singular 字段访问器**：getter、setter、`has_`、`clear_`、字符串额外有 `mutable_`。
4. **repeated 字段访问器**：`_size()`、索引访问、`mutable_`、无 `set_`。
5. **标准方法**：`IsInitialized`、`DebugString`、`CopyFrom`、`Clear` 实现 `Message` 接口。
6. **序列化 API**：`SerializeToString`、`SerializeToOstream`、`ParseFromString`、`ParseFromIstream`。
7. **兼容规则**：字段编号不可改、可删字段、新增字段必须用全新编号。
8. **性能优化**：Arena 批量分配、对象重用、TCMalloc、二进制负载用 `bytes`；高级用法包括反射。
9. **服务定义**：`service` + `rpc` 定义接口；`option cc_generic_services = true;` 生成 Service 基类（服务端继承重写）与 Stub（客户端调用）；`RpcChannel` 需框架自己实现。
10. **RpcChannel**：Stub 的所有方法都转发到 `channel_->CallMethod`；该接口 protobuf 只定义不实现，网络收发由框架（`MprpcChannel`）手写一次、全部 rpc 复用。

---