# 第 3 章　把 v1–v6 摊在 v7 旁边

**这一章你会得到什么**：给定一个旧版本，能找到 v7 中做同一件事的那几行。每组左右证据各用自己文件的行号，标题后的第一框是旧版，第二框是第 2 章的原行。 代码框保留关键原行，行号不连续时，中间行见标注的源文件区间。

|版本|相对 v7 少了什么|多了什么|后来如何取舍|
|---|---|---|---|
|v1|位面、固定行段、缓冲写出|二维整数与每步复制|保留读旧写新，移除复制|
|v2|连续位面和行窗口|预分配的二维整数网格|保留交换与缓冲，缩窄元素|
|v3|字级并行|一格一字节与左右哨兵列|改为一字 64 格，跨字补位|
|v4|线程团队、滚动行|完整缓存的火图|改成三行临时火|
|v5|贯穿多步的团队|每步两次组合并行区及火图|派工范围外移|
|v6|私有的行窗口|全局火图和填火阶段的同步|保留团队、交换与计时口径|

**一句话记住**：对齐同一个问题的两种答案，比对齐两个文件的同一个行号更有用。

## 3.1　v1　用第 64 列反查两种地址

**存一棵树**。取输入坐标 (0,63)。v1 内部地址是 sta[1][64]，早期版本同时给行列留了边；v7 取行 1 的第 0 个字、位 63。v7 的第 69、71 行一基行配零基列，对照第 76–77 行就能自己推出地址。保留的设计是不让森林外部提供火，不是必须给每一方向都留一格。

**选行 · 1-fire_pr.cpp L39–45**

```cpp
 39      int n, m, t;
 40      std::cin >> n >> m >> t;
 41      std::vector<std::vector<int>> sta(n + 2, std::vector<int>(n + 2));
 42      for(int i = 1; i <= n; i++) {
 43          for(int j = 1; j <=n; j++) {
 44              std::cin >> sta[i][j];
 45          }
```
**选行 · 7-fire_final.cpp L69–79**

```cpp
 73              if (p >= end) { std::fprintf(stderr, "input truncated\n"); return 1; }
 77                  uint64_t bit = 1ULL << (j & 63);
```

**算一格，写去哪里**。v1 的 if 用整数是否等于 2 识别火，v7 第 196–200 行把判断拆成两条位面表达式。给三棵相连树的最左格点火，若原地从左往右更新，火能虚假连跳两次；两版都用另一份存储消除此顺序依赖。v7 的两个 swap 在这里交换的是窗口，真正发布代际要看第 214–215 行。

**选行 · 1-fire_pr.cpp L104–120**

```cpp
106          std::vector<std::vector<int>> temp = sta;
110                      (sta[r - 1][c] == 2
112                      || sta[r][c - 1] == 2
116                  else if(sta[r][c] == 2) temp[r][c] = 3;
120          sta = temp;
```
**选行 · 7-fire_final.cpp L189–205**

```cpp
190                          uint64_t f = f_curr[j];
192                          uint64_t R = (f >> 1) | (j + 1 < W ? f_curr[j + 1] << 63 : 0ULL);
196                          uint64_t tree = b1[idx] & ~b0[idx];
200                          nb0[idx] = b0[idx] | burn;
204                      std::swap(f_curr, f_next);
```


**一句话记住**：地址能变，同时更新不变。

## 3.2　v2　仅交换容器还不够

**不变的格子也要落笔**。v2 的第三个分支把旧值原样写到 temp，保证每个位置在发布前已被赋值；v7 第 199、200 行不分情况，两个完整字直接覆盖上一代留下的数据。二者都不靠每轮清零来保持正确。拿一格从来不着火的灰，就能检查新图回收之后有没有丢掉它。

**选行 · 2-fire_pr.cpp L119–137**

```cpp
123                  if(sta[r][c] == 1 &&
125                      || sta[r][c - 1] == 2 || sta[r][c + 1] == 2)) {
128                  else if(sta[r][c] == 2) {
129                      temp[r][c] = 3;
136          std::swap(sta, temp);
```
**选行 · 7-fire_final.cpp L195–200**

```cpp
195                          size_t idx = curr_offset + j;
196                          uint64_t tree = b1[idx] & ~b0[idx];
197                          uint64_t burn = tree & nbr;
198  
199                          nb1[idx] = tree;
200                          nb0[idx] = b0[idx] | burn;
```

**写一个数字，两种翻译**。v2 已有字符缓冲、双字符输出和尾部刷新。它以 '0'+整数 直接造字符；若套在 v7，把编码下标 2 直接加到字符 0 会打印 2，实际下标 2 表示树，要打印 1。第 239、249 行那张逆表因此是 v7 状态编码的必要搭档，缓冲大小则是可以调整的工程选择。

**选行 · 2-fire_pr.cpp L145–165**

