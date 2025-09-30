#pragma once

#include <core/core.h>
#include <cryptalgo/Random.h>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

struct ConsumerContext {
    string appId;
    string connectionId;
    uint32_t requestId { 0 };
};

class CorrelationMap {
public:
    CorrelationMap() = default;
    CorrelationMap(const CorrelationMap&) = delete;
    CorrelationMap& operator=(const CorrelationMap&) = delete;

    // Create a new correlationId and store the associated consumer context.
    string Create(const ConsumerContext& ctx);

    // Retrieve and remove the context for a given correlationId.
    bool Take(const string& correlationId, ConsumerContext& out);

    // Retrieve without removing.
    bool Peek(const string& correlationId, ConsumerContext& out) const;

    // Clear all correlations.
    void Clear();

private:
    string GenerateCorrelationId() const;

private:
    mutable Core::CriticalSection _lock;
    std::unordered_map<string, ConsumerContext> _pending;
};

} // namespace Plugin
} // namespace WPEFramework
