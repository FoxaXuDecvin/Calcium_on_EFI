// ============================================================================
//  efi_core.h —— Calcium EFI 移植版的内核接口层
//  自包含：不依赖 EDK2、不依赖 Windows SDK、不依赖 CRT。
//  提供：EFI 基本类型 / 引导服务与运行时服务子集 / 控制台 / 文件系统
// ============================================================================
#pragma once

// ---------------------------------------------------------------- 基本类型
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned long long  UINT64;
typedef signed char         INT8;
typedef short               INT16;
typedef int                 INT32;
typedef long long           INT64;
typedef unsigned long long  UINTN;
typedef long long           INTN;
typedef unsigned char       BOOLEAN;
typedef unsigned short      CHAR16;
typedef char                CHAR8;
typedef void                VOID;
typedef UINTN               EFI_STATUS;
typedef VOID               *EFI_HANDLE;
typedef VOID               *EFI_EVENT;
typedef UINT64              EFI_PHYSICAL_ADDRESS;
typedef UINT64              EFI_VIRTUAL_ADDRESS;
typedef UINTN               EFI_TPL;

typedef struct { UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8]; } EFI_GUID;
typedef struct { UINT64 Signature; UINT32 Revision; UINT32 HeaderSize; UINT32 CRC32; UINT32 Reserved; } EFI_TABLE_HEADER;
typedef struct { UINT16 Year; UINT8 Month; UINT8 Day; UINT8 Hour; UINT8 Minute; UINT8 Second; UINT8 Pad1;
                 UINT32 Nanosecond; INT16 TimeZone; UINT8 Daylight; UINT8 Pad2; } EFI_TIME;

#define EFI_SUCCESS            0ULL
#define EFI_ERROR_MASK         0x8000000000000000ULL
#define EFI_ERROR(x)           (((x) & EFI_ERROR_MASK) != 0)
#define EFIAPI                 __fastcall
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x0002

