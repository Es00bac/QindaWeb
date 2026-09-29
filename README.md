# QindaWeb

A native QindaTK browser for the QindaQt desktop. Each browsing view owns one Qt window; the desktop provides tabs and splits across those windows.

The initial experimental host uses installed Firefox/QindaFox: real Gecko Wayland
buffers inside native Qt/QindaTK windows. No Chromium backend is included.
The [architecture](docs/architecture.md) describes this host and a later direct
Gecko adapter. [Coordination](docs/coordination.md) tracks toolkit and desktop work.

Build for development; install through Portage:

```sh
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build/dev -j32 -l16 # qinda-top; qinda uses its configured -j24 -l24
ctest --test-dir build/dev --output-on-failure
build/dev/qindaweb https://example.org
```

Dependencies: Qt 6.11 Quick, WebSockets and WaylandCompositor; QindaTK; installed
Firefox/QindaFox with Wayland support. Add the public QindaQt placement SDK prefix
and `-DQINDAWEB_REQUIRE_DESKTOP_SDK=ON` to require native container integration.
An absent SDK/compositor keeps separate windows.

Ctrl+T creates a container tab, Ctrl+Alt+T creates a split and offers its side,
Ctrl+N opens a separate window, Ctrl+L focuses the address field and Ctrl+W closes
the page. The side picker accepts arrow keys and native pointer/touch/pen button
activation. The desktop's richer placement chooser is still being developed.
