// ============================================================================
//  efi_net.cpp —— EFI 网络层实现：Ip4Config2 DHCP + EFI_DNS4 + EFI_TCP4 裸 HTTP
//
//  放弃 EFI_HTTP 协议路线的原因：VMware 的 HttpDxe 对 Request() 一律返回
//  EFI_INVALID_PARAMETER（URL / token / 显式地址 / Host 头等 6 种变体全试过，
//  连 IP 字面量 URL 都被拒），无法作为依赖。TCP4 是 PXE/iSCSI 打底的同层能力，
//  行为与 EDK2 规范一致。
//
//  结构体布局全部与 EDK2 MdePkg 原文核对（本地留档 build/Tcp4.h、build/Dns4.h、
//  build/Ip4Config2.h）。Ip4Config2DataType: InterfaceInfo=0, Policy=1。
// ============================================================================
#include "efi_core.h"
#include "efi_net.h"
#include <string>

// ---------------------------------------------------------------- GUID（权威值 = build/mdepkg.dec）
static EFI_GUID GUID_IP4CONFIG2 = { 0x5b446ed1, 0xe30b, 0x4faa, { 0x87, 0x1a, 0x36, 0x54, 0xec, 0xa3, 0x60, 0x80 } };
static EFI_GUID GUID_TCP4_SB    = { 0x00720665, 0x67eb, 0x4a99, { 0xba, 0xf7, 0xd3, 0xc3, 0x3a, 0x1c, 0x7c, 0xc9 } };
static EFI_GUID GUID_TCP4       = { 0x65530bc7, 0xa359, 0x410f, { 0xb0, 0x10, 0x5a, 0xad, 0xc7, 0xec, 0x2b, 0x62 } };
static EFI_GUID GUID_DNS4_SB    = { 0xb625b186, 0xe063, 0x44f7, { 0x89, 0x05, 0x6a, 0x74, 0xdc, 0x6f, 0x52, 0xb4 } };
static EFI_GUID GUID_DNS4       = { 0xae3d28cc, 0xe05b, 0x4fa1, { 0xa0, 0x11, 0x7e, 0xb5, 0x5a, 0x3f, 0x14, 0x01 } };

// ---------------------------------------------------------------- 基础结构（照抄 EDK2，布局敏感）
typedef struct { UINT8 Addr[4]; } EFI_IPv4_ADDRESS;

// ---- Ip4Config2
typedef struct {
    EFI_STATUS (EFIAPI *SetData)(VOID *This, UINT32 DataType, UINTN DataSize, VOID *Data);
    EFI_STATUS (EFIAPI *GetData)(VOID *This, UINT32 DataType, UINTN *DataSize, VOID *Data);
    EFI_STATUS (EFIAPI *RegisterDataNotify)(VOID *This, UINT32 DataType, EFI_EVENT Event);
    EFI_STATUS (EFIAPI *UnregisterDataNotify)(VOID *This, UINT32 DataType, EFI_EVENT Event);
} EFI_IP4_CONFIG2_PROTOCOL;

typedef struct {
    CHAR16 Name[32];
    UINT8 IfType; UINT32 HwAddressSize; UINT8 HwAddress[32];
    EFI_IPv4_ADDRESS StationAddress; EFI_IPv4_ADDRESS SubnetMask;
    UINT32 RouteTableSize; VOID *RouteTable;
} IP4CFG_INTERFACE_INFO;

// ---- DNS4
typedef struct {
    UINTN   DnsServerListCount;      // 0 = 让驱动自己向 DHCP 要 DNS 服务器
    EFI_IPv4_ADDRESS *DnsServerList;
    BOOLEAN UseDefaultSetting;       // TRUE = 用默认 IP/掩码/路由
    BOOLEAN EnableDnsCache;
    UINT8   Protocol;                // 17 = UDP
    EFI_IPv4_ADDRESS StationIp;
    EFI_IPv4_ADDRESS SubnetMask;
    UINT16  LocalPort;
    UINT32  RetryCount;
    UINT32  RetryInterval;
} DNS4_CONFIG_DATA;

