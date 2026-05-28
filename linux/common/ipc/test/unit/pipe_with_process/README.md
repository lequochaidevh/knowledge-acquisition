### Test pipe class with 3 process:

```sh
mkdir build
cd build
cmake ..
make
```

**Terminal 1**
```sh
./server/server_pipe_test
```

**Terminal 2**
```sh
./client/client_pipe_test 01
```

**Terminal 3**
```sh
./client/client_pipe_test 02
```
