# NetworkDigitalTwin

## If you want to use IWYU check

```shell
rm -rf build
cmake -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -B build
ninja -C build
ln -sf build/compile_commands.json .
iwyu_tool -p build         # or: iwyu_tool -p build -v --output-format=clang
```