typedef struct { UINT32 IpCount; EFI_IPv4_ADDRESS *IpList; } DNS_H2A_DATA;

typedef struct {
    EFI_EVENT Event;
    EFI_STATUS Status;
    UINT32 RetryCount;
    UINT32 RetryInterval;
    union { DNS_H2A_DATA *H2AData; VOID *A2HData; VOID *GLookupData; } RspData;
} DNS4_COMPLETION_TOKEN;

typedef struct {
    EFI_STATUS (EFIAPI *GetModeData)(VOID *, VOID *);
    EFI_STATUS (EFIAPI *Configure)(VOID *, DNS4_CONFIG_DATA *);
    EFI_STATUS (EFIAPI *HostNameToIp)(VOID *, CHAR16 *, DNS4_COMPLETION_TOKEN *);
    VOID *IpToHostName; VOID *GeneralLookUp; VOID *UpdateDnsCache;
    VOID *Poll; VOID *Cancel;
} EFI_DNS4_PROTOCOL;

// ---- TCP4
typedef struct {
    BOOLEAN UseDefaultAddress;
    EFI_IPv4_ADDRESS StationAddress; EFI_IPv4_ADDRESS SubnetMask;
    UINT16 StationPort;
    EFI_IPv4_ADDRESS RemoteAddress;
    UINT16 RemotePort;
    BOOLEAN ActiveFlag;
} TCP4_ACCESS_POINT;

typedef struct {
    UINT8 TypeOfService; UINT8 TimeToLive;
    TCP4_ACCESS_POINT AccessPoint;
    VOID *ControlOption;
} TCP4_CONFIG_DATA;

typedef struct { UINT32 FragmentLength; VOID *FragmentBuffer; } TCP4_FRAGMENT;
typedef struct { BOOLEAN UrgentFlag; UINT32 DataLength; UINT32 FragmentCount; TCP4_FRAGMENT FragmentTable[1]; } TCP4_RX_DATA;
typedef struct { BOOLEAN Push; BOOLEAN Urgent; UINT32 DataLength; UINT32 FragmentCount; TCP4_FRAGMENT FragmentTable[1]; } TCP4_TX_DATA;
typedef struct { EFI_EVENT Event; EFI_STATUS Status; } TCP4_COMPLETION_TOKEN;
typedef struct { TCP4_COMPLETION_TOKEN CompletionToken; } TCP4_CONN_TOKEN;
typedef struct {
    TCP4_COMPLETION_TOKEN CompletionToken;
    union { TCP4_RX_DATA *RxData; TCP4_TX_DATA *TxData; } Packet;
} TCP4_IO_TOKEN;

typedef struct {
    EFI_STATUS (EFIAPI *GetModeData)(VOID *, VOID *, VOID *, VOID *, VOID *, VOID *);
    EFI_STATUS (EFIAPI *Configure)(VOID *, TCP4_CONFIG_DATA *);
    VOID *Routes;
    EFI_STATUS (EFIAPI *Connect)(VOID *, TCP4_CONN_TOKEN *);
    VOID *Accept;
    EFI_STATUS (EFIAPI *Transmit)(VOID *, TCP4_IO_TOKEN *);
    EFI_STATUS (EFIAPI *Receive)(VOID *, TCP4_IO_TOKEN *);
    VOID *Close; VOID *Cancel; VOID *Poll;
} EFI_TCP4_PROTOCOL;

typedef struct {
    EFI_STATUS (EFIAPI *CreateChild)(VOID *This, EFI_HANDLE *ChildHandle);
    EFI_STATUS (EFIAPI *DestroyChild)(VOID *This, EFI_HANDLE ChildHandle);
} EFI_SERVICE_BINDING;

// ---- 引导服务里还是 void* 的成员，这里按真实布局补类型
typedef EFI_STATUS (EFIAPI *CreateEventT)(UINT32 Type, EFI_TPL NotifyTpl,
                                          VOID (EFIAPI *NotifyFunction)(VOID *, VOID *),
                                          VOID *NotifyContext, EFI_EVENT *Event);