```cpp
146      if (!fo) { std::fprintf(stderr, "open out failed: %s\n", argv[2]); return 1; }
149      char* q = obuf.data();
153          std::fwrite(obuf.data(), 1, size_t(q - obuf.data()), fo);
159              if (q + 2 > qend) flush();
161              *q++ = (j == n) ? '\n' : ' ';
```
**选行 · 7-fire_final.cpp L234–250**

```cpp
235          std::fwrite(obuf.data(), 1, size_t(q - obuf.data()), fo);
236          q = obuf.data();
244              if (q + 2 > qend) flush();
249              *q++ = tbl[(v1 << 1) | v0];
250              *q++ = (j == n - 1) ? '\n' : ' ';
```


**一句话记住**：先写全再交换，先解码再造字符。

## 3.3　v3　从相邻字节到相邻位

**同样是扁平数组，偏移单位不一样**。v3 的 stride 数格，v7 的 W 数字。设 n=65，v3 内部坐标 (1,65) 在单字节数组的偏移 132；v7 对同一棵树用内部行 1、外部列 64，字下标 3、位 0。要从 v3 迁移，不能只改元素声明就照搬原公式。

**选行 · 3-fire_pr.cpp L59–72**

```cpp
 60      size_t stride = static_cast<size_t>(n) + 2;
 62      std::vector<uint8_t> cur(stride * stride, 0);
 63      std::vector<uint8_t> nxt(stride * stride, 0);
 66          size_t row_offset = static_cast<size_t>(i) * stride;
 69              if (p >= end) { std::fprintf(stderr, "input truncated\n"); return 1; }
```
**选行 · 7-fire_final.cpp L59–79**

```cpp
 63      std::vector<uint64_t> b1(total_words, 0), b0(total_words, 0);
 64      std::vector<uint64_t> nb1(total_words, 0), nb0(total_words, 0);
 66      const uint64_t b1bit[4] = {0, 1, 1, 0};
 73              if (p >= end) { std::fprintf(stderr, "input truncated\n"); return 1; }
 77                  uint64_t bit = 1ULL << (j & 63);
```

**上下可按行取，左右要跨字拼**。v3 对上、下取 idx±stride，对左、右取 idx±1，因四面补零所以有效。v7 第 191–193 行先移位，还需在 j>0、j+1<W 时借一位。字地址前进 1 跨了 64 列，不是到达相邻一棵树。只有在 j 的首尾加上条件才不会把下一行串进来。

**选行 · 3-fire_pr.cpp L131–149**

```cpp
132              size_t row_offset = r * stride;
138                      (cur[idx - stride] == 2 ||
139                       cur[idx + stride] == 2 ||
```
**选行 · 7-fire_final.cpp L189–200**

```cpp
190                          uint64_t f = f_curr[j];
192                          uint64_t R = (f >> 1) | (j + 1 < W ? f_curr[j + 1] << 63 : 0ULL);
196                          uint64_t tree = b1[idx] & ~b0[idx];
197                          uint64_t burn = tree & nbr;
200                          nb0[idx] = b0[idx] | burn;
```


**一句话记住**：同一个 +1，有时走一格，有时走六十四格。

## 3.4　v4　同样的公式，为什么不存 fire

**第五张图是缓存，不是第五种状态**。两边都用 b1 标树或火、b0 标火或灰，也都要备一套下一代。v4 第 64 行多的 fire 可以由 b1 & b0 恢复，它不是不可丢的数据。v7 不改变编码表，把这份中间结果的保留范围限制到第 120–122 行的三个行向量。

**选行 · 4-fire_bit.cpp L62–68**

```cpp
 62      std::vector<uint64_t> b1(total_words, 0), b0(total_words, 0);
 63      std::vector<uint64_t> nb1(total_words, 0), nb0(total_words, 0);
 64      std::vector<uint64_t> fire(total_words, 0);
 65  
 66      // 读入状态映射表
 67      const uint64_t b1bit[4] = {0, 1, 1, 0};
 68      const uint64_t b0bit[4] = {0, 0, 1, 1};
```
**选行 · 7-fire_final.cpp L59–67**

```cpp
 63      std::vector<uint64_t> b1(total_words, 0), b0(total_words, 0);
 64      std::vector<uint64_t> nb1(total_words, 0), nb0(total_words, 0);
 66      const uint64_t b1bit[4] = {0, 1, 1, 0};
 67      const uint64_t b0bit[4] = {0, 0, 1, 1};
```

**用内存换计算，也能反过来换**。v4 生成整个 fire 后，直接按 idx±W 取上下邻；v7 第 178、183 行在每个线程行段开头和滚动时解码火。线程接缝可能各自重复计算一行，这是真成本；代价换来不必让所有线程先写一遍共享大数组。若图小、线程多，窗口并非必然占优。

