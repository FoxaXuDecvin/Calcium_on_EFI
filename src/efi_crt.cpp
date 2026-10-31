// ============================================================================
//  efi_crt.cpp —— 迷你 C/C++ 运行时（freestanding，替代 libcmt/msvcprt/ucrt）
//  提供：内存、字符串、格式化输出、时间、数学、全局构造器遍历
// ============================================================================
#include "efi_core.h"

// 关键：不要 include <string.h>/<stdio.h>/<stdlib.h>/<math.h>/<time.h>。
// MSVC 的 UCRT 头会把这些函数声明成 intrinsic，导致这里再定义就报
// C2169（内部函数，不能定义）/ C2084（已有主体）。所以只留两个纯宏头，
// 其余声明全部手写（extern "C" 名字不参与名字改编，跨 TU 能对上）。
#include <stddef.h>
#include <stdarg.h>
#include <new>

// ---- 手写 CRT 类型（与 MSVC ABI 一致）
typedef long long __time64_t;
typedef long long time_t;
struct tm {
    int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
};

// ---- 前置声明（后面 realloc/calloc 会先用到；MSVC 要求先声明后使用）
namespace efi { void invalidate_file_cache(); }
extern "C" void * __cdecl memcpy(void *dst, const void *src, size_t n);
extern "C" void * __cdecl memmove(void *dst, const void *src, size_t n);
extern "C" void * __cdecl memset(void *dst, int v, size_t n);
extern "C" int    __cdecl memcmp(const void *a, const void *b, size_t n);
extern "C" size_t __cdecl strlen(const char *s);
extern "C" char * __cdecl strcpy(char *d, const char *s);

// ---------------------------------------------------------------- 内部工具
__declspec(noreturn) static void panic_halt(const char *msg)
{
    eficon::WriteAscii("\n[Calcium-EFI] FATAL: ");
    eficon::WriteAscii(msg ? msg : "unknown");
    eficon::WriteAscii("\nSystem halted.\n");
    for (;;) { efi_stall_us(1000000); }
}

// ================================================================ 内存
// 头部 16 字节：前 8 字节存 class 标记（0=直连固件），后 8 字节存用户大小。
//
// 为什么要 size-class 池：Calcium 的文本读取（getdocmax / LineReader）会产生
// 数以万计的 std::string 小块分配，每次都调固件 AllocatePool 极慢
// （实测 ls 一个 64 条目的目录要等几十秒）。这里给 <=2KB 的块做分级空闲链表，
// 只会向固件申请大块 slab，释放后回流链表复用。
// 程序退出时固件会回收全部内存，因此池不归还也不泄漏。
#define EFI_MEM_CLASSES 8
static const size_t kClassSize[EFI_MEM_CLASSES] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
static void *s_freeList[EFI_MEM_CLASSES];

static unsigned int class_of(size_t total)
{
    for (unsigned int i = 0; i < EFI_MEM_CLASSES; i++)
        if (total <= kClassSize[i]) return i;
    return EFI_MEM_CLASSES;                   // 太大 → 直连固件
}

extern "C" void * __cdecl malloc(size_t n)
{
    if (n == 0) n = 1;
    size_t total = n + 16;
    unsigned int c = class_of(total);

    unsigned char *raw = 0;
    if (c < EFI_MEM_CLASSES) {
        size_t bs = kClassSize[c];
        if (s_freeList[c]) {
            raw = (unsigned char *)s_freeList[c];
            s_freeList[c] = *(void **)raw;
        } else {
            // 一次向固件要 32 块，其余挂到空闲链表
            const unsigned int kSlab = 32;
            unsigned char *slab = (unsigned char *)efi_alloc(bs * kSlab);
            if (!slab) return 0;
            for (unsigned int i = 1; i < kSlab; i++) {
                unsigned char *blk = slab + (size_t)i * bs;
                *(void **)blk = s_freeList[c];
                s_freeList[c] = blk;
            }
            raw = slab;
        }
        *(void **)(raw) = (void *)(size_t)(c + 1);   // 1..8 表示来自池
        *(size_t *)(raw + 8) = n;
        return raw + 16;
    }

    raw = (unsigned char *)efi_alloc(total);
    if (!raw) return 0;
    *(void **)(raw) = (void *)0;                     // 0 表示直连固件
    *(size_t *)(raw + 8) = n;
    return raw + 16;
}

