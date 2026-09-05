# Helper Fast Compile

### Prebuild:
```sh
g++ -std=c++17 -O2 -c exprtk_wrapper.cpp -o exprtk_wrapper.o
```
### Build:
```sh
g++ plot_function.cpp exprtk_wrapper.o -std=c++17 -O2 -lboost_iostreams -lboost_system -lboost_filesystem -o plot_function
```

### User guide:

| Toán học | Cú pháp đúng trong exprtk |   |          |
| -------- | ------------------------- | - | -------- |
| sin(x)   | `sin(x)`                  |   |          |
| cos(x)   | `cos(x)`                  |   |          |
| e^x      | `exp(x)`                  |   |          |
| ln(x)    | `log(x)`                  |   |          |
| log10(x) | `log10(x)`                |   |          |
| \|x\|    | `abs(x)`                  |   |          |
| sqrt(x)  | `sqrt(x)`                 |   |          |
| pow(x,2) | `pow(x,2)`                |   |          |
| π        | `pi`                      |   |          |
| e        | `e`                       |   |          |
-------------------------------------------------------


```sh
if(x>0, log(x), 0) -> (dùng hàm if(cond, then, else) thay thế)

if(x>0, log(x), 0)
```