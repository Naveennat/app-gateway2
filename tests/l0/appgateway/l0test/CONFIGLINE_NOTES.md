# ConfigLine in AppGateway L0 harness (why other build systems may see invalid JSON)

## Background

The AppGateway plugin (and Thunder in general) may call:

- `IShell::Root<T>(...)`

That path constructs `WPEFramework::Plugin::Config::RootConfig(const IShell* info)` which does:

```cpp
RootObject config;
config.FromString(info->ConfigLine(), error);
```

`RootObject` expects a **JSON object** at the top level containing a `"root"` field:

```json
{ "root": "<json-string>" }
```

If the incoming string starts with `"` (a quoted blob) or is empty/garbled, Thunder logs:

> Parsing failed: Invalid value. "null" or "{" expected.  
> At character 1: "

## How ConfigLine is supplied in this repo’s L0 harness

The L0 tests do **not** use the real Thunder host. They create a plugin instance and pass a test shell:

- `tests/l0/appgateway/l0test/ServiceMock.h` implements `PluginHost::IShell`.

`ServiceMock::ConfigLine()` is the authoritative source of ConfigLine for L0:

- Default (used by most tests):

```cpp
return "{\"root\":\"{}\"}";
```

- Optional override:

```cpp
if (!_cfg.configLineOverride.empty()) return _cfg.configLineOverride;
```

Some responder tests override this to pass plugin-specific websocket config.

## Why another build system can fail

If the test runner in another build system does not use `ServiceMock` (or uses a different IShell),
`info->ConfigLine()` may be:

- empty
- a JSON string (double-encoded), e.g. `"{"root":"{}"}"`
- some other non-object string

Any of these will break `RootConfig` parsing and produce the “`At character 1: "`” parse error.

## What to check in the failing environment

1. Confirm the IShell passed to `AppGateway::Initialize()` is the L0 `ServiceMock`.
2. Log or print `service->ConfigLine()` before any `Root<T>()` call.
3. Ensure it is a JSON object like:

```json
{"root":"{}"}
```

(not a quoted string containing that JSON).