extern "C" void __cdecl free(void *p)
{
    if (!p) return;
    unsigned char *raw = (unsigned char *)p - 16;
    size_t tag = (size_t)*(void **)raw;
    if (tag == 0) { efi_free(raw); return; }
    unsigned int c = (unsigned int)(tag - 1);
    if (c >= EFI_MEM_CLASSES) { efi_free(raw); return; }
    *(void **)raw = s_freeList[c];
    s_freeList[c] = raw;
}

extern "C" void * __cdecl realloc(void *p, size_t n)
{
    if (!p) return malloc(n);
    if (n == 0) { free(p); return 0; }
    unsigned char *raw = (unsigned char *)p - 16;
    size_t old = *(size_t *)(raw + 8);
    void *np = malloc(n);
    if (!np) return 0;
    memcpy(np, p, old < n ? old : n);
    free(p);
    return np;
}

extern "C" void * __cdecl calloc(size_t num, size_t size)
{
    size_t n = num * size;
    void *p = malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

// ---- C++ operator new / delete ----
void *operator new(size_t n)                 { void *p = malloc(n); if (!p) panic_halt("out of memory"); return p; }
void *operator new[](size_t n)               { void *p = malloc(n); if (!p) panic_halt("out of memory"); return p; }
void  operator delete(void *p) noexcept      { free(p); }
void  operator delete[](void *p) noexcept    { free(p); }
void  operator delete(void *p, size_t) noexcept   { free(p); }
void  operator delete[](void *p, size_t) noexcept { free(p); }
void *operator new(size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void *operator new[](size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void  operator delete(void *p, const std::nothrow_t &) noexcept { free(p); }
void  operator delete[](void *p, const std::nothrow_t &) noexcept { free(p); }

// 对齐版本（STL 可能用）
void *operator new(size_t n, std::align_val_t a)
{
    size_t align = (size_t)a;
    unsigned char *p = (unsigned char *)malloc(n + align);
    if (!p) panic_halt("out of memory");
    return p;   // 简化：malloc 已 16 字节对齐，够 std::string 用
}
void operator delete(void *p, std::align_val_t) noexcept { free(p); }
void operator delete(void *p, size_t, std::align_val_t) noexcept { free(p); }

// ================================================================ 字符串
extern "C" void * __cdecl memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n >= 8) { *(unsigned long long *)d = *(const unsigned long long *)s; d += 8; s += 8; n -= 8; }
    while (n--) *d++ = *s++;
    return dst;
}

extern "C" void * __cdecl memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dst;
    if (d < s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

extern "C" void * __cdecl memset(void *dst, int v, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned long long pat = (unsigned char)v;
    pat |= pat << 8; pat |= pat << 16; pat |= pat << 32;
    while (n >= 8) { *(unsigned long long *)d = pat; d += 8; n -= 8; }
    while (n--) *d++ = (unsigned char)v;
    return dst;
}

extern "C" int __cdecl memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    while (n--) { if (*x != *y) return (int)*x - (int)*y; x++; y++; }
    return 0;
}

extern "C" const void * __cdecl memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = (const unsigned char *)s;
    while (n--) { if (*p == (unsigned char)c) return (const void *)p; p++; }
    return 0;
}

extern "C" size_t __cdecl strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

extern "C" size_t __cdecl strnlen(const char *s, size_t maxlen)
{
    size_t i = 0;
    while (i < maxlen && s[i]) i++;
    return i;
}

extern "C" char * __cdecl strcpy(char *d, const char *s)  { char *r = d; while ((*d++ = *s++)) {} return r; }
extern "C" char * __cdecl strncpy(char *d, const char *s, size_t n)
{
    char *r = d;
    while (n && *s) { *d++ = *s++; n--; }
    while (n--) *d++ = 0;
    return r;
}
extern "C" char * __cdecl strcat(char *d, const char *s) { char *r = d; while (*d) d++; while ((*d++ = *s++)) {} return r; }
extern "C" int __cdecl strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}
extern "C" int __cdecl strncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && *a == *b) { a++; b++; n--; }
    if (!n) return 0;
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}
extern "C" const char * __cdecl strchr(const char *s, int c)
{
    while (*s) { if (*s == (char)c) return s; s++; }
    return (c == 0) ? s : 0;
}
extern "C" const char * __cdecl strrchr(const char *s, int c)
{
    const char *last = 0;
    do { if (*s == (char)c) last = s; } while (*s++);
    return last;
}
extern "C" const char * __cdecl strstr(const char *h, const char *n)
{
    if (!*n) return h;
    for (; *h; h++) {
        const char *a = h, *b = n;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*b) return h;
    }
    return 0;
}

