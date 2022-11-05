#pragma once

#include "common.hpp"

class Registry : public CRegHelper {
  private:
    std::wstring _key;
  public:
    Registry(std::wstring key) : CRegHelper(key), _key(key) {}

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    auto querykeyinfo(LPDWORD subkeys, LPDWORD values, PFILETIME ft);
    template<typename... ArgTypes>
    auto querykeyvalue(PWSTR value, ArgTypes... args) {
      return ::RegQueryValueEx(c_key, value, nullptr, nullptr, args...);
    }
    auto readvaldword(PWSTR name, DWORD& value);
    bool checkdriver(void);
    auto readvalstr(PWSTR name);
    auto getdrvpath(void);
    void enumdrivers(void);
};

auto Registry::querykeyinfo(LPDWORD subkeys, LPDWORD values, PFILETIME ft) {
  return ::RegQueryInfoKey(
    c_key, nullptr, nullptr, nullptr, subkeys, nullptr,
    nullptr, values, nullptr, nullptr, nullptr, ft
  );
}

auto Registry::readvaldword(PWSTR name, DWORD& value) {
  DWORD size = sizeof(DWORD);
  return querykeyvalue(name, reinterpret_cast<PBYTE>(&value), &size);
}

bool Registry::checkdriver(void) {
  DWORD value{};
  auto status = readvaldword(L"Type", value);
  if (ERROR_SUCCESS != status) {
#ifdef DEBUG
    getlaserror(status);
#endif
    return false;
  }

  return value <= 3UL;
}

auto Registry::readvalstr(PWSTR name) {
  DWORD size{};
  std::wstring res = L"";
  while (1) {
    auto status = querykeyvalue(name, nullptr, &size);
    if (ERROR_SUCCESS != status) {
#ifdef DEBUG
      getlaserror(status);
#endif
      break;
    }

    std::vector<BYTE> buf(size);
    status = querykeyvalue(name, &buf[0], &size);
    if (ERROR_SUCCESS != status) {
#ifdef DEBUG
      getlaserror(status);
#endif
      break;
    }

    res = reinterpret_cast<PWSTR>(&buf[0]);
    break;
  }

  return res;
}

auto Registry::getdrvpath(void) {
  std::wregex re(L"\\\\SystemRoot\\\\", std::regex_constants::icase);
  std::wstring res = readvalstr(L"ImagePath");
  return res.length() ? systemroot + L"\\" + std::regex_replace(res, re, L"") : res;
}

void Registry::enumdrivers(void) {
  static WCHAR *start[] = {L"Boot", L"System", L"Automatic", L"Manual", L"Disabled"};
  WCHAR subname[MAX_KEY_LENGTH];
  DWORD subkeys{}, size{}, value{};
  auto status = querykeyinfo(&subkeys, nullptr, nullptr);
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
    if (sub.checkdriver()) {
      if (ERROR_SUCCESS == sub.readvaldword(L"Start", value))
        printf("%-10ws %-25ws %ws\n", start[value],
                                      sub.readvalstr(L"Group").c_str(),
                                      sub.getdrvpath().c_str()
        );
    }
  }
}
