# 第 5 章　另一位作者：只把可能变化的块排进队列

**这一章你会得到什么**：能沿实际入口追到每一类函数，并把活动块、局部线程与跨进程通信分开。下文行号属于 br-fire-pool.cpp；锁的十二行另标文件名。文件共 716 行。 代码框保留关键原行，行号不连续时，中间行见标注的源文件区间。

## 5.1　读这条路线，先换一张编码卡

同学版不是 v9。它用相邻的一对字 a、b 存一排 64 棵树。a 是题面数字的低位，b 是高位。按 (b,a) 顺序写，空 00、树 01、火 10、灰 11；按内存里取出的 (a,b) 顺序读，树是 (1,0)，火是 (0,1)。这和作者版的 b1、b0 符号含义不一样。遇到“第一个字”和“高位”这两个词务必拆开：内存顺序没规定第一个一定代表高位。

自己算一次：旧树 a=1,b=0。有邻火就令 ignite=1，新 a=(1|0)&~1=0，新 b=0|1=1，读回高低顺序为 10，即火。旧火 a=0,b=1，不在旧树集合里，ignite=0，于是新 a=1、新 b=1，正好是灰。若旧格是灰，两个位原本都为 1，仍两位皆 1。空格两位皆 0，纵使 near=1 也不会烧。看完这四种取值，再看左右移动才不会把题面数字和数学运算搅在一起。

**br 选行 · L94–101**

```cpp
 94              const ui near = prev[j] | next[j] | _shld<1>(C, Lw) | _shld<63>(R, C);
 97              const ui ignite = (a & ~b) & near;
 98              dst[2 * j] = (a | b) & ~ignite;
 99              dst[2 * j + 1] = b | ignite;
101              row_burn |= ignite;
```

**br 选行 · L335–344**

```cpp
335                      for (int w = 0; w < TileSize; ++w) {
340                              b |= ui(v >> 1) << k;
344                      }
```


**一句话记住**：名字只给提示，实际编码以装入和解码的公式为准。

## 5.2　跨字位移：_shld 不会替你完成火灾规则

模板（template，让编译器按参数生成具体版本的定义）此处把位移量也作为参数；`_shld<1>` 中的 1 在编译前就确定。static_assert 在编译期检查 amount<64，不通过便不会生成这个实例。if constexpr 按编译期条件保留分支：位移量为零直接返回 a，避免计算 b>>(64−0)。inline 允许相应的函数定义用法，不保证每次都展开成一条机器指令。

把两个八位玩具字代进公式：C=00000010，左字 Lw=10000000。左移一位，C 变为 00000100；左字最高位落到最右边的低位，结果 00000101。原代码是 64 位，左邻方向用 `_shld<1>`(C,Lw)，右邻用 `_shld<63>`(R,C)，后者等价于 C>>1 再从 R 的最低位补最高位。可选宏 ASM_SHLD 才挑选内联汇编（inline assembly，源码内嵌目标机器指令）；默认分支只是等价 C++ 表达式。没有汇编清单时，不能声称评测一定执行了某一条指令。

**br 选行 · L17–34**

```cpp
 17  using ui = uint64_t;
 20      static_assert(amount < 64);
 23          return a;
 26          return a;
 32          return (a << amount) | (b >> (64 - amount));
```

**br 选行 · L88–99**

```cpp
 91              const ui C = curr[j];
 93              const ui R = curr[j + 1];
 94              const ui near = prev[j] | next[j] | _shld<1>(C, Lw) | _shld<63>(R, C);
 97              const ui ignite = (a & ~b) & near;
 99              dst[2 * j + 1] = b | ignite;
```


**一句话记住**：双字移位解决接缝，一整个转移还要看树、火和四邻。

## 5.3　给活动块排队：enqueue_next 与去重表

块队列（task queue，保存待处理任务编号的容器）用 deque 表示，可在尾部加入、在头部取出。marked 是去重表。如果本步新火出现在两个块的公共角落，两个工作者都可能希望下步处理同一块。不去重，既重复做工，也可能同时写同一块 next，令双缓冲失去‘独占写入者’这个保证。函数本身不负责加锁，调用者必须知道当前处于单人准备阶段还是多线程阶段。

