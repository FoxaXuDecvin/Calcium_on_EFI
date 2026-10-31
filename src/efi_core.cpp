// ============================================================================
//  efi_core.cpp —— 控制台 / 文件系统 / 内存 的 EFI 实现
// ============================================================================
#include "efi_core.h"

EFI_SYSTEM_TABLE *gST = 0;
EFI_BOOT_SERVICES *gBS = 0;
EFI_RUNTIME_SERVICES *gRT = 0;
EFI_HANDLE gImageHandle = 0;
EFI_FILE *gFsRoot = 0;

// ---------------------------------------------------------------- GUID
static EFI_GUID GUID_LOADED_IMAGE = { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
static EFI_GUID GUID_SIMPLE_FS   = { 0x964E5B22, 0x6459, 0x11d2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
static EFI_GUID GUID_FILE_INFO   = { 0x09576E92, 0x6D3F, 0x11d2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };

// ---------------------------------------------------------------- 内存
void *efi_alloc(unsigned long long size)
{
    void *p = 0;
    if (size == 0) size = 1;
    if (EFI_ERROR(gBS->AllocatePool(EfiPoolLoaderData, size, &p))) return 0;
    return p;
}

void efi_free(void *p)
{
    if (p) gBS->FreePool(p);
}

void efi_stall_us(unsigned long long us)
{
    if (us) gBS->Stall(us);
}

// ---------------------------------------------------------------- 跟踪
// 打开 EFI_TRACE_OUT 后才会往控制台打标记；平时是空函数（见 efi_core.h 宏）
#ifdef EFI_TRACE_OUT
void efi_trace(const char *tag)
{
    if (!gST || !gST->ConOut) return;
    CHAR16 w[64];
    UINTN n = 0;
    w[n++] = '[';
    while (*tag && n < 60) w[n++] = (CHAR16)(unsigned char)*tag++;
    w[n++] = ']';
    w[n] = 0;
    gST->ConOut->OutputString(gST->ConOut, w);
}
#endif

// ---------------------------------------------------------------- 路径：char* -> CHAR16*
// 统一分隔符为 '\\'，去掉前导分隔符（路径相对卷根）。
static bool PathToW(const char *path, CHAR16 *out, unsigned long long cap)
{
    if (cap < 2) return false;
    UINTN n = 0;
    const char *p = path;
    while (*p == '/' || *p == '\\') p++;          // 去前导
    while (*p && n + 1 < cap) {
        out[n++] = (CHAR16)((*p == '/') ? '\\' : (unsigned char)*p);
        p++;
    }
    out[n] = 0;
    if (n == 0) { out[0] = '.'; out[1] = 0; }     // 空路径视为根
    return true;
}

// ---------------------------------------------------------------- 文件系统
namespace efifs {

char SelfName[260] = "\\EFI\\BOOT\\Calcium.efi";

static EFI_FILE *OpenPath(const char *path, UINT64 mode, UINT64 attr)
{
    if (!gFsRoot) return 0;
    CHAR16 w[512];
    if (!PathToW(path, w, 512)) return 0;
    EFI_FILE *f = 0;
    if (EFI_ERROR(gFsRoot->Open(gFsRoot, &f, w, mode, attr))) return 0;
    return f;
}

// 前置声明（定义在 MakeDir 附近）
static bool MakeOneDir(const char *rel);

bool Init()
{
    if (!gBS || !gImageHandle) return false;
    EFI_LOADED_IMAGE_PROTOCOL *li = 0;
    if (EFI_ERROR(gBS->HandleProtocol(gImageHandle, &GUID_LOADED_IMAGE, (void **)&li)) || !li)
        return false;

    void *dp = li->FilePath;
    EFI_HANDLE devHandle = gImageHandle;
    if (dp) {
        if (EFI_ERROR(gBS->LocateDevicePath(&GUID_SIMPLE_FS, &dp, &devHandle)))
            devHandle = li->DeviceHandle;
    } else {
        devHandle = li->DeviceHandle;
    }

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = 0;
    if (EFI_ERROR(gBS->HandleProtocol(devHandle, &GUID_SIMPLE_FS, (void **)&fs)) || !fs) return false;
    if (EFI_ERROR(fs->OpenVolume(fs, &gFsRoot)) || !gFsRoot) return false;
    return true;
}

bool ReadAll(const char *path, char *&out, unsigned int &len)
{
    efi_trace("R");
    out = 0; len = 0;
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ, 0);
    if (!f) return false;

    // 取文件大小
    unsigned char infoBuf[600];
    UINTN infoSize = sizeof(infoBuf);
    UINT64 fileSize = 0;
    if (!EFI_ERROR(f->GetInfo(f, &GUID_FILE_INFO, &infoSize, infoBuf))) {
        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)infoBuf;
        fileSize = fi->FileSize;
    }

    if (fileSize > (64ULL * 1024 * 1024)) { f->Close(f); return false; }

    UINTN cap = (UINTN)fileSize + 1;
    char *buf = (char *)efi_alloc(cap + 16);
    if (!buf) { f->Close(f); return false; }

    UINTN total = 0;
    while (total < (UINTN)fileSize) {
        UINTN chunk = (UINTN)fileSize - total;
        if (EFI_ERROR(f->Read(f, &chunk, buf + total))) break;
        if (chunk == 0) break;
        total += chunk;
    }
    buf[total] = 0;
    f->Close(f);
    out = buf; len = (unsigned int)total;
    return true;
}

void FreeBuf(void *p) { efi_free(p); }

bool WriteAll(const char *path, const char *data, unsigned int len)
{
    efi_trace("W");
    // 注意：这里绝不可以在打开失败时自动补建父目录！
    // Coral 的 _dapi_ExistFolder_check 靠"往目录里写 mark 文件"验证目录存在，
    // 若写失败自动建目录，就会把刚删掉的目录复活，导致删除永远失败。
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (!f) return false;
    // 截断到 0
    f->SetPosition(f, 0);
    UINTN sz = len;
    EFI_STATUS st = EFI_SUCCESS;
    if (sz) st = f->Write(f, &sz, (void *)data);
    f->Flush(f);
    f->Close(f);
    return !EFI_ERROR(st) && sz == (UINTN)len;
}

bool Exists(const char *path)
{
    efi_trace("E");
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ, 0);
    if (!f) return false;
    f->Close(f);
    return true;
}

