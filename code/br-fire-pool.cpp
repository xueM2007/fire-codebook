#include "spin_lock.hpp"
#include <chrono>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mpi.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>


using ui = uint64_t;
template <ui amount>
inline ui _shld(ui a, ui b) {
    static_assert(amount < 64);
#ifdef ASM_SHLD
    if constexpr (amount == 0) {
        return a;
    } else {
        asm("{shldq %2, %1, %0|shld %0, %1, %2}" : "+r"(a) : "r"(b), "i"(amount) : "cc");
        return a;
    }
#else
    if constexpr (amount == 0)
        return a;
    else
        return (a << amount) | (b >> (64 - amount));
#endif
}

template <int __tiles_side_cnt>
void enqueue_next(int id, bool* marked, std::deque<int>& tasks) {
    if (!marked[id]) {
        marked[id] = true;
        if (id / (__tiles_side_cnt + 2) && id % (__tiles_side_cnt + 2) &&
            id / (__tiles_side_cnt + 2) != (__tiles_side_cnt + 1) &&
            id % (__tiles_side_cnt + 2) != (__tiles_side_cnt + 1))
            tasks.push_back(id);
    }
}



template <int __tile_size, int __tiles_side_cnt> // 均为方形
void propagate_tile_and_append(const int id, const ui* me, const ui* up, const ui* dn, const ui* left,
                               const ui* right, ui* pong_buf, std::deque<int>& tasks,
                               bool* marked /*__tiles_side_cnt+2为边长，多一个halo*/, SpinLock& queue_lock) {
    static_assert(__tile_size > 0);
    constexpr int width = __tile_size;
    constexpr int height = 64 * width;
    // 汇总本轮新火，决定哪些 tile 进入下一轮。
    ui all_burn = 0, top_burn = 0, bottom_burn = 0;
    ui left_burn = 0, right_burn = 0;
    // me/pong_buf: 行优先，每行 width 对 (低位平面 a, 高位平面 b)。
    // bit 0 对应较小的列坐标。输入必须是魔法事件处理后的旧状态。
    // up/dn: 相邻 tile 紧贴本 tile 的一行，共 width 对 a,b。
    // left/right: 指向相邻 tile 第一行的边界字对；每行跨 width*2 个 ui。
    // 左 halo 用 bit 63，右 halo 用 bit 0；物理边界可传 nullptr，视为空地。
    // pong_buf 必须与所有输入不重叠；相邻 tile 同样只能读取旧状态。
    ui row1[__tile_size + 2];
    ui row2[__tile_size + 2];
    ui row3[__tile_size + 2];

    auto load_burn = [](const ui* src, ui* dst) {
        for (int j = 0; j < width; ++j)
            dst[j] = src ? src[2 * j + 1] & ~src[2 * j] : ui{0};
    };
    ui* prev = row1 + 1;
    ui* curr = row2 + 1;
    ui* next = row3 + 1;
    load_burn(up, prev);
    load_burn(me, curr);

    for (int i = 0; i < height; ++i) {
        load_burn(i + 1 < height ? me + size_t(i + 1) * width * 2 : dn, next);
        curr[-1] = left ? left[2 * i * width + 1] & ~left[2 * i * width] : ui{0};
        curr[width] = right ? right[2 * i * width + 1] & ~right[2 * i * width] : ui{0};

        const ui* src = me + size_t(i) * width * 2;
        ui* dst = pong_buf + size_t(i) * width * 2;
        ui row_burn = 0;
        ui first_ignite = 0, last_ignite = 0;
        // [优化] 去掉 L/C 的循环携带依赖，改用下标直接读 curr[j-1]/curr[j+1]
        //（边界由 curr[-1] / curr[width] 两个 halo 槽兜住），数学与原版逐位等价。
        for (int j = 0; j < width; ++j) {
            const ui C = curr[j];
            const ui Lw = curr[j - 1];
            const ui R = curr[j + 1];
            const ui near = prev[j] | next[j] | _shld<1>(C, Lw) | _shld<63>(R, C);
            const ui a = src[2 * j];
            const ui b = src[2 * j + 1];
            const ui ignite = (a & ~b) & near;
            dst[2 * j] = (a | b) & ~ignite;
            dst[2 * j + 1] = b | ignite;
            // 新状态的燃烧掩码恰好就是 ignite，旧火已转成灰烬。
            row_burn |= ignite;
            if (j == 0)
                first_ignite = ignite;
            last_ignite = ignite;
        }
        left_burn |= first_ignite & ui{1};
        right_burn |= last_ignite >> 63;
        all_burn |= row_burn;
        if (i == 0)
            top_burn = row_burn;
        if (i == height - 1)
            bottom_burn = row_burn;
        ui* recycled = prev;
        prev = curr;
        curr = next;
        next = recycled;
    }
    {
        std::lock_guard<SpinLock> guard(queue_lock);
        if (all_burn != 0)
            enqueue_next<__tiles_side_cnt>(id, marked, tasks);
        if (top_burn != 0)
            enqueue_next<__tiles_side_cnt>(id - __tiles_side_cnt - 2, marked, tasks);
        if (bottom_burn != 0)
            enqueue_next<__tiles_side_cnt>(id + __tiles_side_cnt + 2, marked, tasks);
        if (left_burn != 0)
            enqueue_next<__tiles_side_cnt>(id - 1, marked, tasks);
        if (right_burn != 0)
            enqueue_next<__tiles_side_cnt>(id + 1, marked, tasks);
    }
}

