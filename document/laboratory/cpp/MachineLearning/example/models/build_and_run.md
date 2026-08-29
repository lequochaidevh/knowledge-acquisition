### Build and Run model

```sh
# [build]
cd - && g++ train.cpp \
-o build/builder -std=gnu++17 \
-O3 -fopenmp \
&& cd - \
&& ./builder \
&& ./runner
```
