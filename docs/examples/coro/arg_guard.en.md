# ArgGuard Example {#arg_guard_example}
Assume there is an existing coroutine function whose parameter is a reference, and this coroutine function may be `detached` or passed to other places as a `Task`, then the lifetime of this reference becomes very important.
```cpp
Task<int> foo(std::string_view name, int age) {
  co_await do_something();
  co_return 42;s
}
```
Obviously, it is possible to `suspend` here. If this `Task` is `detached`, since the parameter `name` of the `foo` function only stores a reference, then after the lifetime of `name` ends, the `name` inside the `foo` function will become a dangling reference, which is very dangerous. To avoid this, we can use `ArgGuard` to protect the lifetime of the parameter.
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
In safe_bar , ArgGuard ensures that the lifetime of name is at least as long as the lifetime of task , thus avoiding the dangling reference problem.