// A tiny "link table": changing a tile means swapping its two storage pointers.
// Non-owning: both backing arrays must remain alive and must not be resized.
struct TileLink {
    ui* current;
    ui* next;
};

// Received neighbor fire, represented as a=0,b=fire for the existing kernel.
// Up/down store one row per tile; left/right retain the kernel's row stride.
template <int TileSize, int N>
struct FireHalo {
    std::vector<ui> edges[4]; // up, down, left, right as seen by this rank

    const ui* tile_edge(int direction, int i) const {
        if (edges[direction].empty()) return nullptr;
        const size_t stride = direction < 2 ? size_t(2) * TileSize
                                           : size_t(128) * TileSize * TileSize;
        return edges[direction].data() + size_t(i) * stride;
    }
};

// Call once, before the first step. Both arrays contain N*N complete tiles.
// Only initial_data needs initialization; each active output tile is overwritten fully.
template <int TileSize, int N>
std::vector<TileLink> make_tile_links(ui* initial_data, ui* spare_data) {
    static_assert(TileSize > 0 && N > 0);
    constexpr size_t tile_words = size_t(128) * TileSize * TileSize;
    std::vector<TileLink> tiles;
    tiles.reserve(size_t(N) * N);
    for (size_t id = 0; id < size_t(N) * N; ++id)
        tiles.push_back({initial_data + id * tile_words, spare_data + id * tile_words});
    return tiles;
}

