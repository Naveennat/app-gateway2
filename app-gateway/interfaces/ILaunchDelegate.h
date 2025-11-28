#pragma once
#include <plugins/Module.h>

#ifndef ID_LAUNCHDELEGATE
#define ID_LAUNCHDELEGATE 0xFA100400
#endif

// @stubgen:include <com/IIteratorType.h>
namespace WPEFramework {
namespace Exchange {
          
	 // @text:keep
    struct EXTERNAL ILaunchDelegate : virtual public Core::IUnknown {

        enum { ID = ID_LAUNCHDELEGATE }; // Launch Delegate ID
        public:

        virtual ~ILaunchDelegate() override = default;

	// ---- GetContentPartnerId ----
        // @json:omit
        // @text getContentPartnerId
        // @param appId: AppId of the current application.
        // @brief Get the contentPartnerId for a given application provided to the delegate
        virtual Core::hresult GetContentPartnerId(const string& appId /* @in */ , string& contentPartnerId /* @out */) = 0;
        
        };
}
}


