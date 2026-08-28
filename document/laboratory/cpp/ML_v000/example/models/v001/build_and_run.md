### Build and Run model

```sh
┌[devh@ .../example/models/v001/build]                                      (main) 08/28/26-22:26:36
│
╰──> cd - && g++ train.cpp -o build/builder -std=gnu++17  -O3 -fopenmp && cd - && ./builder && ./runner
```