### note synctax

```cpp
// The base class CRTP can get data from child with template by:
template <typename Derived>
class IPCSenderBase {
    void func() {
        Derived* derived = static_cast<Derived*>(this);
        auto buffer = derived->attribute_from_child;
    }
}
```