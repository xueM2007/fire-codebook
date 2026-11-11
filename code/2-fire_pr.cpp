#include <iostream>
#include <vector>
#include <chrono>
#include <cstdio>

namespace {
    using Clock = std::chrono::steady_clock;
}
auto ms_since = [](Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
};

int main(int argc, char** argv) {
    // 【修改①】t_all 之后、fopen 之前开始计时，把文件 I/O 包进 read 段
    auto t_all = Clock::now();
    auto t0 = Clock::now();

    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <输入文件> <输出文件>\n";
        return 1;
    }

    // ==========================================
    // 1. 一次性读入全部内容到内存 (包含在 read 计时中)
    // ==========================================
    FILE* fp = std::fopen(argv[1], "rb");
    if (!fp) { std::fprintf(stderr, "open in failed: %s\n", argv[1]); return 1; }
    std::fseek(fp, 0, SEEK_END);
    long fsz = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    std::vector<char> buf(fsz + 1);
    std::fread(buf.data(), 1, fsz, fp);
    std::fclose(fp);
    buf[fsz] = '\0';

    const char* p = buf.data();
    const char* end = buf.data() + fsz;

    auto read_int = [&]() {
        while (p < end && (*p < '0' || *p > '9')) ++p;
        int v = 0;
        while (p < end && *p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); ++p; }
        return v;
    };

    // ==========================================
    // 2. 读入阶段
    // ==========================================
    int n = read_int(), m = read_int(), t = read_int();
    std::vector<std::vector<int>> sta(n + 2, std::vector<int>(n + 2));

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            while (p < end && *p < '0') ++p;  // 跳过空白

            // 【修改③】解析兜底，防止文件截断导致写入 -48
            if (p >= end) {
                std::fprintf(stderr, "input truncated\n");
                return 1;
            }

            sta[i][j] = *p - '0';
            ++p;
        }
    }

    struct magic_event {
        int time, type;
        int para1[2]{};
        int para2[4]{};
    };

    std::vector<magic_event> magic(m);
    for(int i = 0; i < m; i++) {
        magic[i].time = read_int();
        magic[i].type = read_int();
        if(magic[i].type == 1) {
            for(int j = 0; j < 2; j++) magic[i].para1[j] = read_int();
        }
        if(magic[i].type == 2) {
            for(int j = 0; j < 4; j++) magic[i].para2[j] = read_int();
        }
    }

    double ms_read = ms_since(t0);

    // ==========================================
    // 3. 主循环计算
    // ==========================================
    double ms_event = 0, ms_move = 0;
    int cnt_event = 0;
    std::vector<std::vector<int>> temp(n + 2, std::vector<int>(n + 2));

    for(int i = 1; i <= t; i++) {
        t0 = Clock::now();

        // 【修改②】将 == i 改为 <= i，增强鲁棒性，防止乱序或0导致卡死
        while (cnt_event < m && magic[cnt_event].time <= i) {
            if(magic[cnt_event].type == 1) {
                int x = magic[cnt_event].para1[0] + 1;
                int y = magic[cnt_event].para1[1] + 1;
                if(sta[x][y] == 1) sta[x][y] = 2;
            }
            if(magic[cnt_event].type == 2) {
                int sx = magic[cnt_event].para2[0] + 1;
                int sy = magic[cnt_event].para2[1] + 1;
                int ex = magic[cnt_event].para2[2] + 1;
                int ey = magic[cnt_event].para2[3] + 1;
                for(int r = sx; r <= ex; r++) {
                    for(int c = sy; c <= ey; c++) {
                        if(sta[r][c] == 3) sta[r][c] = 1;
                    }
                }
            }
            cnt_event++;
        }
        ms_event += ms_since(t0);

        t0 = Clock::now();

        for(int r = 1; r <= n; r++) {
            for(int c = 1; c <= n; c++) {
                if(sta[r][c] == 1 &&
                    (sta[r - 1][c] == 2 || sta[r + 1][c] == 2
                    || sta[r][c - 1] == 2 || sta[r][c + 1] == 2)) {
                        temp[r][c] = 2;
                }
                else if(sta[r][c] == 2) {
                    temp[r][c] = 3;
                }
                else {
                    temp[r][c] = sta[r][c];
                }
            }
        }
        std::swap(sta, temp);
        ms_move += ms_since(t0);
    }

    // ==========================================
    // 4. 快速缓冲输出
    // ==========================================
    t0 = Clock::now();

    FILE* fo = std::fopen(argv[2], "wb");
    if (!fo) { std::fprintf(stderr, "open out failed: %s\n", argv[2]); return 1; }

    std::vector<char> obuf(1 << 22);
    char* q = obuf.data();
    char* qend = q + obuf.size();

    auto flush = [&]() {
        std::fwrite(obuf.data(), 1, size_t(q - obuf.data()), fo);
        q = obuf.data();
    };

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (q + 2 > qend) flush();
            *q++ = char('0' + sta[i][j]);
            *q++ = (j == n) ? '\n' : ' ';
        }
    }
    flush();
    std::fclose(fo);

    double ms_out = ms_since(t0);

    fprintf(stderr, "read=%.1f event=%.1f move=%.1f out=%.1f total=%.1f\n",
            ms_read, ms_event, ms_move, ms_out, ms_since(t_all));

    return 0;
}
