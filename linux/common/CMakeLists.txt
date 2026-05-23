
add_library(ipc_pipe STATIC 
    ipc/pipe/*.cpp
)

target_include_directories(ipc_pipe PUBLIC ipc/pipe)
