# 森林中的一位火

HPCGame「fire」中文源码讲解书。用一张 6×6 的森林讲清楚：火为什么一拍只能传一格，以及同样的规则怎样用一个 64 位整数处理一整排格子。面向会写循环、但还没有接触位运算与并行编程的读者。

**[下载整本书（ZIP）](https://github.com/xueM2007/fire-codebook/releases/download/v1.0.0/fire-codebook.zip)** · **[打开 PDF](fire-codebook.pdf)** · **[从第 0 章在线阅读](book/00-overview.md)**

PDF 共 96 页。电脑或手机离线阅读时下载压缩包并解压，直接打开 PDF 或单文件 HTML；不用编译、安装依赖或登录账号。PDF 中第 13–51 页是第七版完整的 261 行精读。

|阅读顺序|你能跟着做什么|
|---|---|
|[阅读导引](book/README.md) · [第 0 章](book/00-overview.md)|手推一次惊雷与传播；区分整机吞吐和每核预算|
|[第 1 章：版本演化](book/01-evolution.md)|从二维整数，走到位平面、常驻线程团队与三行窗口|
|[第 2 章：逐行精读](book/02-v7-line-by-line.md)|按真实行号重写 v7，从读文件一直到输出|
|[第 3 章：逐版对照](book/03-compare-1-6.md)|在 v1–v6 中找出和 v7 同一职责的代码|
|[第 4 章：为什么换掉 OpenMP](book/04-v8.md)|分清编译选项、线程栅栏和进程编号|
|[第 5 章：另一条实现路线](book/05-br-pool.md)|跟随活动块队列、边界火情和工作线程|
|[术语](book/06-appendix-a-glossary.md) · [性能账本](book/07-appendix-b-cost.md) · [语法索引](book/08-appendix-c-index.md) · [已知问题](book/09-appendix-d-issues.md)|回查符号、公式、行号，以及哪些说法还没有实测支持|

材料中的 v1–v8 是一条作者迭代线，br 是另一位同学的实现。本仓库现包含学习讲解、原始 C++ 源文件及题目/评测辅助文件；保留原文件，不代表已验证所有版本可在当前环境编译或满足比赛要求。性能预算标作推算。

[查看自检记录与实页截图](SELF-CHECK.md) · [制作进度](PROGRESS.md) · [发布记录与校验值](https://github.com/xueM2007/fire-codebook/releases/tag/v1.0.0)


## 完整项目（v1.1.0）

[下载完整项目 ZIP](https://github.com/xueM2007/fire-codebook/releases/download/v1.1.0/fire-project-complete.zip)。旧 v1.0.0 ZIP 仍是仅阅读包。

- `code/`：v1–v8 的原始 `.cpp`、另一条实现 `br-fire-pool.cpp` 及其依赖 `spin_lock.hpp`。
- `context/`：题目说明、参考实现 `std-main.cpp`、数据生成 `gencase.py`、输出比较 `compare.py`。
- `book/`、PDF、HTML：既有讲解书；`SOURCE-NOTES.md` 说明原始材料来源与使用边界。

这不是同一磁盘目录内的 HPC Forge Node/Electron 应用；未混入该应用、个人提示词、笔记风格样本、依赖缓存或原项目 Git 历史。

运行前请先阅读 `context/intro.md` 中的环境和输入输出要求及书中已知问题。OpenMP 版本需要支持 OpenMP 的 C++ 编译环境；MPI 实现需要 MPI 编译器和运行环境，`br-fire-pool.cpp` 必须与 `spin_lock.hpp` 同目录。`gencase.py` 和 `compare.py` 为 Python 脚本，参数在文件开头说明。本次补充只保持原文件并校验完整性，未改写算法、未重做讲解书、未进行所有版本的编译或性能验证。