# 📘 C++实现轻量级RPC分布式网络通信框架

> 来源说明：CSDN 博客《C++实现轻量级RPC分布式网络通信框架》，作者 Yugang_Yang (T_Solotov)，2022-04
> 原文链接：<https://blog.csdn.net/T_Solotov/article/details/124107667>
> 项目源码：<https://github.com/yyg192/yyg_rpc_server>
> 本篇涵盖：基于 `ZooKeeper + ProtoBuf + Muduo` 从零搭建 RPC 框架的完整设计与实现，作为 **QwenRPC** 开发的理论参考。
> 说明：原文部分代码以截图形式呈现，无法逐字提取；此类代码块依据原文流程描述整理，已标注 `// 依据原文整理`。

---

## 🧠 核心概念总览（严格按原文顺序）

> 📎 返回：[QwenRPC 仓库首页](../README.md)

**一、理论基础**
- [*知识点1: RPC 核心概念与整体架构*](#id1)
- [*知识点2: 三大技术支柱分工*](#id2)

**二、框架使用层**
- [*知识点3: ProtoBuf 协议定义与代码生成*](#id3)
- [*知识点4: Callee 业务层实现*](#id4)
- [*知识点5: Caller 业务层实现*](#id5)

**三、服务端 RpcServer 实现**
- [*知识点6: RpcProvider 核心设计*](#id6)
- [*知识点7: NotifyService 服务注册流程*](#id7)
- [*知识点8: ZooKeeper 原理*](#id8)
- [*知识点9: Watcher 机制与连接同步*](#id9)
- [*知识点10: RpcProvider::Run() 启动流程*](#id10)
- [*知识点11: OnMessage 请求分发流程*](#id11)

**四、客户端实现**
- [*知识点12: MprpcChannel::CallMethod 六步调用链*](#id12)

**五、辅助模块**
- [*知识点13: 异步日志模块*](#id13)

---

<a id="id1"></a>
## ✅ 知识点1: RPC 核心概念与整体架构

**RPC 要解决的核心问题：如何让客户端像调用本地函数一样，调用远端服务器上的函数？**

- 分布式系统中服务分散在不同机器上，直接裸写网络编程（socket、序列化、寻址）成本极高。RPC 框架把这些细节全部封装起来，向业务层提供"透明调用"的体验

- `远程过程调用(RPC, Remote Procedure Call)`：允许客户端程序**像调用本地函数一样**直接调用远端服务器上的函数
- 两个核心角色：
  - `服务调用方(Caller)`：发起远程调用的一端
  - `服务提供方(Callee)`：提供服务方法、执行本地函数并返回结果的一端
- 整体架构流程（原文流程图内容）：
  1. Callee 提前将自己的服务方法注册到配置中心
  2. Caller 发起调用时先查询目标服务所在的服务器地址
  3. Caller 通过网络向 Callee 发送调用请求
  4. Callee 执行本地方法，将结果通过网络返回给 Caller
  ![图片来自作者“我在地铁站吃闸机”](images/1.png)

- 下图把上述 4 步流程细化到 `桩(Stub)` 与序列化层面，共 7 步——对调用方来说，第 2~7 步全部由框架完成：

  ```mermaid
  sequenceDiagram
      participant C as 调用方 Caller
      participant S as 桩 Stub
      participant R as 注册中心<br>(服务发现)
      participant P as 提供方 Callee

      Note over P,R: 服务启动时：注册 服务名 → IP:Port
      C->>S: 1. 调用 stub.Login(request)
      S->>R: 2. 查询 Login 服务的地址
      R-->>S: 3. 返回目标服务器 IP:Port
      S->>P: 4. 请求序列化后经 TCP 发送
      P->>P: 5. 反序列化 → 定位方法 → 执行本地函数
      P-->>S: 6. 结果序列化后返回
      S-->>C: 7. 反序列化，像本地函数一样拿到返回值
  ```


> 💡 **理解技巧**：RPC 的本质 = **函数调用的语义 + 网络传输的实现**。业务代码只看到 `stub.Login(request, response)`，底下的序列化、寻址、TCP 收发全被框架"藏"起来了


---

<a id="id2"></a>
## ✅ 知识点2: 三大技术支柱分工

**一个最小可用的 RPC 框架 = 服务寻址 + 数据序列化 + 网络收发，三者各由一个成熟组件承担**

| 组件 | 角色 | 核心能力 |
|------|------|----------|
| `ZooKeeper` | **服务注册与发现中心** | 分布式服务协调中间件，管理"服务名 → IP:Port"映射 |
| `ProtoBuf(Protocol Buffers)` | **序列化/反序列化** | Key-Value **二进制**格式存储，体积小，适合网络传输 |
| `Muduo` | **网络通信库** | 基于 `(Multi-)Reactor` 模型的多线程网络库，高并发处理远端请求 |

**各组件协作方式：**
- **ZooKeeper**：
  - 服务方法提供者提前将本端对外提供的**服务方法名及自己的通信地址信息（IP:Port）注册到 ZooKeeper**。
  - Caller 发起调用时，先向 ZooKeeper 查询目标服务所在的服务器地址，再向该服务器（Callee）发起请求
- **[ProtoBuf](https://protobuf.com.cn/overview/)**：
  - Caller 和 Callee 之间的数据交互全部先经 ProtoBuf 序列化成二进制字节流再走网络
- **[Muduo](https://github.com/chenshuo/muduo)**：
  - 承担 RPC 框架中的网络通信部分（监听、连接、收发）

**注意点**
> ⚠️ **关键区分**：三者职责**完全正交**——ZooKeeper 只管"找到谁"，ProtoBuf 只管"说什么格式"，Muduo 只管"怎么送到"。设计 QwenRPC 时应保持这种分层解耦。
> 💡 **理解技巧**：类比寄快递——ZooKeeper 是地址簿，ProtoBuf 是打包规范，Muduo 是快递员。


---

<a id="id3"></a>
## ✅ 知识点3: ProtoBuf 协议定义与代码生成

**一份 `.proto` 文件，protoc 编译后生成"孪生两类"——理解这对类的分工是理解整个框架的钥匙**

原文通过一个 Login/Register 业务场景展示框架用法：先在 `.proto` 中定义消息体和服务，再用 protoc 编译生成 C++ 代码。

**协议定义（`user.proto`）：**
- 定义的消息体：`LoginRequest`、`LoginResponse`、`RegisterResponse`（内部嵌套 `ResultCode` 错误码消息）
- 注册服务：`service UserServiceRpc`，包含 `Login` 和 `Register` 两个 rpc 方法

**示例/实践**
```protobuf
// user.proto —— 依据原文整理（原文为代码截图）
syntax = "proto3";
package fixbug;

option cc_generic_services = true;   // 生成 service 类（Stub 与 Service）

message ResultCode {                 // 嵌套在响应中的错误码
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

// 在 proto 中注册服务及其方法
service UserServiceRpc {
    rpc Login (LoginRequest) returns (LoginResponse);
    rpc Register (RegisterRequest) returns (RegisterResponse);
}
```

```bash
# 编译命令（原文）
protoc user.proto -I ./ --cpp_out=./user
```

**编译生成的两个关键类：**
- `UserServiceRpc_Stub`：**给 Caller 用**——桩类，持有 channel，把方法调用转发到网络
- `UserServiceRpc`：**给 Callee 用**——服务基类，业务方**继承它并重写虚方法**提供真正实现

**注意点**
> ⚠️ **关键区分**：`Stub` 类与 `Service` 类是同一服务的两面——**Stub 在调用端"假装是函数"，Service 在提供端"真正干活"**。两者接口签名一致，由同一份 proto 生成。
> 💡 **理解技巧**：`桩(Stub)` 一词源于"树桩"——看起来是完整的树（函数），实际只是留在本地的一小截，真正的"树干"在远端。
> 📋 **术语提醒**：`序列化(Serialization)` / `反序列化(Deserialization)`；`服务描述符(ServiceDescriptor)`、`方法描述符(MethodDescriptor)` 将在知识点7 中用于反射。

---

<a id="id4"></a>
## ✅ 知识点4: Callee 业务层实现

**Callee 只需做一件事：继承 protoc 生成的服务基类，重写虚方法，最后 `done->Run()`**

这是框架"对业务友好"的体现：业务方完全不接触网络代码，只写纯业务逻辑。

**实现流程（`userservice.cc`）：**
1. 定义 `UserService` **继承** `UserServiceRpc`
2. **重写**虚方法 `Login`
3. 从 `request` 中提取参数
4. 调用本地业务逻辑 `Login(name, pwd)`
5. 将结果写入 `response`
6. 调用 `done->Run()` 发送结果

**示例/实践**
```cpp
// userservice.cc —— 依据原文流程整理（原文为代码截图）
class UserService : public fixbug::UserServiceRpc {  // 继承 protoc 生成的服务基类
public:
    // 本地业务逻辑（与网络完全无关）
    bool Login(std::string name, std::string pwd) {
        std::cout << "doing local service: Login" << std::endl;
        return true;
    }

    // 重写基类虚方法：框架收到远端 Login 请求后会多态调用到这里
    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done) override {
        // 1. 从 request 取出远端传来的参数
        std::string name = request->name();
        std::string pwd  = request->pwd();
        // 2. 调用本地业务
        bool ret = Login(name, pwd);
        // 3. 把结果写入 response（错误码、错误消息、返回值）
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(ret);
        // 4. 执行回调：触发 response 的序列化与网络发送
        done->Run();
    }
};

int main(int argc, char **argv) {
    MprpcApplication::Init(argc, argv);          // 初始化框架（读配置）
    RpcProvider provider;                        // 网络服务发布对象
    provider.NotifyService(new UserService());   // 把服务注册进框架
    provider.Run();                              // 启动服务，进入事件循环
    return 0;
}
```

**注意点**
> ⚠️ **关键区分**：`done->Run()` 不是"函数结束"，而是**主动触发框架回调**（内部执行 `SendRpcResponse`，见知识点11）——忘记调用则响应永远发不出去。
> 💡 **理解技巧**：重写的 `Login` 有 4 个参数（controller/request/response/done），但业务只关心中间两个：**request 拿进来，response 填出去**。
> 🔄 **知识关联**：框架如何多态调用到这个重写方法，见 [知识点11](#id11) 的 `service->CallMethod()`。

---

<a id="id5"></a>
## ✅ 知识点5: Caller 业务层实现

**Caller 端三行核心代码：造 Stub、填 Request、调方法——远程调用被伪装成本地调用**

**实现流程（`calluserservice.cc`）：**
1. 初始化框架：`MprpcApplication::Init(argc, argv)`
2. 创建 `UserServiceRpc_Stub` 对象，**构造时传入 `MprpcChannel`**
3. 构造 `LoginRequest`（原文示例：name="zhang san", pwd="123456"）
4. 调用 `stub.Login()` 发起远端调用
5. 检查 `response` 中的错误码并打印结果

**示例/实践**
```cpp
// calluserservice.cc —— 依据原文流程整理（原文为代码截图）
int main(int argc, char **argv) {
    MprpcApplication::Init(argc, argv);               // 初始化框架

    // Stub 构造时注入自定义 Channel：所有 rpc 方法最终都走 channel->CallMethod
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    fixbug::LoginRequest request;                     // 组装请求参数
    request.set_name("zhang san");
    request.set_pwd("123456");

    fixbug::LoginResponse response;
    stub.Login(nullptr, &request, &response, nullptr); // 发起远端调用（同步阻塞）

    if (0 == response.result().errcode()) {            // 检查错误码，读取结果
        std::cout << "rpc login response: " << response.success() << std::endl;
    } else {
        std::cout << "rpc login error: " << response.result().errmsg() << std::endl;
    }
    return 0;
}
```

**注意点**
> ⚠️ **关键区分**：`stub.Login()` **不执行任何业务逻辑**——protoc 生成的 Stub 方法体内部只有一行转发：`channel_->CallMethod(...)`（见知识点12）。
> 💡 **理解技巧**：Caller 与 Callee 代码里出现的是**同一份** proto 生成的 request/response 类型——协议文件就是两端的"合同"。
> 🔄 **知识关联**：`MprpcChannel` 是用户对 `google::protobuf::RpcChannel` 虚基类的自定义实现，完整调用链见 [知识点12](#id12)。

---

<a id="id6"></a>
## ✅ 知识点6: RpcProvider 核心设计

**框架如何做到不依赖任何具体业务类？——用 `google::protobuf::Service` 基类指针实现多态解耦**

`RpcProvider` 是服务端框架核心类，负责将本地服务方法注册到 ZooKeeper、接受 Caller 的远端调用并返回结果。对外只暴露两个方法：`NotifyService` 和 `Run`。

**核心数据结构：**

**示例/实践**
```cpp
// rpcprovider.h —— 原文提取
class RpcProvider {
public:
    void NotifyService(google::protobuf::Service *service);  // 注册服务
    void Run();                                               // 启动网络服务
private:
    muduo::net::EventLoop m_eventLoop;                        // Muduo 事件循环

    // 一个服务对象 + 它的所有方法描述符
    struct ServiceInfo {
        google::protobuf::Service *m_service;                 // 服务对象（基类指针 → 多态）
        std::unordered_map<std::string,
            const google::protobuf::MethodDescriptor*> m_methodMap;  // 方法名 → 方法描述符
    };
    // 服务名 → 服务信息：框架可同时管理多个服务
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    // 回调函数：OnConnection, OnMessage, SendRpcResponse
};
```

**设计要点：**
- **多态解耦**：`NotifyService` 参数是 `google::protobuf::Service*` **基类指针**——框架层完全不知道 `UserService` 的存在，与业务层解耦
- **两级映射**：`m_serviceMap`（服务名 → ServiceInfo）→ `m_methodMap`（方法名 → MethodDescriptor），收到请求后两次查表即可定位到具体方法

**注意点**
> ⚠️ **关键区分**：框架依赖的是 **protobuf 的抽象层**（Service/MethodDescriptor），而非业务类型——这是"框架代码不 include 业务头文件"的关键。
> 💡 **理解技巧**：`m_serviceMap` 就是一张"电话总机转接表"：先按服务名找到部门（ServiceInfo），再按方法名转接到具体分机（MethodDescriptor）。
> 🔄 **知识关联**：这张表在 [知识点7](#id7) 中被填充，在 [知识点11](#id11) 中被查询。

---

<a id="id7"></a>
## ✅ 知识点7: NotifyService 服务注册流程

**不写一行硬编码，靠 protobuf 反射机制自动"摸清"服务的全部方法**

`NotifyService` 的任务：把业务传入的服务对象登记进 `m_serviceMap`。它利用 protobuf 的描述符体系在运行时获取服务元信息。

**注册流程（`rpcprovider.cc`）：**
1. 通过服务对象获取其 `服务描述符(ServiceDescriptor)`
2. 从描述符获取**服务名**
3. **遍历该服务的所有方法**，获取每个方法的 `方法描述符(MethodDescriptor)`
4. 填充 `ServiceInfo`（服务对象指针 + 方法映射表）
5. 插入 `m_serviceMap`

**示例/实践**
```cpp
// rpcprovider.cc —— 依据原文流程整理（原文为代码截图）
void RpcProvider::NotifyService(google::protobuf::Service *service) {
    ServiceInfo service_info;

    // 1. 反射：获取服务描述符
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 2. 服务名，如 "UserServiceRpc"
    std::string service_name = pserviceDesc->name();
    // 3. 遍历服务下的所有方法
    int methodCnt = pserviceDesc->method_count();
    for (int i = 0; i < methodCnt; ++i) {
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();       // 如 "Login"
        service_info.m_methodMap.insert({method_name, pmethodDesc});
    }
    // 4~5. 记录服务对象并入总表
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}
```

**注意点**
> 💡 **理解技巧**：`反射(Reflection)` 在这里指——不需要知道类型的源码，运行时就能问出"你叫什么、有哪些方法"。C++ 本身无反射，**protobuf 用生成的描述符类补上了这块能力**。
> 📋 **术语提醒**：`ServiceDescriptor`（服务描述符）↔ `MethodDescriptor`（方法描述符），二者都来自 `.proto` 编译期生成的元数据。

---

<a id="id8"></a>
## ✅ 知识点8: ZooKeeper 原理

**服务名挂"永久节点"，方法名挂"临时节点"——一棵内存树管理整个集群的服务地址**

ZooKeeper 是一个 `分布式服务协调中间件(Distributed Coordination Middleware)`。在本框架中：Callee 将服务方法及网络地址注册到 ZooKeeper；Caller 通过 ZooKeeper 发现目标服务器地址。

**数据存储方式：**
- ZooKeeper 在**内存中维护树状目录结构**，每个 `节点(znode)` 数据上限 **1MB**
- **服务名以 `永久性节点(Persistent Node)` 存在**，如 `/UserServiceRpc`
- **方法名以 `临时性节点(Ephemeral Node)` 存在**，如 `/UserServiceRpc/Login`
- 节点数据内容为 RpcServer 的 **IP:Port**

**线程模型（`zookeeper_mt` 多线程版客户端库）：**
1. **主线程**：用户调用 API 的线程
2. **IO 线程**：负责与 ZooKeeper 服务端的网络通信
3. **completion 线程**：处理异步请求回调及 `Watcher` 响应

**客户端封装类：**

**示例/实践**
```cpp
// zookeeperutil.h —— 原文提取
class ZkClient {
public:
    void Start();   // 连接 ZooKeeper server
    void Create(const char *path, const char *data, int datalen, int state = 0); // 创建节点
    std::string GetData(const char *path);  // 读取节点数据（Caller 用它拿 IP:Port）
private:
    zhandle_t *m_zhandle;   // ZooKeeper 客户端句柄
};
```

**注意点**
> ⚠️ **关键区分**：为什么方法名用**临时节点**？——临时节点与客户端**会话(Session)** 绑定，**RpcServer 宕机断连后节点自动删除**，Caller 不会再查到已死服务的地址；永久节点则保留服务的"目录骨架"。
> 💡 **理解技巧**：把 ZooKeeper 想成一个"带心跳检测的分布式电话簿"——商户（服务）注销或倒闭（宕机），电话簿自动撕掉那一页。
> 🔄 **知识关联**：节点在 [知识点10](#id10) `Run()` 中被创建；地址在 [知识点12](#id12) `CallMethod` 中被查询。
> 📋 **术语提醒**：`znode` = ZooKeeper 树中的节点；`Persistent(永久)` vs `Ephemeral(临时)` 是节点的两种基本类型。

---

<a id="id9"></a>
## ✅ 知识点9: Watcher 机制与连接同步

**`zookeeper_init` 是异步的——如何让 `Start()` 等到"真正连上"才返回？答案：信号量 + Watcher 回调**

`Watcher` 机制类似于**观察者模式在分布式场景下的实现**：

**Watcher 机制流程：**
- 客户端将 Watcher **注册到服务端**，同时保存在**客户端的 Watcher 管理器**中
- 当 `znode` 发生变化时，**服务端通知客户端**，客户端 Watcher 管理器触发对应回调
- **Watcher 是一次性的：触发后立即销毁**（如需持续监听必须重新注册）

**`ZkClient::Start()` 的同步化技巧（状态机式流程）：**

- **状态1: 发起异步连接**
  - 事件: 用户调用 `Start()`
  - 动作: 调用 `zookeeper_init(host, global_watcher, ...)`（**该函数立即返回，连接是异步建立的**） → 创建信号量 `sem_t sem`，初值 0
  - 下一状态: 阻塞等待
- **状态2: 阻塞等待**
  - 事件: `zoo_set_context(m_zhandle, &sem)` 已把信号量指针存入句柄
  - 动作: 主线程执行 `sem_wait(&sem)` **阻塞**
  - 下一状态: 回调触发
- **状态3: 回调触发**
  - 事件: 连接真正建立，IO/completion 线程触发 `global_watcher` 回调
  - 动作: 回调中检查状态为 `ZOO_CONNECTED_STATE` → 从句柄取回信号量 → `sem_post(&sem)` **解除主线程阻塞**
  - 下一状态: 连接就绪，`Start()` 返回

**示例/实践**
```cpp
// zookeeperutil.cc —— 依据原文流程整理（原文为代码截图）
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx) {
    if (type == ZOO_SESSION_EVENT) {              // 会话类型事件
        if (state == ZOO_CONNECTED_STATE) {        // 与服务端连接成功
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);                         // 唤醒阻塞在 sem_wait 的主线程
        }
    }
}

void ZkClient::Start() {
    // 异步建立连接：返回 != 连接成功
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);   // 把信号量塞进句柄，供回调取用

    sem_wait(&sem);                     // 阻塞，直到 global_watcher 中 sem_post
    std::cout << "zookeeper_init success" << std::endl;
}
```

**注意点**
> ⚠️ **关键区分**：`zookeeper_init` **返回成功 ≠ 连接成功**——它只是创建了句柄并启动异步连接；真正的"已连接"信号来自 Watcher 回调中的 `ZOO_CONNECTED_STATE`。这是本文**最高频的面试考点**。
> ⚠️ **关键区分**：**Watcher 一次性触发，触发后即销毁**——与常驻的观察者模式监听器不同。
> 💡 **理解技巧**：`信号量(semaphore)` 在这里当"门铃"用：主线程按下 `sem_wait` 在门口等，回调线程 `sem_post` 按响门铃放行——**用同步原语把异步 API 包装成同步接口**。
> 🔄 **知识关联**：三线程模型（[知识点8](#id8)）解释了为什么回调在**另一个线程**（completion 线程）执行，所以需要跨线程同步。

---

<a id="id10"></a>
## ✅ 知识点10: RpcProvider::Run() 启动流程

**六步走：读配置 → 建 TcpServer → 设线程 → 连 ZK → 挂节点 → 进循环**

`Run()` 是服务端的总装配线，把 Muduo 网络服务与 ZooKeeper 注册串联起来（状态机式流程）：

- **状态1: 读取配置**
  - 事件: `Run()` 被调用
  - 动作: 从配置文件读取 RpcServer 的 `IP` 和 `Port` → 构造监听地址
  - 下一状态: 创建网络服务
- **状态2: 创建网络服务**
  - 事件: 地址就绪
  - 动作: 创建 Muduo `TcpServer` 对象 → **绑定 `OnConnection` 和 `OnMessage` 回调**
  - 下一状态: 设置线程
- **状态3: 设置线程**
  - 事件: TcpServer 创建完成
  - 动作: 设置 **4 个 SubEventLoop 线程**（Multi-Reactor：主循环管连接，子循环管读写）
  - 下一状态: 连接 ZooKeeper
- **状态4: 连接 ZooKeeper**
  - 事件: 网络层就绪
  - 动作: `ZkClient::Start()` 连接 ZooKeeper 服务器（见知识点9 的同步机制）
  - 下一状态: 注册服务节点
- **状态5: 注册服务节点**
  - 事件: ZK 连接成功
  - 动作: **遍历 `m_serviceMap`**：为每个服务创建**永久性节点**（如 `/UserServiceRpc`）→ 为每个方法创建**临时性节点**（如 `/UserServiceRpc/Login`），**节点数据 = 本机 IP:Port**
  - 下一状态: 事件循环
- **状态6: 事件循环**
  - 事件: 全部注册完成
  - 动作: `server.start()` 启动网络服务 → `m_eventLoop.loop()` **进入事件循环，阻塞等待请求**
  - 下一状态: （常驻运行，直到进程退出）

**注意点**
> ⚠️ **关键区分**：必须**先连 ZK 再注册节点、先注册完再进事件循环**——顺序颠倒会导致 Caller 查到地址却连不上，或服务已可连但查不到。
> 💡 **理解技巧**：`Run()` 之后线程就"交给" Muduo 了——后续一切逻辑都由回调（`OnMessage`）驱动，这是 Reactor 模型的典型形态。
> 🔄 **知识关联**：`OnMessage` 回调的处理逻辑即 [知识点11](#id11)。

---

<a id="id11"></a>
## ✅ 知识点11: OnMessage 请求分发流程

**收到字节流后的七步分发：反序列化 → 两次查表 → 闭包封装 → 多态调用 → 回发响应**

`OnMessage` 是服务端处理每一次 RPC 请求的核心回调（状态机式流程）：

- **状态1: 接收请求**
  - 事件: Muduo 通知连接上有数据可读
  - 动作: 接收 Caller 发来的远程 RPC 调用请求**字符流**
  - 下一状态: 反序列化
- **状态2: 反序列化**
  - 事件: 字符流接收完毕
  - 动作: 反序列化字符流 → 解析出 `service_name`、`method_name` 和**参数**
  - 下一状态: 查表定位
- **状态3: 查表定位**
  - 事件: 服务名/方法名已知
  - 动作: 从 `m_serviceMap` 查到服务对象（`Service*`）→ 从 `m_methodMap` 查到方法描述符（`MethodDescriptor*`）
  - 下一状态: 准备响应与闭包
- **状态4: 准备响应与闭包**
  - 事件: 服务与方法均已定位
  - 动作: 创建空的 `response` 对象 → 用 `NewCallback` 创建 `闭包(Closure)` 对象，**封装 `SendRpcResponse` 函数**
  - 下一状态: 多态调用
- **状态5: 多态调用**
  - 事件: 参数就绪
  - 动作: `service->CallMethod(method, nullptr, request, response, done)` → 框架根据 `method` 描述符**多态地调用到 `UserService` 中重写的 `Login`**
  - 下一状态: 业务执行
- **状态6: 业务执行**
  - 事件: 进入重写的 `Login`
  - 动作: 从 request 取参 → 执行本地业务 → 填写 response → **`done->Run()`**
  - 下一状态: 发送响应
- **状态7: 发送响应（`SendRpcResponse`）**
  - 事件: `done->Run()` 被触发
  - 动作: 将 `response` **序列化**后通过网络发送给 Caller → **主动关闭连接（模拟 HTTP 短连接）**
  - 下一状态: 处理结束，等待下一请求

**示例/实践**
```cpp
// rpcprovider.cc 核心片段 —— 依据原文流程整理（原文为代码截图）
// NewCallback 创建闭包对象：其 Run() 会执行 RpcProvider::SendRpcResponse(conn, response)
google::protobuf::Closure *done =
    google::protobuf::NewCallback<RpcProvider,
                                  const muduo::net::TcpConnectionPtr&,
                                  google::protobuf::Message*>
        (this, &RpcProvider::SendRpcResponse, conn, response);

// 关键一行：按 method 描述符多态分发到业务重写的方法（如 UserService::Login）
service->CallMethod(method, nullptr, request, response, done);
```

**注意点**
> ⚠️ **关键区分**：`CallMethod` 是 protoc 为每个 Service 生成的**分发器**——内部按 `method->index()` switch 到对应虚方法。框架调用它而**不直接调用 `Login`**，因此不需要知道业务类型。
> 💡 **理解技巧**：`闭包(Closure)` 在这里 = "一张预填好参数的待办卡片"——业务层不需要懂网络，只要在做完事后执行 `done->Run()`，发送逻辑自动完成。
> ⚠️ **关键区分**：发送完响应后**服务端主动断开连接**（短连接模式）——每次 RPC 调用都要重新建连，这也是后续可优化点（连接复用）。
> 🔄 **知识关联**：状态6 即 [知识点4](#id4) 中业务层重写的方法体。

---

<a id="id12"></a>
## ✅ 知识点12: MprpcChannel::CallMethod 六步调用链

**Stub 所有方法的"总出口"——客户端框架的全部秘密都在这一个函数里**

protoc 生成的 `UserServiceRpc_Stub::Login` 内部只做一件事：调用 `channel_->CallMethod(...)`。`channel_` 是 `google::protobuf::RpcChannel` 类型（**虚基类**），用户必须实现派生类 `MprpcChannel` 并重写 `CallMethod()`。

**六步调用链（状态机式流程）：**

- **状态1: 组装标识**
  - 事件: `stub.Login()` 转发进入 `CallMethod`
  - 动作: 组装 `service_name` + `method_name`（从 method 描述符获取）
  - 下一状态: 序列化
- **状态2: 序列化**
  - 事件: 请求标识就绪
  - 动作: 用 ProtoBuf 将请求参数**序列化**为字节流（并组装协议头：服务名/方法名/参数长度）
  - 下一状态: 服务发现
- **状态3: 服务发现**
  - 事件: 数据打包完成
  - 动作: **查询 ZooKeeper**（`GetData("/服务名/方法名")`）获取目标 RpcServer 的 `IP:Port`
  - 下一状态: 建立连接
- **状态4: 建立连接**
  - 事件: 拿到目标地址
  - 动作: 向目标服务器**发起 TCP 连接**
  - 下一状态: 发送请求
- **状态5: 发送请求**
  - 事件: 连接建立
  - 动作: 发送序列化后的请求数据
  - 下一状态: 等待结果
- **状态6: 等待结果**
  - 事件: 数据发出
  - 动作: **阻塞等待**接收服务端返回 → 反序列化到 `response` → `CallMethod` 返回，`stub.Login()` 随之返回
  - 下一状态: 调用完成，业务代码读取 `response`

**注意点**
> ⚠️ **关键区分**：`RpcChannel` 是 protobuf 留给框架实现者的**扩展点(hook)**——protobuf 只规定"Stub 会把调用交给 Channel"，网络细节全由 `MprpcChannel` 自己实现。
> 💡 **理解技巧**：对照服务端（知识点11）看，客户端调用链与服务端处理链**镜像对称**：序列化↔反序列化、查 ZK↔注册 ZK、发送↔接收。
> ⚠️ **关键区分**：当前实现为**同步阻塞调用**——客户端异步调用在原文中列为未实现功能（见知识点14）。

---

<a id="id13"></a>
## ✅ 知识点13: 异步日志模块

**日志不能拖慢主流程——用"消息队列 + 后台线程"把写盘从业务线程剥离**

- 核心思路：借助 `消息队列(Message Queue)`，业务/网络线程把日志**压入队列**即返回，由**单独的后台线程**负责将日志从队列取出并**写入磁盘文件**
- 这是典型的 `生产者-消费者模型(Producer-Consumer Model)`：多个工作线程是生产者，唯一的写盘线程是消费者
- 原文说明：具体实现见项目源码

**注意点**
> 💡 **理解技巧**：磁盘 I/O 比内存操作慢几个数量级——若在网络回调里直接写文件，高并发下事件循环会被拖垮。
> 🔄 **知识关联**：Muduo 库本身的 `AsyncLogging` 也是同样的设计思想（双缓冲 + 后台线程），QwenRPC 可参考。
> 📋 **术语提醒**：`异步日志(Asynchronous Logging)`；队列需要**线程安全**（互斥锁 + 条件变量）。

---

<a id="id14"></a>
## ✅ 知识点14: 待完善功能 → QwenRPC Roadmap

**原文作者列出的"未完成清单"，恰好是 QwenRPC 可以超越参考项目的迭代方向**

原文明确指出该框架**尚处于迭代中**，以下功能未实现：

| 待实现功能 | 说明 | 对 QwenRPC 的意义 |
|-----------|------|------------------|
| `客户端异步调用(Async Call)` | 当前 CallMethod 为同步阻塞 | 可引入 future/promise 或回调式 API |
| `负载均衡(Load Balancing)` | 同一服务多实例时如何选择 | ZK 下挂多个地址 + 轮询/随机/一致性哈希策略 |
| `异常重试(Retry)` | 网络抖动导致的调用失败 | 需配合幂等性设计 |
| `健康检测(Health Check)` | 主动探测服务实例存活 | 与 ZK 临时节点的被动下线互补 |
| `熔断机制(Circuit Breaker)` | 下游持续故障时快速失败 | 防止雪崩，参考三态熔断器（关闭/打开/半开） |

**注意点**
> 💡 **理解技巧**：这五项正是"玩具级 RPC"与"生产级 RPC"（如 gRPC、bRPC）之间的主要差距。
> 🔄 **知识关联**：作者另有两篇配套文章可作延伸阅读——《RPC框架基础概念篇》（纯理论）与《长文梳理Muduo库核心代码及优秀编程细节剖析》。
> ⚠️ **关键区分**：短连接（知识点11）也是隐含的待优化点——生产框架普遍使用**长连接 + 连接池**。



---


