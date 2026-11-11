// 8-fire_final.cpp —— HPCGame fire 题的最终版
//
// 设计要点（与 7-fire_final.cpp 的 OpenMP 版相比）：
//   1. 用 std::thread + 自旋栅栏替换 OpenMP —— 评测命令是
//      `mpiicpx forest.cpp -O3 -xHost -o forest`，没有 -fopenmp，
//      OpenMP 的 pragma 会被静默忽略、退化成单线程。
//   2. 事件段（妙手回春的大矩形）从串行改成和转移共用线程团队。
//   3. 线程常驻（一次创建、2000 步复用），避免每步 fork/join。
//   4. 若编译器能找到 mpi.h，则自动启用 MPI：只有 rank 0 干活，
//      其余进程立即退出，避免 64 个进程同时写同一个输出文件。
//
// 状态编码：两张位平面
//   b1 = 「是树或火」，b0 = 「火或灰」
//   空地 00 / 树 10 / 火 11 / 灰 01
//
// 转移（一步）：
//   fire = b1 & b0 ; tree = b1 & ~b0
//   burn = tree & (上下左右邻居里的 fire)
//   b1' = tree ; b0' = b0 | burn

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#if defined(__has_include)
#if __has_include(<mpi.h>)
#include <mpi.h>
#define FIRE_HAS_MPI 1
#endif
#endif

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define FIRE_PAUSE() _mm_pause()
#else
#define FIRE_PAUSE() ((void)0)
#endif

using ui = uint64_t;
using Clock = std::chrono::steady_clock;

namespace {

double ms_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// 自旋栅栏（sense-reversing）：16 个线程在 16 个核上自旋，比条件变量快一个量级。
class SpinBarrier {
public:
    explicit SpinBarrier(int n) : n_(n) {}
    void wait() {
        const unsigned gen = gen_.load(std::memory_order_acquire);
        if (cnt_.fetch_add(1, std::memory_order_acq_rel) + 1 == n_) {
            cnt_.store(0, std::memory_order_relaxed);
            gen_.store(gen + 1, std::memory_order_release);
        } else {
            while (gen_.load(std::memory_order_acquire) == gen) FIRE_PAUSE();
        }
    }

private:
    int n_;
    std::atomic<int> cnt_{0};
    std::atomic<unsigned> gen_{0};
};

} // namespace

int main(int argc, char** argv) {
    const auto t_all = Clock::now();

#ifdef FIRE_HAS_MPI
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) { // 其余进程不参与计算，直接退出
        MPI_Finalize();
        return 0;
    }
#else
    int rank = 0;
    (void)rank;
