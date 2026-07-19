# QwenRPC

>QwenRPC (Qinwen's Remote Procedure Call) —— C++ 高性能远程程序调用服务框架

---

## 📖 什么是 RPC？

- `RPC (Remote Procedure Call, 远程过程调用)` 是一种通信技术：让程序**像调用本地函数一样，调用另一台机器上的函数**，而无需关心底层的网络细节。

    ```cpp
    // 本地调用：函数就在当前进程里
    bool ok = Login("zhang san", "123456");

    // RPC 调用：函数实际运行在远端服务器上，但写法几乎一样
    stub.Login(nullptr, &request, &response, nullptr);
    ```

- 一句话总结：**RPC 的本质 = 函数调用的语义 + 网络传输的实现**。

---
## 🤔 为什么需要 RPC？


**1. 简化远程调用的复杂性**

- 在分布式系统中，服务往往部署在不同的机器或网络中。如果没有 RPC，每一次跨机调用，开发人员都需要手动编写复杂的网络通信代码：
    - **创建和管理连接**
    - **数据的序列化和反序列化**
    - **请求和响应的发送与接收**
    - **错误处理（如超时、断线重连等）**

- RPC 对上述底层逻辑进行了统一封装，开发人员只需专注于业务逻辑，无需关心底层的通信细节。

**2. 提升开发效率**

- **抽象通信过程**：只需定义远程调用的接口（如方法签名），无需实现复杂的通信逻辑
- **统一接口定义**：主流 RPC 框架（如 gRPC）提供 IDL（接口描述语言），只需编写一份接口定义，框架即可自动生成客户端和服务端代码
- **减少出错概率**：复杂的通信逻辑由框架封装，避免手写网络代码时的常见错误（如序列化失败、超时处理不当等）

--- 
## 🎯 RPC 的应用场景

- RPC 不仅用于微服务，还广泛应用于以下场景：

    1. **分布式系统** —— 解决跨网络的通信问题，提供一致的调用体验
    2. **跨语言服务调用** —— 如 gRPC 支持 C++、Java、Python 等多语言，实现异构服务之间的互相调用
    3. **高性能内部通信** —— 利用高性能序列化协议和连接复用能力，减少网络通信开销
    4. **服务间依赖调用** —— 如电商系统中，订单服务调用库存服务、库存服务调用支付服务，RPC 让服务间协作高效可靠

--- 

## 📚 学习笔记

- 想揭开 RPC 的奥秘，推荐参考以下学习笔记：
    - [01 - C++实现轻量级RPC分布式网络通信框架（ZooKeeper + ProtoBuf + Muduo）](notes/01-RPC框架实现-ZooKeeper-ProtoBuf-Muduo.md)
    - [02 - Protocol Buffers 概览](notes/02-Protocol-Buffers-概览.md)
    - [03 - Protocol Buffers C++ 教程](notes/03-Protocol-Buffers-C++教程.md)

- 前置网络知识补充：
    - [高性能网络编程基础知识（I/O 多路复用，poll/select/epoll，Reactor/Proactor）](https://github.com/QinWenYan1/Network_System/tree/main/Chapter02_Application_Layer)


---

## 🐳 Docker 开发环境

本项目提供一键可用的 Docker 开发镜像，预装了 C++ 工具链、Protobuf、Muduo、ZooKeeper 等全部依赖。

- 详细说明：[docs/docker-dev-setup.md](docs/docker-dev-setup.md)
- 快速开始：

  ```bash
  docker pull cspenguin/qwenrpc-dev:latest
  docker run -it --rm -v "$(pwd)":/workspace -w /workspace cspenguin/qwenrpc-dev:latest
  ```

> 请将 `cspenguin` 替换为实际的 Docker Hub 用户名。


---

## 🗺️ Roadmap

> 采用「完成即划掉」的待办清单风格，随项目推进持续更新。

- ~~[x] 配置容器化开发环境（Docker + Muduo + ZooKeeper）~~
- [ ] 设计并实现 RPC 通信基础框架
- [ ] 集成 Protobuf 作为序列化方案
- [ ] 集成 Muduo 作为网络通信层
- [ ] 集成 ZooKeeper 实现服务注册与发现
- [ ] 支持异步调用
- [ ] 支持负载均衡
- [ ] 编写完整的使用文档和示例


