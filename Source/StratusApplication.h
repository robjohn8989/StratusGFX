#pragma once

#include "StratusSystemModule.h"
#include <string>

namespace stratus {
    // Special interface class which the engine knows is the entry point
    // for the application (e.g. editor or game)
    class Application : public SystemModule {
    public:
        virtual ~Application() = default;

        // Sets the name of the window
        virtual const char * GetAppName() const = 0;

        virtual const char * Name() const {
            return GetAppName();
        }
    };
}