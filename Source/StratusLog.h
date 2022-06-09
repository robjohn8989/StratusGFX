#pragma once

#include <iostream>

// Usage example:
//      STRATUS_LOG << "Initializing system" << std::endl;
#define STRATUS_LOG stratus::Log::Instance()->Inform(__FUNCTION__, __LINE__)
#define STRATUS_WARN stratus::Log::Instance()->Warn(__FUNCTION__, __LINE__)
#define STRATUS_ERROR stratus::Log::Instance()->Error(__FUNCTION__, __LINE__)

namespace stratus {
    class Log {
        friend class Engine;
        Log();

    public:
        ~Log() = default;

        Log(const Log&) = delete;
        Log(Log&&) = delete;
        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;

        // Main logging functions
        std::ostream& Inform(const std::string & function, const int line) const;
        std::ostream& Warn(const std::string & function, const int line) const;
        std::ostream& Error(const std::string & function, const int line) const;

        // Static instance
        static Log * Instance() { return _instance; }

    private:
        static Log * _instance;
    };
}