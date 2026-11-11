#include <iostream>
#include <vector>
#include <chrono>

namespace{
    using Clock = std::chrono::steady_clock;
}
auto ms_since = [](Clock::time_point t0) {       // 返回“从 t0 到现在”多少毫秒
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
};

int main(int argc, char** argv) {               //如果argc用不到就注释掉
    //argc (Argument Count)：代表命令行参数的个数。程序名字本身算 1 个，所以 argc 至少是 1。
    //argv (Argument Vector)：是一个字符串数组，存储了具体的参数内容。

    //计时零点
    auto t_all = Clock::now();

    //参数检查
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <输入文件> <输出文件>\n";
        return 1;
    }

    //从forest.in读取输入
    if(!freopen(argv[1], "r", stdin)) {             //
        std::cerr << "无法打开文件 foresst.in" << "\n";
        return 1;
    }
    //输出到forest.out
    if (!freopen(argv[2], "w", stdout)) {
        std::cerr << "无法打开输出文件\n";
        return 1;
    }

    //---计时点1---
    auto t0 = Clock::now();

    int n, m, t;
    std::cin >> n >> m >> t;
    std::vector<std::vector<int>> sta(n + 2, std::vector<int>(n + 2));
    for(int i = 1; i <= n; i++) {
        for(int j = 1; j <=n; j++) {
            std::cin >> sta[i][j];
        }
    }
    struct magic_event{
        int time, type;
        int para1[2]{};
        int para2[4]{};
    };

    std::vector<magic_event> magic(m);
    for(int i = 0; i < m; i++) {
        std::cin >> magic[i].time >> magic[i].type;
        if(magic[i].type == 1) {
            for(int j = 0; j < 2; j++) {
                std::cin >> magic[i].para1[j];
            }
        }
        if(magic[i].type == 2) {
            for(int j = 0; j < 4; j++) {
                std::cin >> magic[i].para2[j];
            }
        }
    }

    //---计算读入时间---
    double ms_read = ms_since(t0);

    //---主循环---
    double ms_event = 0, ms_move = 0;       //循环计时

    int cnt_event = 0;
    for(int i = 1; i <= t; i++) {
        t0 = Clock::now();

        //魔法事件在每个时间步最前面发生
        if(cnt_event < m && magic[cnt_event].time == i) {               //加一个边际检查
            if(magic[cnt_event].type == 1) {
                int x = magic[cnt_event].para1[0] + 1,
                    y = magic[cnt_event].para1[1] + 1;
                if(sta[x][y] == 1) {
                    sta[x][y] = 2;
                }
            }
            if(magic[cnt_event].type == 2) {
                int sx = magic[cnt_event].para2[0] + 1,
                    sy = magic[cnt_event].para2[1] + 1,
                    ex = magic[cnt_event].para2[2] + 1,
                    ey = magic[cnt_event].para2[3] + 1;
                for(int r = sx; r <= ex; r++) {
                    for(int c = sy; c <= ey; c++) {
                        if(sta[r][c] == 3) {
                            sta[r][c] = 1;
                        }
                    }
                }
            }
            cnt_event++;
        }
        ms_event += ms_since(t0);

        t0 = Clock::now();
        //发生状态转移
        std::vector<std::vector<int>> temp = sta;
        for(int r = 1; r <= n; r++) {
            for(int c = 1; c <= n; c++) {
                if(sta[r][c] == 1 &&
                    (sta[r - 1][c] == 2
                    || sta[r + 1][c] == 2
                    || sta[r][c - 1] == 2
                    || sta[r][c + 1] == 2)) {
                        temp[r][c] = 2;
                    }
                else if(sta[r][c] == 2) temp[r][c] = 3;
                else temp[r][c] = sta[r][c];
            }
        }
        sta = temp;
        ms_move += ms_since(t0);
    }

    //---输出最后的状态---
    t0 = Clock::now();               //输出计时
    for(int r = 1; r <= n; r++) {
        for(int c = 1; c <= n; c++) {
            std::cout << sta[r][c] <<((c == n) ? "\n" : " ");
        }
    }
    double ms_out = ms_since(t0);

    fprintf(stderr, "read=%.1f event=%.1f move=%.1f out=%.1f total=%.1f\n", ms_read, ms_event, ms_move, ms_out, ms_since(t_all));
    return 0;
}
