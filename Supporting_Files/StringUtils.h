#pragma once
//
// Compatibility shim:
// AppGateway sources use `StringUtils::toLower(...)` (entservices-infra style).
// In this repo, Supporting_Docs_Files/helpers/StringUtils.h is namespaced and does not
// provide that API. Provide a local shim that matches the expected API.
//
#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

#include <algorithm>
#include <cctype>
#include <string>

class StringUtils {
public:
    static std::string toLower(const std::string& input)
    {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    static bool rfindInsensitive(const std::string& reference, const std::string& key)
    {
        const std::string lowerRef = toLower(reference);
        const std::string lowerKey = toLower(key);
        return (lowerRef.rfind(lowerKey) != std::string::npos);
    }

    static bool checkStartsWithCaseInsensitive(const std::string& method, const std::string& key)
    {
        const std::string lowerMethod = toLower(method);
        const std::string lowerKey = toLower(key);
        return (lowerMethod.rfind(lowerKey) == 0);
    }

    static std::string extractMethodName(const std::string& method)
    {
        const size_t lastDot = method.rfind('.');
        if (lastDot == std::string::npos || (lastDot + 1) >= method.length()) {
            return "";
        }
        return toLower(method.substr(lastDot + 1));
    }
};

#endif
