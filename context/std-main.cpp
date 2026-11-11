#include <iostream>
#include <fstream>
#include <mpi.h>
#include <vector>

const int TREE = 1;
const int FIRE = 2;
const int ASH = 3;
const int EMPTY = 0;

struct Event {
    int ts;         // 事件触发的时间步
    int type;       // 事件类型：1（天降惊雷），2（妙手回春）
    int x1, y1;     // 事件的坐标或区域范围
    int x2, y2;     // 仅用于“妙手回春”事件
};

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    const char *input_file = argv[1];
    const char *output_file = argv[2];

    int n, m, max_ts;
    int rank, size;

    double t1, t2, t3, t4, t5, t6, t7, t8;

    MPI_Init(&argc, &argv);

    t1 = MPI_Wtime();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    t2 = MPI_Wtime();
    
    int *grid;
    int *sendcounts = new int[size];
    int *displs = new int[size];
    std::vector<Event> events;

    if (rank == 0) {
        std::ifstream fi(input_file, std::ios::in);
        if (!fi) {
            std::cerr << "Error: Cannot open input file " << input_file << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        fi >> n >> m >> max_ts;

        // 读取地图
        grid = new int[n * n];
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                fi >> grid[i * n + j];
            }
        }

        // 读取事件
        for (int i = 0; i < m; i++) {
            Event event;
            fi >> event.ts >> event.type >> event.x1 >> event.y1;
            if (event.type == 2) {
                fi >> event.x2 >> event.y2;
            }
            events.push_back(event);
        }
        fi.close();

        // 计算每个进程的数据量和偏移量，用于 MPI_Scatter 地图
        // 数据的划分以行为单位进行，同一行的数据一定被划分给同一个进程
        for (int i = 0; i < size; ++i) {
            int rows = (i == size - 1) ? (n / size + n % size) : (n / size);
            sendcounts[i] = rows * n;
            displs[i] = n / size * i * n;
        }
    }

    t3 = MPI_Wtime();

    // 广播森林尺寸、事件数量和时间步数
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_ts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // 广播每进程数据量和偏移量
    MPI_Bcast(sendcounts, size, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, size, MPI_INT, 0, MPI_COMM_WORLD);

    // 广播时间数据
    if (rank != 0) {
        events.resize(m);
    }
    for (int i = 0; i < m; ++i) {
        MPI_Bcast(&events[i].ts, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&events[i].type, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&events[i].x1, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&events[i].y1, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (events[i].type == 2) {
            MPI_Bcast(&events[i].x2, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&events[i].y2, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }    
    }

    t4 = MPI_Wtime();

    int local_n = sendcounts[rank];
    int *local_grid[2];
    local_grid[0] = new int[local_n];
    local_grid[1] = new int[local_n];

    // 分发地图数据到各个进程
    MPI_Scatterv(grid, sendcounts, displs, MPI_INT, local_grid[0], local_n, MPI_INT, 0, MPI_COMM_WORLD);

    t5 = MPI_Wtime();

    int cur_event_id = 0;

    // 序号为 rank 的进程负责的行号范围为 [row_min, row_max]
    int row_min = n / size * rank;
    int row_max = (rank == size - 1) ? (n - 1): (n / size * (rank + 1) - 1); 

    int cur = 0;
    // 开始模拟
    for (int ts = 1; ts <= max_ts; ++ts) {
        // 处理事件
        if (ts == events[cur_event_id].ts) {
            Event event = events[cur_event_id];
            if (event.type == 1) {  // 天降惊雷
                int x = event.x1;
                int y = event.y1;
                if (x >= row_min && x <= row_max) {
                    int local_x = x - row_min;
                    if (local_grid[cur][local_x * n + y] == TREE) {
                        local_grid[cur][local_x * n + y] = FIRE;
                    }
                }
            } else if (event.type == 2) {  // 妙手回春
                int sx = event.x1, ex = event.x2;
                int sy = event.y1, ey = event.y2;
                int x_range_st = std::max(sx, row_min);
                int x_range_ed = std::min(ex, row_max);
                for (int x = x_range_st; x <= x_range_ed; ++x) {
                    int local_x = x - row_min;
                    for (int y = sy; y <= ey; ++y) {
                        if (local_grid[cur][local_x * n + y] == ASH) {
                            local_grid[cur][local_x * n + y] = TREE;
                        }
                    }
                }
            }
            cur_event_id++;
        }

        // 状态传播
        MPI_Request send_request_1, send_request_2, recv_request_1, recv_request_2;
        int *prev_line = new int[n];
        int *next_line = new int[n];

        // 进程之间交换边界信息
        if (rank == 0) {
            for (int i = 0; i < n; i++) {
                prev_line[i] = EMPTY;
            }
        } else {
            MPI_Isend(local_grid[cur], n, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &send_request_1);
            MPI_Irecv(prev_line, n, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &recv_request_1);
        }
        if (rank == size - 1) {
            for (int i = 0; i < n; i++) {
                next_line[i] = EMPTY;
            }
        } else {
            MPI_Isend(local_grid[cur] + (local_n - n), n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &send_request_2);
            MPI_Irecv(next_line, n, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &recv_request_2);
        }

        int next = 1 - cur;

        if (rank != 0) {
            MPI_Wait(&send_request_1, MPI_STATUS_IGNORE);
            MPI_Wait(&recv_request_1, MPI_STATUS_IGNORE);
        }
        if (rank != size - 1) {
            MPI_Wait(&send_request_2, MPI_STATUS_IGNORE);
            MPI_Wait(&recv_request_2, MPI_STATUS_IGNORE);
        }

        // 状态转移
        int rows = local_n / n;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < n; c++) {
                int i = r * n + c;
                if (local_grid[cur][i] == TREE) {
                    local_grid[next][i] = TREE;
                    if (r == 0 && prev_line[c] == FIRE) local_grid[next][i] = FIRE;
                    else if (r == rows - 1 && next_line[c] == FIRE) local_grid[next][i] = FIRE;
                    else if (r > 0 && local_grid[cur][i - n] == FIRE) local_grid[next][i] = FIRE;
                    else if (r < rows - 1 && local_grid[cur][i + n] == FIRE) local_grid[next][i] = FIRE;
                    else if (c > 0 && local_grid[cur][i - 1] == FIRE) local_grid[next][i] = FIRE;
                    else if (c < n - 1 && local_grid[cur][i + 1] == FIRE) local_grid[next][i] = FIRE;
                }
                else if (local_grid[cur][i] == FIRE) {
                    local_grid[next][i] = ASH;
                }
                else {
                    local_grid[next][i] = local_grid[cur][i];
                }
            }
        }

        cur = next;

        delete[] prev_line;
        delete[] next_line;
    }

    t6 = MPI_Wtime();

    // 收集结果
    MPI_Gatherv(local_grid[cur], local_n, MPI_INT, grid, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    
    t7 = MPI_Wtime();
    
    if (rank == 0) {
        // 输出结果到输出文件
        std::ofstream fo(output_file, std::ios::out);
        if (!fo) {
            std::cerr << "Error: Cannot open output file " << output_file << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                fo << grid[i * n + j] << " ";
            }
            fo << std::endl;
        }
        fo.close();

        t8 = MPI_Wtime();
        std::cout << "Time 1-2: " << t2 - t1 << std::endl;
        std::cout << "Time 2-3: " << t3 - t2 << std::endl;
        std::cout << "Time 3-4: " << t4 - t3 << std::endl;
        std::cout << "Time 4-5: " << t5 - t4 << std::endl;
        std::cout << "Time 5-6: " << t6 - t5 << std::endl;
        std::cout << "Time 6-7: " << t7 - t6 << std::endl;
        std::cout << "Time 7-8: " << t8 - t7 << std::endl;
    }

    MPI_Finalize();
    return 0;
}