template <int __thread_cnt, int __tile_size, int __tiles_side_cnt>
void node_step(std::deque<int>& deque_A, std::deque<int>& deque_B, std::vector<TileLink>& tiles, bool* marked,
               const FireHalo<__tile_size, __tiles_side_cnt>* halo = nullptr) {
    // A队列构造完成，B队列已空
    // marked已经清零（由调用者负责）
    // A会被弹空，保留本轮列表，join后只调换这些tile的指针。
    const std::vector<int> processed(deque_A.begin(), deque_A.end());
    SpinLock A_lock, B_lock;
    std::vector<std::thread> workers;
    workers.reserve(__thread_cnt - 1);
    for (int tid = 1; tid < __thread_cnt; ++tid) {
        workers.emplace_back([&, tid] {
            while (true) {
                int id_me;
                {
                    std::lock_guard<SpinLock> lock(A_lock);
                    if (deque_A.empty())
                        return;
                    id_me = deque_A.front();
                    deque_A.pop_front();
                } // 抢锁成功，拿到了自己的id
                constexpr int N = __tiles_side_cnt;
                constexpr size_t row_words = size_t(__tile_size) * 2;
                constexpr size_t tile_words = size_t(128) * __tile_size * __tile_size;
                // marked 编号 -> 实际 tile 坐标
                const int r = id_me / (N + 2) - 1;
                const int c = id_me % (N + 2) - 1;
                const int id = r * N + c;
                auto tile_ptr = [&](int tile_id) -> const ui* {
                    return tiles[tile_id].current;
                };
                const ui* me = tile_ptr(id);
                ui* buf = tiles[id].next;
                const ui* up = halo ? halo->tile_edge(0, c) : nullptr;
                const ui* dn = halo ? halo->tile_edge(1, c) : nullptr;
                const ui* lf = halo ? halo->tile_edge(2, r) : nullptr;
                const ui* rh = halo ? halo->tile_edge(3, r) : nullptr;
                if (r > 0)
                    up = tile_ptr(id - N) + tile_words - row_words;
                if (r + 1 < N)
                    dn = tile_ptr(id + N);
                if (c > 0)
                    lf = tile_ptr(id - 1) + row_words - 2;
                if (c + 1 < N)
                    rh = tile_ptr(id + 1);
                propagate_tile_and_append<__tile_size, __tiles_side_cnt>(id_me, me, up, dn, lf, rh, buf, deque_B,
                                                                         marked, B_lock);
            }
        });
    }
    while (true) {
        int id_me;
        {
            std::lock_guard<SpinLock> lock(A_lock);
            if (deque_A.empty())
                goto empty;
            id_me = deque_A.front();
            deque_A.pop_front();
        } // 抢锁成功，拿到了自己的id
        constexpr int N = __tiles_side_cnt;
        constexpr size_t row_words = size_t(__tile_size) * 2;
        constexpr size_t tile_words = size_t(128) * __tile_size * __tile_size;
        // marked 编号 → 实际 tile 坐标
        const int r = id_me / (N + 2) - 1;
        const int c = id_me % (N + 2) - 1;
        const int id = r * N + c;
        auto tile_ptr = [&](int tile_id) -> const ui* {
            return tiles[tile_id].current;
        };
        const ui* me = tile_ptr(id);
        ui* buf = tiles[id].next;
        const ui* up = halo ? halo->tile_edge(0, c) : nullptr;
        const ui* dn = halo ? halo->tile_edge(1, c) : nullptr;
        const ui* lf = halo ? halo->tile_edge(2, r) : nullptr;
        const ui* rh = halo ? halo->tile_edge(3, r) : nullptr;
        if (r > 0)
            up = tile_ptr(id - N) + tile_words - row_words;
        if (r + 1 < N)
            dn = tile_ptr(id + N);
        if (c > 0)
            lf = tile_ptr(id - 1) + row_words - 2;
        if (c + 1 < N)
            rh = tile_ptr(id + 1);
        propagate_tile_and_append<__tile_size, __tiles_side_cnt>(id_me, me, up, dn, lf, rh, buf, deque_B, marked,
                                                                 B_lock);
    }
empty:
    for (auto& t : workers) {
        t.join();
    }
    // 此前所有线程只读current、只写next；全部结束后才发布新状态。
    for (int id_me : processed) {
        const int N = __tiles_side_cnt;
        const int id = (id_me / (N + 2) - 1) * N + id_me % (N + 2) - 1;
        std::swap(tiles[id].current, tiles[id].next);
    }
    deque_A.swap(deque_B);
    deque_B.clear();
}






// Byte offsets must remain physical file offsets: open the input with "rb".
static void seek_input(std::FILE* fp, int64_t offset) {
#ifdef _WIN32
    ::_fseeki64(fp, offset, SEEK_SET);
#else
    ::fseeko(fp, static_cast<off_t>(offset), SEEK_SET);
#endif
}

static int64_t tell_input(std::FILE* fp) {
#ifdef _WIN32
    return ::_ftelli64(fp);
#else
    return static_cast<int64_t>(::ftello(fp));
#endif
}

