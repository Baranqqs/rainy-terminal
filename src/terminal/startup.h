#pragma once

#include <string>

// Starts the rain show together with cmd.exe.
//
// Windows gives every cmd.exe a small command line that it runs before the
// first prompt: the per-user AutoRun value under
// HKCU\Software\Microsoft\Command Processor. We point it at this program.
namespace startup {

// Adds our command to AutoRun. Anything that was already there is kept.
// A short result is written into `message`.
bool install(std::string& message);

// Removes our command again. A short result is written into `message`.
bool uninstall(std::string& message);

}  // namespace startup