typedef EFI_STATUS (EFIAPI *CloseEventT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *LocateHBT)(int SearchType, EFI_GUID *Protocol, VOID *SearchKey,
                                       UINTN *NoHandles, EFI_HANDLE **Buffer);
#define EVT_NOTIFY_SIGNAL 0x00000200
#define TPL_CALLBACK      8

// 状态码（ENCODE_ERROR(n)）
#define ST_NOT_READY     0x8000000000000006ULL
#define ST_BUF_TOO_SMALL 0x8000000000000005ULL

// ---------------------------------------------------------------- 小工具
static void EFIAPI NetNoopNotify(VOID *Event, VOID *Ctx) { (void)Event; (void)Ctx; }

static void *net_alloc(UINTN n) { VOID *p = 0; if (EFI_ERROR(gBS->AllocatePool(EfiPoolLoaderData, n, &p))) return 0; return p; }

static bool wait_token(EFI_STATUS *st, UINT32 ms)
{
    UINT64 loops = ms;               // 每次 Stall 1ms；协议栈在自己的定时器事件里推进
    while (*st == ST_NOT_READY) {
        if (loops-- == 0) return false;
        gBS->Stall(1000);
    }
    return true;
}

static CHAR16 *ascii_to_wide(const char *s)
{
    UINTN n = 0; while (s[n]) n++;
    CHAR16 *w = (CHAR16 *)net_alloc((n + 1) * sizeof(CHAR16));
    if (!w) return 0;
    for (UINTN i = 0; i <= n; i++) w[i] = (CHAR16)(unsigned char)s[i];
    return w;
}

static UINT8 g_StationIp[4];
static UINT8 g_Subnet[4];

static void Ip4Dotted(const UINT8 *ip, char *out)   // out 至少 16 字节
{
    int p = 0;
    for (int i = 0; i < 4; i++) {
        UINT8 v = ip[i];
        if (v >= 100) out[p++] = (char)('0' + v / 100);
        if (v >= 10)  out[p++] = (char)('0' + (v / 10) % 10);
        out[p++] = (char)('0' + v % 10);
        if (i < 3) out[p++] = '.';
    }
    out[p] = 0;
}

// ---------------------------------------------------------------- URL 解析
struct ParsedUrl { std::string host, port, path; bool isIpLiteral; UINT8 ip[4]; };

static bool ParseHttpUrl(const char *url, ParsedUrl &out)
{
    std::string s(url);
    if (s.compare(0, 7, "http://") == 0) s = s.substr(7);
    else return false;                       // https（无 TLS）及其它 scheme 一律拒绝

    size_t slash = s.find('/');
    std::string auth = (slash == std::string::npos) ? s : s.substr(0, slash);
    out.path = (slash == std::string::npos) ? "/" : s.substr(slash);

    size_t colon = auth.rfind(':');
    if (colon != std::string::npos) {
        out.host = auth.substr(0, colon);
        out.port = auth.substr(colon + 1);
    } else {
        out.host = auth;
        out.port = "80";
    }
    if (out.host.empty()) return false;

    // IP 字面量检测（a.b.c.d）
    out.isIpLiteral = false;
    {
        int parts = 0; size_t i = 0;
        UINT8 bytes[4] = { 0, 0, 0, 0 };
        while (i < out.host.size() && parts < 4) {
            if (out.host[i] < '0' || out.host[i] > '9') break;
            int v = 0, digits = 0;
            while (i < out.host.size() && out.host[i] >= '0' && out.host[i] <= '9') {
                v = v * 10 + (out.host[i] - '0'); i++; digits++;
            }
            if (!digits || v > 255) break;
            bytes[parts++] = (UINT8)v;
            if (i < out.host.size() && out.host[i] == '.') i++;
        }
        if (parts == 4 && i == out.host.size()) {
            out.isIpLiteral = true;
            for (int k = 0; k < 4; k++) out.ip[k] = bytes[k];
        }
    }
    return true;
}

