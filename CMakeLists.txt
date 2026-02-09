cmake_minimum_required(VERSION 3.20)
project(scheduler_sim C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(scheduler_sim
    src/main.c
    src/msg_queue.c
    lib/ringbuf.c
    lib/i2c_mock.c
    lib/i2c_util.c
)


target_include_directories(scheduler_sim PRIVATE include)
target_link_libraries(scheduler_sim PRIVATE Threads::Threads)
