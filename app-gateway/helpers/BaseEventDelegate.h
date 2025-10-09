#pragma once

#include <set>
#include <string>
#include <mutex>

// Use the local stubbed IAppNotifications interface to enable compile-time dispatch calls.
#include <interfaces/IAppNotifications.h>

// Base class for event delegates. Provides a small notification registry
// and a Dispatch method that forwards events to the AppNotifications service.
class BaseEventDelegate {
public:
    explicit BaseEventDelegate(WPEFramework::Exchange::IAppNotifications* appNotifications)
        : mAppNotifications(appNotifications) {
    }

    virtual ~BaseEventDelegate() = default;

    // PUBLIC_INTERFACE
    // Derived classes must implement event handling/registration logic and set registrationError accordingly.
    virtual bool HandleEvent(const std::string& event, const bool listen, bool& registrationError) = 0;

    // Registers an event in a local set (no-op if already present).
    inline void AddNotification(const std::string& event) {
        std::lock_guard<std::mutex> lg(_lock);
        _events.insert(event);
    }

    // Removes an event from the local set (no-op if not present).
    inline void RemoveNotification(const std::string& event) {
        std::lock_guard<std::mutex> lg(_lock);
        auto it = _events.find(event);
        if (it != _events.end()) {
            _events.erase(it);
        }
    }

    // PUBLIC_INTERFACE
    inline void Dispatch(const std::string& event, const std::string& payload) {
        // Forward the event to the AppNotifications service if it is available.
        if (mAppNotifications != nullptr) {
            mAppNotifications->Notify(event, payload);
        }
    }

protected:
    WPEFramework::Exchange::IAppNotifications* mAppNotifications { nullptr };

private:
    std::mutex _lock;
    std::set<std::string> _events;
};