队列编号是带外框网格的位置，实际存储编号却不含外框。以 N=2 为例，带边框每行 4 个位置，四个真块编号是 5、6、9、10，对应存储编号 0、1、2、3。id=5 左边的 4 属于框；函数可以给框标记真，却不把框压入队列。这样上层可以统一提出‘请排我上方的块’，不必在每条边界分支先判断物理邻居是否存在。前提是传来的 id 连外框也不越过。

**br 选行 · L36–45**

```cpp
 36  template <int __tiles_side_cnt>
 37  void enqueue_next(int id, bool* marked, std::deque<int>& tasks) {
 43              tasks.push_back(id);
```

**br 选行 · L118–130**

```cpp
121              enqueue_next<__tiles_side_cnt>(id, marked, tasks);
123              enqueue_next<__tiles_side_cnt>(id - __tiles_side_cnt - 2, marked, tasks);
125              enqueue_next<__tiles_side_cnt>(id + __tiles_side_cnt + 2, marked, tasks);
127              enqueue_next<__tiles_side_cnt>(id - 1, marked, tasks);
129              enqueue_next<__tiles_side_cnt>(id + 1, marked, tasks);
```


**一句话记住**：先统一编号和去重，再谈一块分给谁。

## 5.4　propagate_tile_and_append：在哪里算，在哪里锁

一个 TileSize=S 的块不是 S×S 个格。它每行有 S 对字，每字代表 64 列；共有 64S 行，所以边长是 64S。S=16 时边长 1024，一块含 1,048,576 格，一代占 256 KiB。三个局部火行各多两个字作边沿槽；prev=row1+1 后，prev[-1] 仍落在 row1[0] 内，不是随便允许对数组用负下标。load_burn 是本函数里的小函数，把输入字对转成 b & ~a。邻居指针为空时写零，表示地图之外没有火。

每处理一行，curr[-1] 使用左块末字，curr[width] 使用右块首字；上、下边缘可能由邻块某行或进程边界缓冲提供。核心内层只读当前代、写独立 pong_buf。三行轮转的不只是算当前块：row_burn 汇总一行的新火，all_burn 汇总整块，top/bottom/left/right 记录四条边有没有新火。新火下步会变灰，在自己块内必须再处理一次；如果新火触边，隔壁块下一步也可能发生变化。因此块即使只剩一处新火，也不能跳过自己的下一步。

所有按位计算都在锁外完成。只在末段插入下轮队列时取得 B 锁；一次拿锁可以把本块和至多四邻的请求放好，再自动释放。若把整块百万格都算在锁里，队伍即使有 16 人也只能依次算。若为了更快删掉末端的锁，去重检查与入队这两个动作就不再是一个不可拆的决定。

**br 选行 · L53–81**

```cpp
 53      static_assert(__tile_size > 0);
 74      ui* curr = row2 + 1;
 77      load_burn(me, curr);
 81          curr[-1] = left ? left[2 * i * width + 1] & ~left[2 * i * width] : ui{0};
```

**br 选行 · L95–116**

```cpp
 97              const ui ignite = (a & ~b) & near;
 99              dst[2 * j + 1] = b | ignite;
103                  first_ignite = ignite;
104              last_ignite = ignite;
115          curr = next;
```

**br 选行 · L118–130**

```cpp
121              enqueue_next<__tiles_side_cnt>(id, marked, tasks);
123              enqueue_next<__tiles_side_cnt>(id - __tiles_side_cnt - 2, marked, tasks);
125              enqueue_next<__tiles_side_cnt>(id + __tiles_side_cnt + 2, marked, tasks);
127              enqueue_next<__tiles_side_cnt>(id - 1, marked, tasks);
129              enqueue_next<__tiles_side_cnt>(id + 1, marked, tasks);
```


**一句话记住**：重活留在锁外，队列与去重表作为一件事保护。

## 5.5　谁拥有存储：TileLink、FireHalo 和 make_tile_links

