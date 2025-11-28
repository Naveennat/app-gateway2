#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>

#include "OttServices/OttPermissionCache.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static bool ReadNonEmptyLines(const std::string& path, std::vector<std::string>& out) {
    out.clear();
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        bool onlyWhitespace = true;
        for (char c : line) {
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                onlyWhitespace = false;
                break;
            }
        }
        if (!onlyWhitespace) {
            out.emplace_back(line);
        }
    }
    return true;
}

int main() {
    int failures = 0;

    // Use a unique temp path for this test run and point the cache at it.
    const std::string permsPath = std::string("/tmp/ott_perms_cache_test_") + std::to_string(::getpid()) + ".json";
    ::setenv("OTT_PERMS_FILE", permsPath.c_str(), 1);

    // Ensure clean slate
    (void)std::remove(permsPath.c_str());

    // Use the cache and update multiple times for the same app.
    auto& cache = OttPermissionCache::Instance();
    cache.Clear();

    const std::string appId = "xumo";
    std::vector<std::string> perms = { "perm.read", "perm.write" };

    // First update
    cache.UpdateCache(appId, perms);

    // Repeated updates to verify idempotency and no duplicates/malformed entries.
    cache.UpdateCache(appId, perms);
    cache.UpdateCache(appId, perms);

    // Validate file content
    std::vector<std::string> lines;
    if (!ReadNonEmptyLines(permsPath, lines)) {
        std::cerr << "FAIL: Unable to read permissions file: " << permsPath << std::endl;
        return 1;
    }

    if (lines.size() != 1) {
        std::cerr << "FAIL: Expected exactly one line in " << permsPath << ", got " << lines.size() << std::endl;
        failures++;
    } else {
        const std::string& l = lines[0];
        if (l.find("\"appId\":\"xumo\"") == std::string::npos) {
            std::cerr << "FAIL: Expected appId 'xumo' in line: " << l << std::endl;
            failures++;
        }
        if (l.find("\"appId\":\"\"") != std::string::npos) {
            std::cerr << "FAIL: Found empty appId in line: " << l << std::endl;
            failures++;
        }
        if (l.find("\"appId\":\",\"") != std::string::npos) {
            std::cerr << "FAIL: Found malformed appId=\",\" in line: " << l << std::endl;
            failures++;
        }
    }

    // Attempt to write with empty appId; should be skipped and not change the file.
    cache.UpdateCache("", std::vector<std::string>{ "should.not.write" });

    std::vector<std::string> linesAfter;
    if (!ReadNonEmptyLines(permsPath, linesAfter)) {
        std::cerr << "FAIL: Unable to re-read permissions file: " << permsPath << std::endl;
        return 1;
    }
    if (linesAfter.size() != 1) {
        std::cerr << "FAIL: After empty appId update, expected exactly one line, got " << linesAfter.size() << std::endl;
        failures++;
    }

    if (failures == 0) {
        std::cout << "OttPermissionCache tests passed." << std::endl;
    } else {
        std::cerr << failures << " OttPermissionCache test(s) failed." << std::endl;
    }

    // best-effort cleanup
    (void)std::remove(permsPath.c_str());
    return failures == 0 ? 0 : 1;
}
