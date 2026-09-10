#pragma once
#include <string>

// Minimal logger: every call prints to stdout and appends to a log file.
// Only ever called from the single GQCS loop thread, so no locking needed.
namespace Logger {
    // Creates Logs/ next to the working directory if missing and opens
    // Logs/server_<port>.txt in append mode.
    void Init(unsigned short port);

    void Log(const std::string& message);
}