// ---------------------------------------------------------------- DHCP（Ip4Config2）
static bool EnsureIPv4Address()
{
    UINTN hn = 0; EFI_HANDLE *handles = 0;
    LocateHBT Locate = (LocateHBT)gBS->LocateHandleBuffer;
    if (EFI_ERROR(Locate(2 /*ByProtocol*/, &GUID_IP4CONFIG2, 0, &hn, &handles)) || !hn) return false;

    EFI_IP4_CONFIG2_PROTOCOL *cfg = 0;
    gBS->HandleProtocol(handles[0], &GUID_IP4CONFIG2, (VOID **)&cfg);
    gBS->FreePool(handles);
    if (!cfg) return false;

    // 先看有没有现成地址（静态/默认配置），有就直接用
    {
        UINTN sz = sizeof(IP4CFG_INTERFACE_INFO) + 4096;
        IP4CFG_INTERFACE_INFO *info = (IP4CFG_INTERFACE_INFO *)net_alloc(sz);
        if (info) {
            EFI_STATUS st = cfg->GetData(cfg, 0 /*InterfaceInfo*/, &sz, info);
            if (!EFI_ERROR(st) && info->StationAddress.Addr[0]) {
                for (int q = 0; q < 4; q++) { g_StationIp[q] = info->StationAddress.Addr[q]; g_Subnet[q] = info->SubnetMask.Addr[q]; }
                gBS->FreePool(info);
                return true;
            }
            gBS->FreePool(info);
        }
    }

    // 切 DHCP 策略，轮询拿地址（VMware NAT / QEMU slirp 都有 DHCP）
    UINT32 pol = 1;   // Ip4Config2PolicyDhcp
    if (EFI_ERROR(cfg->SetData(cfg, 1 /*Policy*/, sizeof(UINT32), &pol))) return false;

    for (int i = 0; i < 200; i++) {                  // 最多 20s
        UINTN sz = 0;
        EFI_STATUS st = cfg->GetData(cfg, 0, &sz, 0);
        if (st == ST_BUF_TOO_SMALL && sz >= sizeof(IP4CFG_INTERFACE_INFO)) {
            IP4CFG_INTERFACE_INFO *info = (IP4CFG_INTERFACE_INFO *)net_alloc(sz);
            if (!info) return false;
            st = cfg->GetData(cfg, 0, &sz, info);
            bool ok = !EFI_ERROR(st) && info->StationAddress.Addr[0];
            if (ok) {
                for (int q = 0; q < 4; q++) { g_StationIp[q] = info->StationAddress.Addr[q]; g_Subnet[q] = info->SubnetMask.Addr[q]; }
                char tb[32], ipb[16];
                Ip4Dotted(g_StationIp, ipb);
                int n = 0;
                const char *m = "net: dhcp ip ";
                while (m[n]) { tb[n] = m[n]; n++; }
                for (int k = 0; ipb[k] && n < 30; k++) tb[n++] = ipb[k];
                tb[n] = 0;
                eficon::SerialAscii(tb);
            }
            gBS->FreePool(info);
            if (ok) return true;
        }
        gBS->Stall(100000);                          // 100ms
    }
    return false;
}

