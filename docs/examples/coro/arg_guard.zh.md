# ArgGuard Example {#arg_guard_example}
假设这里有一个已有的协程函数，它的参数是一个引用，而这个协程函数可能会被`detach`或者作为一个`Task`传递给其他地方，那么这个引用的生命周期就会变得非常重要。
```cpp
Task<int> foo(std::string_view name, int age) {
  co_await do_something();
  co_return 42;s
}
```
显然，这里是有可能`suspend`的，如果此`Task`被`detach`，由于`foo`函数的参数`name`只保存了一个引用，那么在`name`的生命周期结束后，`foo`函数内部的`name`将会变成悬空引用，这是非常危险的。为了避免这种情况，我们可以使用`ArgGuard`来保护参数的生命周期。
```cpp
void bar() {
  std::string name = "Alice";
  auto task = foo(name, 42);
  task.detach();
}
void safe_bar() {
  std::string name = "Alice";
  auto task = ArgGuard(foo，std::move(name), 42);
  task.detach();
}
```
在`safe_bar`中，`ArgGuard`会保证`name`的生命周期至少和`task`的生命周期一样长，这样就可以避免悬空引用的问题。



