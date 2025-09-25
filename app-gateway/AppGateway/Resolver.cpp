#include "Resolver.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    // Helper: Read a whole file into a string.
    bool ReadFileToString(const string& path, string& out, string& error) {
        Core::File file(path);
        if (file.Exists() == false) {
            error = "File not found: " + path;
            return false;
        }
        if (file.Open(false) == false) {
            error = "Failed to open: " + path;
            return false;
        }
        uint64_t size = file.Size();
        out.clear();
        out.reserve(static_cast<size_t>(size));
        string chunk;
        chunk.resize(4096);
        while (size > 0) {
            uint32_t toRead = static_cast<uint32_t>(std::min<uint64_t>(size, chunk.size()));
            uint32_t actuallyRead = file.Read(const_cast<char*>(chunk.data()), toRead);
            if (actuallyRead == static_cast<uint32_t>(~0)) {
                error = "Read failed: " + path;
                file.Close();
                return false;
            }
            out.append(chunk.c_str(), actuallyRead);
            size -= actuallyRead;
        }
        file.Close();
        return true;
    }

    // Extract last-wins overlay from a JSON object:
    // Expecting: { "resolutions": { "<method>": { <resolution> }, ... } }
    std::unordered_map<string, Core::JSON::Object> ExtractResolutions(const string& jsonText) {
        std::unordered_map<string, Core::JSON::Object> results;

        Core::OptionalType<Core::JSON::Error> error;
        Core::JSON::Object document;
        if (document.IElement::FromString(jsonText, error) == false) {
            return results;
        }
        if (document.HasLabel(_T("resolutions")) == false) {
            return results;
        }
        Core::JSON::Object resolutions = document.Get<Core::JSON::Object>(_T("resolutions"));
        // Iterate labels (methods) and copy their JSON object bodies.
        Core::JSON::Iterator it = resolutions.Elements();
        while (it.Next() == true) {
            const string key = it.Label();
            if (resolutions.HasLabel(key)) {
                Core::JSON::Object value = resolutions.Get<Core::JSON::Object>(key);
                results[key] = value;
            }
        }
        return results;
    }
}

bool Resolver::LoadPaths(const std::vector<string>& paths, string& error) {
    _store.Clear();

    for (const auto& p : paths) {
        if (!LoadFile(p, error)) {
            return false;
        }
    }
    return true;
}

bool Resolver::LoadFile(const string& path, string& error) {
    string text;
    if (!ReadFileToString(path, text, error)) {
        return false;
    }
    auto overlay = ExtractResolutions(text);
    _store.Overlay(overlay);
    return true;
}

} // namespace Plugin
} // namespace WPEFramework