// ---------------------------------------------------------------- DNS（DNS4；服务器由驱动从 DHCP 自动获取）
static bool DnsResolve(const char *host, UINT8 outIp[4])
{
    UINTN hn = 0; EFI_HANDLE *handles = 0;
    LocateHBT Locate = (LocateHBT)gBS->LocateHandleBuffer;
    if (EFI_ERROR(Locate(2, &GUID_DNS4_SB, 0, &hn, &handles)) || !hn) {
        eficon::SerialAscii("net: no DNS4 service binding");
        return false;
    }

    EFI_SERVICE_BINDING *sb = 0;
    gBS->HandleProtocol(handles[0], &GUID_DNS4_SB, (VOID **)&sb);
    gBS->FreePool(handles);
    if (!sb) return false;

    EFI_HANDLE child = 0;
    EFI_DNS4_PROTOCOL *dns = 0;
    EFI_EVENT ev = 0;
    DNS4_COMPLETION_TOKEN token;
    bool ok = false;

    if (EFI_ERROR(sb->CreateChild(sb, &child)) || !child) return false;
    gBS->HandleProtocol(child, &GUID_DNS4, (VOID **)&dns);
    if (!dns) goto cleanup;

    {
        DNS4_CONFIG_DATA cfg;
        cfg.DnsServerListCount = 0;              // 驱动自动向 DHCP 要 DNS 服务器
        cfg.DnsServerList = 0;
        cfg.UseDefaultSetting = 1;
        cfg.EnableDnsCache = 1;
        cfg.Protocol = 17;                       // UDP
        for (int i = 0; i < 4; i++) { cfg.StationIp.Addr[i] = 0; cfg.SubnetMask.Addr[i] = 0; }
        cfg.LocalPort = 0;
        cfg.RetryCount = 3;
        cfg.RetryInterval = 2;
        if (EFI_ERROR(dns->Configure(dns, &cfg))) {
            eficon::SerialAscii("net: dns configure failed");
            goto cleanup;
        }

        CreateEventT CreateEvent = (CreateEventT)gBS->CreateEvent;
        if (EFI_ERROR(CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NetNoopNotify, 0, &ev))) goto cleanup;

        CHAR16 *whost = ascii_to_wide(host);
        if (!whost) goto cleanup;

        token.Event = ev; token.Status = ST_NOT_READY;
        token.RetryCount = 0; token.RetryInterval = 0; token.RspData.H2AData = 0;
        if (EFI_ERROR(dns->HostNameToIp(dns, whost, &token))) {
            eficon::SerialAscii("net: dns query call failed");
            gBS->FreePool(whost);
            goto cleanup;
        }
        if (!wait_token(&token.Status, 20000)) {
            eficon::SerialAscii("net: dns query timeout");
            gBS->FreePool(whost);
            goto cleanup;
        }
        gBS->FreePool(whost);
        if (EFI_ERROR(token.Status) || !token.RspData.H2AData || token.RspData.H2AData->IpCount == 0) {
            eficon::SerialAscii("net: dns query failed (status/nx)");
            goto cleanup;
        }
        for (int i = 0; i < 4; i++) outIp[i] = token.RspData.H2AData->IpList[0].Addr[i];
        ok = true;
    }

cleanup:
    if (ev) { CloseEventT CloseEvent = (CloseEventT)gBS->CloseEvent; CloseEvent(ev); }
    sb->DestroyChild(sb, child);
    return ok;
}