// ---------------------------------------------------------------- 控制台
typedef struct {
    INT32 MaxMode; INT32 Mode; INT32 Attribute;
    INT32 CursorColumn; INT32 CursorRow; BOOLEAN CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct {
    EFI_STATUS (EFIAPI *Reset)(VOID *This, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *OutputString)(VOID *This, CHAR16 *String);
    EFI_STATUS (EFIAPI *TestString)(VOID *This, CHAR16 *String);
    EFI_STATUS (EFIAPI *QueryMode)(VOID *This, UINTN ModeNumber, UINTN *Columns, UINTN *Rows);
    EFI_STATUS (EFIAPI *SetMode)(VOID *This, UINTN ModeNumber);
    EFI_STATUS (EFIAPI *SetAttribute)(VOID *This, UINTN Attribute);
    EFI_STATUS (EFIAPI *ClearScreen)(VOID *This);
    EFI_STATUS (EFIAPI *SetCursorPosition)(VOID *This, UINTN Column, UINTN Row);
    EFI_STATUS (EFIAPI *EnableCursor)(VOID *This, BOOLEAN Visible);
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct { UINT16 ScanCode; CHAR16 UnicodeChar; } EFI_INPUT_KEY;

typedef struct {
    EFI_STATUS (EFIAPI *Reset)(VOID *This, BOOLEAN ExtendedVerification);
    EFI_STATUS (EFIAPI *ReadKeyStroke)(VOID *This, EFI_INPUT_KEY *Key);
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

// ---------------------------------------------------------------- 文件系统
typedef struct _EFI_FILE EFI_FILE;
struct _EFI_FILE {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *Open)(EFI_FILE *This, EFI_FILE **NewHandle, CHAR16 *FileName, UINT64 OpenMode, UINT64 Attributes);
    EFI_STATUS (EFIAPI *Close)(EFI_FILE *This);
    EFI_STATUS (EFIAPI *Delete)(EFI_FILE *This);
    EFI_STATUS (EFIAPI *Read)(EFI_FILE *This, UINTN *BufferSize, VOID *Buffer);
    EFI_STATUS (EFIAPI *Write)(EFI_FILE *This, UINTN *BufferSize, VOID *Buffer);
    EFI_STATUS (EFIAPI *GetPosition)(EFI_FILE *This, UINT64 *Position);
    EFI_STATUS (EFIAPI *SetPosition)(EFI_FILE *This, UINT64 Position);
    EFI_STATUS (EFIAPI *GetInfo)(EFI_FILE *This, EFI_GUID *InformationType, UINTN *BufferSize, VOID *Buffer);
    EFI_STATUS (EFIAPI *SetInfo)(EFI_FILE *This, EFI_GUID *InformationType, UINTN BufferSize, VOID *Buffer);
    EFI_STATUS (EFIAPI *Flush)(EFI_FILE *This);
};

typedef struct {
    UINT64 Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(VOID *This, EFI_FILE **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define EFI_FILE_MODE_READ      0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE     0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE    0x8000000000000000ULL
#define EFI_FILE_DIRECTORY      0x0000000000000010ULL
#define EFI_FILE_READ_ONLY      0x0000000000000001ULL

typedef struct {
    UINT64 Size; UINT64 FileSize; UINT64 PhysicalSize;
    EFI_TIME CreateTime; EFI_TIME LastAccessTime; EFI_TIME ModificationTime;
    UINT64 Attribute; CHAR16 FileName[1];
} EFI_FILE_INFO;

// ---------------------------------------------------------------- 已加载映像
typedef struct {
    UINT32 Revision; EFI_HANDLE ParentHandle; void *SystemTable;
    EFI_HANDLE DeviceHandle; void *FilePath; void *Reserved;
    UINT32 LoadOptionsSize; void *LoadOptions;
} EFI_LOADED_IMAGE_PROTOCOL;

// ---------------------------------------------------------------- 引导服务 / 运行时服务
typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;  void *RestoreTPL;
    void *AllocatePages; void *FreePages; void *GetMemoryMap;
    EFI_STATUS (EFIAPI *AllocatePool)(UINT32 PoolType, UINTN Size, VOID **Buffer);
    EFI_STATUS (EFIAPI *FreePool)(VOID *Buffer);
    void *CreateEvent; void *SetTimer;
    EFI_STATUS (EFIAPI *WaitForEvent)(UINTN NumberOfEvents, EFI_EVENT *Event, UINTN *Index);
    void *SignalEvent; void *CloseEvent; void *CheckEvent;
    void *InstallProtocolInterface; void *ReinstallProtocolInterface; void *UninstallProtocolInterface;
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface);
    void *Reserved;
    void *RegisterProtocolNotify; void *LocateHandle;
    EFI_STATUS (EFIAPI *LocateDevicePath)(EFI_GUID *Protocol, void **DevicePath, EFI_HANDLE *Device);
    void *InstallConfigurationTable;
    void *LoadImage; void *StartImage; void *Exit; void *UnloadImage; void *ExitBootServices;
    void *GetNextMonotonicCount;
    EFI_STATUS (EFIAPI *Stall)(UINTN Microseconds);
    void *SetWatchdogTimer;
    void *ConnectController; void *DisconnectController;
    void *OpenProtocol; void *CloseProtocol; void *OpenProtocolInformation;
    void *ProtocolsPerHandle; void *LocateHandleBuffer; void *LocateProtocol;
    void *InstallMultipleProtocolInterfaces; void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32; void *CopyMem; void *SetMem; void *CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    EFI_STATUS (EFIAPI *GetTime)(EFI_TIME *Time, VOID *Capabilities);
    void *SetTime; void *GetWakeupTime; void *SetWakeupTime;
    void *SetVirtualAddressMap; void *ConvertPointer;
    void *GetVariable; void *GetNextVariableName; void *SetVariable;
    void *GetNextHighMonotonicCount; void *ResetSystem;
} EFI_RUNTIME_SERVICES;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    UINT32 _pad;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES    *BootServices;
    UINTN NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

// ---------------------------------------------------------------- 全局状态
extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_RUNTIME_SERVICES *gRT;
extern EFI_HANDLE gImageHandle;
extern EFI_FILE *gFsRoot;      // 卷根目录

#define EfiPoolLoaderData 4

// ---------------------------------------------------------------- 控制台 API
namespace eficon {
    void Init();
    void WriteUtf8(const char *s, unsigned long long n);
    void WriteAscii(const char *s);
    void SerialAscii(const char *s);   // 只写 COM1（VMware serial 落盘），不污染屏幕
    void Clear();
    bool ReadLine(char *buf, unsigned long long cap);   // 返回是否读到一行
    bool HasKey();
    // 无人值守测试：卷根 autoinput.txt 的每行作为一次 ReadLine 的输入
    void LoadAutoInput(const char *path);
}

// ---------------------------------------------------------------- 文件系统 API（路径用 '/' 或 '\\' 均可，相对于卷根）
namespace efifs {
    bool Init();
    bool ReadAll(const char *path, char *&out, unsigned int &len);   // out 由 AllocatePool 分配
    void FreeBuf(void *p);
    bool WriteAll(const char *path, const char *data, unsigned int len);
    // 追加写（SetPosition(-1) 定位到 EOF），不知道原长度也不需要读原内容
    bool AppendAll(const char *path, const char *data, unsigned int len);
    bool Exists(const char *path);
    bool IsDir(const char *path);
    bool Remove(const char *path);
    bool RemoveDir(const char *path);
    bool MakeDir(const char *path);       // 递归创建
    bool CopyFile(const char *src, const char *dst);
    // 目录枚举：回调收到文件/目录名（不含路径）。返回 false 可提前中断。
    bool ListDir(const char *path, bool (*cb)(const char *name, bool isDir, void *user), void *user);
    extern char SelfName[260];   // 自身映像路径（仅用于显示）
}

// ---------------------------------------------------------------- 内存 / 基础工具
void *efi_alloc(unsigned long long size);
void  efi_free(void *p);
void  efi_stall_us(unsigned long long us);
// 调试期定位崩溃点用：默认空实现，编译时加 EFI_TRACE_OUT 才真正输出
#ifdef EFI_TRACE_OUT
void efi_trace(const char *tag);
#else
inline void efi_trace(const char *) {}
#endif
