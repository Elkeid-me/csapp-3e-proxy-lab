# Proxy Lab

北京大学 2022 年秋季学期《计算机系统导论》课程 Proxy Lab。

## 编译

你需要使用 `xmake` 进行编译。

## 说明 (2024.3.2 更新)

进行了彻底的重构。

重构版本在 Basic、Concurrency 和 Caching 都能达到满分。Real Pages 部分 real-2048 失败。

但注意到如下情况：

1. 重构前的版本，现在也过不了 real-2048
2. 使用新版本作为代理，可以正常访问 bilibili 看视频。

可以推测：real-2048 这个测试点本身有问题。而我的 proxy 没有问题。

## 说明 (2024.2.21 更新)

实际上这个代码有很大的问题：

1. [socket.cpp](src/socket.cpp) 的多数函数都不够优雅，因为是从 CSAPP 抄来的。
2. 停机清理不够优雅。
2. 整个 RIO 都是不必要的，但是我却把它封装得非常池沼。
3. [proxy.cpp](src/proxy.cpp) 的 `do_https_proxy()` 中涉及到对 `n` 的判断：

```c
n = read(fd_to_server, buf, lab::MAX_OBJECT_SIZE);
// ...
if (write(fd_to_client, buf, n) != n)
{
    lab::Close(fd_to_server);
    return;
}

```

注意，在特定条件下，`write` 即便执行成功，也可能写入少于 `n` 字节。然而我的 `proxy` 却会退出（悲）

不要碰 RIO！不要碰 RIO！

也许你可以参考 asio 库。
