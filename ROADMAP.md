# WebC Feature Roadmap (C/C++ Web Framework 방향)

This roadmap is organized by feature area so WebC can grow from a lightweight C HTTP library into a practical C/C++ web framework runtime.

## 1) HTTP Core
### Must-have
- [ ] HTTP/1.1 keep-alive + `Connection` header behavior
- [ ] Proper status text table for common 1xx~5xx codes
- [ ] `Content-Type` auto helpers (`text/plain`, `application/json`, etc.)
- [ ] Chunked transfer decoding for requests
- [ ] Chunked transfer encoding for streaming responses

### Nice-to-have
- [ ] HTTP pipelining safety checks
- [ ] HTTP parser hardening against malformed headers/body boundaries

## 2) Router & Middleware
### Must-have
- [ ] Path params: `/users/:id`
- [ ] Wildcard routes: `/static/*`
- [ ] Route grouping with prefix: `/api/v1`
- [ ] Middleware chain (before/after handler)
- [ ] Method fallback / 405 Method Not Allowed

### Nice-to-have
- [ ] Trie-based router for faster matching
- [ ] Mount sub-apps/modules

## 3) Request / Response API
### Must-have
- [ ] Case-insensitive header lookup
- [ ] URL decode utility for query/form values
- [ ] JSON request parsing hooks (cJSON / yyjson adapter)
- [ ] Binary-safe body buffer APIs (length + pointer)
- [ ] Response builder with append/write semantics

### Nice-to-have
- [ ] Typed query extractors (`int`, `bool`, `double`) with validation
- [ ] Multipart/form-data parser for file uploads

## 4) Concurrency & Runtime
### Must-have
- [ ] Multi-client handling (thread-per-conn or worker pool)
- [ ] Graceful shutdown (signal handling + connection drain)
- [ ] Configurable socket timeouts
- [ ] Optional non-blocking I/O mode

### Nice-to-have
- [ ] Event loop backend (`epoll`/`kqueue`)
- [ ] Work-stealing thread pool

## 5) Static Files & Templating
### Must-have
- [ ] `sendfile`-based static file serving
- [ ] MIME type mapping table
- [ ] Directory traversal guard (`..` sanitization)
- [ ] ETag / Last-Modified / conditional GET

### Nice-to-have
- [ ] Minimal C template engine hooks
- [ ] C++ adapter for templating engines

## 6) Session, Cookie, Auth
### Must-have
- [ ] Cookie parser + serializer
- [ ] Signed cookie support
- [ ] In-memory session store
- [ ] Pluggable session backend interface

### Nice-to-have
- [ ] Redis session backend
- [ ] JWT middleware helper

## 7) Observability & DX
### Must-have
- [ ] Structured logger (level + timestamp + request id)
- [ ] Access log format (Nginx-like)
- [ ] Error object / error propagation conventions
- [ ] Better startup/runtime diagnostics

### Nice-to-have
- [ ] Prometheus metrics endpoint
- [ ] OpenTelemetry trace hooks

## 8) Security
### Must-have
- [ ] Header size / body size limits
- [ ] Slowloris mitigation (read timeout + header deadline)
- [ ] Basic CORS middleware
- [ ] Security headers middleware (`X-Content-Type-Options`, CSP baseline)

### Nice-to-have
- [ ] TLS termination integration guidance
- [ ] mTLS support option

## 9) C++ Framework Layer (Optional but recommended)
### Must-have
- [ ] C API stability policy (semver + ABI notes)
- [ ] C++ RAII wrappers for request/response/server objects
- [ ] Strongly typed route handler interface (`std::function` adapter)
- [ ] Header-only convenience wrapper for rapid prototype

### Nice-to-have
- [ ] Coroutine-friendly async wrapper (C++20)
- [ ] CMake package export + Conan/vcpkg metadata

## 10) Testing & Release Engineering
### Must-have
- [ ] Unit tests for parser/router/dict
- [ ] Integration tests with curl-based golden cases
- [ ] Fuzz tests for HTTP parser
- [ ] Sanitizer CI (`ASan`, `UBSan`)
- [ ] Compiler matrix (gcc/clang)

### Nice-to-have
- [ ] Benchmark suite (RPS, latency p95/p99)
- [ ] ABI compatibility check in CI

---

## Suggested implementation order (short)
1. HTTP core hardening + router params
2. middleware + better request/response API
3. concurrency runtime + graceful shutdown
4. static files + cookie/session
5. logging/security/test+CI
6. C++ wrapper layer
