#include "Logger.h"
#include "DirectXCommon.h"

namespace Logger {
void Log(const std::string& message)
{
    std::string finalMessage = message + "\n";
    OutputDebugStringA(message.c_str());
}
}