extern "C" int __cdecl atoi(const char *s)
{
    if (!s) return 0;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; } else if (*s == '+') s++;
    long long v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return (int)(sign * v);
}

extern "C" long __cdecl atol(const char *s) { return atoi(s); }

// ================================================================ 格式化输出
namespace {

struct OutBuf {
    char *buf; size_t cap; size_t len;
    void put(char c) { if (buf && len + 1 < cap) buf[len] = c; len++; }
};

int fmt_int(unsigned long long v, int base, bool upper, char *out)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32]; int n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v) { tmp[n++] = digits[v % base]; v /= base; }
    for (int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    return n;
}

void fmt_flt(double v, int prec, char *out, int &outLen)
{
    // 极简 %f：整数部分 + 小数部分（够 Calcium 用）
    if (v != v) { strcpy(out, "nan"); outLen = 3; return; }
    bool neg = false;
    if (v < 0) { neg = true; v = -v; }
    if (prec < 0) prec = 6;
    unsigned long long ip = (unsigned long long)v;
    double frac = v - (double)ip;
    char ib[32]; int il = fmt_int(ip, 10, false, ib);
    int p = 0;
    if (neg) out[p++] = '-';
    for (int i = 0; i < il; i++) out[p++] = ib[i];
    if (prec > 0) {
        out[p++] = '.';
        for (int i = 0; i < prec; i++) { frac *= 10; int d = (int)frac; frac -= d; out[p++] = (char)('0' + d); }
    }
    outLen = p;
}

int format_core(char *buf, size_t cap, const char *fmt, va_list ap)
{
    OutBuf o; o.buf = buf; o.cap = cap; o.len = 0;
    const char *p = fmt;
    while (*p) {
        if (*p != '%') { o.put(*p++); continue; }
        // 解析格式
        const char *start = p;
        p++;
        if (*p == '%') { o.put('%'); p++; continue; }

        bool left = false, zero = false, plus = false, space = false;
        for (;;) {
            if (*p == '-') { left = true; p++; }
            else if (*p == '0') { zero = true; p++; }
            else if (*p == '+') { plus = true; p++; }
            else if (*p == ' ') { space = true; p++; }
            else break;
        }
        int width = 0;
        while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
        int prec = -1;
        if (*p == '.') { p++; prec = 0; while (*p >= '0' && *p <= '9') { prec = prec * 10 + (*p - '0'); p++; } }
        bool isLong = false, isLongLong = false;
        while (*p == 'l' || *p == 'h' || *p == 'z' || *p == 'I') {
            if (*p == 'l') { if (isLong) isLongLong = true; isLong = true; p++; }
            else if (*p == 'I' && *(p + 1) == '6' && *(p + 2) == '4') { isLongLong = true; p += 3; }
            else if (*p == 'z') { isLongLong = true; p++; }
            else p++;
        }

        char tmp[512]; int tlen = 0;
        const char *prefix = "";
        switch (*p) {
        case 'd': case 'i': {
            long long v = isLongLong ? va_arg(ap, long long) : (long long)va_arg(ap, int);
            bool neg = v < 0;
            unsigned long long uv = neg ? (unsigned long long)(-v) : (unsigned long long)v;
            tlen = fmt_int(uv, 10, false, tmp);
            if (neg) prefix = "-"; else if (plus) prefix = "+"; else if (space) prefix = " ";
            break;
        }
        case 'u': {
            unsigned long long v = isLongLong ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned int);
            tlen = fmt_int(v, 10, false, tmp);
            break;
        }
        case 'x': case 'X': {
            unsigned long long v = isLongLong ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned int);
            tlen = fmt_int(v, 16, *p == 'X', tmp);
            break;
        }
        case 'p': {
            void *v = va_arg(ap, void *);
            tmp[0] = '0'; tmp[1] = 'x';
            tlen = 2 + fmt_int((unsigned long long)v, 16, false, tmp + 2);
            break;
        }
        case 'c': {
            tmp[0] = (char)va_arg(ap, int);
            tlen = 1;
            break;
        }
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int n = 0;
            while (s[n]) n++;
            if (prec >= 0 && n > prec) n = prec;
            for (int i = 0; i < n; i++) tmp[i] = s[i];
            tlen = n;
            break;
        }
        case 'f': case 'F': {
            double v = va_arg(ap, double);
            fmt_flt(v, prec < 0 ? 6 : prec, tmp, tlen);
            break;
        }
        case 0:
            o.put('%'); p = start + 1; continue;
        default:
            // 未知转换符：原样输出（Calcium 里 printf(用户文本) 会走到这里）
            for (const char *q = start; q <= p; q++) o.put(*q);
            p++;
            continue;
        }
        p++;

        int plen = 0;
        while (prefix[plen]) plen++;
        int total = plen + tlen;
        int pad = (width > total) ? width - total : 0;
        if (!left) {
            if (zero) { for (int i = 0; i < plen; i++) o.put(prefix[i]); for (int i = 0; i < pad; i++) o.put('0'); }
            else { for (int i = 0; i < pad; i++) o.put(' '); for (int i = 0; i < plen; i++) o.put(prefix[i]); }
        } else {
            for (int i = 0; i < plen; i++) o.put(prefix[i]);
        }
        for (int i = 0; i < tlen; i++) o.put(tmp[i]);
        if (left) for (int i = 0; i < pad; i++) o.put(' ');
    }
    if (buf && cap) buf[(o.len < cap) ? o.len : cap - 1] = 0;
    return (int)o.len;
}

} // namespace

