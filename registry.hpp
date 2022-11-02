#pragma once

#include "common.hpp"
#include <vector>
#include <regex>

static const std::wstring systemroot = *reinterpret_cast<wchar_t(*)[260]>(0x7FFE0030);

class Registry : public CRegHelper {
  private:
    std::wstring _key;
  public:
    Registry(std::wstring key) : CRegHelper(key), _key(key) {}

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    auto queryinfo(LPDWORD subkeys, LPDWORD values, PFILETIME ft);
    template<typename... ArgTypes>
    auto queryvalue(PWSTR value, ArgTypes... args) {
      return ::RegQueryValueEx(c_key, value, nullptr, nullptr, args...);
    }
    bool testdword(PWSTR name, DWORD limit);
    void enumsubkeys(void);
    void readvalue(PWSTR name);
};

auto Registry::queryinfo(LPDWORD subkeys, LPDWORD values, PFILETIME ft) {
  return ::RegQueryInfoKey(
    c_key, nullptr, nullptr, nullptr, subkeys, nullptr,
    nullptr, values, nullptr, nullptr, nullptr, ft
  );
}

bool Registry::testdword(PWSTR name, DWORD limit) {
  DWORD value{}, size = sizeof(DWORD);
  auto status = queryvalue(name, reinterpret_cast<PBYTE>(&value), &size);
  if (ERROR_SUCCESS != status) {
#ifdef DEBUG
    getlaserror(status);
#endif
    return false;
  }

  return value <= limit;
}

void Registry::readvalue(PWSTR name) {
  DWORD size{};
  auto status = queryvalue(name, nullptr, &size);
  if (ERROR_SUCCESS != status) {
#ifdef DEBUG
    getlaserror(status);
#endif
    return;
  }

  std::vector<BYTE> buf(size);
  status = queryvalue(name, &buf[0], &size);
  if (ERROR_SUCCESS != status) {
#ifdef DEBUG
    getlaserror(status);
#endif
    return;
  }

  std::wregex re(L"\\\\SystemRoot", std::regex_constants::icase);
  std::wstring s = std::regex_replace(reinterpret_cast<PWSTR>(&buf[0]), re, systemroot);
  printf("%ws\n", (L's' == tolower(s[0]) ? systemroot + L"\\" + s : s).c_str());
}

void Registry::enumsubkeys(void) {
  WCHAR subname[MAX_KEY_LENGTH];
  DWORD subkeys{}, size{};
  auto status = queryinfo(&subkeys, nullptr, nullptr);
  if (ERROR_SUCCESS != status) {
    getlaserror(status);
    return;
  }

  for (auto i = 0; i < subkeys; i++) {
    size = MAX_KEY_LENGTH;
    if (ERROR_SUCCESS != ::RegEnumKeyEx(
      c_key, i, subname, &size, nullptr, nullptr, nullptr, nullptr
    )) continue;
    Registry sub(_key + subname);
    if (sub.testdword(L"Type", 3))
      sub.readvalue(L"ImagePath");
  }
}
