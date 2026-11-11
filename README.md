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

材料中的 v1–v8 是一条作者迭代线，br 是另一位同学的实现。本仓库发布的是学习讲解、必要的源码摘录、分章原稿及成品，不是可直接提交评测的比赛程序。性能预算标作推算。

[查看自检记录与实页截图](SELF-CHECK.md) · [制作进度](PROGRESS.md) · [发布记录与校验值](https://github.com/xueM2007/fire-codebook/releases/tag/v1.0.0)

