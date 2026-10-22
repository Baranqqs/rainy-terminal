#include "startup.h"

#include <windows.h>

#include <string>

#include "terminal.h"

namespace {

constexpr wchar_t kAutoRunKey[] = L"Software\\Microsoft\\Command Processor";
constexpr wchar_t kAutoRunValue[] = L"AutoRun";

// Full path of the running RainyTerminal.exe.
std::wstring modulePath() {
    std::wstring path(MAX_PATH, L'\0');

    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0) {
            return {};
        }
        if (length < path.size()) {
            path.resize(length);
            return path;
        }
        path.resize(path.size() * 2);
    }
}

// The command that cmd.exe should run on every start.
std::string autoRunCommand() {
    const std::wstring path = modulePath();
    if (path.empty()) {
        return {};
    }

    // Only interactive shells should wait for the animation. "cmd /c ..." and
    // "cmd /r ..." come from scripts, editors and build tools and must never
    // be blocked. cmd expands %CMDCMDLINE% itself and does not pass it on to
    // child processes, so the check has to happen here in the AutoRun line.
    std::string command = "echo %CMDCMDLINE%|find /i \" /c\" >nul||";
    command += "echo %CMDCMDLINE%|find /i \" /r\" >nul||\"";
    command += toUtf8(path);
    command += "\" --startup --banner=hold";
    return command;
}

// Reads the current AutoRun value. A missing value counts as empty.
bool readAutoRun(std::wstring& value, DWORD& type) {
    value.clear();
    type = REG_SZ;

    DWORD bytes = 0;
    LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, kAutoRunKey, kAutoRunValue,
                                  RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
                                  &type, nullptr, &bytes);
    if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND) {
        return true;
    }
    if (status != ERROR_SUCCESS) {
        return false;
    }
    if (bytes == 0) {
        return true;
    }

    std::wstring buffer(bytes / sizeof(wchar_t), L'\0');
    status = RegGetValueW(HKEY_CURRENT_USER, kAutoRunKey, kAutoRunValue,
                          RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
                          &type, buffer.data(), &bytes);
    if (status != ERROR_SUCCESS) {
        return false;
    }

    buffer.resize(bytes / sizeof(wchar_t));
    while (!buffer.empty() && buffer.back() == L'\0') {
        buffer.pop_back();
    }

    value = buffer;
    return true;
}

bool writeAutoRun(const std::wstring& value, DWORD type) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kAutoRunKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const LSTATUS status = RegSetValueExW(key, kAutoRunValue, 0, type,
                                          reinterpret_cast<const BYTE*>(value.c_str()), bytes);
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

bool deleteAutoRun() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kAutoRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return true;
    }

    const LSTATUS status = RegDeleteValueW(key, kAutoRunValue);
    RegCloseKey(key);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

// Keeps the original value type, so REG_EXPAND_SZ entries still expand.
DWORD valueType(DWORD original) {
    return original == REG_EXPAND_SZ ? REG_EXPAND_SZ : REG_SZ;
}

// Removes every Rainy Terminal command from `value`, even one that points to
// an older location of the executable. Returns true when something was found.
//
// cmd.exe entries are separated by " & ". Our command itself never contains
// that sequence, so the last separator before the marker is the start of the
// entry.
bool removeOurCommands(std::string& value) {
    constexpr const char* marker = "--startup --banner=hold";
    const std::string tail(marker);
    bool removed = false;

    std::size_t position = value.find(tail);
    while (position != std::string::npos) {
        std::size_t start = 0;
        std::size_t end = position + tail.size();

        const std::size_t separator = value.rfind(" & ", position);
        if (separator != std::string::npos) {
            start = separator;  // take the separator with the command
        } else if (value.compare(end, 3, " & ") == 0) {
            end += 3;           // our command is the first entry
        }

        value.erase(start, end - start);
        removed = true;
        position = value.find(tail);
    }

    return removed;
}

}  // namespace

namespace startup {

bool install(std::string& message) {
    const std::string ours = autoRunCommand();
    if (ours.empty()) {
        message = "Could not find the path of RainyTerminal.exe.";
        return false;
    }

    std::wstring current;
    DWORD type = REG_SZ;
    if (!readAutoRun(current, type)) {
        message = "Could not read HKCU\\Software\\Microsoft\\Command Processor\\AutoRun.";
        return false;
    }

    const std::string existing = toUtf8(current);

    // Drop an entry from an older location before adding the current one.
    std::string next = existing;
    const bool hadEntry = removeOurCommands(next);
    if (!next.empty()) {
        next += " & ";
    }
    next += ours;

    if (next == existing) {
        message = "Rainy Terminal is already set up for cmd.exe.";
        return true;
    }

    if (!writeAutoRun(toWide(next), valueType(type))) {
        message = "Could not write HKCU\\Software\\Microsoft\\Command Processor\\AutoRun.";
        return false;
    }

    message = hadEntry
        ? "Rainy Terminal now starts from its new location."
        : "Rainy Terminal now starts with every new cmd.exe window.";
    return true;
}

bool uninstall(std::string& message) {
    const std::string ours = autoRunCommand();
    if (ours.empty()) {
        message = "Could not find the path of RainyTerminal.exe.";
        return false;
    }

    std::wstring current;
    DWORD type = REG_SZ;
    if (!readAutoRun(current, type)) {
        message = "Could not read HKCU\\Software\\Microsoft\\Command Processor\\AutoRun.";
        return false;
    }

    std::string next = toUtf8(current);
    if (!removeOurCommands(next)) {
        message = "Rainy Terminal is not set up for cmd.exe.";
        return true;
    }

    const bool ok = next.empty()
        ? deleteAutoRun()
        : writeAutoRun(toWide(next), valueType(type));

    message = ok
        ? "Rainy Terminal will no longer start with cmd.exe."
        : "Could not update HKCU\\Software\\Microsoft\\Command Processor\\AutoRun.";
    return ok;
}

}  // namespace startup
