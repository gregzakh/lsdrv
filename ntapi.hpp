#pragma once

#include "common.hpp"

constexpr auto SystemModuleInformation = static_cast<SYSTEM_INFORMATION_CLASS>(11);
constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH = 0xC0000004L;

struct RTL_PROCESS_MODULE_INFORMATION {
   HANDLE Section;
   PVOID  MappedBase;
   PVOID  ImageBase;
   ULONG  ImageSize;
   ULONG  Flags;
   USHORT LoadOrderIndex;
   USHORT InitOrderIndex;
   USHORT LoadCount;
   USHORT OffsetToFileName;
   UCHAR  FullPathName[256];
};

struct RTL_PROCESS_MODULES {
   ULONG NumberOfModules;
   RTL_PROCESS_MODULE_INFORMATION Modules[1];
};

auto NtGetSystemModules(void) {
  ULONG buf_len{};
  NTSTATUS nts = ::NtQuerySystemInformation(SystemModuleInformation, nullptr, 0, &buf_len);
  if (STATUS_INFO_LENGTH_MISMATCH != nts) return nts;

  std::vector<RTL_PROCESS_MODULES> modules(buf_len);
  nts = ::NtQuerySystemInformation(SystemModuleInformation, &modules[0], buf_len, nullptr);
  if (!NT_SUCCESS(nts)) return nts;

  size_t out{};
  std::string s(systemroot.length() + 1, '0');
  wcstombs_s(&out, &s[0], s.length(), &systemroot[0], s.length() - 1);
  std::regex re("\\\\SystemRoot", std::regex_constants::icase);
  s.erase(std::find(s.begin(), s.end(), '\0'), s.end());

  for (auto i = 0; i < modules[0].NumberOfModules; i++) {
    printf("%p %10lu %s\n", modules[0].Modules[i].ImageBase,
                            modules[0].Modules[i].ImageSize,
    std::regex_replace(reinterpret_cast<PSTR>(modules[0].Modules[i].FullPathName), re, s).c_str());
  }

  return 0L;
}
