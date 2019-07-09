<h1 align="center">Welcome to WebServer 👋</h1>
<p>
  <img src="https://img.shields.io/badge/version-1.0.0-blue.svg?cacheSeconds=2592000" />
</p>

> 一款C++轻量型Web服务器

## Introduction
本项目是基于C++11开发的Web服务器。本项目支持http协议，解析了get、head请求，仅支持静态网页，支持http长连接；本项目还实现了异步日志，记录服务器程序的运行过程。

| Section Ⅰ | Section Ⅱ  | Section Ⅲ |
| --------- | ---------- | ---------- |
| [模型解析](https://github.com/LynnTh/WebServer/blob/master/%E6%A8%A1%E5%9E%8B%E8%A7%A3%E6%9E%90.md)  | [技术点解析](https://github.com/LynnTh/WebServer/blob/master/%E6%8A%80%E6%9C%AF%E7%82%B9%E8%A7%A3%E6%9E%90.md) | [测试结果](https://github.com/LynnTh/WebServer/blob/master/%E6%B5%8B%E8%AF%95%E7%BB%93%E6%9E%9C.md) |

## Environment
- 运行环境：Ubuntu 16.04
- 编译：gcc 7.4.0

## Install

```sh
./build.sh
```

## Usage

```sh
./webserver [-t thread_numbers] [-p port]
```

## Technique Point
- 采用Reactor + 非阻塞IO + 线程池的并发模型
- 采用Round Robin法分配新连接，实现线程间负载均衡
- 采用eventfd实现线程的异步唤醒
- 采用timerfd和最小堆实现计时器，并利用timing wheel法踢掉空闲连接
- 采用双缓冲技术实现了简单的异步日志线程
- 利用智能指针和RAII方法解决内存泄漏问题
- 解析HTTP请求，支持MIME，支持长连接
- 优雅关闭连接
- 利用空闲文件描述符，限制并发连接数

## Author

👤 **Lynn Tao**

* Github: [@LynnTh](https://github.com/LynnTh)