bool IsDir(const char *path)
{
    efi_trace("D");
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ, 0);
    if (!f) return false;
    unsigned char infoBuf[600];
    UINTN infoSize = sizeof(infoBuf);
    bool isDir = false;
    if (!EFI_ERROR(f->GetInfo(f, &GUID_FILE_INFO, &infoSize, infoBuf))) {
        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)infoBuf;
        isDir = (fi->Attribute & EFI_FILE_DIRECTORY) != 0;
    }
    f->Close(f);
    return isDir;
}

bool Remove(const char *path)
{
    efi_trace("X");
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (!f) return false;
    // 注意：EFI_FILE_PROTOCOL.Delete() 成功后句柄已被关闭，
    // 不能再调 Close()，否则是 use-after-free，会把固件堆搞坏。
    EFI_STATUS st = f->Delete(f);
    return !EFI_ERROR(st);
}

bool RemoveDir(const char *path)
{
    efi_trace("Xr");
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (!f) return false;
    // 注意：Delete 内部已关闭句柄，不能再调 Close()（use-after-free）
    EFI_STATUS st = f->Delete(f);
    return !EFI_ERROR(st);
}

// 逐级创建：a/b/c
// 关键点：分隔符必须累积进 cur[]，否则下一层的前缀会丢掉父级，
// 把 "script/~$RAMDISK" 变成 "script~$RAMDISK"（曾在真实 ROM 上踩过）。
static bool MakeOneDir(const char *rel)
{
    if (!rel || !*rel) return true;
    EFI_FILE *d = OpenPath(rel, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                           EFI_FILE_DIRECTORY);
    if (!d) return false;
    d->Close(d);
    return true;
}

