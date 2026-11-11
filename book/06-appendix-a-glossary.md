# 附录 A　术语第一次出现在哪里

**这一章你会得到什么**：认出拼法、查到含义，并能回到正文看它到底干了什么。顺序以正文从第 0 章到第 5 章，再到性能账本和问题记录的实际出现为准；A 组补充只在词典中详释的源码词。源码位置用 v1–v8、br、锁、参考标出，必要时可直接翻原件。

## 正文第 0 章

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`*`|pointer declarator, dereference or multiplication|区分指针声明、读写所指对象与乘法|p 是地址，*p 是地址处的字符|第 0 章；v1 L12|
|`时间步`|time step|一次魔法加一次同步转移|决定哪些火能在这一拍传播|0.1；v1 L78|
|`状态`|state|一个格子某时刻的取值|在空、树、火、灰中取一|0.1；v1 L105|
|`闭矩形`|inclusive rectangle|两端坐标都计入的矩形|单点回春也要生效|0.1|
|`双缓冲`|double buffering|用不同存储承载旧代与新代|防止一拍内连续传火|0.1|
|`double`|double precision floating-point type|双精度浮点类型|保存可带小数的耗时|0.1；v1 L9|
|`坐标`|coordinate|以行、列定位一个格子|外部从零，v7 内部真行从一|0.2；br L60|
|`文本文件`|text file|以字符存数据|地图数字要编码、解码|0.3|
|`标准错误流`|standard error stream|专门写诊断的输出通道|计时不污染森林文件|0.3|
|`字段`|token|切分出的一段文字|比对脚本逐项比较|0.3|
|`节点`|compute node|一台独立计算机|评测有四台|0.4|
|`核`|processor core|执行指令的硬件单元|64 核构成理想预算|0.4|
|`进程`|process|拥有独立地址空间的程序实例|不能直接引用另一进程的地图|0.4；v8 L10|
|`线程`|thread|进程中共享地址空间的执行路径|可并行读同一代位面|0.4；v6 L118|
|`thread`|standard thread object|代表一条标准 C++ 线程|v8 和 br 显式建队|0.4；v8 L4|
|`MPI`|Message Passing Interface|跨进程传递消息的标准接口|参考实现交换上下边界|0.4；v8 L9|
|`OpenMP`|Open Multi-Processing|共享内存并行指令与运行库|v5–v7 组织工作|0.4；v6 L112|
|`墙钟`|wall-clock time|实际经过的时间|评测限时的口径|0.4|
|`位`|bit|一个二值信息位置|两位可编码四个状态|0.5|
|`字`|word|此处为一个 64 位无符号整数|字循环一次处理至多 64 列|0.5|
|`位平面`|bit-plane|把同一种状态属性的位排在一起|一字保存 64 个格子的同一属性|0.5；v4 L55|
|`行窗口`|rolling window|复用相邻几行的暂存空间|v7 不再保存全图火位|0.5|
|`分块`|tile|一片可以独立排任务的正方形网格|br 按活跃区域调度|0.5|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 正文第 1 章

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`int`|integer type|带符号普通整数类型|状态、坐标和循环编号|1.1；v1 L12|
|`if`|if statement|条件为真才进入分支|惊雷只作用于树|1.1；v1 L20|
|`for`|for statement|依次执行初始化、条件判断、循环体与更新|遍历行、字、位、事件|1.1；v1 L42|
|`std::`|standard namespace qualification|从标准库命名空间查找名字|区分库函数和程序私有函数|1.1；v1 L6|
|`::`|scope resolution|在指定作用域查找名字|从 Clock 或 std 查找成员|1.1；v1 L6|
|`cin`|standard character input stream|C++ 标准字符输入流|v1 用抽取运算读取整数|1.1；v1 L40|
|`>>`|right shift or stream extraction|依操作数决定移位或流输入|除以 64 找字，或读取整数|1.1；v1 L40|
|`cout`|standard character output stream|C++ 标准字符输出流|v1 按格格式化输出|1.1；v1 L128|
|`<<`|left shift or stream insertion|依操作数决定移位或流输出|区别位面传播和打印用法|1.1；v1 L21|
|`二维数组`|two-dimensional array|以行列两级定位元素的结构|v1 用嵌套容器模拟|1.1|
|`vector`|std::vector, dynamic array|管理一段连续可变长存储的容器|装字节、状态字、事件或线程|1.1；v1 L2|
|`GiB`|gibibyte|2³⁰ 字节|估算两张 int 图|1.1|
|`freopen`|file stream reopen|按路径和模式重接既有流，失败返回空|v1 把 stdin 或 stdout 接到文件|1.1；v1 L26|
|`++`|increment|数值加一或指针移到下一元素|消费事件或移动字符游标|1.1；v1 L42|
|`==`|equality comparison|比较两个值是否相等|识别一种事件或一个边界|1.1；v1 L56|
|`else`|else clause|前一条件不成立的分支|决定是否保留旧状态|1.1；v1 L116|
|`char`|character type|容纳一个字符代码的类型|字符 2 要减字符 0 才得数值 2|1.2；v1 L12|
|`const`|const qualifier|限制通过这个名字修改值|游标可移动，不能通过它改字符|1.2；v2 L36|
|`fopen`|file open|按路径与模式开流，失败返回空指针|rb 读原始字节、wb 写结果|1.2；v2 L14|
|`fread`|file read|读入指定数目和大小的项，返回完整项数|与预期字节数比较|1.2；v2 L32|
|`fclose`|file close|关闭文件流并报告成功或失败|结束读或完成写出|1.2；v2 L33|
|`while`|while statement|只要条件成立就重复执行|跳过空白或消费到期事件|1.2；v2 L40|
|`缓冲区`|buffer|临时存放一批数据的区域|合并文件读写|1.2|
|`swap`|swap containers|交换两个对象或容器的内容归属|轮换窗口与两代地图|1.2；v2 L136|
|`!`|logical negation|把真假取反|检查文件句柄为空|1.2；v1 L26|
|`fprintf`|formatted file print|按格式向指定流写文本|格式化错误路径及毫秒数|1.2；v1 L133|
|`stderr`|standard error|C 的标准错误流|向人报告用法和耗时|1.2；v1 L133|
|`%`|remainder|求整数除法的余数|用块坐标找块内位置|1.2；v1 L133|
|`argv`|argument vector|保存各个参数字符串指针的数组视图|argv[1] 输入、argv[2] 输出|1.2；v1 L12|
|`return`|return statement|结束当前函数并可带回值|读数返回整数，main 返回状态|1.2；v1 L9|
|`data()`|container data pointer|返回连续元素区首地址|传给文件函数或建立游标|1.2；v2 L32|
|`&`|address-of, reference or bitwise AND|看上下文区分取地址、引用声明与按位交集|MPI 填 rank、队列按引用传递、火位求交|1.2；v1 L79|
|`&&`|logical AND|两边都为真，左边假时不求右边|先检查边界再访问数据|1.2；v1 L79|
|`uint8_t`|unsigned 8-bit integer|恰为 8 位的无符号整数|v3 的一个元素存一格|1.3；v3 L62|
|`unsigned`|unsigned integer|无符号整数|保存世代编号和移位所用类型|1.3；v8 L59|
|`size_t`|size type|能表示对象大小的无符号整数类型|计算字偏移和字节计数|1.3；v2 L153|
|`static_cast`|static cast|显式执行编译期可检查的类型转换|先转换尺寸，再算偏移|1.3；v3 L60|
|`explicit`|explicit constructor specifier|禁止利用该构造函数做隐式转换|参与人数要明确传给栅栏|1.3；v8 L57|
|`byte`|byte|由八位组成的字节，在本题平台成立|估算地图容量|1.3|
|`stride`|row stride|相邻两行对应位置的地址间距|分别以格、字或字节计|1.3；v3 L59|
|`MiB`|mebibyte|2²⁰ 字节|衡量整幅位面与输出缓存|1.3|
|`按位与`|bitwise AND|逐位取交集|筛出树、火或灰|1.4|
|`按位或`|bitwise OR|逐位取并集|合并四个方向的火来源|1.4|
|`~`|bitwise complement|每一位独立取反|从高面中去掉低面已置位位置|1.4；v4 L148|
|`uint64_t`|unsigned 64-bit integer|恰为 64 位的无符号整数|一位表示一格的某个属性|1.4；v4 L62|
|`掩码`|bit mask|选择待处理二进制位置的一串位|回春只改变矩形里的灰|1.4；v4 L143|
|`0ULL`|unsigned long long zero|具有指定宽度类别的无符号零|取反得到整字全一|1.4；v4 L148|
|`#pragma`|pragma directive|向编译器提供实现或并行指示|OpenMP 以此引入|1.5；v5 L159|
|`#pragma omp parallel`|OpenMP parallel construct|同队线程各执行结构块|v6、v7 跨时步保留团队|1.5；v5 L159|
|`#pragma omp parallel for`|OpenMP combined parallel worksharing loop|同时建立团队和分派循环|v5 对两段扫描分别派工|1.5；v5 L159|
|`schedule(static)`|static loop schedule|提前给团队分配循环迭代|固定大段，避免逐项抢任务|1.5；v5 L159|
|`static`|static storage or internal linkage|视位置表示静态存储期或内部链接|br 标记表与文件内辅助函数|1.5；v5 L159|
|`#pragma omp single`|OpenMP single construct|由团队任一线程执行，通常末尾等待|v7 一人处理事件及交换|1.6；v6 L122|
|`#pragma omp for`|OpenMP worksharing-loop construct|在已有团队内分摊循环|v6 分派造火图与计算两段|1.6；v6 L171|
|`KiB`|kibibyte|2¹⁰ 字节|衡量一个线程的三行窗口|1.7|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 正文第 2 章

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`#include`|header inclusion|在预处理时引入头文件声明|让编译器识别库设施|第2章 第 1–25 行；v1 L1|
|`[]`|subscript or lambda introducer|依上下文表示下标或匿名函数开头|取数组元素或建立小函数|第2章 第 1–25 行；v1 L8|
|`lambda`|lambda expression|在使用位置构造可调用对象|解析整数、刷新缓冲、线程工作|第2章 第 1–25 行|
|`namespace`|namespace definition|给名字划分范围|匿名命名空间内放时钟别名|第2章 第 1–25 行；v1 L5|
|`using`|type alias declaration|为一种类型定义别名|Clock 和 ui 都不是新造的类型|第2章 第 1–25 行；v1 L6|
|`steady_clock`|monotonic clock|用来量经过时长的单调时钟|系统日历调整不应让阶段时长倒走|第2章 第 1–25 行；v1 L6|
|`auto`|placeholder type specifier|令编译器从初始化推断类型|保存时刻、匿名函数和迭代元素|第2章 第 1–25 行；v1 L8|
|`time_point`|clock time point|某种时钟上的一个时刻|传给 ms_since 的起点|第2章 第 1–25 行；v1 L8|
|`duration`|time duration|带计数类型和单位的时间间隔|换算成浮点毫秒|第2章 第 1–25 行；v1 L9|
|`std::milli`|millisecond period|一千分之一秒为时长周期|让 count 的数值采用毫秒|第2章 第 1–25 行；v1 L9|
|`milli`|one thousandth ratio|表示千分之一的单位比例|duration 的一格计数为 1 毫秒|第2章 第 1–25 行；v1 L9|
|`now()`|clock now|返回该时钟当前时刻|起点终点相减|第2章 第 1–25 行；v1 L9|
|`count()`|duration count|取得以当前单位计量的数值|传给 printf 风格的输出|第2章 第 1–25 行；v1 L9|
|`int main(int argc, char** argv)`|program entry signature|参数个数加参数字符串指针|题面通过命令行传入两个文件名|第2章 第 1–25 行；v1 L12|
|`main`|program entry point|程序入口函数|接收路径并组织全过程|第2章 第 1–25 行；v1 L12|
|`argc`|argument count|命令行参数数目，含程序名|检查是否提供两个路径|第2章 第 1–25 行；v1 L12|
|`char**`|pointer to pointer to char|指向字符串指针的指针|argv 按下标取路径|第2章 第 1–25 行；v1 L12|
|`cerr`|standard character error stream|C++ 标准错误流|v7 写用法提示|第2章 第 1–25 行；v1 L21|
|`min`|minimum|返回较小的候选值|截住末段的行号|第2章 第 1–25 行；v7 L118|
|`fill`|fill range|把半开区间中的每个元素赋给同一值|清边沿火或去重表|第2章 第 1–25 行；v7 L174|
|`FILE`|file stream object|C 文件流的类型|以指针传递已打开的文件|第2章 第 26–55 行；v2 L26|
|`fseek`|file seek|以起点、当前位置或末端为基准移动文件位置|测长和回到开头|第2章 第 26–55 行；v2 L28|
|`SEEK_END`|seek relative to end|以文件末端定位的常量|测量输入字节数|第2章 第 26–55 行；v2 L28|
|`long`|long integer type|带符号整型，宽度依平台|ftell 的返回类型|第2章 第 26–55 行；v2 L29|
|`ftell`|file tell|返回当前文件位置，失败可返回负数|末端位置当作文件长度|第2章 第 26–55 行；v2 L29|
|`SEEK_SET`|seek relative to start|以文件开头定位的常量|定位首字节或指定行|第2章 第 26–55 行；v2 L30|
|`!=`|not equal|比较值不相等|短读时进入失败分支|第2章 第 26–55 行；v3 L36|
|`[&]`|reference capture|匿名函数按引用使用外部局部量|跨调用更新字符游标|第2章 第 26–55 行；v2 L39|
|`按位或赋值`|bitwise OR assignment|只把掩码选中的位设成 1|不覆盖同一字中其他树|第2章 第 56–84 行|
|`struct`|structure type declaration|将相关字段组合成一种类型|记录不同魔法的字段|第2章 第 85–110 行；v1 L47|
|`reserve`|reserve capacity|预留容量而不创建新元素|线程池和链接表少搬家|第2章 第 85–110 行；v8 L291|
|`+=`|addition assignment|把计算量加到已有变量|累计若干时间步耗时|第2章 第 85–110 行；v1 L102|
|`omp_get_num_threads`|OpenMP team size query|无参数，取当前团队人数|算行段长度|第2章 第 111–140 行；v7 L113|
|`omp_get_thread_num`|OpenMP thread number query|无参数，取团队内零基编号|找本线程的起始行|第2章 第 111–140 行；v7 L114|
|`std::min`|minimum of two values|比较两个参数取较小者|截去超出本段的终行|第2章 第 111–140 行；v7 L118|
|`nowait`|OpenMP nowait clause|在允许的构造末尾取消隐式等待|v7 的 single 未指定该子句|第2章 第 111–140 行|
|`&=`|bitwise AND assignment|用按位交集更新原值|配合补集清除特定位|第2章 第 141–168 行；v4 L157|
|`?:`|conditional expression|按条件选择两个表达式之一|字两端取不同掩码|第2章 第 141–168 行|
|`begin()`|begin iterator|指向区间第一个元素|与 end 配对用于填充或复制|第2章 第 169–188 行；v7 L174|
|`end()`|past-the-end iterator|指向区间末端之后|不应解引用|第2章 第 169–188 行；v7 L174|
|`#pragma omp barrier`|OpenMP barrier construct|要求团队全部到达同步点|交换前等待所有写入完成|第2章 第 189–221 行；v7 L209|
|`size()`|container size|返回已存在的元素个数|字符向量里等于字节数|第2章 第 222–261 行；v2 L150|
|`fwrite`|file write|写入多项并返回成功写出的完整项数|将一段字符送入输出流|第2章 第 222–261 行；v2 L153|
|`memset`|memory set|按字节填充一段存储|只用于解释为何复用输出缓冲时不必先清零|第2章 第 222–261 行|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 正文第 4 章

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`mpiicpx`|Intel MPI C++ compiler wrapper|面向 C++ 的 MPI 编译驱动|题目指定的编译入口|4.1；v8 L5|
|`-O3`|optimization level three|请求较高级别优化|不等同主动启用 OpenMP|4.1；v8 L5|
|`-xHost`|host processor target|为编译主机处理器指令集优化|限制编译目标代机条件|4.1；v8 L5|
|`-o`|output file option|指定生成文件名|题面命名为 forest|4.1；v8 L5|
|`mpirun`|MPI launcher|启动多个进程|与每进程线程数是不同维度|4.1|
|`-np`|number of processes|向启动器指定进程数量|评测指定 64|4.1|
|`-fopenmp`|OpenMP enable option|启用 OpenMP 指令的一种编译选项|题面命令未列|4.1；v8 L5|
|`-fiopenmp`|Intel OpenMP enable option|启用 Intel 实现的一种编译选项|说明并非只有一种启用拼法|4.1|
|`class`|class type|把状态与方法封装为类型|SpinBarrier 把计数细节藏起来|4.2；v8 L55|
|`public`|public access|允许外部使用成员|调用 wait 和构造器|4.2；v8 L56|
|`private`|private access|仅类自身及获权者可直接使用|避免外部随意重置栅栏|4.2；v8 L68|
|`atomic`|atomic object|为特定对象提供不可分割的并发操作|栅栏的计数、代号以及锁旗标|4.2；v8 L22|
|`fetch_add`|atomic fetch and add|原子地加值并返回旧值|认出末位到站线程|4.2；v8 L60|
|`_mm_pause`|processor pause hint|忙等时的处理器提示|不承担同步或释放锁的工作|4.2；v8 L40|
|`#if`|conditional preprocessing|按预处理条件保留一段源码|选择平台或可用头文件|4.2；v8 L31|
|`defined`|macro definition test|判断指定宏是否存在|选择支持的指令集分支|4.2；v8 L31|
|`#define`|macro definition|定义预处理宏|设置可选 MPI 与等待提示|4.2；v8 L34|
|`#else`|alternate preprocessing branch|条件编译未选前支时改选此支|非 x86 等情况下使用替代写法|4.2；v8 L41|
|`void`|void type|函数不返回数据值|wait、lock、unlock 的价值在操作本身|4.2；v8 L42|
|`#endif`|end conditional preprocessing|结束条件编译区段|确定 MPI 专属语句覆盖范围|4.2；v8 L35|
|`load`|atomic load|取一次原子对象当前值|读取世代或累计计时|4.2；v8 L59|
|`memory_order_acquire`|acquire order|获取对方已发布的工作|读新世代后使用上一阶段数据|4.2；v8 L59|
|`store`|atomic store|把新值发布到原子对象|推进世代或解锁|4.2；v8 L61|
|`memory_order_relaxed`|relaxed order|只提供本对象的原子操作，不单独发布其他数据|栅栏先重置计数|4.2；v8 L61|
|`memory_order_release`|release order|把此前的修改随发布操作交给读者|推进代号或交回队列锁|4.2；v8 L62|
|`getenv`|environment variable lookup|按名取环境字符串，不存在则返回空指针|v8 查询 FIRE_THREADS|4.3；v8 L184|
|`FIRE_THREADS`|application thread-count setting|v8 读取的环境变量|覆盖默认线程人数|4.3；v8 L184|
|`atoi`|ASCII to integer|从字符串解析整数，无法转换时得到 0|v8 随后限制最小团队规模|4.3；v8 L184|
|`hardware_concurrency`|hardware concurrency hint|给出并行能力提示，可能为零|不能当作可用绑定核数|4.3；v8 L185|
|`I_MPI_PIN_DOMAIN`|Intel MPI process pinning domain|配置绑定域的环境变量|题面设置为 core|4.3|
|`join`|thread join|等待一个可汇合线程执行结束|发布或结束前确认工作完成|4.3；v8 L8|
|`emplace_back`|construct at end|在容器末端直接构造元素|以函数与编号创建线程对象|4.3；v8 L292|
|`auto&`|deduced reference|由表达式推导对象类型并保留引用|按引用遍历线程对象|4.3；v8 L294|
|`max`|maximum|返回较大的候选值|求两个矩形或行段的起端|4.4；v8 L219|
|`->`|member access through pointer|通过指针访问对象成员|halo 指针调用 tile_edge|4.4；v4 L213|
|`clear`|clear container|移除元素而不承诺释放容量|输入结束或下一队列重置|4.5；v8 L176|
|`shrink_to_fit`|capacity reduction request|请求容量缩至接近现有大小，不强制执行|v8 尝试释放大输入缓冲|4.5；v8 L177|
|`__has_include`|header availability test|预处理时检查某头文件能否找到|并不验证 MPI 库已链接|4.6；v8 L31|
|`FIRE_HAS_MPI`|compile-time MPI feature macro|源码内部声明的条件编译标记|启用编号查询与非零进程收尾|4.6；v8 L34|
|`MPI_Init`|MPI initialization|初始化进程的 MPI 环境|在查询进程编号前调用|4.6；v8 L80|
|`MPI_Comm_rank`|communicator rank query|把本进程在通信组内的编号写入输出参数|v8 用于区分唯一计算者|4.6；v8 L82|
|`MPI_COMM_WORLD`|world communicator|通常包含本次启动全部参与进程的通信组|供 rank 查询|4.6；v8 L82|
|`MPI_Finalize`|MPI finalization|完成 MPI 环境收尾，是集体操作|非零 rank 可先进入收尾|4.6；v8 L84|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 正文第 5 章

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`template`|template declaration|按编译期参数生成具体实现|br 用块宽、队伍人数定制内核|5.2；br L18|
|`static_assert`|compile-time assertion|条件不满足则拒绝编译|拒绝位移量大于等于 64|5.2；br L20|
|`constexpr`|constant expression specifier|可用于常量表达式的声明方式|固定块尺寸、裁剪编译期分支|5.2；br L22|
|`inline`|inline specifier|函数说明符，不保证展开|用在小型双字移位函数|5.2；br L19|
|`ASM_SHLD`|assembly shift feature macro|选择双字移位汇编形式|未定义则表达式实现|5.2；br L21|
|`deque`|double-ended queue|支持从两端操作的顺序容器|A 取本步任务，B 收下一步任务|5.3；br L8|
|`bool`|Boolean type|真或假|锁旗标及块的去重记录|5.3；br L37|
|`bool*`|pointer to Boolean|指向布尔标记数组|标记哪些块已经入队|5.3；br L37|
|`push_back`|append element|在尾端添加元素|将块加入工作队列|5.3；br L43|
|`nullptr`|null pointer literal|不指向对象的指针值|物理边界传空表示没有邻火|5.5；br L63|
|`empty()`|empty check|检查容器是否为空|不对空队列调用 front|5.5；br L147|
|`SpinLock`|spin lock|等待时反复测试的互斥锁|保护活动块队列|5.6；br L52|
|`goto`|jump to label|跳转到当前函数中的指定标号|旧 node_step 跳到等待工作者处|5.6；br L222|
|`false`|Boolean false|布尔假值|未占锁或仍未排队|5.6；br L681|
|`lock()`|acquire exclusive lock|尝试独占临界区，未获时等待|保护共享队列|5.6；锁 L7|
|`exchange`|atomic exchange|原子换新值、返回旧值|抢锁者由旧旗标判定是否成功|5.6；锁 L8|
|`true`|Boolean true|布尔真值|块已经被标记或锁已占用|5.6；br L39|
|`unlock()`|release exclusive lock|释放互斥|让下一工作者能领取任务|5.6；锁 L11|
|`lock_guard`|scoped lock guard|构造时加锁、离开作用域时解锁|短范围封住取任务或排任务|5.6；br L119|
|`mutex`|mutual exclusion|使临界区互斥进入的设施|用作与自旋的比较，不是 br 实际队列锁类型|5.6；br L10|
|`const ui*`|pointer to read-only words|通过此指针只读字内容|内核只读旧块|5.6；br L50|
|`#pragma once`|single inclusion pragma|同一编译单元只处理该头一次|spin_lock.hpp 避免重复定义|5.6；锁 L1|
|`_ftelli64`|Windows 64-bit file tell|查询大文件位置|算第一行的物理长度|5.7；br L283|
|`int64_t`|signed 64-bit integer|恰为 64 位的带符号整数|存物理文件偏移|5.7；br L273|
|`ftello`|POSIX file tell|用 off_t 报告当前位置|与 fseeko 的单位配套|5.7；br L285|
|`static_cast<int>`|explicit conversion to int|把值转换到 int|将尺寸或索引传给整型接口|5.8；br L316|
|`std::max`|maximum of two values|比较两个参数取较大者|求矩形交集的起端|5.8；v8 L219|
|`__builtin_popcountll`|population count|数无符号长长整数中置位数|为边界选中块分配打包空间|5.9；br L450|
|`assign`|assign a range or count|用给定范围或数量的元素替换内容|重建 halo 或复制 processed|5.9；br L491|
|`MPI_Send`|blocking send|阻塞发送接口|仅作为 br 主路径没有的通信举例|5.9|
|`MPI_Recv`|blocking receive|阻塞接收接口|同样没有出现在 br 主路径|5.9|
|`resize`|resize container|改变已有元素数量|为位数据与打包内容提供空间|5.9；br L307|
|`front`|front element|取得队列首元素|先读编号再弹出|5.10；br L185|
|`pop_front`|remove first element|移除首元素，不返回取出的值|领取一个活动块|5.10；br L186|
|`unordered_map`|unordered associative container|按键查关联值的散列表|br 用步号查事件|5.11；br L12|
|`MPI_Comm_size`|communicator size query|取通信组参与进程数|参考版按这个数分行，br 入口无此检查|5.11；参考 L36|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 问题记录

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`fgets`|read a line into a buffer|最多读容量减一的字符并终止字符串|br 按行装图或事件|D.2；br L299|
|`sscanf`|scan formatted string|从字符缓冲按格式提取字段并返回成功数|br 解析头行与事件|D.2；br L300|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。

