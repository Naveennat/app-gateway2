#pragma once

#include <string>
#include <ctime>

namespace WPEFramework {
namespace Utils {

    std::string GetCurrentISO8601Time()
    {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        gmtime_r(&now, &tstruct);
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tstruct);
        return buf;
    }

}
}