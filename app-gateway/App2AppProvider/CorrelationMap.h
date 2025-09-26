#pragma once

#include "Module.h"
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

/**
 * CorrelationMap
 * Maintains correlationId -> ConsumerContext mapping for in-flight invocations.
 */
class CorrelationMap {
public:
    struct ConsumerContext {
        string appId;
        string connectionId;
        uint32_t requestId;
    };

public:
    CorrelationMap()
        : _nextId(1) {
    }

    // PUBLIC_INTERFACE
    /**
     * Create a new correlation and return its id (string).
     * The id generation uses a monotonically increasing counter, sufficient
     * for local correlation in Phase 1.
     */
    string Create(const ConsumerContext& ctx) {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        const uint64_t id = _nextId++;
        string cid = Core::NumberType<uint64_t>(id).Text();
        _pending.emplace(cid, ctx);
        return cid;
    }

    // PUBLIC_INTERFACE
    /**
     * Take a correlation (remove and return it).
     * Returns true if found; otherwise false.
     */
    bool Take(const string& correlationId, ConsumerContext& out) {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        auto it = _pending.find(correlationId);
        if (it == _pending.end()) {
            return false;
        }
        out = it->second;
        _pending.erase(it);
        return true;
    }

    // PUBLIC_INTERFACE
    /**
     * Peek a correlation without removing it.
     * Returns true if found; otherwise false.
     */
    bool Peek(const string& correlationId, ConsumerContext& out) const {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        auto it = _pending.find(correlationId);
        if (it == _pending.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    void Clear() {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        _pending.clear();
    }

private:
    mutable Core::CriticalSection _lock;
    std::unordered_map<string, ConsumerContext> _pending;
    uint64_t _nextId;
};

} // namespace Plugin
} // namespace WPEFramework