// ---------------------------------------------------------------- TCP4 单次 GET
// 输入：ip/port/host(Host 头用)/path。输出：完整原始响应。连接用 Connection: close。
static bool TcpHttpGet(const UINT8 *ip, UINT16 port, const char *host, const char *path,
                       std::string &response)
{
    UINTN hn = 0; EFI_HANDLE *handles = 0;
    LocateHBT Locate = (LocateHBT)gBS->LocateHandleBuffer;
    if (EFI_ERROR(Locate(2, &GUID_TCP4_SB, 0, &hn, &handles)) || !hn) {
        eficon::SerialAscii("net: no TCP4 service binding");
        return false;
    }

    EFI_SERVICE_BINDING *sb = 0;
    gBS->HandleProtocol(handles[0], &GUID_TCP4_SB, (VOID **)&sb);
    gBS->FreePool(handles);
    if (!sb) return false;

    EFI_HANDLE child = 0;
    EFI_TCP4_PROTOCOL *tcp = 0;
    EFI_EVENT ev = 0;
    bool ok = false;

    if (EFI_ERROR(sb->CreateChild(sb, &child)) || !child) return false;
    gBS->HandleProtocol(child, &GUID_TCP4, (VOID **)&tcp);
    if (!tcp) goto cleanup;

    {
        CreateEventT CreateEvent = (CreateEventT)gBS->CreateEvent;
        if (EFI_ERROR(CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NetNoopNotify, 0, &ev))) goto cleanup;

        TCP4_CONFIG_DATA cfg;
        cfg.TypeOfService = 0; cfg.TimeToLive = 64; cfg.ControlOption = 0;
        cfg.AccessPoint.UseDefaultAddress = 1;
        for (int i = 0; i < 4; i++) { cfg.AccessPoint.StationAddress.Addr[i] = 0; cfg.AccessPoint.SubnetMask.Addr[i] = 0; }
        cfg.AccessPoint.StationPort = 0;
        for (int i = 0; i < 4; i++) cfg.AccessPoint.RemoteAddress.Addr[i] = ip[i];
        cfg.AccessPoint.RemotePort = port;
        cfg.AccessPoint.ActiveFlag = 1;
        EFI_STATUS cst = tcp->Configure(tcp, &cfg);
        if (EFI_ERROR(cst)) {
            if (cst == ST_NOT_READY) {           // 默认地址尚未生效，稍等重试
                for (int i = 0; i < 50 && cst == ST_NOT_READY; i++) { gBS->Stall(100000); cst = tcp->Configure(tcp, &cfg); }
            }
            if (EFI_ERROR(cst)) {
                char tb[32]; UINT64 v = (UINT64)cst;
                tb[0]='n'; tb[1]='e'; tb[2]='t'; tb[3]=':'; tb[4]=' '; tb[5]='t'; tb[6]='c'; tb[7]='p'; tb[8]=' '; tb[9]='c'; tb[10]='f'; tb[11]='g'; tb[12]=' ';
                for (int q = 3; q >= 0; q--) { tb[13 + (3 - q)] = "0123456789ABCDEF"[(v >> (q * 4)) & 0xF]; }
                tb[17] = 0;
                eficon::SerialAscii(tb);
                goto cleanup;
            }
        }

        // ---- 连接
        TCP4_CONN_TOKEN conn; conn.CompletionToken.Event = ev; conn.CompletionToken.Status = ST_NOT_READY;
        if (EFI_ERROR(tcp->Connect(tcp, &conn))) { eficon::SerialAscii("net: connect call failed"); goto cleanup; }
        if (!wait_token(&conn.CompletionToken.Status, 20000)) { eficon::SerialAscii("net: connect timeout"); goto cleanup; }
        if (EFI_ERROR(conn.CompletionToken.Status)) { eficon::SerialAscii("net: connect failed"); goto cleanup; }

        // ---- 发请求
        std::string req = std::string("GET ") + path + " HTTP/1.1\r\nHost: " + host +
                          "\r\nUser-Agent: Calcium-EFI\r\nAccept: */*\r\nConnection: close\r\n\r\n";
        TCP4_TX_DATA tx; tx.Push = 1; tx.Urgent = 0;
        tx.DataLength = (UINT32)req.size(); tx.FragmentCount = 1;
        tx.FragmentTable[0].FragmentLength = (UINT32)req.size();
        tx.FragmentTable[0].FragmentBuffer = (VOID *)req.data();
        TCP4_IO_TOKEN iot; iot.CompletionToken.Event = ev; iot.CompletionToken.Status = ST_NOT_READY;
        iot.Packet.TxData = &tx;
        if (EFI_ERROR(tcp->Transmit(tcp, &iot))) { eficon::SerialAscii("net: transmit call failed"); goto cleanup; }
        if (!wait_token(&iot.CompletionToken.Status, 20000)) { eficon::SerialAscii("net: transmit timeout"); goto cleanup; }
        if (EFI_ERROR(iot.CompletionToken.Status)) { eficon::SerialAscii("net: transmit failed"); goto cleanup; }

        // ---- 收流（Connection: close；错误 = 对端关闭 = EOF）
        char *buf = (char *)net_alloc(16384);
        if (!buf) goto cleanup;
        for (;;) {
            TCP4_RX_DATA rx; rx.UrgentFlag = 0;
            rx.DataLength = 16384; rx.FragmentCount = 1;
            rx.FragmentTable[0].FragmentLength = 16384;
            rx.FragmentTable[0].FragmentBuffer = buf;
            TCP4_IO_TOKEN rt; rt.CompletionToken.Event = ev; rt.CompletionToken.Status = ST_NOT_READY;
            rt.Packet.RxData = &rx;
            EFI_STATUS rst = tcp->Receive(tcp, &rt);
            if (EFI_ERROR(rst)) {                      // 队列拒收等同步错误
                if (response.empty()) eficon::SerialAscii("net: receive call failed");
                break;
            }
            if (!wait_token(&rt.CompletionToken.Status, 60000)) break;   // 60s 无数据 = 超时收尾
            if (EFI_ERROR(rt.CompletionToken.Status)) break;             // FIN/RESET 等 = EOF
            if (rx.DataLength == 0) break;
            response.append(buf, rx.DataLength);
            if (response.size() > (64ULL << 20)) break;                  // 64MB 保险丝
        }
        gBS->FreePool(buf);
        ok = !response.empty();
    }