**选行 · 4-fire_bit.cpp L166–190**

```cpp
168              fire[k] = b1[k] & b0[k];
179                  uint64_t L = (f << 1) | (j > 0 ? fire[idx - 1] >> 63 : 0ULL);
182                  uint64_t nbr = L | R | fire[idx - W] | fire[idx + W];
185                  uint64_t burn = tree & nbr;
188                  nb0[idx] = b0[idx] | burn;
```
**选行 · 7-fire_final.cpp L177–200**

```cpp
190                          uint64_t f = f_curr[j];
192                          uint64_t R = (f >> 1) | (j + 1 < W ? f_curr[j + 1] << 63 : 0ULL);
196                          uint64_t tree = b1[idx] & ~b0[idx];
197                          uint64_t burn = tree & nbr;
200                          nb0[idx] = b0[idx] | burn;
```


**一句话记住**：可从旧面重算的火，不必保留整图。

## 3.5　v5　把派工位置画出来

**两次并行区对一个常驻区**。v5 两个 parallel for 各自覆盖一段循环。v7 第 111 行 parallel 包住时间步，连自己的起止行都要在第 116–118 行计算。若照抄旧程序，既自己分好区又让运行库只分给你一部分工作，可能分重或漏行；两种分工方式应完整择一。

**选行 · 5-fire_bit.cpp L156–170**

```cpp
159          #pragma omp parallel for schedule(static)
161              fire[k] = b1[k] & b0[k];
166          #pragma omp parallel for schedule(static)
```
**选行 · 7-fire_final.cpp L111–124**

```cpp
111      #pragma omp parallel
120          std::vector<uint64_t> f_prev(W, 0);
121          std::vector<uint64_t> f_curr(W, 0);
122          std::vector<uint64_t> f_next(W, 0);
```

**在哪个位置需要全员到齐**。v5 两个组合构造正常结束才能交换全图，结构上自带先后；v7 手工循环结束只说明本线程写完，必须在第 209 行显式等待。即使处理的行永远不重叠，交换的却是团队公用的两个容器，不能由最快的人随时更换。

**选行 · 5-fire_bit.cpp L171–186**

```cpp
171                  uint64_t f = fire[idx];
174                  uint64_t R = (f >> 1) | (j + 1 < W ? fire[idx + 1] << 63 : 0ULL);
177                  uint64_t tree = b1[idx] & ~b0[idx];
181                  nb0[idx] = b0[idx] | burn;
185          std::swap(b0, nb0);
```
**选行 · 7-fire_final.cpp L203–217**

```cpp
203                      std::swap(f_prev, f_curr);
204                      std::swap(f_curr, f_next);
209              #pragma omp barrier
214                  std::swap(b1, nb1);
215                  std::swap(b0, nb0);
```


**一句话记住**：分工方式变了，等待的位置也得重新找。

## 3.6　v6　少掉的是一轮全图生产

**不要把已有的改进再发明一次**。v6 已把事件终点记下并重置 t_step，v7 第 161、164 行保留。v6 的下一段让所有人填满 fire，并依赖 for 的末端等待；v7 第 169–178 行各自从不再被修改的旧面提取火，不必等待邻线程先生产一块 fire。取消同步的依据是依赖已经移除。

**选行 · 6-fire_bit.cpp L162–179**

```cpp
171              #pragma omp for schedule(static)
173                  fire[k] = b1[k] & b0[k];
178              #pragma omp for schedule(static)
```
**选行 · 7-fire_final.cpp L161–178**

```cpp
161                  ms_event += ms_since(t_step);
170                  if (start_r > 1) {
178                  for(int j = 0; j < W; ++j) f_curr[j] = b1[offset + j] & b0[offset + j];
```

**写回公式没变，窗口与发布不能混讲**。v6 四邻的逻辑式与 v7 第 191–200 行等价，区别在读取源。v6 第 201、202 行交换全局面；v7 额外有第 203、204 行的小窗口交换，但每步仍只在第 214、215 行发布一次完整地图。对一个中间字检查 64 个格都对，不代表窗口跨行轮换与发布时序也对。

**选行 · 6-fire_bit.cpp L182–205**

```cpp
183                      uint64_t f = fire[idx];
186                      uint64_t R = (f >> 1) | (j + 1 < W ? fire[idx + 1] << 63 : 0ULL);
190                      uint64_t burn = tree & nbr;
199              #pragma omp single
202                  std::swap(b0, nb0);
```
**选行 · 7-fire_final.cpp L189–218**

```cpp
190                          uint64_t f = f_curr[j];
193                          uint64_t nbr = L | R | f_prev[j] | f_next[j];
199                          nb1[idx] = tree;
209              #pragma omp barrier
215                  std::swap(b0, nb0);
```


**一句话记住**：四邻逻辑延续，火图的生存范围变了。