// ---------------------------------------------------------------- UCRT 底层
// UCRT 的 <stdio.h> 里 printf/sprintf/snprintf 全是 _CRT_STDIO_INLINE 的内联壳，
// 最终落到这三个 __stdio_common_* 上。所以这里只实现底层，不重复定义 printf
// （重复定义会和 launch.obj 里的 COMDAT 撞车，报 LNK2005）。
typedef void *EFI_FILEPTR;

extern "C" EFI_FILEPTR __cdecl __acrt_iob_func(unsigned idx)
{
    (void)idx;
    return (EFI_FILEPTR)(size_t)(idx + 1);      // 假句柄，仅作透传
}

// 把格式化结果整体写到控制台
static int emit_to_console(const char *fmt, va_list ap)
{
    efi_trace("P");
    const size_t cap = 16384;
    char *buf = (char *)malloc(cap);
    if (!buf) return -1;
    int n = format_core(buf, cap, fmt, ap);
    size_t len = strlen(buf);
    eficon::WriteUtf8(buf, (unsigned long long)len);
    free(buf);
    return n;
}

extern "C" int __cdecl __stdio_common_vfprintf(unsigned __int64 options, EFI_FILEPTR stream,
                                              const char *fmt, const void *locale, va_list ap)
{
    (void)options; (void)stream; (void)locale;
    return emit_to_console(fmt, ap);
}

extern "C" int __cdecl __stdio_common_vsprintf(unsigned __int64 options, char *buf, size_t cap,
                                              const char *fmt, const void *locale, va_list ap)
{
    (void)options; (void)locale;
    return format_core(buf, cap, fmt, ap);
}

extern "C" int __cdecl __stdio_common_vsprintf_s(unsigned __int64 options, char *buf, size_t cap,
                                                const char *fmt, const void *locale, va_list ap)
{
    (void)options; (void)locale;
    return format_core(buf, cap, fmt, ap);
}

// 下面这几个是 UCRT 里少见的非内联入口，Calcium 会直接用到
extern "C" int __cdecl _vsnprintf(char *buf, size_t cap, const char *fmt, va_list ap)
{
    return format_core(buf, cap, fmt, ap);
}

extern "C" int __cdecl puts(const char *s)
{
    eficon::WriteUtf8(s, strlen(s));
    eficon::WriteAscii("\n");
    return 1;
}
extern "C" int __cdecl putchar(int c)
{
    char b[1] = { (char)c };
    eficon::WriteUtf8(b, 1);
    return c;
}
extern "C" int __cdecl fputs(const char *s, void *) { eficon::WriteUtf8(s, strlen(s)); return 1; }

