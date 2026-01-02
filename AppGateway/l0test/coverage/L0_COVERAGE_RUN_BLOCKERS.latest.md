# L0 coverage run blockers (latest)

## Blocker: Missing `WPEFramework::string` alias in stub SDK

Headers under `dependencies/install/include/WPEFramework/plugins/*` use `string` without qualification.
Fix by adding a top-level alias in `dependencies/install/include/WPEFramework/core/core.h`:

```cpp
namespace WPEFramework { using string = Core::string; }
```
