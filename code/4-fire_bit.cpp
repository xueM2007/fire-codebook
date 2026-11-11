#include <iostream>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstddef>

namespace {
    using Clock = std::chrono::steady_clock;
}
auto ms_since = [](Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
};

int main(int argc, char** argv) {
    auto t_all = Clock::now();
    auto t0 = Clock::now();

    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <输入文件> <输出文件>\n";
        return 1;
    }

    // ==========================================
    // 1. 快速读入
    // ==========================================
    FILE* fp = std::fopen(argv[1], "rb");
    if (!fp) { std::fprintf(stderr, "open in failed: %s\n", argv[1]); return 1; }

    std::fseek(fp, 0, SEEK_END);
    long fsz = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    std::vector<char> buf(fsz + 1);
    size_t got = std::fread(buf.data(), 1, fsz, fp);
    if (got != size_t(fsz)) {
        std::fprintf(stderr, "read failed: expected %ld bytes, got %zu\n", fsz, got);
        std::fclose(fp);
        return 1;
    }
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
    // 2. 初始化位平面
    // ==========================================
    int n = read_int(), m = read_int(), t = read_int();
    int W = (n + 63) >> 6;
    size_t total_words = size_t(n + 2) * W;

    // 两张状态位平面 + 下一步缓冲
    std::vector<uint64_t> b1(total_words, 0), b0(total_words, 0);
    std::vector<uint64_t> nb1(total_words, 0), nb0(total_words, 0);
    std::vector<uint64_t> fire(total_words, 0);

    // 读入状态映射表
    const uint64_t b1bit[4] = {0, 1, 1, 0};
    const uint64_t b0bit[4] = {0, 0, 1, 1};

    for (int i = 1; i <= n; ++i) {
        size_t row_offset = size_t(i) * W;
        for (int j = 0; j < n; ++j) {
            while (p < end && *p < '0') ++p;
            if (p >= end) { std::fprintf(stderr, "input truncated\n"); return 1; }

            int v = *p - '0';
            if (v > 0) {
                size_t idx = row_offset + (size_t(j) >> 6);
                uint64_t bit = 1ULL << (j & 63);
                if (b1bit[v]) b1[idx] |= bit;
                if (b0bit[v]) b0[idx] |= bit;
            }
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

    for(int i = 1; i <= t; i++) {
        t0 = Clock::now();

        // 处理魔法事件
        while (cnt_event < m && magic[cnt_event].time <= i) {
            if(magic[cnt_event].type == 1) { // 惊雷
                int x = magic[cnt_event].para1[0] + 1;
                int y = magic[cnt_event].para1[1] + 1;
                int c = y - 1; // 转换为 0-indexed 列
                size_t idx = size_t(x) * W + (size_t(c) >> 6);
                uint64_t bit = 1ULL << (c & 63);
                // 若该位是树 (b1=1, b0=0)，则变成火 (b1=1, b0=1)
                if ((b1[idx] & bit) && !(b0[idx] & bit)) {
                    b0[idx] |= bit;
                }
            }
            else if(magic[cnt_event].type == 2) { // 回春
                int sx = magic[cnt_event].para2[0] + 1;
                int sy = magic[cnt_event].para2[1] + 1;
                int ex = magic[cnt_event].para2[2] + 1;
                int ey = magic[cnt_event].para2[3] + 1;

                int c_sy = sy - 1;
                int c_ey = ey - 1;
                int j_start = c_sy >> 6;
                int j_end = c_ey >> 6;

                for (int r = sx; r <= ex; ++r) {
                    size_t row_offset = size_t(r) * W;
                    for (int j = j_start; j <= j_end; ++j) {
                        // 安全地计算边界掩码
                        int lo = (j == j_start) ? (c_sy & 63) : 0;
                        int hi = (j == j_end) ? (c_ey & 63) : 63;
                        uint64_t mask;
                        if (lo == 0 && hi == 63) {
                            mask = ~0ULL;
                        } else {
                            mask = ((1ULL << (hi - lo + 1)) - 1) << lo;
                        }

                        size_t idx = row_offset + j;
                        // 灰 (b1=0, b0=1) 变回树 (b1=1, b0=0)
                        uint64_t ash = mask & b0[idx] & ~b1[idx];
                        b1[idx] |= ash;
                        b0[idx] &= ~ash;
                    }
                }
            }
            cnt_event++;
        }
        ms_event += ms_since(t0);

        // 计算这一步的 fire 平面
        t0 = Clock::now();
        for (size_t k = 0; k < total_words; ++k) {
            fire[k] = b1[k] & b0[k];
        }

        // 状态转移
        for (int r = 1; r <= n; ++r) {
            size_t row_offset = size_t(r) * W;
            for (int j = 0; j < W; ++j) {
                size_t idx = row_offset + j;
                uint64_t f = fire[idx];

                // 左右邻居（注意跨字进位）
                uint64_t L = (f << 1) | (j > 0 ? fire[idx - 1] >> 63 : 0ULL);
                uint64_t R = (f >> 1) | (j + 1 < W ? fire[idx + 1] << 63 : 0ULL);
                // 上下邻居
                uint64_t nbr = L | R | fire[idx - W] | fire[idx + W];

                uint64_t tree = b1[idx] & ~b0[idx];
                uint64_t burn = tree & nbr;

                nb1[idx] = tree;
                nb0[idx] = b0[idx] | burn;
            }
        }
        std::swap(b1, nb1);
        std::swap(b0, nb0);
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

    // 状态查表：(b1<<1)|b0 -> {0, 3, 1, 2}
    const char tbl[4] = {'0', '3', '1', '2'};

    for (int i = 1; i <= n; ++i) {
        size_t row_offset = size_t(i) * W;
        for (int j = 0; j < n; ++j) {
            if (q + 2 > qend) flush();

            size_t idx = row_offset + (size_t(j) >> 6);
            uint64_t bit = 1ULL << (j & 63);
            int v1 = (b1[idx] & bit) ? 1 : 0;
            int v0 = (b0[idx] & bit) ? 1 : 0;

            *q++ = tbl[(v1 << 1) | v0];
            *q++ = (j == n - 1) ? '\n' : ' ';
        }
    }
    flush();
    std::fclose(fo);

    double ms_out = ms_since(t0);

    fprintf(stderr, "read=%.1f event=%.1f move=%.1f out=%.1f total=%.1f\n",
            ms_read, ms_event, ms_move, ms_out, ms_since(t_all));

    return 0;
}