// ================================================================ 时间
static int *errno_slot()
{
    static int e = 0;
    return &e;
}
extern "C" int * __cdecl _errno(void) { return errno_slot(); }
extern "C" int * __cdecl __acrt_errno(void) { return errno_slot(); }

static bool efi_time_now(EFI_TIME &t)
{
    if (!gRT) return false;
    return !EFI_ERROR(gRT->GetTime(&t, 0));
}

// 公历日期 -> 1970 起的天数
static long long days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (long long)era * 146097 + (long long)doe - 719468;
}

extern "C" __time64_t __cdecl _time64(__time64_t *out)
{
    EFI_TIME t;
    long long secs = 0;
    if (efi_time_now(t)) {
        long long days = days_from_civil((int)t.Year, t.Month, t.Day);
        secs = days * 86400LL + t.Hour * 3600LL + t.Minute * 60LL + t.Second;
    }
    if (out) *out = secs;
    return secs;
}
// time()/localtime() 在 <time.h> 里是内联壳，分别落到 _time64 / _localtime64，
// 所以这里只提供后者（再定义 time/localtime 会 LNK2005）。

static struct tm g_tm;
extern "C" struct tm * __cdecl _localtime64(const __time64_t *pt)
{
    long long secs = pt ? *pt : 0;
    // 如果没人调过 time()，直接读固件时间
    if (secs == 0) {
        EFI_TIME t;
        if (efi_time_now(t)) {
            g_tm.tm_year = (int)t.Year - 1900;
            g_tm.tm_mon  = (int)t.Month - 1;
            g_tm.tm_mday = t.Day;
            g_tm.tm_hour = t.Hour;
            g_tm.tm_min  = t.Minute;
            g_tm.tm_sec  = t.Second;
            g_tm.tm_wday = 0; g_tm.tm_yday = 0; g_tm.tm_isdst = 0;
            return &g_tm;
        }
    }
    long long days = secs / 86400;
    long long sod  = secs % 86400;
    if (sod < 0) { sod += 86400; days--; }
    // 1970-01-01 起的天数 -> 年月日
    long long z = days + 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned long long doe = (unsigned long long)(z - era * 146097);
    unsigned long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long y = (long long)yoe + era * 400;
    unsigned long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned long long mp = (5 * doy + 2) / 153;
    unsigned long long d = doy - (153 * mp + 2) / 5 + 1;
    unsigned long long m = mp + (mp < 10 ? 3 : -9);
    y += (m <= 2);

    g_tm.tm_year = (int)(y - 1900);
    g_tm.tm_mon  = (int)(m - 1);
    g_tm.tm_mday = (int)d;
    g_tm.tm_hour = (int)(sod / 3600);
    g_tm.tm_min  = (int)((sod % 3600) / 60);
    g_tm.tm_sec  = (int)(sod % 60);
    g_tm.tm_wday = (int)((days + 4) % 7);
    g_tm.tm_yday = 0; g_tm.tm_isdst = 0;
    return &g_tm;
}
// localtime 的实体在 _localtime64，见上。

// ================================================================ 数学
// 不用 <emmintrin.h>（会带入 intrinsic 声明），用牛顿迭代自己算
static double fabs_d(double x) { return x < 0 ? -x : x; }

extern "C" double __cdecl sqrt(double x)
{
    if (!(x > 0.0)) return 0.0;                 // 含 NaN / 0 / 负数
    union { double d; unsigned long long u; } v;
    v.d = x;
    v.u = (v.u >> 1) + 0x1FF8000000000000ULL;   // 位技巧给个 ~5% 的初值
    double r = v.d;
    for (int i = 0; i < 6; i++) {
        if (r <= 0.0) r = 1.0;
        r = 0.5 * (r + x / r);
    }
    return r;
}

extern "C" double __cdecl fabs(double x) { return fabs_d(x); }
// fabsf 由编译器按核心语言规则内建，无需（也不允许）自定义

// ================================================================ 编译器/运行时杂项
extern "C" int _fltused = 1;                 // 告知链接器用了浮点
extern "C" void __chkstk() {}                // UEFI 栈已全映射，探测可空实现
extern "C" void _chkstk() {}
extern "C" char * __cdecl getenv(const char *) { return 0; }

