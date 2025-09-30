#include "CorrelationMap.h"

namespace WPEFramework {
namespace Plugin {

string CorrelationMap::GenerateCorrelationId() const {
    // Generate 16 random bytes and stringify as a 32-hex string grouped as UUID-like
    uint8_t bytes[16] = {0};
    Cryptalgo::Random random;
    random.Generate(sizeof(bytes), bytes);

    static constexpr char hexchars[] = "0123456789abcdef";
    string s;
    s.reserve(36);
    for (size_t i = 0; i < sizeof(bytes); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            s.push_back('-');
        }
        s.push_back(hexchars[(bytes[i] >> 4) & 0x0F]);
        s.push_back(hexchars[(bytes[i]     ) & 0x0F]);
    }
    return s;
}

string CorrelationMap::Create(const ConsumerContext& ctx) {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    string id = GenerateCorrelationId();
    // Ensure uniqueness (unlikely to collide, but check)
    while (_pending.find(id) != _pending.end()) {
        id = GenerateCorrelationId();
    }
    _pending.emplace(id, ctx);
    return id;
}

bool CorrelationMap::Take(const string& correlationId, ConsumerContext& out) {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    auto it = _pending.find(correlationId);
    if (it == _pending.end()) {
        return false;
    }
    out = it->second;
    _pending.erase(it);
    return true;
}

bool CorrelationMap::Peek(const string& correlationId, ConsumerContext& out) const {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    auto it = _pending.find(correlationId);
    if (it == _pending.end()) {
        return false;
    }
    out = it->second;
    return true;
}

void CorrelationMap::Clear() {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    _pending.clear();
}

} // namespace Plugin
} // namespace WPEFramework
