// ============================================================================
//  efi_main.cpp —— UEFI 入口：初始化固件环境 → 跑全局构造器 → 组装 argv → 调 main()
// ============================================================================
#include "efi_core.h"
#include "efi_bootname.h"

extern "C" int main(int argc, char *argv[]);
void efi_run_initializers();

// 切栈小工具（见 stack_switch.asm）
extern "C" unsigned long long efi_switch_stack(void *new_top,
                                               int (*fn)(int, char **),
                                               int argc, char **argv);

// Calcium 的脚本执行器是深递归 + 大栈帧，固件给应用的栈太小
// （实测 _script<> 嵌套 5 层就三重故障复位）。这里给解释器单开一块大栈。
#define EFI_BIG_STACK (16ULL * 1024 * 1024)

// 调试用：直接用固件 ConOut 打印标记（不走我们自己的转换层）
// Windows 上 wchar_t 就是 16 位，与 CHAR16 内存布局一致，直接转型即可
static void trace(const wchar_t *s)
{
    if (gST && gST->ConOut) gST->ConOut->OutputString(gST->ConOut, (CHAR16 *)s);
}

// ---------------------------------------------------------------- 命令行拆分
// UEFI Shell 通过 LoadOptions 传入 UCS-2 命令行（不含程序名）。
// 直接由固件 BootXXXX 启动时没有 LoadOptions，此时退化为读卷根下的
// startup.args（纯 ASCII，'#' 开头为注释），方便开机自动跑脚本。
static int BuildArgs(EFI_LOADED_IMAGE_PROTOCOL *li, char **argv, int maxArgs)
{
    static char storage[2048];
    int argc = 0;
    argv[argc++] = (char *)"Calcium.efi";
    if (maxArgs <= 1) return argc;

    CHAR16 *src = 0;
    unsigned int chars = 0;

    // 优先看卷根的 startup.args：固件直接启动 BOOTXXXX 时 LoadOptions 不可控，
    // 而且 UEFI 下没法方便地传命令行，用这个文件做「开机参数」最实用。
    {
        char *data = 0;
        unsigned int len = 0;
        if (efifs::ReadAll("startup.ini", data, len) && len) {
            char *p = data, *end = data + len;
            unsigned int n = 0;
            while (p < end && *p) {
                while (p < end && (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')) p++;
                if (p >= end || *p == 0) break;
                if (*p == '#') {                       // 注释：跳到行尾继续
                    while (p < end && *p && *p != '\n') p++;
                    continue;
                }
                if (argc >= maxArgs || n + 1 >= sizeof(storage)) break;
                argv[argc++] = &storage[n];

                // 双引号包裹：剥掉引号，且引号内的空格不再当分隔符。
                // 这是 Windows CRT 的既有语义，必须对齐 —— 否则 -args "NativeShell"
                // 会让脚本里的 _getargs 拿到带引号的 "NativeShell"，
                // Coral 的所有字符串比较全部落空（真实踩过）。
                bool quoted = false;
                if (*p == '"') { quoted = true; p++; }
                while (p < end && *p) {
                    if (quoted) {
                        if (*p == '"') { p++; break; }
                    } else {
                        if (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t') break;
                    }
                    if (n + 1 >= sizeof(storage)) break;
                    storage[n++] = *p++;
                }
                storage[n++] = 0;
            }
            efifs::FreeBuf(data);
            return argc;
        }
        if (data) efifs::FreeBuf(data);
    }

    if (li && li->LoadOptions && li->LoadOptionsSize) {
        src = (CHAR16 *)li->LoadOptions;
        chars = li->LoadOptionsSize / sizeof(CHAR16);
    } else {
        return argc;
    }

    unsigned int i = 0, n = 0;
    while (i < chars && src[i] == ' ') i++;
    while (i < chars && src[i] && n + 1 < sizeof(storage) && argc < maxArgs) {
        // 支持双引号包裹含空格的参数
        bool quoted = (src[i] == L'"');
        if (quoted) i++;
        argv[argc++] = &storage[n];
        while (i < chars && src[i] && n + 1 < sizeof(storage)) {
            CHAR16 c = src[i];
            if (quoted) { if (c == L'"') { i++; break; } }
            else if (c == L' ') break;
            storage[n++] = (char)(c < 0x80 ? c : '?');
            i++;
        }
        storage[n++] = 0;
        while (i < chars && src[i] == ' ') i++;
    }
    return argc;
}

extern "C" EFI_STATUS EFIAPI EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gRT = SystemTable->RuntimeServices;
    gImageHandle = ImageHandle;

    // ---------------------------------------------------------------- 关看门狗
    // 固件（AMI/Insyde 常见默认 5 分钟）在 POST 时武装了看门狗定时器，
    // 一旦到时无人喂狗就 ResetSystem(EfiResetCold) —— 表现为"跑几分钟后自己重启"。
    // 从 UEFI Shell 里起程序时 Shell 已经关过它，所以只在"固件直接启动
    // BOOTX64.EFI"时才会踩到。
    //
    // BootServices.SetWatchdogTimer 只有 4 个参数（没有 TimerType），语义：
    //   Timeout == 0  -> 彻底停用定时器（UEFI 规范 / EDK2 实现均如此）
    //   Timeout != 0  -> 重新武装，单位秒（不是"在剩余时间上叠加"）
    // 所以下面这一句是"关掉"，不是"延长"。
    EFI_STATUS wdStatus = 0xFFFFFFFFFFFFFFFFULL;   // 哨兵：服务不存在
    if (gBS && gBS->SetWatchdogTimer) {
        typedef EFI_STATUS (EFIAPI *SET_WATCHDOG_T)(UINTN, UINT64, UINTN, CHAR16 *);
        SET_WATCHDOG_T setWd = (SET_WATCHDOG_T)gBS->SetWatchdogTimer;
        wdStatus = setWd(0, 0, 0, 0);
        if (EFI_ERROR(wdStatus)) {
            // 极少数固件把 Timeout=0 当非法参数，退而求其次设成极大值
            // （0xFFFFFFFF 秒 ≈ 136 年），效果等同于关闭。
            wdStatus = setWd((UINTN)0xFFFFFFFFu, 0, 0, 0);
        }
    }

    eficon::Init();
    eficon::Clear();

    // 把固件的真实答复打到屏幕/串口，避免"以为关了其实没关"
    if (!gBS || !gBS->SetWatchdogTimer)
        eficon::WriteAscii("[Calcium-EFI] watchdog: service not present\n");
    else if (EFI_ERROR(wdStatus))
        eficon::WriteAscii("[Calcium-EFI] watchdog: disable FAILED (firmware rejected)\n");
    else
        eficon::WriteAscii("[Calcium-EFI] watchdog: disabled\n");

    if (!efifs::Init()) {
        eficon::WriteAscii("[Calcium-EFI] Cannot mount file system volume.\n");
        return 1;
    }

    // 无人值守测试：卷根 autoinput.txt 里的每行依次当作键盘输入
    eficon::LoadAutoInput("autoinput.txt");

    // ------------------------------------------------------- 自定义启动项名字
    // 固件（EDK2 系 BDS）对可移动介质上的 \EFI\BOOT\BOOTX64.EFI 会临时合成一个
    // 描述为 "UEFI OS" 的启动项 —— 字符串硬编码在固件里，且不写 NVRAM，
    // 所以改我们自己的 .efi 是改不掉的。要拿到自定义名字，只能在 NVRAM 里
    // 建一条 Boot#### 持久项，Description 由我们填。
    // 触发：卷根 setbootname.txt（内容 = 新名字，取第一行）
    //       卷根 setbootname.on （用默认名 "Coral OS Based on Calcium"）
    {
        char *cfg = 0;
        unsigned int cfgLen = 0;
        if (efifs::ReadAll("setbootname.txt", cfg, cfgLen) && cfgLen > 0 && cfg) {
            char name[80];
            unsigned int k = 0;
            while (k < cfgLen && k < 78 && cfg[k] != '\r' && cfg[k] != '\n') { name[k] = cfg[k]; ++k; }
            name[k] = 0;
            efifs::FreeBuf(cfg);
            if (k > 0) efi_bootname::Register(name);
        } else if (efifs::Exists("setbootname.on")) {
            efi_bootname::Register("Coral OS Based on Calcium");
        }
    }

    // ---------------------------------------------------------------- 网络探针
    // 诊断工具：卷根放 netprobe.on 才启用。枚举固件所有句柄的全部协议 GUID，
    // 用于判断网络栈（SNP/MNP/TCP4/HTTP…）是否存在（2026-10-29 实测：
    // QEMU 自带 OVMF 只有 SNP+TlsDxe，无 TCP4/HTTP —— 见工作日志）。
    if (efifs::Exists("netprobe.on")) {
        typedef EFI_STATUS (EFIAPI *LocateHandleBufferT)(int, EFI_GUID *, void *, unsigned long long *, EFI_HANDLE **);
        typedef EFI_STATUS (EFIAPI *ProtocolsPerHandleT)(EFI_HANDLE, EFI_GUID ***, unsigned long long *);
        typedef EFI_STATUS (EFIAPI *FreePoolT)(void *);
        LocateHandleBufferT LocateHandleBuffer = (LocateHandleBufferT)gBS->LocateHandleBuffer;
        ProtocolsPerHandleT ProtocolsPerHandle = (ProtocolsPerHandleT)gBS->ProtocolsPerHandle;
        FreePoolT FreePool = (FreePoolT)gBS->FreePool;

        unsigned long long hn = 0;
        EFI_HANDLE *handles = 0;
        if (!EFI_ERROR(LocateHandleBuffer(0 /*AllHandles*/, 0, 0, &hn, &handles))) {
            char line[96];
            eficon::WriteAscii("\n[NETPROBE] handles=");
            unsigned v = (unsigned)hn; char num[12]; int k = 0;
            if (!v) num[k++] = '0';
            while (v) { num[k++] = (char)('0' + v % 10); v /= 10; }
            while (k) { line[0] = num[--k]; line[1] = 0; eficon::WriteAscii(line); }
            eficon::WriteAscii("\n");

            for (unsigned long long i = 0; i < hn; i++) {
                EFI_GUID **protos = 0;
                unsigned long long pn = 0;
                if (EFI_ERROR(ProtocolsPerHandle(handles[i], &protos, &pn)) || !protos) continue;
                for (unsigned long long j = 0; j < pn; j++) {
                    EFI_GUID *g = protos[j];
                    if (!g) continue;
                    char buf[96];
                    static const char *hx = "0123456789ABCDEF";
                    auto w16 = [&](unsigned short v, char *d) {
                        for (int s = 12; s >= 0; s -= 4) *d++ = hx[(v >> s) & 0xF];
                    };
                    auto w32 = [&](unsigned int v, char *d) {
                        for (int s = 28; s >= 0; s -= 4) *d++ = hx[(v >> s) & 0xF];
                    };
                    char *p = buf;
                    *p++ = 'G'; *p++ = ' '; *p++ = ' ';
                    w32(g->Data1, p); p += 8; *p++ = '-';
                    w16(g->Data2, p); p += 4; *p++ = '-';
                    w16(g->Data3, p); p += 4; *p++ = '-';
                    for (int b = 0; b < 8; b++) {
                        if (b == 2) *p++ = '-';
                        *p++ = hx[(g->Data4[b] >> 4) & 0xF];
                        *p++ = hx[g->Data4[b] & 0xF];
                    }
                    *p++ = '\n'; *p = 0;
                    eficon::WriteAscii(buf);
                }
                FreePool(protos);
            }
            eficon::WriteAscii("[NETPROBE] end\n");
            FreePool(handles);
        } else {
            eficon::WriteAscii("[NETPROBE] LocateHandleBuffer failed\n");
        }
    }

    // 记录自身路径（仅用于显示）
    EFI_LOADED_IMAGE_PROTOCOL *li = 0;
    static EFI_GUID guid_li = { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
    if (!EFI_ERROR(gBS->HandleProtocol(ImageHandle, &guid_li, (void **)&li)) && li) {
        (void)li;
    }

    // 运行全局构造器（std::string 等全局对象依赖它）
    efi_run_initializers();

    static char *argv[64];
    int argc = BuildArgs(li, argv, 64);

    // 在自备的大栈上跑解释器；分配失败就退回固件栈
    void *stk = 0;
    int rc;
    if (!EFI_ERROR(gBS->AllocatePool(EfiPoolLoaderData, EFI_BIG_STACK, &stk)) && stk) {
        char *top = (char *)stk + EFI_BIG_STACK;
        rc = (int)efi_switch_stack(top, main, argc, argv);
        gBS->FreePool(stk);
    } else {
        rc = main(argc, argv);
    }

    eficon::WriteAscii("\n[Calcium-EFI] exited with code ");
    {
        char num[16]; int n = 0, v = rc < 0 ? -rc : rc;
        if (v == 0) num[n++] = '0';
        while (v) { num[n++] = (char)('0' + v % 10); v /= 10; }
        if (rc < 0) eficon::WriteAscii("-");
        char out[16]; int k = 0;
        while (n) out[k++] = num[--n];
        out[k] = 0;
        eficon::WriteAscii(out);
    }
    // ---------------------------------------------------------------- 退出即关机
    // Calcium 跑完直接 ACPI 关机（S5），不再落回固件或 UEFI Shell。
    // 调试期如果要看固件画面/继续敲 shell：卷根放一个 noshutdown.on 即可禁用。
    if (!efifs::Exists("noshutdown.on")) {
        eficon::WriteAscii("[Calcium-EFI] shutting down...\n");
        if (gRT && gRT->ResetSystem) {
            // EFI_RESET_TYPE：Cold=0 / Warm=1 / Shutdown=2 / PlatformSpecific=3
            typedef EFI_STATUS (EFIAPI *RESET_SYSTEM_T)(UINT32, EFI_STATUS, UINTN, VOID *);
            ((RESET_SYSTEM_T)gRT->ResetSystem)(2 /*EfiResetShutdown*/, EFI_SUCCESS, 0, 0);
        }
        // 能走到这里说明固件没关机（不支持或调用失败），退化为旧行为
        eficon::WriteAscii("[Calcium-EFI] shutdown unsupported, falling back.\n");
    }

    eficon::WriteAscii("\nPress any key to return to firmware...\n");
    {
        if (gST && gST->ConIn) {
            EFI_INPUT_KEY key;
            gBS->Stall(300000);
            gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        }
    }
    return EFI_SUCCESS;
}