// Four ranks in row-major order: 0=top-left, 1=top-right,
// 2=bottom-left, 3=bottom-right. Each rank owns complete square tiles.
// Assumes four ranks, n divisible by twice the tile side, and valid input.
// Supports gencase.py's single-space rows with LF or CRLF (uniform per file).
// Every rank leaves fp at the first magic event, ready for fscanf.
template <int TileSize, int Threads>
void read_forest_region(std::FILE* fp, int rank, int& n, int& m, int& t, std::vector<ui>& data_A,
                        std::vector<ui>& data_B, std::vector<TileLink>& tile_table) {
    static_assert(TileSize > 0 && Threads > 0);
    char header[256];
    std::fgets(header, sizeof(header), fp);
    std::sscanf(header, "%d %d %d", &n, &m, &t);
    constexpr int tile_side = 64 * TileSize;
    constexpr size_t tile_words = size_t(128) * TileSize * TileSize;

    const int local_side = n; // [补] 单 rank 拥有整张图（原四 rank 配置是 n/2）
    const int tiles_per_side = local_side / tile_side;
    const size_t words = size_t(tiles_per_side) * tiles_per_side * tile_words;
    data_A.resize(words);
    data_B.resize(words);
    tile_table.resize(size_t(tiles_per_side) * tiles_per_side);
    for (size_t id = 0; id < tile_table.size(); ++id)
        tile_table[id] = {data_A.data() + id * tile_words, data_B.data() + id * tile_words};

    const int64_t base = tell_input(fp);
    const size_t line_capacity = size_t(2) * n + 4;
    std::vector<char> buffer_data(line_capacity * tile_side);
    std::fgets(buffer_data.data(), static_cast<int>(line_capacity), fp);
    const int64_t stride = tell_input(fp) - base;
    const int row_begin = (rank / 2) * local_side;
    const int col_begin = (rank % 2) * local_side;
    seek_input(fp, base + int64_t(row_begin) * stride);

    for (int tile_row = 0; tile_row < tiles_per_side; ++tile_row) {
        // One reader owns fp. Worker threads only access the completed batch.
        for (int r = 0; r < tile_side; ++r) {
            char* row = buffer_data.data() + size_t(r) * line_capacity;
            std::fgets(row, static_cast<int>(line_capacity), fp);
        }
        auto parse = [&](int tid) {
            for (int r = tid; r < tile_side; r += Threads) {
                const char* row = buffer_data.data() + size_t(r) * line_capacity;
                for (int tc = 0; tc < tiles_per_side; ++tc) {
                    const size_t id = size_t(tile_row) * tiles_per_side + tc;
                    ui* dst = tile_table[id].current + size_t(r) * TileSize * 2;
                    const char* src = row + size_t(2) * (col_begin + tc * tile_side);
                    for (int w = 0; w < TileSize; ++w) {
                        ui a = 0, b = 0;
                        for (int k = 0; k < 64; ++k) {
                            const unsigned v = unsigned(src[2 * (64 * w + k)] - '0');
                            a |= ui(v & 1u) << k;
                            b |= ui(v >> 1) << k;
                        }
                        dst[2 * w] = a;
                        dst[2 * w + 1] = b;
                    }
                }
            }
        };
        std::vector<std::thread> workers;
        workers.reserve(Threads - 1);
        for (int tid = 1; tid < Threads; ++tid)
            workers.emplace_back(parse, tid);
        parse(0);
        for (auto& worker : workers)
            worker.join();
    }
    seek_input(fp, base + int64_t(n) * stride);
}
// LocalSide defaults to the current 8192x8192 / four-rank configuration.
// Coordinates i,j are global row,column; tile ids are local and row-major.
template <int TileSize, int LocalSide = 4096>
void handle_magic_ignite(int rank, ui i, ui j, TileLink* table, bool* marked, std::deque<int>& deque_A) {
    constexpr int side = 64 * TileSize;
    constexpr int tiles_per_side = LocalSide / side;
    const ui row_begin = ui(rank / 2) * LocalSide;
    const ui col_begin = ui(rank % 2) * LocalSide;
    if (i < row_begin || i >= row_begin + LocalSide || j < col_begin || j >= col_begin + LocalSide)
        return;
    const ui r = i - row_begin, c = j - col_begin;
    const size_t id = size_t(r / side) * tiles_per_side + c / side;
    const size_t pos = (size_t(r % side) * TileSize + (c % side) / 64) * 2;
    ui* data = table[id].current;
    const ui bit = ui{1} << (c % 64);
    const ui ignite = data[pos] & ~data[pos + 1] & bit;
    data[pos] &= ~ignite;
    data[pos + 1] |= ignite;
    if (data[pos + 1] & ~data[pos] & bit) {
        const size_t marked_id = (size_t(r / side) + 1) * (tiles_per_side + 2) + c / side + 1;
        enqueue_next<tiles_per_side>(static_cast<int>(marked_id), marked, deque_A);
        const int stride = tiles_per_side + 2;
        if (r % side == 0)
            enqueue_next<tiles_per_side>(static_cast<int>(marked_id) - stride, marked, deque_A);
        if (r % side == side - 1)
            enqueue_next<tiles_per_side>(static_cast<int>(marked_id) + stride, marked, deque_A);
        if (c % side == 0)
            enqueue_next<tiles_per_side>(static_cast<int>(marked_id) - 1, marked, deque_A);
        if (c % side == side - 1)
            enqueue_next<tiles_per_side>(static_cast<int>(marked_id) + 1, marked, deque_A);
    }
}

