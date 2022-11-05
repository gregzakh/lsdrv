#include "registry.hpp"
#include "native.hpp"

void GetLoadedDrivers(void) {
  printf("%-16ws %10ws %5ws %ws\n", L"Address", L"Size", L"Count", L"Path");
  printf("%-16s %10s %5s %s\n", "--------", "-----", "-----", "-----");
  auto nts = NtGetSystemModules();
  if (!NT_SUCCESS(nts)) getlaserror(::RtlNtStatusToDosError(nts));
}

void RegEnumDrivers(void) {
  printf("%-10ws %-25ws %ws\n", L"Start", L"Group", L"Path");
  printf("%-10s %-25s %s\n", "------", "-------", "-----");
  Registry key(L"HKLM\\SYSTEM\\CurrentControlSet\\Services\\");
  key.enumdrivers();
}

int wmain(int argc, wchar_t **argv) {
  static const struct {
    const wchar_t *param;
    const wchar_t *desc;
    const void    *fncall;
  } params[] = {
    L"-l", L"shows currently loaded drivers", GetLoadedDrivers,
    L"-s", L"prints a list of service drivers", RegEnumDrivers
  };
  auto ok = false;

  if (2 != argc) {
    printf("lstdrv v1.0 - list system drivers\n");
    for (auto x : params)
      printf("%5ws - %ws\n", x.param, x.desc);
    return 1;
  }

  for (auto x : params) {
    if (0 == _wcsicmp(argv[1], x.param)) {
      static_cast<void(*)(void)>(x.fncall)();
      ok = true;
    }
  }

  if (!ok) printf("Invalid command line.\n");

  return 0;
}