TileLink 只保存 current、next 两个指针，自己不拥有那两段内存。真实空间由 data_A、data_B 两个向量拥有；建表后若让它们再次分配，旧指针会失效。make_tile_links 接收两段起点，按 id×tile_words 填成指针对并返回表。主路径读取函数里自己做了类似的建表，没有调用这个辅助函数；同一设计出现两处不等于运行时做了两遍。

为什么不能每步换掉全部块？假设 A、B 两个块本步都在旧图，但只有 A 入队。A 的 next 会写全，B 的 next 却可能是两步以前的内容。若整片交换，B 就倒退了。这里每个块独立决定 current 指向哪一侧，主线程只给 processed 清单上的块换指针。下次读取 B 时仍见它保留下来的正确现状。

FireHalo 是邻界数据（halo，邻区贴着本区边沿的一层信息）。tile_edge(direction,i) 先判断某方向有没有数据，没有就返回 nullptr；上下边缘每个块仅需一行字对，左右方向则按原内核行跨度存放，跨行步长不一样。若错用一个统一 stride，水平方向也许正确，竖直方向却会在第二行起拿错字。

**br 选行 · L133–151**

```cpp
142  template <int TileSize, int N>
144      std::vector<ui> edges[4]; // up, down, left, right as seen by this rank
147          if (edges[direction].empty()) return nullptr;
148          const size_t stride = direction < 2 ? size_t(2) * TileSize
150          return edges[direction].data() + size_t(i) * stride;
```

**br 选行 · L156–165**

```cpp
156  template <int TileSize, int N>
157  std::vector<TileLink> make_tile_links(ui* initial_data, ui* spare_data) {
158      static_assert(TileSize > 0 && N > 0);
163          tiles.push_back({initial_data + id * tile_words, spare_data + id * tile_words});
164      return tiles;
```


**一句话记住**：块指针交换得有选择，边界数据也得按方向解释。

## 5.6　两种工作调度：node_step 与真正用到的 SpinLock

互斥（mutual exclusion，一次只准一个执行者进入临界区）在这里通过自旋锁实现。临界区（critical section，会碰共享队列的不容交错的操作范围）只覆盖领取或加入任务，不应把整块计算也关进去。线程结束时的 join 只处理线程生命周期，不保护 deque 内部。

node_step 是保留的每步建队版本，不是当前 main 的默认入口。它先复制 A 中的编号到 processed；接下来 A 会被弹空，这张清单是事后知道哪些块要换指针的凭据。一把 A_lock 管领取任务，一把 B_lock 管下轮提名。工作者拿到 id，退出短锁区后拆成块行、块列，从 table 找 current 和 next；若同进程内有相邻块，优先取真实邻块的边，否则保留 halo 或空指针。线程 0 也执行取任务的循环，不只是管理者。goto empty 在这里从主线程的空队列分支跳到等待和发布区，不是让后台线程直接消失。

看随包 spin_lock.hpp。flag 是原子布尔，false 表示无人占锁。lock() 用 exchange(true,acquire) 写入 true 并返回旧值；读到 false 的那个人拿到锁，读到 true 的继续循环。unlock() 用 release 写回 false，下一位成功获取锁的人能在正确顺序下看到上位留下的队列状态。lock_guard 是作用域锁卫士：构造时上锁，离开花括号、遇到 return 时也会析构解锁。所谓‘不用 mutex’是没有拿 std::mutex 当那把队列锁；源码仍引用 mutex 头里的通用 lock_guard。

自旋（spin waiting，反复检查直到条件满足）可能适合非常短的任务领取，但并不保证公平，也不保证一定比休眠划算。若持锁线程没获调度，其他线程会不断消耗处理器时间。代码没有用条件变量，没有等待新任务出现的休眠消费者；A 空就结束这一轮取任务，B 里的任务要等下一步。

**br 选行 · L167–187**

```cpp
167  template <int __thread_cnt, int __tile_size, int __tiles_side_cnt>
168  void node_step(std::deque<int>& deque_A, std::deque<int>& deque_B, std::vector<TileLink>& tiles, bool* marked,
173      const std::vector<int> processed(deque_A.begin(), deque_A.end());
178          workers.emplace_back([&, tid] {
184                          return;
```

**br 选行 · L191–213**