// Inclusive rectangle in one tile's coordinates. Each caller exclusively owns
// this tile while applying the event; only current is modified, never next.
template <int TileSize>
void revive_tile(ui* data, int r0, int c0, int r1, int c1) {
    for (int word = c0 / 64; word <= c1 / 64; ++word) {
        const int lo = std::max(c0, word * 64) - word * 64;
        const int hi = std::min(c1, word * 64 + 63) - word * 64;
        const ui mask = (~ui{0} << lo) & (~ui{0} >> (63 - hi));
        for (int r = r0; r <= r1; ++r) {
            const size_t pos = (size_t(r) * TileSize + word) * 2;
            const ui ash = data[pos] & data[pos + 1] & mask;
            data[pos + 1] &= ~ash; // 11 -> 01; all other states unchanged
        }
    }
}

template <int TileSize, int Threads = 4, int LocalSide = 4096>
void handle_magic_revive(int rank, ui i1, ui j1, ui i2, ui j2, TileLink* table) {
    constexpr int side = 64 * TileSize;
    constexpr int tiles_per_side = LocalSide / side;
    const ui row_begin = ui(rank / 2) * LocalSide;
    const ui col_begin = ui(rank % 2) * LocalSide;
    // Intersect in global coordinates before subtracting unsigned origins.
    const ui top = std::max(i1, row_begin);
    const ui left = std::max(j1, col_begin);
    const ui bottom = std::min(i2, row_begin + LocalSide - 1);
    const ui right = std::min(j2, col_begin + LocalSide - 1);
    if (top > bottom || left > right)
        return;
    const int r0 = int(top - row_begin), c0 = int(left - col_begin);
    const int r1 = int(bottom - row_begin), c1 = int(right - col_begin);
    const int tr0 = r0 / side, tc0 = c0 / side;
    const int cols = c1 / side - tc0 + 1;
    const int jobs = (r1 / side - tr0 + 1) * cols;
    const int thread_count = std::min(Threads, jobs);
    auto apply = [&](int tid) {
        for (int job = tid; job < jobs; job += thread_count) {
            const int tr = tr0 + job / cols, tc = tc0 + job % cols;
            const size_t id = size_t(tr) * tiles_per_side + tc;
            revive_tile<TileSize>(table[id].current, std::max(r0 - tr * side, 0), std::max(c0 - tc * side, 0),
                                  std::min(r1 - tr * side, side - 1), std::min(c1 - tc * side, side - 1));
        }
    };
    std::vector<std::thread> workers;
    workers.reserve(thread_count - 1);
    for (int tid = 1; tid < thread_count; ++tid)
        workers.emplace_back(apply, tid);
    apply(0);
    for (auto& worker : workers)
        worker.join();
}

