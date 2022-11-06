#ifndef UNICODE
  #define UNICODE
#endif

#include <windows.h>
#include <cstdio>
#include <vector>
#include <locale>

#pragma comment (lib, "ntdll.lib")

using NTSTATUS = LONG;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0L)
#define fnget(P) reinterpret_cast<P>(GetProcAddress(GetModuleHandle(L"ntdll.dll"), (&((#P)[1]))))

struct RTL_MODULE_BASIC_INFO {
   PVOID  ImageBase;
};

struct RTL_MODULE_EXTENDED_INFO {
  RTL_MODULE_BASIC_INFO BasicInfo;
  ULONG  ImageSize;
  USHORT FileNameOffset;
  UCHAR  FullPathName[256];
};

using pRtlQueryModuleInformation = NTSTATUS(__stdcall *)(PULONG, ULONG, PVOID);
using pRtlNtStatusToDosError = ULONG(__stdcall *)(NTSTATUS);

struct ntdll {
  pRtlQueryModuleInformation RtlQueryModuleInformation = fnget(pRtlQueryModuleInformation);
  pRtlNtStatusToDosError RtlNtStatusToDosError = fnget(pRtlNtStatusToDosError);

  bool isvalid(void) {
    return this->RtlQueryModuleInformation != nullptr && this->RtlNtStatusToDosError != nullptr;
  }
};

int main(void) {
  ntdll nt{};
  if (!nt.isvalid()) {
    printf("Cannot find required signature.\n");
    return 1;
  }

  auto getlasterror = [&nt](NTSTATUS nts) {
    HLOCAL loc{};
    std::locale::global(std::locale(""));
    DWORD size = ::FormatMessage(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      nullptr, nt.RtlNtStatusToDosError(nts), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&loc), 0, nullptr
    );
    if (size)
      printf("[!] %.*ws\n", static_cast<INT>(size - sizeof(WCHAR)), reinterpret_cast<LPWSTR>(loc));
    else
      printf("[?] Unknows error has been occured.\n");

    if (nullptr != ::LocalFree(loc))
      printf("[!] LocalFree (%lu) fatal error.\n", ::GetLastError());
  };

  ULONG buf_len{}, size = sizeof(RTL_MODULE_EXTENDED_INFO);
  auto nts = nt.RtlQueryModuleInformation(&buf_len, size, nullptr);
  if (!NT_SUCCESS(nts)) {
    getlasterror(nts);
    return 1;
  }

  std::vector<RTL_MODULE_EXTENDED_INFO> modules(buf_len / size);
  nts = nt.RtlQueryModuleInformation(&buf_len, size, &modules[0]);
  if (!NT_SUCCESS(nts)) {
    getlasterror(nts);
    return 1;
  }

  for (const auto & module : modules) {
    printf("%p %10lu %s\n", module.BasicInfo.ImageBase, module.ImageSize, module.FullPathName);
  }

  return 0;
}
