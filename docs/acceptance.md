# Development acceptance

2026-09-29. This is the first experimental Gecko host, not a production-qualified
desktop Gecko embedding SDK. Runtime tests always use a new owned profile.

Verified on qinda-top's actual QindaQt Wayland desktop before the resource pause:

- Firefox 155.0.1 identified itself through session capabilities and Gecko UA.
- Real private Wayland buffers rendered the local HTML fixture in QindaTK chrome.
- Native pointer and keyboard forwarding typed `gecko` into the page's input.
- Ctrl+L focused the native address field; Ctrl+T created distinct Qt windows/views.
- Hidden controls expanded content to the full window, while File/Navigate/View
  remained on the published D-Bus wire endpoint.
- A D-Bus menu activation restored the controls; native app-menu support was detected.
- Input coordinate mapping worked with observed desktop scales 1.25 and 1.7.

The latest popup routing, theme-directory resolution and split focus fixes are
compiled; combined runtime qualification continues on qinda. Do not infer accepted
container topology from a client request or from a separate-window fallback.

Routine checks: C++ build; navigation and bounded cross-process launch IPC tests.
Explicit runtime executable: `build/dev/tst_browser -v1`. Screenshots are local
artifacts under `build/acceptance/`, excluded from source control.

Remaining qualification: native public-SDK tab/split acceptance against the desktop
candidate; the shared interactive placement entrypoint; content accessibility
relationships, IME, clipboard/drag-and-drop, prompts, downloads, persistence and crash
recovery. Keyboard/mouse/touch/pen picker buttons exist; physical pen/touch operation
has not been claimed from synthetic events. GPU acceleration has not been established.
