# QindaWeb

A native QindaTK browser for the QindaQt desktop. Each browsing view owns one Qt window; the desktop provides tabs and splits across those windows.

The initial experimental host uses installed Firefox/QindaFox: real Gecko Wayland
buffers inside native Qt/QindaTK windows. No Chromium backend is included.
The [architecture](docs/architecture.md) describes this host and a later direct
Gecko adapter. [Coordination](docs/coordination.md) tracks toolkit and desktop work.

Build and test on qinda at low priority while the laptop is used for playback.
Install through Portage. Native development requires the public placement SDK:

```sh
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DQINDAWEB_REQUIRE_DESKTOP_SDK=ON -DCMAKE_PREFIX_PATH="$sdk_prefix"
nice -n 10 ionice -c 3 cmake --build build/dev --parallel 8 -- -l24
nice -n 10 ionice -c 3 ctest --test-dir build/dev --output-on-failure
build/dev/qindaweb https://example.org
```

Dependencies: Qt 6.11 Quick, WebSockets and WaylandCompositor; QindaTK with its
QindaQt bridge; QindaQt's public menu/settings/theme libraries; KWayland; installed
Firefox/QindaFox with Wayland support. Qt WaylandCompositor needs the overlay's
multiple-keyboard-resource patch: Firefox's GTK and Mozilla listeners share one
Wayland client. Unpatched Qt silently loses normal page typing. An absent or
refusing compositor keeps the new window separate and reports the result.
`QINDAWEB_REQUIRE_DESKTOP_SDK=OFF` explicitly permits building that fallback.

The manager currently allocates eight browser compilation jobs within qinda's
configured maximum of 24 jobs/load 24. Reconfirm concurrent capacity before
increasing it; preserve configured MAKEOPTS for Portage. No laptop build or
native runtime test is part of this workflow.

Ctrl+T requests a container tab, Ctrl+Shift+Enter creates a split candidate and offers its side,
Ctrl+N opens a separate window, Ctrl+L focuses the address field and Ctrl+W closes
the page. The side picker accepts arrow keys and native pointer/touch/pen button
activation. The desktop's richer placement chooser is still being developed.

Ctrl+Shift+H hides or restores every browser control band. In hidden mode the
page fills the native window and its minimum size permits small corner panes.
Ctrl+L temporarily reveals only the address field; Enter or Escape dismisses it.
File, Navigate and View remain exported to the global menu. Local menus disappear
automatically only after valid publication and detected native app-menu support.

`qindaweb --hide-controls https://example.org` starts without controls. Repeated
launches forward their addresses to the process that owns the profile. The browser
creates and marks its own Firefox profile; it refuses to overwrite existing profiles.

Heavy builds and runtime tests run on qinda while the laptop is in use. See
[acceptance](docs/acceptance.md) for verified behavior and remaining qualification.
`www-client/qindaweb-0.1.0_alpha1` is installed on qinda and the laptop through
signed qinda-built Portage binaries, including QtWayland's keyboard fix. Launch
**QindaWeb** from the application menu or run `qindaweb`.

This installed preview opens ordinary windows. The native Portage revision awaits
the desktop owner's consolidated SDK package; automatic container tabs and splits
are not delivered by the preview. Development staging is never copied into an
installed browser.
