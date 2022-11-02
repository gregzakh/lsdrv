#pragma once

#ifndef UNICODE
  #define UNICODE
#endif

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <cstdio>
#include <string>
#include <locale>

#pragma comment (lib, "advapi32.lib")

auto fmtmsg(const DWORD err, HLOCAL& h) {
  return ::FormatMessage(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    reinterpret_cast<LPWSTR>(&h), 0, nullptr
  );
}

auto getlaserror(const DWORD err) {
  HLOCAL loc{};
  std::locale::global(std::locale(""));
  DWORD size = fmtmsg(err, loc);
  printf("%.*ws\n", static_cast<INT>(size - sizeof(WCHAR)), reinterpret_cast<LPWSTR>(loc));

  if (nullptr != ::LocalFree(loc))
    printf("[!] LocalFree (%lu) fatal error.\n", ::GetLastError());
}

constexpr ULONG MAX_KEY_LENGTH   = 255;
constexpr ULONG MAX_VALUE_LENGTH = 16383;

class CRegHelper {
  protected:
    HKEY c_key;
  private:
    static HKEY openkey(const std::wstring& r_key) {
      auto status = ERROR_SUCCESS;
      static const struct {
        const wchar_t *name;
        const wchar_t *fullname;
        HKEY          key;
      } roots[] = {
        L"HKCR", L"HKEY_CLASSES_ROOT",   HKEY_CLASSES_ROOT,
        L"HKLM", L"HKEY_LOCAL_MACHINE",  HKEY_LOCAL_MACHINE,
        L"HKCU", L"HKEY_CURRENT_USER",   HKEY_CURRENT_USER,
        L"HKU",  L"HKEY_USERS",          HKEY_USERS,
        L"HKCC", L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG
      };

      HKEY key = nullptr;
      auto pos = r_key.find_first_of(L"\\");
      std::wstring root = r_key.substr(0, pos);
      std::wstring path = r_key.substr(pos + 1, r_key.length());
      for (auto i = 0; i < sizeof(roots) / sizeof(roots[0]); i++) {
        if (0 == _wcsicmp(root.c_str(), roots[i].name) ||
            0 == _wcsicmp(root.c_str(), roots[i].fullname)) {
          status = ::RegOpenKeyEx(roots[i].key, path.c_str(), 0, KEY_QUERY_VALUE | KEY_READ, &key);
          if (ERROR_SUCCESS != status) getlaserror(status);
          break;
        }
      }

      return key;
    }
  public:
    CRegHelper(const std::wstring& r_key) { c_key = openkey(r_key); }

    CRegHelper(const CRegHelper&) = delete;
    CRegHelper& operator=(const CRegHelper&) = delete;

    ~CRegHelper() {
      if (nullptr != c_key) {
        auto status = ::RegCloseKey(c_key);
        if (ERROR_SUCCESS != status) getlaserror(status);
#ifdef DEBUG
        else printf("[*] success.\n");
#endif
      }
    }

    operator HKEY()   { return c_key; }
    HKEY* operator&() { return &c_key; }
};