```cpp
196                      return tiles[tile_id].current;
198                  const ui* me = tile_ptr(id);
205                      up = tile_ptr(id - N) + tile_words - row_words;
209                      lf = tile_ptr(id - 1) + row_words - 2;
211                      rh = tile_ptr(id + 1);
```

**br 选行 · L250–265**

```cpp
255          t.join();
261          std::swap(tiles[id].current, tiles[id].next);
263      deque_A.swap(deque_B);
```

**spin_lock.hpp · L1–12**

```cpp
  1  #pragma once
  2  #include <atomic>
  3  
  4  // 最小自旋锁：同学的代码里用它保护任务队列
  5  struct SpinLock {
  6      std::atomic<bool> flag{false};
  7      void lock() {
  8          while (flag.exchange(true, std::memory_order_acquire)) {
  9          }
 10      }
 11      void unlock() { flag.store(false, std::memory_order_release); }
 12  };
```


**一句话记住**：A 空表示这一步领完了，不表示森林永远不再着火。

## 5.7　按块读文本：seek_input、tell_input、read_forest_region

seek_input 以字节偏移定位，Windows 和其他平台分别调用不同的大偏移接口；tell_input 返回同一套单位的当前文件位置。它们不是解析整数的函数。以二进制方式打开，才能通过两次位置相减得到物理行长。若一行是六个单数字、五个单空格和一个 LF，物理行宽 12；同样的内容若用 CRLF，物理行宽 13。用第一行量出的 stride 跳后续行，前提是整份文件行宽一致。这比题目对数值格式的要求更窄。

read_forest_region 读头部 n,m,t，根据 tile_side 申请两份块存储。真正关键在 local_side=n；这和第 289–293 行的‘四进程四象限’注释不同。随后仍用 rank/2、rank%2 乘 local_side 算行列起点。对 rank 0 两者都为零；对 rank 1 列起点已等于整图宽 n，并非右半张图的起点。读一批 tile_side 行后，parse(tid) 每隔 Threads 行选一行来转换，各人写不同的块内行。每个格直接取行起点加 2×列，这假设每个状态一个字符、相邻状态之间恰一个空格。它截取输入数字自身的两个二进制位交织写入 a,b，和 v7 的查表完全不同。

这一读入步骤分批建线程、join 后再读下一批，不能把模拟线程的常驻误写成整个程序从头到尾只建过一次线程。所有工作者完成后，文件重新定位到事件区，下一段才读行为事件。

**br 选行 · L273–286**

```cpp
283      return ::_ftelli64(fp);
285      return static_cast<int64_t>(::ftello(fp));
```

**br 选行 · L294–319**

```cpp
294  template <int TileSize, int Threads>
296                          std::vector<ui>& data_B, std::vector<TileLink>& tile_table) {
305      const int tiles_per_side = local_side / tile_side;
317      const int64_t stride = tell_input(fp) - base;
319      const int col_begin = (rank % 2) * local_side;
```

**br 选行 · L327–344**

```cpp
334                      const char* src = row + size_t(2) * (col_begin + tc * tile_side);
```

**br 选行 · L348–357**

```cpp
348          std::vector<std::thread> workers;
351              workers.emplace_back(parse, tid);
354              worker.join();
356      seek_input(fp, base + int64_t(n) * stride);
```


**一句话记住**：能算出某一行的物理地址，是因为输入满足特定排版，不是因为文本天生可随机访问。

## 5.8　两种魔法为何不以同一方式入队

handle_magic_ignite(rank,i,j,...) 用全局坐标判断归属，再依次确定局部块号、块内行、字号、字内位。先求 data[pos] & ~data[pos+1] & bit，只会选中树；点火要清低位 a、设高位 b，让 01 变 10（高位在前）。源码接着检查目标位置是否为火；原本就在烧的格也可能触发提名，这是允许重复请求但由 marked 去重的设计。火在块内，本块需要处理；若它恰在下沿，还得把下方邻块加入本步 A，而不是等本步传播完才发现边界。

revive_tile 只操作一块里的一个闭矩形，按字取交集。lo=0、hi=63 时 (~0<<lo)&(~0>>(63−hi)) 也能直接得到全 1；两个移位量各在 0..63，无须移 64。旧灰是 a&b；保留 a、只清 b，就从 11 变成树 01。把这一行换成 v7 的‘高面置 1、低面清 0’会把目标状态弄反。