// direction: 0=up, 1=down, 2=left, 3=right.
// Selected tiles follow ascending mask bits. Each contributes TileSize words.
// Horizontal edge bits run left-to-right; vertical edge bits run top-to-bottom.
template <int TileSize, int LocalSide = 4096>
void pack_boundary_fire(int direction, ui mask, const TileLink* table, std::vector<ui>& packed) {
    constexpr int side = 64 * TileSize;
    constexpr int tiles_per_side = LocalSide / side;
    packed.resize(size_t(__builtin_popcountll(mask)) * TileSize);
    size_t out = 0;
    for (int i = 0; i < tiles_per_side; ++i) {
        if (!(mask & (ui{1} << i)))
            continue;
        const int tr = direction == 0 ? 0 : direction == 1 ? tiles_per_side - 1 : i;
        const int tc = direction == 2 ? 0 : direction == 3 ? tiles_per_side - 1 : i;
        const ui* data = table[tr * tiles_per_side + tc].current;
        if (direction < 2) {
            const int row = direction == 0 ? 0 : side - 1;
            const ui* edge = data + size_t(row) * TileSize * 2;
            for (int w = 0; w < TileSize; ++w)
                packed[out++] = edge[2 * w + 1] & ~edge[2 * w];
        } else {
            const int word = direction == 2 ? 0 : TileSize - 1;
            const int bit = direction == 2 ? 0 : 63;
            for (int w = 0; w < TileSize; ++w) {
                ui fire = 0;
                for (int k = 0; k < 64; ++k) {
                    const size_t pos = (size_t(w * 64 + k) * TileSize + word) * 2;
                    const ui burn = data[pos + 1] & ~data[pos];
                    fire |= ((burn >> bit) & ui{1}) << k;
                }
                packed[out++] = fire;
            }
        }
    }
}

// direction is the receiving rank's side, i.e. opposite the sender's side.
// Clear omitted segments too: a zero mask must remove last step's halo fire.
template <int TileSize, int LocalSide = 4096>
void unpack_boundary_fire(int direction, ui mask, const std::vector<ui>& packed,
                          FireHalo<TileSize, LocalSide / (64 * TileSize)>& halo,
                          bool* marked, std::deque<int>& deque_A) {
    constexpr int side = 64 * TileSize;
    constexpr int N = LocalSide / side;
    constexpr size_t row_words = size_t(2) * TileSize;
    constexpr size_t tile_words = size_t(128) * TileSize * TileSize;
    const size_t stride = direction < 2 ? row_words : tile_words;
    auto& edge = halo.edges[direction];
    edge.assign(size_t(N) * stride, ui{0});
    size_t in = 0;
    for (int i = 0; i < N; ++i) {
        if (!(mask & (ui{1} << i))) continue;
        ui* dst = edge.data() + size_t(i) * stride;
        ui any_fire = 0;
        for (int w = 0; w < TileSize; ++w) {
            const ui fire = packed[in++];
            any_fire |= fire;
            if (direction < 2) {
                dst[2 * w + 1] = fire;
            } else {
                // A left neighbor touches bit 63; a right neighbor touches bit 0.
                const int bit = direction == 2 ? 63 : 0;
                for (int k = 0; k < 64; ++k)
                    dst[size_t(w * 64 + k) * row_words + 1] = ((fire >> k) & ui{1}) << bit;
            }
        }
        if (any_fire) {
            const int r = direction == 0 ? 1 : direction == 1 ? N : i + 1;
            const int c = direction == 2 ? 1 : direction == 3 ? N : i + 1;
            enqueue_next<N>(r * (N + 2) + c, marked, deque_A);
        }
    }
}

#ifndef FIRE_NO_MAIN
// =====================================================================
// [优化版] 与原 node_step 完全相同的任务逻辑，只把线程生命周期改成"常驻"：
//   原版：每步 spawn 15 个线程 -> 实测 1.36 s（probe_spawn 测出来的）
//   本版：线程只建一次，用自旋栅栏同步
// 算法（tile 队列、脏表、kernel）一行未改。
// =====================================================================

#include <atomic>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define POOL_PAUSE() _mm_pause()
#else
#define POOL_PAUSE() ((void)0)
#endif

