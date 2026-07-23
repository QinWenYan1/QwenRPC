# 🐳 KopiRPC Docker 开发环境

> 本文档介绍如何使用 Docker 快速搭建 KopiRPC 的开发环境。
>
> 镜像定位：**开发 / CI / 测试** 用途，**不是生产镜像**。

---

## 目录
> 📎 返回：[KopiRPC 仓库首页](../README.md)
- [镜像里有什么](#镜像里有什么)
- [前置条件](#前置条件)
- [方式一：从 Docker Hub 拉取镜像](#方式一从-docker-hub-拉取镜像)
- [方式二：本地构建镜像](#方式二本地构建镜像)
- [启动开发容器](#启动开发容器)
  - [方式 B：VS Code Dev Containers（推荐日常开发）](#方式-bvs-code-dev-containers推荐日常开发)
- [Dockerfile 关键配置说明](#dockerfile-关键配置说明)
- [进阶：推送自己的镜像](#进阶推送自己的镜像)
- [常见问题](#常见问题)

---

## 镜像里有什么

本镜像基于 Ubuntu 22.04 LTS，预装了 KopiRPC 开发所需的全部依赖：

| 组件 | 版本/说明 |
|---|---|
| GCC | 11.4 |
| CMake | 3.22.1 |
| Protocol Buffers | 3.12.4 |
| Muduo | 从源码编译安装到 `/usr/local` |
| Boost | 1.74 |
| glog | 最新可用版本 |
| ZooKeeper C client | `libzookeeper-mt-dev` |
| ZooKeeper server | `zookeeperd`（方便本地测试） |
| 工具 | GDB、vim、git、curl、wget |

当前镜像架构：**linux/arm64**（适用于 Apple Silicon Mac 和 ARM64 服务器）。

---

## 前置条件

1. 安装 [Docker Desktop](https://www.docker.com/products/docker-desktop/)（Mac/Windows）或 Docker Engine（Linux）；
2. 确认 Docker 命令可用：

   ```bash
   docker --version
   ```

3. 推荐给 Docker Desktop 分配至少 **6GB 内存**；
4. 稳定的网络连接（首次拉取或构建需要下载大量依赖）。

---

## 方式一：从 Docker Hub 拉取镜像

```bash
# 拉取最新版本
docker pull cspenguin/qwenrpc-dev:latest

# 或拉取固定版本
docker pull cspenguin/qwenrpc-dev:v0.1.0
```


---

## 方式二：本地构建镜像

如果你希望从源码构建镜像，或者想自定义依赖：

```bash
# 进入项目根目录
cd KopiRPC

# 构建镜像
docker build -f docker/Dockerfile.dev -t qwenrpc-dev .
```

首次构建可能需要 **10-30 分钟**，因为需要：
- 下载 Ubuntu 基础镜像和 apt 依赖；
- 从 GitHub 下载 Muduo 源码并编译；
- 安装 ZooKeeper、Protobuf 等开发库。

后续如果 Dockerfile 没有大改，Docker 会利用缓存加快构建。

---

## 启动开发容器

进入开发环境有两条入口，**镜像和挂载目录完全相同，可按场景混用**：

- **方式 A：`docker run`** —— 纯终端，适合一次性操作（起停 ZooKeeper、手动跑 protoc、临时调试）；
- **方式 B：VS Code Dev Containers** —— 编辑器直接住进容器，日常写代码推荐。

### 方式 A：docker run（纯终端）

```bash
docker run -it --rm \
  -v "$(pwd)":/workspace \
  -w /workspace \
  cspenguin/qwenrpc-dev:latest
```

参数说明：

| 参数 | 含义 |
|---|---|
| `-it` | 交互式终端，允许你和容器内 shell 交互 |
| `--rm` | 退出容器后自动删除容器，保持本地干净 |
| `-v "$(pwd)":/workspace` | 把当前目录挂载到容器的 `/workspace` |
| `-w /workspace` | 设置容器内默认工作目录为 `/workspace` |

执行后，你会看到类似提示符：

```bash
root@xxxxxx:/workspace#
```

这表示你已经进入容器内部，当前目录就是你项目的根目录。

### 方式 B：VS Code Dev Containers（推荐日常开发）

仓库根目录带有 `.devcontainer/devcontainer.json`，VS Code 可以据此把整套开发环境（编辑器、clangd、CMake Tools）直接放进容器里。

**前置条件**：

1. VS Code 安装扩展 **Dev Containers**（`ms-vscode-remote.remote-containers`）；
2. Docker Desktop 处于运行状态（未运行时扩展会提示启动）。

**首次进入**：

1. 用 VS Code 打开项目根目录；
2. 右下角弹出通知 "Folder contains a Dev Container configuration file"，点击 **Reopen in Container**（也可在命令面板执行 `Dev Containers: Reopen in Container`）；
3. 扩展随后自动完成：用镜像新建一个由它管理的容器 → 把仓库挂载到 `/workspace` → 在容器内安装 clangd 与 CMake Tools。首次需稍等片刻。

**之后进入**：VS Code「最近打开」列表中本项目会带上容器标记，从那里打开即直接进入容器。

**退出与生命周期**：

- 关闭 VS Code 窗口，容器**默认继续运行**，下次进入秒开；
- 退出容器环境：点击左下角绿色远程标志 → `Close Remote Connection`；
- 需要停止容器时：`docker ps` 查到它后 `docker stop <容器ID>`。

> ⚠️ **两种入口的共存纪律**：
> - 两种方式启动的是**两个相互独立的容器实例**，但共享同一个 `/workspace`（即本仓库）以及其中的 `build/` 产物目录——**不要同时在两个容器里执行构建**；
> - `.pb.cc/.pb.h` 一律以容器内 `protoc`（3.12.4）生成的版本为准，**一切编译构建只在容器内进行**（mac 本机的 protobuf 版本不同，混用会报错）。

---

## Dockerfile 关键配置说明

项目中的 `docker/Dockerfile.dev` 主要分为三层：

1. **基础环境和 apt 依赖**：安装 C++ 工具链、Protobuf、Boost、ZooKeeper 等；
2. **编译 Muduo**：从 GitHub 拉取 Muduo 源码，编译并安装到 `/usr/local`；
3. **设置工作目录**：将 `/workspace` 设为默认工作目录。

> 📋 **术语提醒**：`Muduo` 是陈硕开发的高性能 C++ 网络库，Ubuntu apt 中没有现成包，所以必须从源码编译。

---
## 进阶：推送自己的镜像

如果你修改了 Dockerfile 并想推送到自己的 Docker Hub：

```bash
# 登录 Docker Hub
docker login

# 给镜像打标签
docker tag qwenrpc-dev cspenguin/qwenrpc-dev:latest

# 推送
docker push cspenguin/qwenrpc-dev:latest
```

---

## 常见问题

### Q1：构建镜像时间为什么这么长？

**A**：正常现象。首次构建需要下载 Ubuntu 基础镜像、大量 apt 包，以及从源码编译 Muduo。耗时取决于网络速度和机器性能，通常 10-30 分钟。

### Q2：Docker Desktop 提示内存不足怎么办？

**A**：打开 Docker Desktop → Settings → Resources → Memory，建议分配 **6GB 或以上**。


如果没有，说明镜像不是最新版本，需要重新构建。

### Q3：Muduo 编译失败？

**A**：通常是网络问题导致 `git clone` 失败。可以重试构建，或检查网络连接。



### Q4：这个镜像能在 x86_64（Intel/AMD）机器上跑吗？

**A**：当前镜像是 `linux/arm64` 架构。如果需要在 x86_64 上运行，需要在 x86_64 机器上重新构建，或使用 `docker buildx` 构建 multi-arch 镜像。

---