cleanup:
    if (ev) { CloseEventT CloseEvent = (CloseEventT)gBS->CloseEvent; CloseEvent(ev); }
    sb->DestroyChild(sb, child);
    return ok;
}

// ---------------------------------------------------------------- HTTP 响应解析
struct HttpResponse { UINT32 code; std::string location; bool chunked; UINTN contentLen; std::string body; };

static std::string Dechunk(const std::string &in);   // 定义在下方

static bool ParseResponse(const std::string &raw, HttpResponse &out)
{
    size_t he = raw.find("\r\n\r\n");
    if (he == std::string::npos) return false;
    std::string head = raw.substr(0, he);
    out.body = raw.substr(he + 4);

    // 状态行：HTTP/1.x CODE
    if (head.size() < 12) return false;
    int v = 0;
    for (int i = 9; i < 12; i++) {
        if (head[i] < '0' || head[i] > '9') return false;
        v = v * 10 + (head[i] - '0');
    }
    out.code = (UINT32)v;

    out.chunked = false; out.contentLen = 0;
    size_t p = head.find("\r\n");
    while (p != std::string::npos) {
        size_t e = head.find("\r\n", p + 2);
        std::string line = head.substr(p + 2, (e == std::string::npos ? head.size() : e) - (p + 2));
        if (!line.empty()) {
            size_t c = line.find(':');
            if (c != std::string::npos) {
                std::string fn = line.substr(0, c), fv = line.substr(c + 1);
                for (size_t k = 0; k < fn.size(); k++) if (fn[k] >= 'A' && fn[k] <= 'Z') fn[k] += 32;
                while (!fv.empty() && fv[0] == ' ') fv.erase(0, 1);
                if (fn == "location") out.location = fv;
                if (fn == "content-length") {
                    UINTN cl = 0;
                    for (size_t k = 0; k < fv.size() && fv[k] >= '0' && fv[k] <= '9'; k++) cl = cl * 10 + (UINTN)(fv[k] - '0');
                    out.contentLen = cl;
                }
                if (fn == "transfer-encoding" && fv.find("chunked") != std::string::npos) out.chunked = true;
            }
        }
        p = e;
    }
    if (out.chunked) out.body = Dechunk(out.body);
    else if (out.contentLen && out.body.size() > out.contentLen) out.body.resize(out.contentLen);
    return true;
}