class SpinBarrier {
public:
    explicit SpinBarrier(int n) : n_(n) {}
    void wait() {
        const unsigned g = gen_.load(std::memory_order_acquire);
        if (cnt_.fetch_add(1, std::memory_order_acq_rel) + 1 == n_) {
            cnt_.store(0, std::memory_order_relaxed);
            gen_.store(g + 1, std::memory_order_release);
        } else {
            while (gen_.load(std::memory_order_acquire) == g) POOL_PAUSE();
        }
    }

private:
    int n_;
    std::atomic<int> cnt_{0};
    std::atomic<unsigned> gen_{0};
};

template <int TileSize>
static void write_forest_output(const char* path, int n, const std::vector<TileLink>& tiles) {
    constexpr int side = 64 * TileSize;
    const int tps = n / side;
    std::FILE* fo = std::fopen(path, "wb");
    if (!fo) {
        std::fprintf(stderr, "open out failed: %s\n", path);
        std::exit(1);
    }
    std::vector<char> buf(1u << 22);
    char* q = buf.data();
    char* qend = q + buf.size();
    auto flush = [&]() {
        std::fwrite(buf.data(), 1, size_t(q - buf.data()), fo);
        q = buf.data();
    };
    for (int r = 0; r < n; ++r) {
        const int tr = r / side, rr = r % side;
        for (int tc = 0; tc < tps; ++tc) {
            const ui* data = tiles[size_t(tr) * tps + tc].current + size_t(rr) * TileSize * 2;
            for (int w = 0; w < TileSize; ++w) {
                const ui a = data[2 * w], b = data[2 * w + 1];
                for (int k = 0; k < 64; ++k) {
                    if (q + 2 > qend) flush();
                    const int v = int((a >> k) & ui{1}) | (int((b >> k) & ui{1}) << 1);
                    *q++ = char('0' + v);
                    *q++ = ' ';
                }
            }
        }
        q[-1] = '\n';
    }
    flush();
    std::fclose(fo);
}

// 从队列里不断取 tile 处理，直到队列空。与原 node_step 的线程体一致。
template <int __tile_size, int __tiles_side_cnt>
void process_tasks(std::deque<int>& A, std::deque<int>& B, std::vector<TileLink>& tiles, bool* marked,
                   const FireHalo<__tile_size, __tiles_side_cnt>* halo, SpinLock& A_lock, SpinLock& B_lock) {
    constexpr int N = __tiles_side_cnt;
    constexpr size_t row_words = size_t(__tile_size) * 2;
    constexpr size_t tile_words = size_t(128) * __tile_size * __tile_size;
    for (;;) {
        int id_me;
        {
            std::lock_guard<SpinLock> lock(A_lock);
            if (A.empty()) return;
            id_me = A.front();
            A.pop_front();
        }
        const int r = id_me / (N + 2) - 1;
        const int c = id_me % (N + 2) - 1;
        const int id = r * N + c;
        auto tile_ptr = [&](int tile_id) -> const ui* { return tiles[tile_id].current; };
        const ui* me = tile_ptr(id);
        ui* buf = tiles[id].next;
        const ui* up = halo ? halo->tile_edge(0, c) : nullptr;
        const ui* dn = halo ? halo->tile_edge(1, c) : nullptr;
        const ui* lf = halo ? halo->tile_edge(2, r) : nullptr;
        const ui* rh = halo ? halo->tile_edge(3, r) : nullptr;
        if (r > 0) up = tile_ptr(id - N) + tile_words - row_words;
        if (r + 1 < N) dn = tile_ptr(id + N);
        if (c > 0) lf = tile_ptr(id - 1) + row_words - 2;
        if (c + 1 < N) rh = tile_ptr(id + 1);
        propagate_tile_and_append<__tile_size, __tiles_side_cnt>(id_me, me, up, dn, lf, rh, buf, B, marked, B_lock);
    }
}