bool MakeDir(const char *path)
{
    efi_trace("M");
    if (!gFsRoot) return false;

    char cur[512]; UINTN n = 0;
    const char *p = path;
    while (*p == '/' || *p == '\\') p++;              // 去前导分隔符

    bool ok = true;
    while (*p && n + 2 < sizeof(cur)) {
        if (*p == '/' || *p == '\\') {
            while (*p == '/' || *p == '\\') p++;      // 折叠连续分隔符
            cur[n] = 0;
            if (n && !MakeOneDir(cur)) ok = false;
            cur[n++] = '\\';                          // 保留分隔符，作为下一层前缀
            continue;
        }
        cur[n++] = *p++;
    }

    // 末级：路径不以分隔符结尾时要单独创建；结尾带分隔符则循环里已建过
    if (n && cur[n - 1] != '\\') {
        cur[n] = 0;
        if (!MakeOneDir(cur)) ok = false;
    }
    return ok;
}

// 追加写：Position = 0xFFFFFFFFFFFFFFFF 表示文件末尾（UEFI 规范）
// 用 CREATE 一次打开即可（已存在则打开，不存在则创建），避免"先试后建"的双倍开销。
bool AppendAll(const char *path, const char *data, unsigned int len)
{
    efi_trace("Wa");
    if (!len) return true;
    EFI_FILE *f = OpenPath(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (!f) return false;
    if (EFI_ERROR(f->SetPosition(f, 0xFFFFFFFFFFFFFFFFULL))) { f->Close(f); return false; }
    UINTN sz = len;
    EFI_STATUS st = f->Write(f, &sz, (void *)data);
    f->Flush(f);
    f->Close(f);
    return !EFI_ERROR(st) && sz == (UINTN)len;
}

bool CopyFile(const char *src, const char *dst)
{
    efi_trace("C");
    char *data = 0; unsigned int len = 0;
    if (!ReadAll(src, data, len)) return false;
    bool ok = WriteAll(dst, data, len);
    FreeBuf(data);
    return ok;
}

bool ListDir(const char *path, bool (*cb)(const char *name, bool isDir, void *user), void *user)
{
    efi_trace("L");
    CHAR16 w[512];
    if (!PathToW(path, w, 512)) return false;
    EFI_FILE *dir = 0;
    if (EFI_ERROR(gFsRoot->Open(gFsRoot, &dir, w, EFI_FILE_MODE_READ, 0)) || !dir) return false;

    bool cont = true;
    unsigned char buf[2048];
    for (;;) {
        UINTN sz = sizeof(buf);
        if (EFI_ERROR(dir->Read(dir, &sz, buf))) break;
        if (sz == 0) break;
        EFI_FILE_INFO *fi = (EFI_FILE_INFO *)buf;
        // 文件名转 ASCII（脚本名基本是 ASCII）
        char name[260]; UINTN i = 0;
        while (fi->FileName[i] && i + 1 < sizeof(name)) { name[i] = (char)(fi->FileName[i] & 0xFF); i++; }
        name[i] = 0;
        // 跳过 . 与 ..
        bool dot = (name[0] == '.' && name[1] == 0) || (name[0] == '.' && name[1] == '.' && name[2] == 0);
        if (!dot) {
            cont = cb(name, (fi->Attribute & EFI_FILE_DIRECTORY) != 0, user);
            if (!cont) break;
        }
    }
    dir->Close(dir);
    return true;
}

} // namespace efifs

