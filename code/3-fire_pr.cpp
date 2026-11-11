#include <iostream>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstddef> // for size_t

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
    // 1. 快速读入 (修复①：检查 fread 返回值)
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
    // 2. 读入数据与初始化扁平数组
    // ==========================================
    int n = read_int(), m = read_int(), t = read_int();

    // 修复③：stride 和后续的索引计算统一使用 size_t，彻底杜绝溢出
    size_t stride = static_cast<size_t>(n) + 2;

    std::vector<uint8_t> cur(stride * stride, 0);
    std::vector<uint8_t> nxt(stride * stride, 0);

    for (int i = 1; i <= n; ++i) {
        size_t row_offset = static_cast<size_t>(i) * stride;
        for (int j = 1; j <= n; ++j) {
            while (p < end && *p < '0') ++p;
            if (p >= end) { std::fprintf(stderr, "input truncated\n"); return 1; }

            cur[row_offset + static_cast<size_t>(j)] = *p - '0';
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

        while (cnt_event < m && magic[cnt_event].time <= i) {
            if(magic[cnt_event].type == 1) {
                size_t x = static_cast<size_t>(magic[cnt_event].para1[0]) + 1;
                size_t y = static_cast<size_t>(magic[cnt_event].para1[1]) + 1;
                size_t idx = x * stride + y;
                if(cur[idx] == 1) cur[idx] = 2;
            }
            if(magic[cnt_event].type == 2) {
                size_t sx = static_cast<size_t>(magic[cnt_event].para2[0]) + 1;
                size_t sy = static_cast<size_t>(magic[cnt_event].para2[1]) + 1;
                size_t ex = static_cast<size_t>(magic[cnt_event].para2[2]) + 1;
                size_t ey = static_cast<size_t>(magic[cnt_event].para2[3]) + 1;
                for(size_t r = sx; r <= ex; r++) {
                    size_t row_offset = r * stride;
                    for(size_t c = sy; c <= ey; c++) {
                        size_t idx = row_offset + c;
                        if(cur[idx] == 3) cur[idx] = 1;
                    }
                }
            }
            cnt_event++;
        }
        ms_event += ms_since(t0);

        t0 = Clock::now();

        for(size_t r = 1; r <= static_cast<size_t>(n); r++) {
            size_t row_offset = r * stride;
            for(size_t c = 1; c <= static_cast<size_t>(n); c++) {
                size_t idx = row_offset + c;
                uint8_t state = cur[idx];

                if(state == 1 &&
                    (cur[idx - stride] == 2 ||
                     cur[idx + stride] == 2 ||
                     cur[idx - 1] == 2      ||
                     cur[idx + 1] == 2))
                {
                    nxt[idx] = 2;
                }
                else if(state == 2) {
                    nxt[idx] = 3;
                }
                else {
                    nxt[idx] = state;
                }
            }
        }

        std::swap(cur, nxt);
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

    for (size_t i = 1; i <= static_cast<size_t>(n); ++i) {
        size_t row_offset = i * stride;
        for (size_t j = 1; j <= static_cast<size_t>(n); ++j) {
            if (q + 2 > qend) flush();
            *q++ = char('0' + cur[row_offset + j]);
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