## 补充源码用语

|术语|英文全称或原名|一句话定义|在本题里的作用|首次位置|
|---|---|---|---|---|
|`#ifdef`|if macro defined|若宏已定义才编译相应段|可选 MPI 与汇编分支|附录 A；v8 L79|
|`#ifndef`|if macro not defined|宏未定义才保留此段|br 可排除主程序|附录 A；br L517|
|`-machinefile`|machine file option|指定参与主机清单|题面每节点 16 个进程位置|附录 A|
|`. / ->`|member access|前者访问对象成员，后者通过指针访问|从事件取时间、从 halo 取块边缘|附录 A|
|`FIRE_NO_MAIN`|main exclusion feature macro|预处理时可排除入口等代码|方便辨认库段和可执行段|附录 A；br L517|
|`I_MPI_PIN`|Intel MPI process pinning|配置进程绑定的环境变量|题面设置为 1|附录 A|
|`MPI_Abort`|MPI abort routine|要求异常终止通信组中的任务|参考版打开文件失败时使用|附录 A；参考 L49|
|`MPI_Bcast`|broadcast|将一份数据从根进程广播到全组|参考版发布输入参数和事件|附录 A；参考 L85|
|`MPI_Gatherv`|variable-count gather|按各进程数量与偏移收集|参考版收齐结果|附录 A；参考 L224|
|`MPI_INT`|MPI integer datatype|消息元素按 C 的 int 描述|参考版传行列和整数格|附录 A；参考 L85|
|`MPI_Irecv`|nonblocking receive|登记非阻塞接收并给出请求|参考版接邻行|附录 A；参考 L170|
|`MPI_Isend`|nonblocking send|启动非阻塞发送并给出请求|参考版送自己边缘行|附录 A；参考 L169|
|`MPI_Request`|MPI request handle|标识一个异步操作的请求|参考版保存发送和接收请求|附录 A；参考 L159|
|`MPI_STATUS_IGNORE`|MPI status ignore indicator|允许调用不返回状态结构|等待邻行到达|附录 A；参考 L184|
|`MPI_Scatterv`|variable-count scatter|按每进程给定数量与偏移分发数据|参考版分行|附录 A；参考 L116|
|`MPI_Wait`|request completion wait|等请求完成|收到边界才更新|附录 A；参考 L184|
|`MPI_Wtime`|MPI wall-clock timer|返回用于量经过时间的秒数|参考版记录阶段耗时|附录 A；参考 L33|
|`ULL`|unsigned long long suffix|标明无符号长长整数字面量|构造能选中位 63 的 1|附录 A|
|`[]{}`|array value initialization|为固定数组提供空初始化列表|坐标字段初值为零|附录 A|
|`_fseeki64`|Windows 64-bit file seek|用大偏移重新定位文件|br 的 Windows 偏移分支|附录 A；br L275|
|`asm`|inline assembly|在高级语言内嵌入机器指令模板|ASM_SHLD 分支可选|附录 A；br L25|
|`continue`|continue statement|跳到当前循环下一次迭代|跳过未选中的边段|附录 A；br L454|
|`exit`|process termination|退出整个进程|br 打不开输出文件时退出|附录 A；br L559|
|`find`|lookup by key|返回对应迭代器，找不到等于 end|只在有事件的时间步处理魔法|附录 A；br L671|
|`for (;;)`|unbounded for loop|省略循环条件，靠显式控制流退出|队列空时返回|附录 A；br L595|
|`fscanf`|formatted stream scan|按格式从文件流读数据|br 注释提及，主入口读事件用 fgets 和 sscanf|附录 A；br L293|
|`fseeko`|POSIX file seek|用 off_t 类型偏移定位|br 的非 Windows 路径|附录 A；br L277|
|`int&`|reference to integer|为已有整数提供可修改的别名|read_forest_region 写回尺寸|附录 A；br L295|
|`memory_order_acq_rel`|acquire-release order|合并获取与释放的读改写顺序|累计多人到站|附录 A；v8 L60|
|`off_t`|file offset type|表示文件偏移的类型|非 Windows 大文件位置转换|附录 A；br L277|
|`operator ()`|call operator|函数对象接受参数的调用形式|read_int 和 flush 加括号才执行|附录 A|
|`shld`|double-precision left shift|从相邻字补位的双字移位|拼左右相邻边界|附录 A；br L25|
|`sizeof`|size of object or type|给出对象或类型所占字节数|控制读取事件行的缓冲上限|附录 A；br L299|
|`stdin`|standard input|默认输入流|v1 重定向到输入路径|附录 A；v1 L26|
|`stdout`|standard output|默认输出流|v1 重定向到结果路径|附录 A；v1 L31|
|`逻辑或`|logical OR|有一边真就为真|识别不在数字范围的字符|附录 A|

**一句话记住**：查到定义后，带着末列的位置回到代码里验一次。
