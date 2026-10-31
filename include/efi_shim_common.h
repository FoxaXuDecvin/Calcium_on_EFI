// ============================================================================
//  efi_shim_common.h —— 影子标准库的公共底座
//
//  设计要点：basic_stream 不用虚函数，而用「函数指针 sink + ctx」。
//  原因是 EFI 下没有 CRT，全局对象的动态初始化依赖 .CRT$XCU 遍历；
//  用函数指针后 cout 可在编译期常量初始化，彻底绕开这个依赖。
// ============================================================================
#pragma once

#include <string>
#include <cstddef>

namespace efi {

// ---------------------------------------------------------------- 平台底板
void out(const char *s, size_t n);        // 写控制台（UTF-8）
bool read_line(std::string &s);           // 读一行（回车结束）

bool file_read_all(const char *path, std::string &outdata);
bool file_write_all(const char *path, const std::string &data);
// 追加写（文件不存在时自动创建）
bool file_append(const char *path, const std::string &data);
bool file_exists(const char *path);

// 文件被删除/改名时清掉读缓存（崩溃与脏读防护）
void invalidate_file_cache();

inline void console_sink(void *ctx, const char *s, size_t n)
{
    (void)ctx;
    out(s, n);
}

// ---------------------------------------------------------------- 流基类
class basic_stream {
public:
    typedef void (*sink_fn)(void *ctx, const char *s, size_t n);

    constexpr basic_stream(void *ctx, sink_fn f) : ctx_(ctx), fn_(f) {}

    void put(char c) { fn_(ctx_, &c, 1); }
    void write(const char *s, size_t n) { if (n) fn_(ctx_, s, n); }
    void flush() {}

protected:
    void *ctx_;
    sink_fn fn_;
};

} // namespace efi