// ---- 进程退出：先跑 atexit 队列，再把控制权还给固件
typedef void (__cdecl *atexit_fn)(void);
static atexit_fn g_atexit[64];
static int g_atexit_n = 0;

extern "C" int __cdecl atexit(atexit_fn fn)
{
    if (g_atexit_n < 64) g_atexit[g_atexit_n++] = fn;
    return 0;
}
extern "C" void __cdecl _cexit(void) {}

extern "C" void __cdecl exit(int code)
{
    while (g_atexit_n > 0) {
        atexit_fn fn = g_atexit[--g_atexit_n];
        if (fn) fn();
    }
    eficon::WriteAscii("\n[Calcium-EFI] exit\n");
    if (gBS) gBS->Stall(3000000);
    if (gBS) {
        typedef EFI_STATUS (__cdecl *exit_fn)(EFI_HANDLE, EFI_STATUS, UINTN, CHAR16 *);
        exit_fn ExitImage = (exit_fn)gBS->Exit;
        ExitImage(gImageHandle, EFI_ERROR_MASK | (unsigned)(code & 0x7fffffff), 0, 0);
    }
    for (;;) efi_stall_us(1000000);
}
extern "C" void __cdecl _exit(int code) { exit(code); }
extern "C" void __cdecl _Exit(int code) { exit(code); }
extern "C" void __cdecl abort(void) { panic_halt("abort()"); }
extern "C" void __cdecl terminate(void) { panic_halt("std::terminate"); }
extern "C" void __cdecl _invalid_parameter_noinfo(void) { panic_halt("invalid parameter"); }
extern "C" int __cdecl _purecall(void) { panic_halt("pure virtual call"); return 0; }

// ---- 输入
extern "C" int __cdecl getchar(void)
{
    if (!gST || !gST->ConIn) return -1;
    EFI_INPUT_KEY k;
    for (;;) {
        if (!EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &k))) return (int)k.UnicodeChar;
        if (gBS) gBS->Stall(20000);
    }
}

// ---- signal：EFI 下没有信号机制，全部假装成功
extern "C" void (__cdecl * __cdecl signal(int, void (__cdecl *)(int)))(int) { return 0; }
extern "C" int __cdecl raise(int) { return 0; }

// ---- 文件系统：映射到 Simple File System
extern "C" int __cdecl remove(const char *path)
{
    efi::invalidate_file_cache();          // 别让读缓存留住已删文件
    return efifs::Remove(path) ? 0 : -1;
}

extern "C" int __cdecl rename(const char *from, const char *to)
{
    efi::invalidate_file_cache();
    char *data = 0;
    unsigned int len = 0;
    if (!efifs::ReadAll(from, data, len)) return -1;
    bool ok = efifs::WriteAll(to, data, len);
    efifs::FreeBuf(data);
    if (!ok) return -1;
    efifs::Remove(from);
    return 0;
}

// ---- 字符串转数字
extern "C" long long __cdecl strtoll(const char *s, char **end, int base)
{
    if (!s) { if (end) *end = 0; return 0; }
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { s += 2; base = 16; }
    if (base == 0) base = (*s == '0') ? 8 : 10;
    unsigned long long v = 0;
    for (;;) {
        int d;
        char c = *s;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned)base + (unsigned)d;
        s++;
    }
    if (end) *end = (char *)s;
    return neg ? -(long long)v : (long long)v;
}

extern "C" long __cdecl strtol(const char *s, char **end, int base)
{
    return (long)strtoll(s, end, base);
}
extern "C" unsigned long __cdecl strtoul(const char *s, char **end, int base)
{
    return (unsigned long)strtoll(s, end, base);
}
extern "C" unsigned long long __cdecl strtoull(const char *s, char **end, int base)
{
    return (unsigned long long)strtoll(s, end, base);
}

extern "C" double __cdecl strtod(const char *s, char **end)
{
    if (!s) { if (end) *end = 0; return 0.0; }
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    int neg = 0;
    if (*p == '-') { neg = 1; p++; } else if (*p == '+') p++;
    double v = 0.0;
    while (*p >= '0' && *p <= '9') { v = v * 10.0 + (double)(*p - '0'); p++; }
    if (*p == '.') {
        p++;
        double f = 0.1;
        while (*p >= '0' && *p <= '9') { v += (double)(*p - '0') * f; f *= 0.1; p++; }
    }
    if (end) *end = (char *)p;
    return neg ? -v : v;
}
extern "C" float __cdecl strtof(const char *s, char **end) { return (float)strtod(s, end); }