// ---------------------------------------------------------------- 控制台
namespace eficon {

static char s_lineBuf[4096];

void Init()
{
    if (gST && gST->ConOut) {
        gST->ConOut->Reset(gST->ConOut, 0);
    }
}

// COM1 直写（定义在下方），WriteUtf8 也要镜像
static bool g_com1_ok = false;
static void Com1Init();
static void Com1Putc(char c);

// UTF-8 -> UTF-16 并输出（同时镜像一份 UTF-8 到 COM1，供 VMware serial 落盘）
void WriteUtf8(const char *s, unsigned long long n)
{
    if (!gST || !gST->ConOut || !s) return;
    if (!g_com1_ok) Com1Init();    // 首笔输出可能直接走这里（无探针引导）

    CHAR16 wbuf[1024];
    UINTN wn = 0;
    UINTN i = 0;
    while (i < n && s[i]) {
        unsigned char c = (unsigned char)s[i];
        unsigned int cp = c;
        int extra = 0;
        unsigned char raw[4]; int rawn = 1; raw[0] = c;
        if (c >= 0xF0) { cp = c & 0x07; extra = 3; }
        else if (c >= 0xE0) { cp = c & 0x0F; extra = 2; }
        else if (c >= 0xC0) { cp = c & 0x1F; extra = 1; }
        i++;
        for (int k = 0; k < extra && i < n; k++, i++) {
            unsigned char cc = (unsigned char)s[i];
            if ((cc & 0xC0) != 0x80) { cp = '?'; break; }
            cp = (cp << 6) | (cc & 0x3F);
            if (k + 1 < 4) raw[rawn++] = cc;
        }
        if (cp == '\n') {
            wbuf[wn++] = '\r'; wbuf[wn++] = '\n';
            Com1Putc('\r'); Com1Putc('\n');
        } else if (cp < 0x10000) {
            wbuf[wn++] = (CHAR16)cp;
            for (int k = 0; k < rawn; k++) Com1Putc((char)raw[k]);
        } else {
            cp -= 0x10000;
            wbuf[wn++] = (CHAR16)(0xD800 + (cp >> 10));
            wbuf[wn++] = (CHAR16)(0xDC00 + (cp & 0x3FF));
            // 补充平面：COM1 侧输出 UTF-8 原始 4 字节（raw 里已是）
            for (int k = 0; k < rawn; k++) Com1Putc((char)raw[k]);
        }
        if (wn >= 1020) { wbuf[wn] = 0; gST->ConOut->OutputString(gST->ConOut, wbuf); wn = 0; }
    }
    if (wn) { wbuf[wn] = 0; gST->ConOut->OutputString(gST->ConOut, wbuf); }
}

// ---------------------------------------------------------------- COM1 直写
// QEMU/OVMF 固件会把 ConOut 镜像到串口，VMware 固件不会 —— 在 VMware 下抓
// serial 落盘必须自己往 0x3F8 写。16550 初始化 + 带超时的单字节写，
// 端口不存在（LSR 恒 0xFF）时永久禁用，绝不挂死。
#include <intrin.h>

static void Com1Init()
{
    const unsigned short p = 0x3F8;
    __outbyte(p + 1, 0x00);        // 关中断
    __outbyte(p + 3, 0x80);        // DLAB=1
    __outbyte(p + 0, 0x01);        // 除数 1 = 115200
    __outbyte(p + 1, 0x00);
    __outbyte(p + 3, 0x03);        // 8N1
    __outbyte(p + 2, 0xC7);        // FIFO 使能+清空
    __outbyte(p + 4, 0x0B);        // DTR|RTS|OUT2
    g_com1_ok = (__inbyte(p + 5) != 0xFF);   // 无 UART 时 LSR 读全 1
}

static void Com1Putc(char c)
{
    if (!g_com1_ok) return;
    const unsigned short p = 0x3F8;
    unsigned long long guard = 0;
    while (!(__inbyte(p + 5) & 0x20)) {        // 等 THR 空
        if (++guard > 200000) return;          // 超时放弃这一个字节
    }
    __outbyte(p, (unsigned char)c);
}

void WriteAscii(const char *s)
{
    if (!g_com1_ok) Com1Init();    // 惰性初始化（首个输出点时 UART 早已就绪）
    if (!gST || !gST->ConOut) return;
    CHAR16 w[512]; UINTN n = 0;
    while (*s) {
        char c = *s++;
        if (c == '\n') { w[n++] = '\r'; w[n++] = '\n'; Com1Putc('\r'); Com1Putc('\n'); }
        else { w[n++] = (CHAR16)(unsigned char)c; Com1Putc(c); }
        if (n >= 500) { w[n] = 0; gST->ConOut->OutputString(gST->ConOut, w); n = 0; }
    }
    if (n) { w[n] = 0; gST->ConOut->OutputString(gST->ConOut, w); }
}

void SerialAscii(const char *s)
{
    if (!g_com1_ok) Com1Init();
    if (!s) return;
    while (*s) {
        if (*s == '\n') Com1Putc('\r');
        Com1Putc(*s++);
    }
    Com1Putc('\r'); Com1Putc('\n');
}

void Clear()
{
    if (!gST || !gST->ConOut) return;
    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
}

bool HasKey()
{
    if (!gST || !gST->ConIn || !gST->ConIn->WaitForKey) return false;
    UINTN idx = 0;
    EFI_EVENT ev = gST->ConIn->WaitForKey;
    EFI_STATUS st = gBS->WaitForEvent(1, &ev, &idx);
    return !EFI_ERROR(st);
}

// ---- 预置输入（无人值守测试用）--------------------------------------------
// 卷根若有 autoinput.txt，每行当作一次 ReadLine 的输入依次消费；
// 文件不存在时完全走键盘，正常使用不受影响。
static char s_autoBuf[8192];
static unsigned int s_autoLen = 0;
static unsigned int s_autoPos = 0;

void LoadAutoInput(const char *path)
{
    s_autoLen = s_autoPos = 0;
    char *data = 0;
    unsigned int len = 0;
    if (!efifs::ReadAll(path, data, len) || !data) return;
    unsigned int n = len < sizeof(s_autoBuf) ? len : (unsigned int)sizeof(s_autoBuf);
    for (unsigned int i = 0; i < n; i++) s_autoBuf[i] = data[i];
    s_autoLen = n;
    efifs::FreeBuf(data);
}

static bool AutoNextLine(char *buf, unsigned long long cap)
{
    if (s_autoPos >= s_autoLen) return false;
    unsigned long long n = 0;
    while (s_autoPos < s_autoLen && s_autoBuf[s_autoPos] != '\n') {
        char c = s_autoBuf[s_autoPos++];
        if (c != '\r' && n + 1 < cap) buf[n++] = c;
    }
    if (s_autoPos < s_autoLen) s_autoPos++;          // 吃掉 '\n'
    buf[n] = 0;
    WriteAscii(buf);
    WriteAscii("\r\n");
    return true;
}

bool ReadLine(char *buf, unsigned long long cap)
{
    if (AutoNextLine(buf, cap)) return true;
    if (!gST || !gST->ConIn) return false;
    UINTN n = 0;
    for (;;) {
        // 没按键时 ReadKeyStroke 返回 EFI_NOT_READY，必须忙等而不是当成失败
        EFI_INPUT_KEY key;
        for (;;) {
            EFI_STATUS st = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
            if (!EFI_ERROR(st)) break;
            if (gBS) gBS->Stall(15000);
        }
        CHAR16 ch = key.UnicodeChar;
        if (ch == 0) continue;                       // 功能键
        if (ch == '\r' || ch == '\n') { WriteAscii("\n"); break; }
        if (ch == 0x08) {                            // 退格
            if (n) { n--; WriteAscii("\b \b"); }
            continue;
        }
        if (ch < 0x20) continue;
        if (n + 1 < cap) {
            buf[n++] = (char)(ch & 0xFF);
            CHAR16 echo[2]; echo[0] = ch; echo[1] = 0;
            if (gST->ConOut) gST->ConOut->OutputString(gST->ConOut, echo);
        }
    }
    buf[n] = 0;
    return true;
}

} // namespace eficon