handle_magic_revive 做的是两层裁剪：先用全局坐标与本进程地块求交，确认非空后才减去无符号起点；再把交集分配到每个相交小块。apply(tid) 按 job=tid,tid+thread_count,… 交错领块，每块只归一人。没点出新火的回春本身不用把所有复活块都排进转移队列：没有火邻时树不变，有火邻时原有火和边缘提名应覆盖那一块。这依赖活动块集合一直包含所有可能受火影响的位置。回春在当前图上已完成，所以即使块不参与转移，它的树也不会丢。

**br 选行 · L361–386**

```cpp
364      const ui row_begin = ui(rank / 2) * LocalSide;
367          return;
374      data[pos] &= ~ignite;
379          const int stride = tiles_per_side + 2;
385              enqueue_next<tiles_per_side>(static_cast<int>(marked_id) - 1, marked, deque_A);
```

**br 选行 · L394–404**

```cpp
395      for (int word = c0 / 64; word <= c1 / 64; ++word) {
398          const ui mask = (~ui{0} << lo) & (~ui{0} >> (63 - hi));
401              const ui ash = data[pos] & data[pos + 1] & mask;
402              data[pos + 1] &= ~ash; // 11 -> 01; all other states unchanged
```

**br 选行 · L413–439**

```cpp
414      const ui top = std::max(i1, row_begin);
416      const ui bottom = std::min(i2, row_begin + LocalSide - 1);
419          return;
421      const int r1 = int(bottom - row_begin), c1 = int(right - col_begin);
437          workers.emplace_back(apply, tid);
```


**一句话记住**：惊雷引入新的火源；回春改变燃料，火源既有的影响范围才决定是否转移。

## 5.9　打包和解包边界：能通信与已通信之间

pack_boundary_fire(direction,mask,table,packed) 把选中的边缘块按掩码位从小到大的次序串起来。__builtin_popcountll 数一个整数的 1 位个数；返回值乘 TileSize 决定总共需要多少个 64 位字。S=16，mask 低三位为 101 时，选两段，一共 32 字也就是 256 字节有效载荷，不包括消息头。上下边缘原来就是一行连续字对，直接提炼火位即可。左右边缘是一列，得每行只抽一个位，再把 64 行的这些位压进一个字。字数相同，拼装的方法不同。

unpack_boundary_fire 反向按同一掩码顺序消费。接收方的左边对应发送方的右边；把收到的火放在模拟边缘的 b 位面，a 默认为零，旧内核的 b&~a 自然能识别火。它先用 assign 把整个方向的边沿填零，连被掩码省略的段也覆写；否则上一步在边上的火会像幽灵一样多活一拍。若一个边段有火，再把本地相应外缘的块加入 A。保证顺序、方向以及掩码配套是协议的一部分，只有一个裸字数组不能告诉接收者应该填哪条边。

检查 main：send_buffers、recv_buffers 定义后只有显式转成 void 的语句，没有 MPI_Send、MPI_Recv 或对应的非阻塞调用。传给 process_tasks 的 halo 是个对象，但四个向量并未通过这个主流程填入收到的远方火。两函数体现了接口和设计意图，不能当作已经打通的四节点交换。

**br 选行 · L446–462**

```cpp
446  template <int TileSize, int LocalSide = 4096>
447  void pack_boundary_fire(int direction, ui mask, const TileLink* table, std::vector<ui>& packed) {
450      packed.resize(size_t(__builtin_popcountll(mask)) * TileSize);
453          if (!(mask & (ui{1} << i)))
```

**br 选行 · L464–474**

```cpp
464              const int word = direction == 2 ? 0 : TileSize - 1;
467                  ui fire = 0;
470                      const ui burn = data[pos + 1] & ~data[pos];
471                      fire |= ((burn >> bit) & ui{1}) << k;
473                  packed[out++] = fire;
```

**br 选行 · L482–506**

