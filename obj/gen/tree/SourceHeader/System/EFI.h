// ============================================================================
//  System/EFI.h —— Calcium 的 UEFI 平台层
//  与 System/WIN.h 保持完全相同的导出符号，内部改用 EFI 引导服务实现。
//  网络模块（URLDown / URLDownloadToFile）在 EFI 下不可用，提供失败桩。
// ============================================================================
#pragma once

#include <string>
#include "efi_core.h"

// ---------------------------------------------------------------- 平台标识
const std::string Win32_kernel = "Win32";
const std::string Linux_kernel = "Linux";
const std::string Efi_kernel   = "UEFI";

// 卷根即工作目录：空串表示根
std::string _windows_userprofile = "";
const std::string _Build_Path = "Calcium";

// ---------------------------------------------------------------- 字符串工具
char *stringtochar;
inline char *stringToCharX(std::string tomessage)
{
    const int len = (int)tomessage.length();
    stringtochar = new char[len + 1];
    for (int i = 0; i < len; i++) stringtochar[i] = tomessage[i];
    stringtochar[len] = 0;
    return stringtochar;
}

const std::string pathsign = "\\";
std::string RunPlatfom = "UEFI (MSVC 64Bit)";
std::string _Run_SysKernel = Efi_kernel;

// ---------------------------------------------------------------- 控制台 / 睡眠
inline void sleepapi(int num) { efi_stall_us((unsigned long long)num * 1000000ULL); }
inline void sleepapi_ms(int ms) { efi_stall_us((unsigned long long)ms * 1000ULL); }

inline void cleanConsole() { eficon::Clear(); }

// ---------------------------------------------------------------- 目录
inline void foldercreateapi(std::string dir) { efifs::MakeDir(dir.c_str()); }

// 拼接子路径：避免 "dir/" + "\\" + name 拼出双分隔符
inline std::string JoinPath(const std::string &base, const char *name)
{
    std::string s = base;
    while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    if (!s.empty()) s += '\\';
    s += name;
    return s;
}

inline bool check_file_existenceA(const std::string &filename) { return efifs::Exists(filename.c_str()); }

// ---------------------------------------------------------------- 自身路径
// UEFI 下没有"进程自身路径"的概念。历史上这里返回 ""，若 calcium_settings.cfg
// 未写 $OriginWorkDir，{path} 就会是空串，脚本里 "{path}/xxx" 拼接后落空。
// 兜底返回卷根 "/"（会话注册时 $OriginWorkDir 仍会正常覆盖 _$GetSelfPath）。
inline std::string GetFilePath(void) { return "/"; }
inline std::string GetFileName(void)
{
    std::string s = efifs::SelfName;
    return s.empty() ? std::string("\\EFI\\BOOT\\Calcium.efi") : s;
}

// ---------------------------------------------------------------- 递归删目录
#include <vector>
#include <utility>
// 注意：必须先把目录条目全部收集完，再逐个删除。
// FAT 文件系统上"边枚举边删"会打乱目录枚举状态，可能漏删（未定义行为）。
int removeDirectoryAPIX(std::string dir)
{
    struct Ctx { std::vector<std::pair<std::string, bool> > items; } ctx;

    efifs::ListDir(dir.c_str(),
        [](const char *name, bool isDir, void *user) -> bool {
            Ctx *c = (Ctx *)user;
            c->items.push_back(std::make_pair(std::string(name), isDir));
            return true;
        },
        &ctx);

    for (size_t i = 0; i < ctx.items.size(); i++) {
        std::string full = JoinPath(dir, ctx.items[i].first.c_str());
        if (ctx.items[i].second) removeDirectoryAPIX(full);
        else efifs::Remove(full.c_str());
    }

    efifs::RemoveDir(dir.c_str());
    return 1;
}

// ---------------------------------------------------------------- 文件映射
std::string Process_cache, Process_cache_dir;
void _fileapi_write(std::string _fa_file, std::string _fa_info);
std::string p1_filepath = "";   // 刻意保持空串：不要把所有映射输出的前导 "/" 剥掉（与 WIN 版语义对齐由内核自己剥离）

struct MapCtx { std::string save; std::string base; };

inline std::string _map_slash(std::string s)
{
    for (size_t i = 0; i < s.size(); i++) if (s[i] == '\\') s[i] = '/';
    return s;
}

// 与 Windows 版 CreateFileMap_txt 一致：递归，只输出普通文件（剥离自身路径前缀）。
void CreateFileMap_txt(std::string savefile, std::string dir)
{
    MapCtx ctx; ctx.save = savefile; ctx.base = dir;
    efifs::ListDir(dir.c_str(),
        [](const char *name, bool isDir, void *user) -> bool {
            MapCtx *c = (MapCtx *)user;
            std::string full = JoinPath(c->base, name);
            if (isDir) {
                CreateFileMap_txt(c->save, full);          // 递归进子目录
            } else {
                std::string rel = full;
                // 对应 Windows 版的 ReplaceChar(Process_cache, p1_filepath, "")
                if (!p1_filepath.empty() && rel.size() > p1_filepath.size() &&
                    rel.compare(0, p1_filepath.size(), p1_filepath) == 0) {
                    rel = rel.substr(p1_filepath.size());
                }
                _fileapi_write(c->save, _map_slash(rel));
            }
            return true;
        },
        &ctx);
}

// 与 Windows 版 CreateDirMap_txt 一致：
//   * 只列当前这一层，不递归（原版里递归那行是注释掉的）
//   * 文件与目录都输出（ls 的语义），目录额外加 "/" 后缀
void CreateDirMap_txt(std::string savefile, std::string dir)
{
    MapCtx ctx; ctx.save = savefile; ctx.base = dir;
    efifs::ListDir(dir.c_str(),
        [](const char *name, bool isDir, void *user) -> bool {
            MapCtx *c = (MapCtx *)user;
            std::string out = _map_slash(JoinPath(c->base, name));
            _fileapi_write(c->save, isDir ? (out + "/") : out);
            return true;
        },
        &ctx);
}

// ---------------------------------------------------------------- 环境变量
char *envbuffer;
inline std::string _SystemAPI_getenv(std::string Environment)
{
    (void)Environment;
    return "Null.SystemAPI.getenv --> NullEnv";
}

// ---------------------------------------------------------------- 进程 / 声音（EFI 无对应能力）
inline bool _Execute_Admin(std::string File, std::string Args) { (void)File; (void)Args; return false; }
inline void __sysbeep_api(int precodeA, int precodeB) { (void)precodeA; (void)precodeB; }

// 无子进程能力：返回 -1 表示失败
inline int system(const char *) { return -1; }

// ---------------------------------------------------------------- 网络
// 2026-10-31: 已实现（src/efi_net.cpp，走固件 Ip4Config2 DHCP + EFI_HTTP）。
// 仅支持 http://；https 在引导服务环境无 TLS，仍返回失败。
#include "efi_net.h"
inline bool URLDown(std::string URL, std::string Save) { return efinet::UrlDownTo(URL.c_str(), Save.c_str()); }
inline std::string stringToLPCWSTR(std::string) { return ""; }

// ---------------------------------------------------------------- 文件复制（WIN.h 里是 Win32 CopyFile）
inline int CopyFile(const char *src, const char *dst, int failIfExists)
{
    (void)failIfExists;
    return efifs::CopyFile(src, dst) ? 1 : 0;
}
inline int CopyFile(const std::string &src, const std::string &dst, int failIfExists)
{
    return CopyFile(src.c_str(), dst.c_str(), failIfExists);
}