#endif

    if (argc < 3) {
        std::fprintf(stderr, "用法: %s <输入文件> <输出文件>\n", argv[0]);
#ifdef FIRE_HAS_MPI
        MPI_Finalize();
#endif
        return 1;
    }

    // ---------------- 1. 一次读入整个文件 ----------------
    auto t0 = Clock::now();
    std::FILE* fp = std::fopen(argv[1], "rb");
    if (!fp) {
        std::fprintf(stderr, "open in failed: %s\n", argv[1]);
        return 1;
    }
    std::fseek(fp, 0, SEEK_END);
    const long fsz = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    std::vector<char> buf(size_t(fsz) + 1);
    const size_t got = std::fread(buf.data(), 1, size_t(fsz), fp);
    std::fclose(fp);
    if (got != size_t(fsz)) {
        std::fprintf(stderr, "read failed\n");
        return 1;
    }
    buf[size_t(fsz)] = '\0';

    // ---------------- 2. 解析 ----------------
    {
        const char* p = buf.data();
        const char* end = buf.data() + fsz;
        auto read_int = [&]() {
            while (p < end && (*p < '0' || *p > '9')) ++p;
            int v = 0;
            while (p < end && *p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
            return v;
        };

        const int n = read_int(), m = read_int(), t = read_int();
        const int W = (n + 63) >> 6;
        const size_t total_words = size_t(n + 2) * W;

        std::vector<ui> b1(total_words, 0), b0(total_words, 0);
        std::vector<ui> nb1(total_words, 0), nb0(total_words, 0);

        for (int i = 1; i <= n; ++i) {
            const size_t row_off = size_t(i) * W;
            for (int j = 0; j < n; ++j) {
                while (p < end && *p < '0') ++p;
                if (p >= end) {
                    std::fprintf(stderr, "input truncated\n");
                    return 1;
                }
                const int v = *p - '0';
                if (v) {
                    const size_t widx = row_off + size_t(j >> 6);
                    const ui bit = ui{1} << (j & 63);
                    if (v == 1 || v == 2) b1[widx] |= bit; // 树或火
                    if (v == 2 || v == 3) b0[widx] |= bit; // 火或灰
                }
                ++p;
            }
        }

        struct Event {
            int time, type;
            int x1, y1, x2, y2;
        };
        const size_t m_sz = size_t(m);
        std::vector<Event> events(m_sz);
        for (int i = 0; i < m; ++i) {
            events[size_t(i)].time = read_int();
            events[size_t(i)].type = read_int();
            if (events[size_t(i)].type == 1) {
                events[size_t(i)].x1 = read_int();
                events[size_t(i)].y1 = read_int();
                events[size_t(i)].x2 = events[size_t(i)].y2 = -1;
            } else {
                events[size_t(i)].x1 = read_int();
                events[size_t(i)].y1 = read_int();
                events[size_t(i)].x2 = read_int();
                events[size_t(i)].y2 = read_int();
            }
        }
        buf.clear();
        buf.shrink_to_fit(); // 536 MB 还给系统

        const double ms_read = ms_since(t0);
        t0 = Clock::now();

        // ---------------- 3. 线程团队 ----------------
        int T = 16;
        if (const char* e = std::getenv("FIRE_THREADS")) T = std::atoi(e);
        const int hw = int(std::thread::hardware_concurrency());
        if (hw > 0 && T > hw) T = hw;
        if (T < 1) T = 1;

        const int chunk = (n + T - 1) / T;

        SpinBarrier bar(T);
        std::atomic<double> ms_event{0.0}, ms_move{0.0};

        auto worker = [&](int tid) {
            int start_r = tid * chunk + 1;
            int end_r = std::min(n, (tid + 1) * chunk);
            if (start_r > end_r) { start_r = 0; end_r = -1; } // 空闲但仍要参与栅栏

            const size_t w_sz = size_t(W);
            std::vector<ui> f_prev(w_sz), f_curr(w_sz), f_next(w_sz);
            int cnt_event = 0;

            for (int ts = 1; ts <= t; ++ts) {
                // ---- 阶段一：魔法事件 ----
                const auto te = Clock::now();
                while (cnt_event < m && events[size_t(cnt_event)].time <= ts) {
                    const Event& ev = events[size_t(cnt_event)];
                    if (ev.type == 1) {
                        if (tid == 0) {
                            const int x = ev.x1 + 1;     // 行（1-based）
                            const int y = ev.y1;         // 列（0-based，对应 bit）
                            const size_t idx = size_t(x) * W + size_t(y >> 6);
                            const ui bit = ui{1} << (y & 63);
                            if ((b1[idx] & bit) && !(b0[idx] & bit)) b0[idx] |= bit;
                        }
                    } else {
                        const int sx = ev.x1 + 1, ex = ev.x2 + 1;
                        const int c_sy = ev.y1, c_ey = ev.y2;
                        const int rs = std::max(sx, start_r), re = std::min(ex, end_r);
                        if (rs <= re) {
                            const int j_start = c_sy >> 6, j_end = c_ey >> 6;
                            for (int r = rs; r <= re; ++r) {
                                const size_t row_off = size_t(r) * W;
                                for (int j = j_start; j <= j_end; ++j) {
                                    const int lo = (j == j_start) ? (c_sy & 63) : 0;
                                    const int hi = (j == j_end) ? (c_ey & 63) : 63;
                                    const ui mask = (lo == 0 && hi == 63)
                                                        ? ~ui{0}
                                                        : (((ui{1} << (hi - lo + 1)) - 1) << lo);
                                    const size_t idx = row_off + size_t(j);
                                    const ui ash = mask & b0[idx] & ~b1[idx];
                                    b1[idx] |= ash;   // 01 -> 10
                                    b0[idx] &= ~ash;
                                }
                            }
                        }
                    }
                    ++cnt_event;
                }
                if (tid == 0) ms_event.store(ms_event.load() + ms_since(te));
                bar.wait();

                // ---- 阶段二：状态转移（三行滚动窗口） ----
                const auto tm = Clock::now();
                if (start_r <= end_r) {
                    if (start_r > 1) {
                        const size_t off = size_t(start_r - 1) * W;
                        for (int j = 0; j < W; ++j) f_prev[j] = b1[off + j] & b0[off + j];
                    } else {
                        std::fill(f_prev.begin(), f_prev.end(), ui{0});
                    }
                    {
                        const size_t off = size_t(start_r) * W;
                        for (int j = 0; j < W; ++j) f_curr[j] = b1[off + j] & b0[off + j];
                    }
                    for (int r = start_r; r <= end_r; ++r) {
                        if (r < n) {
                            const size_t off = size_t(r + 1) * W;
                            for (int j = 0; j < W; ++j) f_next[j] = b1[off + j] & b0[off + j];
                        } else {
                            std::fill(f_next.begin(), f_next.end(), ui{0});
                        }
                        const size_t row = size_t(r) * W;
                        for (int j = 0; j < W; ++j) {
                            const ui f = f_curr[j];
                            const ui L = (f << 1) | (j > 0 ? f_curr[j - 1] >> 63 : ui{0});
                            const ui R = (f >> 1) | (j + 1 < W ? f_curr[j + 1] << 63 : ui{0});
                            const ui nbr = L | R | f_prev[j] | f_next[j];
                            const size_t idx = row + size_t(j);
                            const ui tree = b1[idx] & ~b0[idx];
                            const ui burn = tree & nbr;
                            nb1[idx] = tree;
                            nb0[idx] = b0[idx] | burn;
                        }
                        std::swap(f_prev, f_curr);
                        std::swap(f_curr, f_next);
                    }
                }
                if (tid == 0) ms_move.store(ms_move.load() + ms_since(tm));
                bar.wait();

                if (tid == 0) {
                    std::swap(b1, nb1);
                    std::swap(b0, nb0);
                }
                bar.wait();
            }
        };

        std::vector<std::thread> pool;
        pool.reserve(size_t(T) - 1);
        for (int k = 1; k < T; ++k) pool.emplace_back(worker, k);
        worker(0);
        for (auto& th : pool) th.join();

        // ---------------- 4. 输出 ----------------
        const double ms_sim = ms_since(t0);
        t0 = Clock::now();

        std::FILE* fo = std::fopen(argv[2], "wb");
        if (!fo) {
            std::fprintf(stderr, "open out failed: %s\n", argv[2]);
            return 1;
        }
        std::vector<char> obuf(1u << 22);
        char* q = obuf.data();
        char* qend = q + obuf.size();
        auto flush = [&]() {
            std::fwrite(obuf.data(), 1, size_t(q - obuf.data()), fo);
            q = obuf.data();
        };
        const char tbl[4] = {'0', '3', '1', '2'}; // (b1<<1)|b0
        for (int i = 1; i <= n; ++i) {
            const size_t row_off = size_t(i) * W;
            for (int j = 0; j < n; ++j) {
                if (q + 2 > qend) flush();
                const size_t idx = row_off + size_t(j >> 6);
                const ui bit = ui{1} << (j & 63);
                const int v1 = (b1[idx] & bit) ? 1 : 0;
                const int v0 = (b0[idx] & bit) ? 1 : 0;
                *q++ = tbl[(v1 << 1) | v0];
                *q++ = (j == n - 1) ? '\n' : ' ';
            }
        }
        flush();
        std::fclose(fo);
        const double ms_out = ms_since(t0);

        std::fprintf(stderr, "threads=%d read=%.1f event=%.1f move=%.1f sim=%.1f out=%.1f total=%.1f\n", T, ms_read,
                     ms_event.load(), ms_move.load(), ms_sim, ms_out, ms_since(t_all));
    }

#ifdef FIRE_HAS_MPI
    MPI_Finalize();
#endif
    return 0;
}
