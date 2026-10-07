# AI - Shepherd

![show](assets/sh展示.png)
AI-Shepherd（中文：AI-牧羊人）是一个面向 AI Agent 的进程监控系统。AI 负责决定执行什么命令，而全部进程的创建、登记、约束、超时治理与回收，由常驻的 C++ 监督核心 `shepherd-core` 统一负责。

[![License](https://img.shields.io/github/license/zzz2552114/AI-Shepherd?label=License&color=blue&logo=apache&logoColor=white)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](#技术栈)
[![Python](https://img.shields.io/badge/Python-3.12-3776AB?logo=python&logoColor=white)](#技术栈)
[![Storage](https://img.shields.io/badge/Storage-PostgreSQL%20%C2%B7%20asyncpg-4169E1?logo=postgresql&logoColor=white)](#使用指南)
[![Build](https://img.shields.io/badge/Build-xmake-6C9A2E)](#快速开始)
[![Stars](https://img.shields.io/github/stars/zzz2552114/AI-Shepherd)](https://github.com/zzz2552114/AI-Shepherd/stargazers)


技术栈：现代C++ ( `C++ 20` )、`Python`、`Unix-socket`、`Agent calling`、`PostgreSQL`。

---

## 目录

- [项目介绍](#项目介绍)
- [演示素材](#演示素材)
- [项目架构](#项目架构)
- [快速开始](#快速开始)
- [使用指南](#使用指南)
- [演示](#演示)
- [路线图](#路线图)
- [参与贡献](#参与贡献)
- [许可证](#许可证)
- [参考](#参考)

---

## 项目介绍

> 注：本项目仅在初期架构部分使用到了 AI 辅助，在代码撰写途中不存在 AI 直接上手工作的情况。

受到我最近完成的 [CSAPP](https://github.com/zzz2552114/CSAPP-Lab) 里  `07-shell-lab` 中进程管理的启发，本项目的目标是，针对 `Agent Harness` 等可以自主使用命令行、创建进程的 LLM，进行进程审查和进程管理。

***Shepherd***  控制 AI Agent 所使用的进程的创建、记录、限额、超时治理与回收，保证 CPU、内存、磁盘的正常使用。这在维护 Harness Agent 超长时间运行稳定性上起到重要的作用！

具体实现：通过系统提示词注入，Agent 只能通过五个工具（`spawn_process`、`list_processes`、`inspect_process`、`signal_process`、`kill_process`）提出对进程的请求，安全性由核心保证。

***该项目解决的问题 ：***

| 问题 | 后果 |
|---|---|
| 子进程无人接管 | 父进程先退出时，子进程被 init 收养；失去控制的进程无人审核与清理 |
| 没有约束 | 死循环占用 CPU、递归 fork 打满进程表、写满磁盘、内存泄漏拖垮机器 |
| 没有反馈与审计 | 退出码与异常被忽略，AI 无法自我纠正，事后也无法记忆或复盘 |

***Shepherd***  拥有以下功能：

1. 被监督进程只能由核心创建，全部记录在进程表中，可以枚举和查询。
2. 不留僵尸进程。对进程进行手动分组、触发 SIGCHLD 后只有一个 `waitpid` 回收、`subreaper` 接管孤儿后代、任务结束时针对进程组发信号清理。
4. 超时治理。超时后先发送 `SIGTERM` 给予进程退出的机会，宽限期结束后发送 `SIGKILL`，保证最终能够清理。
5. 数据入库。把进程变化反馈给 Agent，自我学习，同时落库 PostgreSQL 。

用户界面是运行在终端中的交互式 Shell，提供实时进程展板。可以直接调用 Agent，也可以使用 `jobs`、`inspect`、`kill` 等少量内置命令。

---

## 演示

![stack](assets/进程栈.png)

![over](assets/进程结束.png)

## 项目架构


```mermaid
flowchart TB
    U["用户：自然语言任务"] --> SH["Shepherd Shell（终端面板）"]
    SH --> AG["Agent Loop（LLM function calling）"]
    AG --> CC["CoreClient（asyncio）"]
    CC -- "NDJSON over UDS" --> CORE["shepherd-core（C++ 监督核心）"]
    CORE --> G1["进程组 A"]
    CORE --> G2["进程组 B"]
    CORE -- "事件流：exited / timeout / resource" --> CC
    CC --> SH
    CC --> PG["PgWriter → PostgreSQL"]
```

### 一次任务的完整生命周期

```mermaid
sequenceDiagram
    autonumber
    participant U as 用户
    participant S as Shell / Agent
    participant C as shepherd-core
    participant P as 被监督进程
    participant D as PostgreSQL
    U->>S: "统计当前目录下有多少个文件"
    S->>C: spawn(task_id, argv, timeout_seconds)
    C->>C: Policy.validate（白名单与并发上限）
    C->>P: fork + setpgid + PDEATHSIG + 清信号环境 + setrlimit + execve
    C-->>S: ack{ok, pid, jid, pgid}
    C-->>S: event process.started
    C-->>D: 事件与进程行落库
    P-->>C: SIGCHLD（进程退出）
    C->>C: self-pipe 到 drain 线程，更新账本并发出事件
    C-->>S: event process.exited{exit_code: 0}
    S->>U: 汇报真实结果，决定下一步
```

### 与同类方案的差异

| 方案 | 定位 | Shepherd 的不同 |
|---|---|---|
| 在 Agent 中直接使用 `subprocess` | 实现成本最低 | 无账本、无整组回收、无超时治理、无审计 |
| `systemd` | 管理长驻系统服务 | 面向静态单元配置；Shepherd 面向 AI 动态生成的短生命周期进程树，按任务记账并输出事件流 |
| `supervisord` / `pm2` | 进程退出后自动拉起 | 关注保活；Shepherd 关注约束、治理、反馈闭环与复盘 |

---

## 快速开始

### 0. 环境要求

| 依赖 | 版本或说明 |
|---|---|
| Linux | WSL2 ( 我的开发环境 ) 或原生 Linux |
| g++ | 13 及以上 |
| [xmake](https://xmake.io/) | 2.8 及以上 |
| Python | 3.12 及以上 |
| PostgreSQL | 可选，仅持久化与回放功能需要 |

### 1. 获取代码并构建核心

```bash
git clone https://github.com/zzz2552114/AI-Shepherd.git
cd AI-Shepherd

xmake

# 编译后的内容在下面路径
# build/linux/x86_64/release/shepherd-core
```

### 2. Python 环境

```bash
python3 -m venv .venv
source .venv/bin/activate

pip install rich openai asyncpg
```

### 3. 初始化 PostgreSQL（可选）

```bash
sudo apt update && sudo apt install -y postgresql postgresql-contrib
sudo service postgresql start

sudo -u postgres psql -c "CREATE USER shepherd WITH PASSWORD 'shepherd';"
sudo -u postgres psql -c "CREATE DATABASE shepherd OWNER shepherd;"

psql "postgresql://shepherd:shepherd@localhost:5432/shepherd" -f scripts/schema.sql
```

不安装数据库也可以使用。Shell 检测不到可用连接时会跳过持久化，进程监督与 Agent 功能不受影响，只是没有历史回放。

### 4. 配置 LLM

```bash
export OPENAI_BASE_URL="https://api.deepseek.com"
export OPENAI_API_KEY="sk-你的密钥"
export SHEPHERD_MODEL="deepseek-flash"
```

任何 OpenAI 兼容接口都可以使用，换 BaseURL 即可。

### 5. 启动终端 Shell

```bash
./shepherd
```




## 使用指南

### 内置命令

| 命令 | 作用 |
|---|---|
| `jobs`， `ps` | 打印当前进程表：JID、PID、状态、命令、运行时长、退出码 |
| `inspect <pid> / <%jid>` | 查看单个进程详情 |
| `kill <pid> / <%jid> [-9]` | 终止进程；默认发送 `SIGTERM`，`-9` 使用 `SIGKILL`。作用于整个进程组 |
| `logs <task_id> / <pid> / <%jid>` | 查看事件流水；接 PostgreSQL 时按时间回放完整历史时间线 |
| `stats` | 汇总每个任务的进程数、被强杀数与峰值内存 |
| `help` | 显示命令帮助 |
| `exit`， `Ctrl-D` | 优雅停机，清理所有在跑进程后退出 |

除上述命令外，任何整行输入都会被当作自然语言任务交给 Agent。

### 停止与清理

- 在 Shell 中执行 `exit` 或按 `Ctrl-D`：先向核心发送 `shutdown`，核心按停机流程清理所有在跑进程组并删除 socket 文件，随后关闭客户端。
- 直接向核心发送信号：`kill -TERM <core_pid>`，等价于优雅停机。
- `Ctrl-C` 有两种语义：任务运行中按下时，取消当前任务、终止其整个进程组，并打印任务已取消与清理的进程数量，Shell 不退出；空闲时按下则退出 Shell。
- 如果核心被 `kill -9`，无法执行优雅停机。

### 可调参数

所有阈值集中在 `src/util/constants.hpp`，调整行为只需修改该文件：

| 常量                      | 默认值               | 含义                                           |
| ------------------------- | -------------------- | ---------------------------------------------- |
| `kMaxConcurrentProcesses` | `32`                 | 同时在跑的进程上限，超过返回 `over_subscribed` |
| `kMonitorIntervalMs`      | `200`                | 扫描周期                                       |
| `kSampleIntervalMs`       | `1000`               | 采样器读取 `/proc` 的周期                      |
| `kTermGraceMs`            | `2000`               | 发出 `SIGTERM` 后等待多久升级为 `SIGKILL`      |
| `kDefaultMemoryBytes`     | `256 MiB`            | 默认 `RLIMIT_AS`                               |
| `kDefaultCpuSeconds`      | `10`                 | 默认 `RLIMIT_CPU`                              |
| `kDefaultFsizeBytes`      | `16 MiB`             | 默认 `RLIMIT_FSIZE`                            |
| `kDefaultNproc`           | `64`                 | 默认 `RLIMIT_NPROC`                            |
| `kMaxTombstones`          | `128`                | 墓碑队列长度上限                               |
| `kSocketBacklog`          | `16`                 | `listen` 等待队列长度                          |
| `kDefaultSocketPath`      | `/tmp/shepherd.sock` | 默认 UDS 路径                                  |

### legacy 模式 （ 实际上就是现代C++重写的 shell-lab ）

```bash
xmake run shepherd-core --legacy
```

---

## 演示

> 以下内容为 AI 生成，我辅助验证

以下话术可以直接用于复现四个场景。

场景一，正常多步任务：

```text
shepherd> 帮我看看当前目录下都有哪些文件，然后统计一下一共有多少个
```

预期：Agent 先启动一个列目录进程，成功后再启动一个统计进程；`jobs` 可以看到状态变化，事件流中两次 `process.exited` 的 `exit_code` 均为 `0`。

场景二，错误自纠：

```text
shepherd> 统计 /tmp/this_path_does_not_exist 目录下有多少个文件
```

预期：第一个进程非零退出，或核心返回 `error`，事件与观察结果回喂模型；模型读取真实错误后改用正确路径重试并成功。整个过程没有额外的条件分支规则，纠错来自真实结果回灌与提示词中的重试规则。

场景三，超时治理：

```text
shepherd> 启动一个 sleep 60 的任务，超时时间设置为 3 秒
```

预期：`process.started` 之后约 3 秒出现 `process.timeout`，`after=TERM`、`next=KILL`，进程收到 `SIGTERM`；宽限 2 秒后仍存活则收到 `SIGKILL`，并再次产生 `process.timeout`，`after=KILL`，最终产生 `process.killed`。`jobs` 中该进程进入墓碑状态，显示为 `Killed`。

场景四，整组回收：

```text
shepherd> 运行 tests/fork_children.sh 并等它结束
```

预期：脚本 fork 出多个 `sleep` 子进程；任务结束后，账本中所有相关进程都变为 `Exited` 或 `Killed`，系统中不残留任何 `Z` 状态进程。可以在另一个终端验证：

```bash
ps -eo stat,pid,ppid,cmd | grep -w Z | grep -v grep    # 预期没有任何输出
```

---

## 未来路线图

项目按长期主干项目规划，随着我的课程与技术学习，逐步扩展：

> 该段内容为 AI 结合我后续计划课程，给我的建议

| 触发课程或技术 | 演进方向 |
|---|---|
| MIT 6.S081 | 引入 PID 与 MNT namespace 以及 cgroups，从监督升级为容器级沙箱与 mini-init |
| Stanford CS144 | 连接处理改为 epoll reactor，支撑更高并发 |
| Docker | 核心与 worker 镜像化，进程在容器内执行 |
| Nginx | 增加远程访问网关，支持 TLS、鉴权与限流，本地终端 Shell 保留为 CLI |
| MCP | 将核心能力封装为 MCP Server，供外部 Agent 直接调用 |
| CMU 15-445、6.824、DDIA | 引入 lease、崩溃重放与 Raft 复制，走向多机编排与高可用 |

---

## 参与贡献

欢迎提交 Issue 与 PR。为使改动可以被复核，请尽量遵循项目原有的节奏：

1. 先整体阅读项目设计范式。
2. 保持分层纪律。C++ 核心不引入数据库与 LLM 依赖；Python 侧不直接 `fork` 或 `Popen` 被监督进程，一切请求经 core 转发。
3. 信号处理函数内只执行异步信号安全的操作，不使用锁、不分配内存、不使用 iostream、不写日志。
4. 系统调用统一先在 `src/util/sys_wrap.*` 中注册，失败时抛出 `std::system_error`。
5. 改动后自行验证：`xmake` 构建通过，相关演示场景可复现；涉及并发与回收的改动，请补充对应的竞态用例说明。

如果发现文档与实现不一致，欢迎直接提出 Issue，这类问题同样重要。

---

## 许可证

本项目采用 Apache License 2.0 发布，详见 [LICENSE](LICENSE)。

---

## 参考

- CSAPP Shell Lab 及其 `tsh.c`：作业控制、信号处理与 `waitpid` 语义的理解主要来自该实验。
- 项目依赖的开源库：[xmake](https://xmake.io/)、[nlohmann/json](https://github.com/nlohmann/json)、[rich](https://github.com/Textualize/rich)、[asyncpg](https://github.com/MagicStack/asyncpg)、[openai-python](https://github.com/openai/openai-python)。
- 参考书目：《Computer Systems: A Programmer's Perspective》与 《Operating Systems: Three Easy Pieces》，主要用于进程、信号、并发与虚拟内存部分。