```cpp
482  void unpack_boundary_fire(int direction, ui mask, const std::vector<ui>& packed,
491      edge.assign(size_t(N) * stride, ui{0});
495          ui* dst = edge.data() + size_t(i) * stride;
499              any_fire |= fire;
506                      dst[size_t(w * 64 + k) * row_words + 1] = ((fire >> k) & ui{1}) << bit;
```

**br 选行 · L509–514**

```cpp
509          if (any_fire) {
510              const int r = direction == 0 ? 1 : direction == 1 ? N : i + 1;
511              const int c = direction == 2 ? 1 : direction == 3 ? N : i + 1;
512              enqueue_next<N>(r * (N + 2) + c, marked, deque_A);
513          }
514      }
```

**br 选行 · L658–661**

```cpp
658      std::vector<ui> send_buffers[2], recv_buffers[2];
659      FireHalo<tile_size, tiles_per_side> halo;
660      (void)send_buffers;
661      (void)recv_buffers;
```


**一句话记住**：网络上的火必须有发送、接收和解包三段，缺一段就不会越过进程。

## 5.10　常驻池：SpinBarrier、process_tasks 和 worker

SpinBarrier(int n) 保存到站人数；wait() 复用‘计数＋世代’的门。同 v8 的机制相似，但这里的队伍不是按 tid 固定分几行，而是共同抢 A 中不规则出现的任务。若只有一个活动块，16 人里最多一人计算这个块，不能凭常驻线程数承诺并行度。若 256 块全部活跃，任务较易分摊，可队列锁也会更忙。任务稀疏时算得少不代表墙钟按面积同比例缩短。

process_tasks 在一个短锁范围里做 empty、front、pop_front；必须把‘看见任务’与‘拿走它’连成一件事。它再把带边框编号还原成紧凑 id，tile_ptr 局部函数返回对应块的 current。如果本地存在上下左右邻块，替换初始的 halo 指针；若没有，那一方向由 halo 或空边界承担。随后调用 propagate_tile_and_append，用 A 领取、B 提名，互不颠倒。没有等待条件变量，也没有线程一直守着空 A 等 B 生长。

main 里的 worker 包时步：0 号执行魔法、重置下轮去重表、把待处理块的列表抄入 processed；第一道门后所有人抽取 A；第二道门后 0 号按 processed 逐块交换，再互换队列；第三道门才放行下一轮。把 std::fill(marked,...,false) 放在当前任务名单形成以后，正是为了让本步同一块也能被下一步重新提名。工人线程只为此模拟循环创建一次；处理回春矩形时的额外小队是另一码事。

**br 选行 · L533–549**

```cpp
537          const unsigned g = gen_.load(std::memory_order_acquire);
539              cnt_.store(0, std::memory_order_relaxed);
540              gen_.store(g + 1, std::memory_order_release);
542              while (gen_.load(std::memory_order_acquire) == g) POOL_PAUSE();
549      std::atomic<unsigned> gen_{0};
```

**br 选行 · L595–617**

```cpp
599              if (A.empty()) return;
607          const ui* me = tile_ptr(id);
613          if (r > 0) up = tile_ptr(id - N) + tile_words - row_words;
614          if (r + 1 < N) dn = tile_ptr(id + N);
616          if (c + 1 < N) rh = tile_ptr(id + 1);
```

**br 选行 · L681–704**

```cpp
684              bar.wait();                       // 事件 + 脏表清零完成
687              if (tid == 0) {                   // 发布新状态：交换指针 + 换队列
693                  deque_A.swap(deque_B);
700      std::vector<std::thread> pool;
704      for (auto& th : pool) th.join();
```


**一句话记住**：队列决定谁做哪一块，栅栏决定哪一代可以对外可见。

## 5.11　输出与入口：rank 0 没有被特判

write_forest_output(path,n,tiles) 按全图行顺序恢复文本，而非按活动队列输出。给定全局行 r，tr=r/side 找块行、rr=r%side 找块内行，然后横扫每个块列。对一对字还原数值为 a 的这位加 b 的这位乘 2。每写一格先写空格，到行末用 q[-1] 改成换行，尾块再 flush。即使一大片森林 100 步都没入队，完整结果也必须把它打印出来。

