// ============================================================================
//  efi_net.h —— EFI 网络层（HTTP 客户端）
//  依赖固件的 Ip4Config2（DHCP）+ HTTP（HttpDxe，自带 DNS4 解析与 TCP4 连接）
// ============================================================================
#pragma once

namespace efinet {
    // 同步 HTTP GET 下载到卷内文件（savepath 相对卷根）。
    // 仅支持 http://（TLS 在 UEFI 引导服务环境里不可行）；跟随最多 3 次重定向。
    // 成功返回 true。无网卡 / 无协议 / 任何失败都返回 false，不污染控制台。
    bool UrlDownTo(const char *url, const char *savepath);
}
