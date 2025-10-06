#pragma once

#include <set>
#include <string>
#include <mutex>

// The delegates include and depend on Exchange::IAppNotifications. It may not
// be present in the installed Thunder SDK, so we forward-declare it here.
// A local stub for this interface is supplied under FbSettings/interfaces to avoid
// changing the delegates' include statements or logic.
namespace WPEFramework {
namespace Exchange {
    struct IAppNotifications;
}
}

// Base class for event delegates. Provides a small notification registry
// and a stub Dispatch method. Derived delegates must implement HandleEvent.
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

    // Sends a notification via AppNotifications if available. In this environment,
    // this is a stub and does nothing. Keeping the signature to preserve call sites.
    inline void Dispatch(const std::string& event, const std::string& payload) {
        (void)event;
        (void)payload;
        // If a concrete AppNotifications interface is available in the deployment,
        // one could call into it here (e.g., mAppNotifications->Notify(...)).
    }

protected:
    WPEFramework::Exchange::IAppNotifications* mAppNotifications { nullptr };

private:
    std::mutex _lock;
    std::set<std::string> _events;
};