main 写死 tile_size=16、thread_cnt=16、tiles_per_side=16384/(64×16)=16，开局把 16×16 个真块全部塞入 A。用全块开始是因为程序并没有在读入时建好初始着火清单。它读每条事件存到 unordered_map（hash map，按键查值的散列表），键是步号。主路径使用 MPI 的三个调用：MPI_Init 初始化环境，MPI_Comm_rank 将全局进程编号写入 rank，MPI_Finalize 结束本进程的 MPI 使用。没有 MPI_Comm_size 来判断启动数，也没有 rank!=0 就退出的分支。第 708 行每个跑到这里的进程都尝试写同一路径。不能按任务说明原样宣称‘只有 Rank 0 干主要活’；那是 v8 有 MPI 时的行为。

具体比较：官方参考在 n=16384、64 个进程均分时，每个进程应拿 256 行；rank 0 是 0..255，rank 1 是 256..511，rank 63 是 16128..16383；一般 rank k 对应 256k..256k+255。它由 Scatterv 分发，用 Isend/Irecv 送收邻行，Wait 等完成，再 Gatherv 汇合。br 的注释还写有四象限设计，但读入改为 rank 的 local_side=n，四象限的偏移计算却未一起改。取 rank 2，则 row_begin=n，定位已到地图行之后。没有收边界、聚合结果或唯一写者的证据。单进程固定尺寸是可能的使用情景，并非这本书验证过的可运行承诺。

**br 选行 · L553–570**

```cpp
553  static void write_forest_output(const char* path, int n, const std::vector<TileLink>& tiles) {
561      std::vector<char> buf(1u << 22);
562      char* q = buf.data();
565          std::fwrite(buf.data(), 1, size_t(q - buf.data()), fo);
566          q = buf.data();
```

**br 选行 · L574–585**

```cpp
575                      if (q + 2 > qend) flush();
577                      *q++ = char('0' + v);
578                      *q++ = ' ';
582          q[-1] = '\n';
```

**br 选行 · L621–645**

```cpp
621  int main(int argc, char** argv) {
631      int rank;
633      std::FILE* fp = std::fopen(argc > 1 ? argv[1] : "forest.in", "rb");
636      std::vector<TileLink> tile_table;
640          for (int c = 1; c <= tiles_per_side; ++c) enqueue_next<tiles_per_side>(r * marked_stride + c, marked, deque_A);
```

**br 选行 · L647–657**

```cpp
647      std::unordered_map<int, std::vector<int>> events;
```

**br 选行 · L700–716**

```cpp
700      std::vector<std::thread> pool;
704      for (auto& th : pool) th.join();
708      write_forest_output<tile_size>(argc > 2 ? argv[2] : "forest.out", n, tile_table);
710      auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
714      return 0;
```


**一句话记住**：要证明一个进程不写文件，得在入口到输出的路径上找到实际分支。

## 5.12　手推一对相邻块：何时可以不算

画两个横向相邻块 U、V。第 k 步后只有 U 的最右列出现新火，V 最左列是一棵树。U 的 right_burn 被设为非零，所以 U 与 V 都在 k+1 的 A 中。处理 V 时，curr[-1] 读 U 边字，`_shld<1>` 把那一位引到 V 第一位，于是 V 的树成为新火。U 的旧火这拍要变灰，因此 U 也绝不能因‘火已经向右传’而跳过。等大家写完 next，才交换 U、V 的链接，其他未动的块继续保留 current。这串因果能同时检验字边界、块边界、活动队列和双缓冲。

把两个块改成分属不同进程，故事在何处断开？V 不能直接解引用另一个进程的地址。它要收到 U 的火边，解包进自己的 halo，补上 V 的入队请求，再执行同一内核。br 源中局部算子与打包器都能指出来，但实际入口没有组装这条跨进程路径。两位作者都用位平面与线程；v1→v8 并非一直使用同一布局，v8 相对 v7 才是基本保留布局而换团队；br 相对这两版既换编码又换了调度单位。这个区分能防止把‘少算一些块’误归功给另一种线程库。


**一句话记住**：路径完整时才能把设计当成行为；保留在文件里的工具不等于主程序已经使用它。