// ---- strtok（用 UCRT 同款约定：NULL 表示延续上次）
extern "C" char * __cdecl strtok(char *s, const char *delim)
{
    static char *last = 0;
    if (!s) s = last;
    if (!s) return 0;
    while (*s) {
        int hit = 0;
        for (const char *d = delim; *d; d++) if (*d == *s) { hit = 1; break; }
        if (!hit) break;
        s++;
    }
    if (!*s) { last = 0; return 0; }
    char *tok = s;
    while (*s) {
        for (const char *d = delim; *d; d++) {
            if (*d == *s) { *s = 0; last = s + 1; return tok; }
        }
        s++;
    }
    last = 0;
    return tok;
}

// ---- STL 的向量化搜索辅助（原本在静态库里，这里给线性实现）
extern "C" const void * __stdcall __std_search_1(const void *f1, const void *l1,
                                                 const void *f2, size_t n) noexcept
{
    const char *a = (const char *)f1, *end = (const char *)l1, *pat = (const char *)f2;
    if (n == 0) return f1;
    for (; a + n <= end; a++) {
        if (memcmp(a, pat, n) == 0) return a;
    }
    return l1;
}

static size_t find_of(const void *h, size_t hl, const void *n, size_t nl, int want_in)
{
    const char *hs = (const char *)h, *nd = (const char *)n;
    for (size_t i = 0; i < hl; i++) {
        int hit = 0;
        for (size_t j = 0; j < nl; j++) if (hs[i] == nd[j]) { hit = 1; break; }
        if (hit == want_in) return i;
    }
    return (size_t)-1;
}

extern "C" __declspec(noalias) size_t __stdcall __std_find_first_of_trivial_pos_1(
    const void *h, size_t hl, const void *n, size_t nl) noexcept
{
    return find_of(h, hl, n, nl, 1);
}

extern "C" __declspec(noalias) size_t __stdcall __std_find_first_not_of_trivial_pos_1(
    const void *h, size_t hl, const void *n, size_t nl) noexcept
{
    return find_of(h, hl, n, nl, 0);
}

// ---- STL 异常抛出点：无异常环境下直接停机（正常代码路径不会走到）
namespace std {

[[noreturn]] void __cdecl _Xlength_error(const char *what) { (void)what; panic_halt("std::length_error"); }
[[noreturn]] void __cdecl _Xout_of_range(const char *what) { (void)what; panic_halt("std::out_of_range"); }
[[noreturn]] void __cdecl _Xinvalid_argument(const char *what) { (void)what; panic_halt("std::invalid_argument"); }

} // namespace std

namespace stdext { class exception; }
namespace std {
void (__cdecl *_Raise_handler)(const stdext::exception &) = 0;
}

// ---- MSVC STL 向量化算法的外部符号（/NODEFAULTLIB 下 msvcprt 不可用，手工补齐）
extern "C" {
const char* __std_find_trivial_1(const char* first, const char* last, char v) {
    for (; first != last; ++first) if (*first == v) return first;
    return last;
}
const char* __std_find_last_trivial_1(const char* first, const char* last, char v) {
    const char* it = last;
    while (it != first) { --it; if (*it == v) return it; }
    return last;
}
const wchar_t* __std_find_trivial_2(const wchar_t* first, const wchar_t* last, wchar_t v) {
    for (; first != last; ++first) if (*first == v) return first;
    return last;
}
}

// type_info 的虚表原本在 msvcprt 里；用 alternatename 指到一块静内存
extern "C" { __declspec(align(8)) char efi_typeinfo_vtbl[64] = { 0 }; }
#pragma comment(linker, "/alternatename:??_7type_info@@6B@=efi_typeinfo_vtbl")

// 全局构造器遍历（.CRT$XCU 段）
extern "C" {
    typedef void (__cdecl *_PVFV)(void);
#pragma section(".CRT$XCA", read)
#pragma section(".CRT$XCU", read)
#pragma section(".CRT$XCZ", read)
    __declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { 0 };
    __declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { 0 };
}

void efi_run_initializers()
{
    for (_PVFV *p = __xc_a; p < __xc_z; p++) {
        if (*p) (*p)();
    }
}