int main(int argc, char** argv) {
    constexpr int tile_size = 16;
    constexpr int thread_cnt = 16;
    constexpr int tiles_per_side = 16384 / (64 * tile_size);
    constexpr int marked_stride = tiles_per_side + 2;
    static bool marked[marked_stride * marked_stride] = {};
    std::deque<int> deque_A;
    std::deque<int> deque_B;
    const auto t_begin = std::chrono::steady_clock::now();
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::FILE* fp = std::fopen(argc > 1 ? argv[1] : "forest.in", "rb");
    int n, m, t;
    std::vector<ui> data_A, data_B;
    std::vector<TileLink> tile_table;
    read_forest_region<tile_size, thread_cnt>(fp, rank, n, m, t, data_A, data_B, tile_table);
    const auto t_read = std::chrono::steady_clock::now();
    for (int r = 1; r <= tiles_per_side; ++r)
        for (int c = 1; c <= tiles_per_side; ++c) enqueue_next<tiles_per_side>(r * marked_stride + c, marked, deque_A);
    for (int i = 1; i <= tiles_per_side; ++i) {
        marked[i] = true;
        marked[(tiles_per_side + 1) * marked_stride + i] = true;
        marked[i * marked_stride] = true;
        marked[i * marked_stride + tiles_per_side + 1] = true;
    }
    std::unordered_map<int, std::vector<int>> events;
    for (int step = 0; step < m; ++step) {
        char line[100];
        int st, type, a, b, c, d;
        std::fgets(line, sizeof(line), fp);
        std::sscanf(line, "%d %d %d %d %d %d", &st, &type, &a, &b, &c, &d);
        if (type == 1)
            events[st] = {type, a, b};
        else
            events[st] = {type, a, b, c, d};
    }
    std::vector<ui> send_buffers[2], recv_buffers[2];
    FireHalo<tile_size, tiles_per_side> halo;
    (void)send_buffers;
    (void)recv_buffers;

    SpinLock A_lock, B_lock;
    SpinBarrier bar(thread_cnt);
    std::vector<int> processed;
    processed.reserve(size_t(deque_A.size()));

    auto worker = [&](int tid) {
        for (int step = 1; step < t + 1; ++step) {
            if (tid == 0) {
                if (events.find(step) != events.end()) {
                    if (events[step][0] == 1) {
                        handle_magic_ignite<tile_size, 16384>(rank, events[step][1], events[step][2],
                                                              tile_table.data(), marked, deque_A);
                    } else {
                        handle_magic_revive<tile_size, thread_cnt, 16384>(
                            rank, events[step][1], events[step][2], events[step][3], events[step][4],
                            tile_table.data());
                    }
                }
                std::fill(marked, marked + marked_stride * marked_stride, false);
                processed.assign(deque_A.begin(), deque_A.end()); // 本步要处理的 tile 列表
            }
            bar.wait();                       // 事件 + 脏表清零完成
            process_tasks<tile_size, tiles_per_side>(deque_A, deque_B, tile_table, marked, &halo, A_lock, B_lock);
            bar.wait();                       // 所有线程写完 next
            if (tid == 0) {                   // 发布新状态：交换指针 + 换队列
                for (int id_me : processed) {
                    const int N = tiles_per_side;
                    const int id = (id_me / (N + 2) - 1) * N + id_me % (N + 2) - 1;
                    std::swap(tile_table[id].current, tile_table[id].next);
                }
                deque_A.swap(deque_B);
                deque_B.clear();
            }
            bar.wait();                       // 确保所有人看到新的 current
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(thread_cnt - 1);
    for (int k = 1; k < thread_cnt; ++k) pool.emplace_back(worker, k);
    worker(0);
    for (auto& th : pool) th.join();

    const auto t_sim = std::chrono::steady_clock::now();
    std::fclose(fp);
    write_forest_output<tile_size>(argc > 2 ? argv[2] : "forest.out", n, tile_table);
    const auto t_end = std::chrono::steady_clock::now();
    auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    std::fprintf(stderr, "read=%.1f sim=%.1f out=%.1f total=%.1f\n", ms(t_begin, t_read), ms(t_read, t_sim),
                 ms(t_sim, t_end), ms(t_begin, t_end));
    MPI_Finalize();
    return 0;
}
#endif
