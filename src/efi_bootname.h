// ============================================================================
//  efi_bootname.h —— 在 NVRAM 里注册自定义名字的持久 UEFI 启动项
//
//  背景：固件（EDK2 系 BDS）在自动探测可移动介质上的 \EFI\BOOT\BOOTX64.EFI 时，
//  会临时合成一个描述为 "UEFI OS" 的启动项（EDK2 里是硬编码字符串），
//  这个名字既不来自我们的映像，也不会写进 NVRAM，所以改 .efi 内部没有用。
//
//  要拿到自己的名字，只有一条正路：在 Boot#### 变量里放一个 Description 由我们
//  指定的持久启动项。本模块就是干这件事的——把"本映像自身的设备路径"包成
//  EFI_LOAD_OPTION 写进 NVRAM，并登记到 BootOrder。
// ============================================================================
#pragma once

namespace efi_bootname {
    // 注册一个指向"当前运行的映像"的持久启动项，描述为 asciiDesc（ASCII/半角）。
    // 已存在同名同路径的项时直接返回 true，不重复写。
    bool Register(const char *asciiDesc);

    // 列出当前 BootOrder 里的所有启动项（编号 + 描述），诊断用。
    void Dump();
}