// ---------------------------------------------------------------- 对外接口
bool efinet::UrlDownTo(const char *url, const char *savepath)
{
    if (!gST || !gBS) return false;
    if (!url || !savepath) return false;

    ParsedUrl u;
    if (!ParseHttpUrl(url, u)) {
        eficon::SerialAscii("net: url parse failed (https unsupported?)");
        return false;
    }
    if (!EnsureIPv4Address()) {
        eficon::SerialAscii("net: no NIC or DHCP failed");
        return false;
    }

    std::string curUrl = url;
    for (int hop = 0; hop < 3; hop++) {
        ParsedUrl cu;
        if (!ParseHttpUrl(curUrl.c_str(), cu)) return false;

        UINT8 ip[4];
        if (cu.isIpLiteral) {
            for (int i = 0; i < 4; i++) ip[i] = cu.ip[i];
        } else {
            if (!DnsResolve(cu.host.c_str(), ip)) return false;
            char ipb[16], tb[32];
            Ip4Dotted(ip, ipb);
            int n = 0;
            const char *m = "net: dns -> ";
            while (m[n]) { tb[n] = m[n]; n++; }
            for (int k = 0; ipb[k] && n < 30; k++) tb[n++] = ipb[k];
            tb[n] = 0;
            eficon::SerialAscii(tb);
        }

        UINT16 port = 80;
        if (cu.port.size()) {
            UINT16 pv = 0;
            for (size_t k = 0; k < cu.port.size(); k++) {
                if (cu.port[k] < '0' || cu.port[k] > '9') { pv = 0; break; }
                pv = (UINT16)(pv * 10 + (cu.port[k] - '0'));
            }
            if (pv) port = pv;
        }

        std::string raw;
        if (!TcpHttpGet(ip, port, cu.host.c_str(), cu.path.c_str(), raw)) return false;

        HttpResponse resp;
        if (!ParseResponse(raw, resp)) {
            eficon::SerialAscii("net: response parse failed");
            return false;
        }

        if (resp.code == 301 || resp.code == 302 || resp.code == 303 || resp.code == 307 || resp.code == 308) {
            if (resp.location.empty()) return false;
            if (resp.location.compare(0, 7, "http://") == 0) curUrl = resp.location;
            else {
                std::string base = curUrl.substr(0, curUrl.find('/', 7));
                curUrl = base + (resp.location[0] == '/' ? resp.location : "/" + resp.location);
            }
            continue;
        }
        if (resp.code < 200 || resp.code >= 300) {
            char tb[32] = "net: http status ";
            int n = 17;
            UINT32 v = resp.code;
            char num[8]; int dn = 0;
            if (!v) num[dn++] = '0';
            while (v) { num[dn++] = (char)('0' + v % 10); v /= 10; }
            while (dn && n < 31) tb[n++] = num[--dn];
            tb[n] = 0;
            eficon::SerialAscii(tb);
            return false;
        }

        return efifs::WriteAll(savepath, resp.body.data(), (unsigned int)resp.body.size());
    }
    return false;
}

// ---------------------------------------------------------------- chunked 兜底
// TCP4 路线收到的是原始 chunk 帧（首行是十六进制块长），手动剥掉分块框架
static std::string Dechunk(const std::string &in)
{
    size_t p = 0, n = in.size();
    while (p < n && in[p] != '\r' && in[p] != '\n' && in[p] != ';') {
        char c = in[p];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return in;
        p++;
    }
    if (p == 0 || p >= n) return in;

    std::string out; out.reserve(in.size());
    size_t i = 0;
    while (i < n) {
        size_t e = in.find("\r\n", i);
        if (e == std::string::npos) break;
        UINT64 sz = 0; size_t k = i;
        bool hexok = false;
        while (k < e) {
            char c = in[k];
            if (c == ';') break;
            if (c >= '0' && c <= '9') { sz = sz * 16 + (UINT64)(c - '0'); hexok = true; }
            else if (c >= 'a' && c <= 'f') { sz = sz * 16 + (UINT64)(c - 'a' + 10); hexok = true; }
            else if (c >= 'A' && c <= 'F') { sz = sz * 16 + (UINT64)(c - 'A' + 10); hexok = true; }
            else break;
            k++;
        }
        if (!hexok) break;
        if (sz == 0) return out;                     // 最后一块
        i = e + 2;
        if (i + sz > n) return out;                  // 帧不完整，交出已有部分
        out.append(in, i, (size_t)sz);
        i += sz;
        if (i + 2 <= n && in[i] == '\r' && in[i + 1] == '\n') i += 2;
    }
    return out;
}
