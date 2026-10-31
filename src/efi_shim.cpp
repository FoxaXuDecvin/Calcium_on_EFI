// ============================================================================
//  efi_shim.cpp —— 影子标准库的平台底板（控制台 / 文件）
// ============================================================================
#include "efi_core.h"
#include "efi_shim_common.h"

namespace efi {

void out(const char *s, size_t n) { eficon::WriteUtf8(s, (unsigned long long)n); }

bool read_line(std::string &s)
{
    char buf[1024];
    if (!eficon::ReadLine(buf, sizeof(buf))) return false;
    s = buf;
    return true;
}

// ---------------------------------------------------------------------------
//  单文件读缓存
//  为什么需要：Calcium 的 getdocmax() 用"逐行探测到 overline"的方式求行数，
//  每个文件会被完整读 (行数+1) 次，且每次还要先 CountLines 再读一遍，
//  再加上 _existfile() 的 open。真实 ROM 里 ls 一个 64 条目的目录要做
//  上百次整文件读取，全走固件路径解析要 60 秒以上（表现为"ls 没反应"）。
//  这里缓存"最近一次成功读取"，任何写入/删除都失效。
// ---------------------------------------------------------------------------
static std::string s_cachePath;
static std::string s_cacheData;
static bool s_cacheValid = false;

void invalidate_file_cache()
{
    s_cacheValid = false;
    s_cachePath.clear();
    s_cacheData.clear();
}

bool file_read_all(const char *path, std::string &outdata)
{
    if (s_cacheValid && path && s_cachePath == path) {
        outdata = s_cacheData;
        return true;
    }

    char *data = 0;
    unsigned int len = 0;
    if (!efifs::ReadAll(path, data, len)) return false;
    if (data && len) outdata.assign(data, len);
    else outdata.clear();
    efifs::FreeBuf(data);

    s_cachePath = path ? path : "";
    s_cacheData = outdata;
    s_cacheValid = true;
    return true;
}

bool file_write_all(const char *path, const std::string &data)
{
    invalidate_file_cache();
    return efifs::WriteAll(path, data.data(), (unsigned int)data.size());
}

// 追加写：Calcium 的 _fileapi_write 是 open(app)+<<+close，
// EFI 下每次 close 都真落盘，若每次都整文件重写会退化成 O(n^2)
// （64 行的目录列表要几十秒，表现为 ls "没反应"）。
bool file_append(const char *path, const std::string &data)
{
    invalidate_file_cache();
    return efifs::AppendAll(path, data.data(), (unsigned int)data.size());
}

bool file_exists(const char *path) { return efifs::Exists(path); }

} // namespace efi
