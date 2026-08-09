# Service Registry

**Milestone:** 0.1.0 — Alpha · Core Runtime
**Status:** Implemented and tested
**Related ADRs:** ADR-0004 (Service Registry), ADR-0015 (Dependency Injection),
ADR-0014 (No Global State), ADR-0026 (Free Functions Over Static Classes)

---

## Purpose

The Service Registry is the container that holds the engine's core services.
Subsystems no longer reach out to find one another — they are given what they
need. A system that needs logging, configuration, or time receives them through
the registry instead of constructing or locating them itself.

This is the Service Architecture stone of Milestone 0.1.0.

---

## Interface

```cpp
namespace LCE::Runtime
{
    class ServiceRegistry
    {
    public:
        ServiceRegistry() = default;
        ~ServiceRegistry() = default;

        ServiceRegistry(const ServiceRegistry&) = delete;
        ServiceRegistry& operator=(const ServiceRegistry&) = delete;

        template <typename T>
        void Register(std::shared_ptr<T> service);

        template <typename T>
        [[nodiscard]] bool Has() const noexcept;

        template <typename T>
        [[nodiscard]] std::shared_ptr<T> Get() const noexcept;

    private:
        std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
    };
}
```

### Semantics

| Method | Behaviour |
|--------|-----------|
| `Register<T>(service)` | Stores the service under the type `T`. Overwrites any previous registration — implementations are replaceable. |
| `Has<T>()` | Returns whether a service of type `T` is registered. |
| `Get<T>()` | Returns the registered service as `std::shared_ptr<T>`, or an empty pointer if none is registered. |

Registration is keyed by the *static* type `T` passed at the call site.
`Register<Derived>` is found by `Get<Derived>`, not `Get<Base>` — register under
the interface you intend to consume.

---

## Responsibilities

- Own a single, ordered set of services for the engine runtime.
- Provide services to consumers through typed access.
- Permit implementations to be swapped (replaceable — ADR-0004).

## Non-Responsibilities

- Does **not** construct services. Constructors stay explicit; the registry
  stores what it is given (ADR-0015).
- Does **not** know what a service is or does.
- Does **not** hold global state. Each engine owns its registry (ADR-0014).

---

## Dependencies

- **Upward:** nothing.
- **Downward:** standard C++ only (`<memory>`, `<typeindex>`, `<unordered_map>`).
- **Third-party:** none.

The implementation is a header-only template: type erasure through
`std::shared_ptr<void>` requires the definition to be visible at the call site,
which keeps the registry dependency-free and trivially testable.

---

## Why This Shape

1. **Type-safe.** A service is fetched by its C++ type, not a string name.
   A wrong key is a compile error, not a silent runtime miss.
2. **Simple.** The entire implementation is one map and three template
   functions. Design Law 001: simple things should be simple.
3. **Testable by design** (ADR-0004). A test registers a fake and verifies the
   consumer receives it — no global state to reset, no singleton to mock.

## Why `LCE::Runtime`

`Include/LCE/Runtime/` already exists as the home of the systems active while
the engine is running (the README's Runtime layer: ModuleLoader · Scheduler ·
EventBus). The registry is the runtime's container, so it lives beside them.
If the Services layer from the architecture diagram becomes a real folder, the
registry moves with it — one namespace rename, no behaviour change.

---

## Test Plan

| Case | Expectation |
|------|-------------|
| `Get<T>()` before any registration | Empty pointer. |
| `Has<T>()` before any registration | `false`. |
| `Register<T>()` then `Has<T>()` | `true`. |
| `Register<T>()` then `Get<T>()` | Same instance (pointer equality), state intact. |
| Overwrite: register two instances of `T` | `Get<T>()` returns the second. |
| Unrelated types | Registrations do not collide. |

All cases run in the harness as `ServiceRegistryTest`.
