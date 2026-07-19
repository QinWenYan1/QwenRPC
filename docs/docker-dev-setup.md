# 🐳 QwenRPC Docker 开发环境

> 本文档介绍如何使用 Docker 快速搭建 QwenRPC 的开发环境。
>
> 镜像定位：**开发 / CI / 测试** 用途，**不是生产镜像**。

---

## 目录
> 返回[根目录](../README.md)
- [镜像里有什么](#镜像里有什么)
- [前置条件](#前置条件)
- [方式一：从 Docker Hub 拉取镜像](#方式一从-docker-hub-拉取镜像)
- [方式二：本地构建镜像](#方式二本地构建镜像)
- [启动开发容器](#启动开发容器)
- [Dockerfile 关键配置说明](#dockerfile-关键配置说明)
- [进阶：推送自己的镜像](#进阶推送自己的镜像)
- [常见问题](#常见问题)

---

## 镜像里有什么

本镜像基于 Ubuntu 22.04 LTS，预装了 QwenRPC 开发所需的全部依赖：

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
cd QwenRPC

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



、


