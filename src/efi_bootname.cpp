// ============================================================================
//  efi_bootname.cpp —— 在 NVRAM 里注册自定义名字的持久 UEFI 启动项
//
//  写进 NVRAM 的是一条 EFI_LOAD_OPTION（变量名 Boot####，GUID 为全局变量 GUID）：
//
//      +0  UINT32  Attributes            (LOAD_OPTION_ACTIVE = 1)
//      +4  UINT16  FilePathListLength
//      +6  CHAR16  Description[]         <- 启动菜单里显示的就是这一串
//      +?  UINT8   FilePathList[]        <- 本映像自身的设备路径
//      +?  UINT8   OptionalData[]
//
//  所以"名字"完全由我们自己填，跟固件硬编码的 "UEFI OS" 无关。
// ============================================================================
#include "efi_core.h"
#include "efi_bootname.h"

namespace efi_bootname {

// ---------------------------------------------------------------- GUID
static EFI_GUID sLoadedImageGuid = { 0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
static EFI_GUID sGlobalVarGuid   = { 0x8BE4DF61, 0x93CA, 0x11D2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C } };

typedef EFI_STATUS (EFIAPI *GETVAR_T)(CHAR16 *, EFI_GUID *, UINT32 *, UINTN *, void *);
typedef EFI_STATUS (EFIAPI *SETVAR_T)(CHAR16 *, EFI_GUID *, UINT32, UINTN, void *);
typedef EFI_STATUS (EFIAPI *HANDLEPROTO_T)(EFI_HANDLE, EFI_GUID *, void **);
typedef EFI_STATUS (EFIAPI *ALLOCPOOL_T)(UINT32, UINTN, void **);
typedef EFI_STATUS (EFIAPI *FREEPOOL_T)(void *);

static const UINT32 kVarAttr    = 0x00000007u;   // NV | BOOTSERVICE_ACCESS | RUNTIME_ACCESS
static const UINT32 kLoadActive = 0x00000001u;   // LOAD_OPTION_ACTIVE
static const int    kMaxBootIdx = 0x0100;        // 扫 Boot0000..Boot00FF 找空位

// ---------------------------------------------------------------- 小工具
static void Out(const char *s) { eficon::WriteAscii(s); }

static void OutDec(unsigned long long v)
{
    char b[24];
    int n = 0;
    if (!v) { b[n++] = '0'; }
    while (v) { b[n++] = (char)('0' + (v % 10)); v /= 10; }
    char line[2] = { 0, 0 };
    while (n--) { line[0] = b[n]; Out(line); }
}

static void OutHex4(unsigned v)
{
    static const char *hex = "0123456789ABCDEF";
    char s[5];
    s[0] = hex[(v >> 12) & 0xF]; s[1] = hex[(v >> 8) & 0xF];
    s[2] = hex[(v >> 4) & 0xF];  s[3] = hex[v & 0xF]; s[4] = 0;
    Out(s);
}

// "Boot%04X" -> 9 个 CHAR16
static void MakeName(int idx, CHAR16 *out)
{
    static const CHAR16 hex[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F', 0 };
    out[0] = 'B'; out[1] = 'o'; out[2] = 'o'; out[3] = 't';
    out[4] = hex[(idx >> 12) & 0xF]; out[5] = hex[(idx >> 8) & 0xF];
    out[6] = hex[(idx >> 4) & 0xF];  out[7] = hex[idx & 0xF];
    out[8] = 0;
}

static CHAR16 *BootOrderName()
{
    static CHAR16 n[] = { 'B','o','o','t','O','r','d','e','r', 0 };
    return n;
}

// 设备路径总长（含 End of Entire Device Path）
static UINTN DpLength(const UINT8 *dp)
{
    UINTN total = 0;
    if (!dp) return 0;
    for (int guard = 0; guard < 256; ++guard) {
        UINT8  type = dp[0];
        UINT16 len  = (UINT16)(dp[2] | (dp[3] << 8));
        if (len < 4) return 0;                 // 路径损坏
        if (type == 0x7F) return total + len;  // End of entire device path
        total += len;
        dp += len;
    }
    return 0;
}

// 取 EFI_LOAD_OPTION 里的 Description 长度（字节，含结尾 0）；失败返回 0
static UINTN DescBytes(const UINT8 *var, UINTN varSize)
{
    if (varSize < 8) return 0;
    const CHAR16 *d = (const CHAR16 *)(var + 6);
    UINTN maxChars = (varSize - 6) / 2;
    for (UINTN i = 0; i < maxChars; ++i) {
        if (d[i] == 0) return (i + 1) * 2;
    }
    return 0;
}

static bool DescEquals(const UINT8 *var, UINTN varSize, const CHAR16 *want, UINTN wantChars)
{
    UINTN db = DescBytes(var, varSize);
    if (!db || db != wantChars * 2) return false;
    const CHAR16 *d = (const CHAR16 *)(var + 6);
    for (UINTN i = 0; i < wantChars; ++i)
        if (d[i] != want[i]) return false;
    return true;
}

static bool PathEquals(const UINT8 *var, UINTN varSize, const UINT8 *dp, UINTN dpLen)
{
    if (varSize < 8) return false;
    UINT16 pathLen = *(const UINT16 *)(var + 4);
    if (pathLen != dpLen) return false;
    UINTN db = DescBytes(var, varSize);
    if (!db || 6 + db + pathLen > varSize) return false;
    const UINT8 *p = var + 6 + db;
    for (UINTN i = 0; i < pathLen; ++i)
        if (p[i] != dp[i]) return false;
    return true;
}

// ---------------------------------------------------------------- 主体
bool Register(const char *asciiDesc)
{
    if (!gBS || !gRT || !gRT->GetVariable || !gRT->SetVariable) {
        Out("[bootname] RuntimeServices.GetVariable/SetVariable missing\n");
        return false;
    }

    GETVAR_T     getVar      = (GETVAR_T)gRT->GetVariable;
    SETVAR_T     setVar      = (SETVAR_T)gRT->SetVariable;
    HANDLEPROTO_T handleProto = (HANDLEPROTO_T)gBS->HandleProtocol;
    ALLOCPOOL_T  allocPool   = (ALLOCPOOL_T)gBS->AllocatePool;
    FREEPOOL_T   freePool    = (FREEPOOL_T)gBS->FreePool;

    // --- 1) 自身设备路径（由固件在 LoadImage 时填好，绝对准确）
    void *li = 0;
    if (!gImageHandle || EFI_ERROR(handleProto(gImageHandle, &sLoadedImageGuid, &li)) || !li) {
        Out("[bootname] LoadedImage protocol not available\n");
        return false;
    }
    EFI_LOADED_IMAGE_PROTOCOL *img = (EFI_LOADED_IMAGE_PROTOCOL *)li;
    UINT8 *selfPath = (UINT8 *)img->FilePath;
    UINTN  pathLen  = DpLength(selfPath);
    if (!pathLen) {
        Out("[bootname] cannot resolve own device path\n");
        return false;
    }

    // --- 2) 描述转 CHAR16
    CHAR16 desc[80];
    UINTN  n = 0;
    while (asciiDesc[n] && n < 78) { desc[n] = (CHAR16)(UINT8)asciiDesc[n]; ++n; }
    desc[n] = 0;
    UINTN descChars = n + 1;
    UINTN descBytes = descChars * 2;

    Out("[bootname] target name : ");
    Out(asciiDesc);
    Out("\n[bootname] self path   : ");
    OutDec(pathLen);
    Out(" bytes\n");

    // --- 3) 读 BootOrder
    UINT16 *order = 0;
    UINTN   orderCount = 0;
    {
        UINT32 attr = 0;
        UINTN  sz = 0;
        if (!EFI_ERROR(getVar(BootOrderName(), &sGlobalVarGuid, &attr, &sz, 0)) && sz >= 2) {
            void *b = 0;
            if (!EFI_ERROR(allocPool(4, sz, &b))) {
                UINTN sz2 = sz;
                if (!EFI_ERROR(getVar(BootOrderName(), &sGlobalVarGuid, &attr, &sz2, b))) {
                    order = (UINT16 *)b;
                    orderCount = sz2 / 2;
                } else {
                    freePool(b);
                }
            }
        }
    }

    // --- 4) 已有同名项？
    for (UINTN i = 0; i < orderCount; ++i) {
        int idx = (int)order[i];
        CHAR16 nm[9]; MakeName(idx, nm);
        UINT32 attr = 0; UINTN sz = 0;
        if (EFI_ERROR(getVar(nm, &sGlobalVarGuid, &attr, &sz, 0)) || sz < 8) continue;
        void *vb = 0;
        if (EFI_ERROR(allocPool(4, sz, &vb))) continue;
        UINTN sz2 = sz;
        bool sameName = false, samePath = false;
        if (!EFI_ERROR(getVar(nm, &sGlobalVarGuid, &attr, &sz2, vb))) {
            sameName = DescEquals((UINT8 *)vb, sz2, desc, descChars);
            samePath = sameName && PathEquals((UINT8 *)vb, sz2, selfPath, pathLen);
        }
        freePool(vb);

        if (samePath) {
            Out("[bootname] already registered: Boot");
            OutHex4((unsigned)idx);
            Out(" (nothing to do)\n");
            if (order) freePool(order);
            return true;
        }
        if (sameName) {
            // 同名但路径不同（典型：换了 USB 口 / 换了一台机器的同一个 U 盘）
            Out("[bootname] same name exists but points elsewhere: Boot");
            OutHex4((unsigned)idx);
            Out(" -> creating a new entry\n");
        }
    }

    // --- 5) 找空闲编号
    int freeIdx = -1;
    for (int c = 0; c < kMaxBootIdx; ++c) {
        bool used = false;
        for (UINTN i = 0; i < orderCount; ++i) {
            if (order[i] == (UINT16)c) { used = true; break; }
        }
        if (used) continue;
        CHAR16 nm[9]; MakeName(c, nm);
        UINT32 attr = 0; UINTN sz = 0;
        if (EFI_ERROR(getVar(nm, &sGlobalVarGuid, &attr, &sz, 0))) { freeIdx = c; break; }
    }
    if (freeIdx < 0) {
        Out("[bootname] no free Boot#### slot\n");
        if (order) freePool(order);
        return false;
    }

    // --- 6) 组装 EFI_LOAD_OPTION 并写变量
    UINTN total = 6 + descBytes + pathLen;
    void *nb = 0;
    if (EFI_ERROR(allocPool(4, total, &nb))) {
        Out("[bootname] AllocatePool failed\n");
        if (order) freePool(order);
        return false;
    }
    UINT8 *p = (UINT8 *)nb;
    *(UINT32 *)(p + 0) = kLoadActive;
    *(UINT16 *)(p + 4) = (UINT16)pathLen;
    for (UINTN i = 0; i < descChars; ++i) ((CHAR16 *)(p + 6))[i] = desc[i];
    for (UINTN i = 0; i < pathLen; ++i) p[6 + descBytes + i] = selfPath[i];

    CHAR16 newName[9]; MakeName(freeIdx, newName);
    EFI_STATUS st = setVar(newName, &sGlobalVarGuid, kVarAttr, total, nb);
    freePool(nb);
    if (EFI_ERROR(st)) {
        Out("[bootname] SetVariable(Boot####) FAILED, status=0x");
        OutHex4((unsigned)(st & 0xFFFF));
        Out("\n[bootname] firmware may reject NVRAM writes (read-only var store?)\n");
        if (order) freePool(order);
        return false;
    }
    Out("[bootname] created Boot");
    OutHex4((unsigned)freeIdx);
    Out(" = \"");
    Out(asciiDesc);
    Out("\"\n");

    // --- 7) 追加到 BootOrder（放末尾，不动原有顺序）
    {
        UINTN newCount = orderCount + 1;
        void *nob = 0;
        if (!EFI_ERROR(allocPool(4, newCount * 2, &nob))) {
            UINT16 *no = (UINT16 *)nob;
            for (UINTN i = 0; i < orderCount; ++i) no[i] = order[i];
            no[orderCount] = (UINT16)freeIdx;
            st = setVar(BootOrderName(), &sGlobalVarGuid, kVarAttr, newCount * 2, nob);
            freePool(nob);
            if (EFI_ERROR(st)) {
                Out("[bootname] WARNING: BootOrder update failed (entry exists but may not be listed)\n");
            } else {
                Out("[bootname] BootOrder updated, position ");
                OutDec(orderCount + 1);
                Out(" of ");
                OutDec(newCount);
                Out("\n");
            }
        }
    }

    if (order) freePool(order);
    Out("[bootname] done. 进 BIOS 把这一项调到第一位即可默认启动\n");
    return true;
}

// ---------------------------------------------------------------- 诊断
void Dump()
{
    if (!gRT || !gRT->GetVariable) return;
    GETVAR_T   getVar   = (GETVAR_T)gRT->GetVariable;
    ALLOCPOOL_T allocPool = (ALLOCPOOL_T)gBS->AllocatePool;
    FREEPOOL_T  freePool  = (FREEPOOL_T)gBS->FreePool;

    UINT32 attr = 0; UINTN sz = 0;
    if (EFI_ERROR(getVar(BootOrderName(), &sGlobalVarGuid, &attr, &sz, 0)) || sz < 2) {
        Out("[bootname] BootOrder empty\n");
        return;
    }
    void *b = 0;
    if (EFI_ERROR(allocPool(4, sz, &b))) return;
    UINTN sz2 = sz;
    if (EFI_ERROR(getVar(BootOrderName(), &sGlobalVarGuid, &attr, &sz2, b))) { freePool(b); return; }

    UINT16 *order = (UINT16 *)b;
    UINTN count = sz2 / 2;
    Out("[bootname] BootOrder:\n");
    for (UINTN i = 0; i < count; ++i) {
        CHAR16 nm[9]; MakeName((int)order[i], nm);
        UINT32 a2 = 0; UINTN vs = 0;
        Out("  ");
        OutHex4((unsigned)order[i]);
        Out("  ");
        if (EFI_ERROR(getVar(nm, &sGlobalVarGuid, &a2, &vs, 0)) || vs < 8) { Out("(missing)\n"); continue; }
        void *vb = 0;
        if (EFI_ERROR(allocPool(4, vs, &vb))) { Out("(alloc fail)\n"); continue; }
        UINTN vs2 = vs;
        if (!EFI_ERROR(getVar(nm, &sGlobalVarGuid, &a2, &vs2, vb))) {
            const CHAR16 *d = (const CHAR16 *)((UINT8 *)vb + 6);
            char line[2] = { 0, 0 };
            for (UINTN k = 0; k < (vs2 - 6) / 2 && d[k]; ++k) {
                line[0] = (d[k] < 0x80) ? (char)d[k] : '?';
                Out(line);
            }
        }
        freePool(vb);
        Out("\n");
    }
    freePool(b);
}

} // namespace efi_bootname
