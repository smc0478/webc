# WebC Roadmap

This roadmap outlines practical next steps for turning WebC into a more production-ready C web server library.

## 1) Essential (Must-have)

These items are foundational for reliability, safety, and maintainability.

- **HTTP correctness and protocol hardening**
  - Validate request line/header parsing more strictly.
  - Enforce maximum sizes for line, headers, and body.
  - Return consistent status codes for malformed input.

- **Security baseline**
  - Prevent common parsing abuse (oversized headers/body, malformed request flooding).
  - Add safer defaults for memory and string handling paths.
  - Document supported/unsupported HTTP behavior clearly.

- **Core observability**
  - Add structured request logging (method, path, status, latency).
  - Add configurable log levels (`error`, `warn`, `info`, `debug`).

- **Robust test coverage**
  - Unit tests for parser, dictionary/list utilities, and response builder.
  - Integration tests for routing and status code behavior.
  - Regression tests for previously fixed bugs.

- **Documentation completeness**
  - API reference for all public headers.
  - Minimal production usage guide and error-handling examples.
  - Versioning and compatibility policy.

## 2) Nice-to-have (Should-have)

These features improve developer experience and real-world usability after the essentials are stable.

- **Middleware-style extension points**
  - Pre/post handler hooks (e.g., auth, request ID, custom logging).

- **Cookie and session utilities**
  - Cookie parsing/serialization helpers.
  - Simple pluggable session interface.

- **Static file serving and upload helpers**
  - Static directory mapping with basic cache headers.
  - Multipart/form-data helper utilities.

- **Configuration ergonomics**
  - Centralized server config object and environment overrides.
  - Runtime toggles for limits and logging.

- **Performance tooling**
  - Benchmark scenarios for route throughput and latency.
  - Profiling guide and optimization checklist.

## 3) Longer-term (Could-have)

- TLS termination guidance (or reverse-proxy integration guide).
- Graceful shutdown/reload behavior.
- Optional plugin ecosystem examples.

## Suggested implementation order

1. Protocol hardening + limits.
2. Logging + test expansion.
3. Documentation/API references.
4. Middleware hooks.
5. Cookie/session and static-file features.
6. Benchmarking and long-term features.
