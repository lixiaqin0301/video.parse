# 项目信息
project (Video.Parse)
# 支持 gdb
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -Werror -g3 -ggdb3")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall")
# 指定生成目标
add_executable(flvparse flv/flvparser.c flv/flvparsescriptdata.c flv/flvparseaudiodata.c flv/flvparsevideodata.c flv/main.c)
add_executable(mp4parse mp4/atom-desc.c mp4/mp4tree.c)
