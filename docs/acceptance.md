# Development acceptance

2026-09-30 UTC. Experimental Gecko host; production embedding qualification remains
open. Runtime tests use a fresh QindaWeb-owned profile and private display/bus.

Qualified browser code: `82b091aa2f32570a1226ee135233d364242fa28b`.
Runtime: Portage-installed Firefox 155.0.1 and Qt WaylandCompositor 6.11.1-r2 from
`QindaGentoo feature/qindaweb-packaging`, patch commit `f35ce16`.
Public desktop SDK implementation: `6db8864bf1ce4eb59f87153a1478af42eebe8297`.

On qinda, all three routine CTest gates passed: raw keyboard resources, navigation,
and bounded cross-process launch IPC. The raw wire gate covers two listeners on
one explicit wl_client, late binding, representative release, focus transfer,
last-binding release and public modifier publication. It failed at modifier
publication with r1 and passed with the completed r2 package.

The combined native suite passed **5 tests, no failures or skips**, independently
at actual compositor scales 1.0 and 1.5. It verified:

- Firefox session capabilities and Gecko UA, real private Wayland buffers and pixels.
- Pointer coordinate mapping, normal scan-code C/N/V input and DOM keydown/text.
  GTK keyboard 15 and Mozilla keyboard 28 were enumerated on the page surface's
  same wl_client. The private compositor's normal input chain delivered the keys.
- Real web-created popup navigation, native adoption and page-driven closure.
- Hidden controls filling the native window, temporary Ctrl+L address and dismissal.
- File/Navigate/View and checked actions on the D-Bus menu endpoint while hidden;
  D-Bus activation restored controls and native app-menu support was detected.
- Ctrl+T from a solo window, Ctrl+Shift+Enter plus Right, and Ctrl+T from the resulting
  split each received real accepted public-SDK completion and the same container ID.
- Read-only desktop scene evidence confirmed active-page visibility, separate split
  frames and one container after adding the final tab. Grouped hidden UI stayed hidden.

Remote evidence is under `QindaWeb/build/native-r2-100/` and `native-r2-150/`, including
probe/session logs, scene snapshots, scenario, source context and binary hashes.
Both runs mapped `container-wm/build/dev/plugins/qindaqt-kwin/plugins/qindaqt_compositor.so`,
SHA256 `fc3c7cb81ede6f20aaf4d27bddfe6521ae7940cf23a43edb958e7fbd0adf77fe`.
Desktop checkout context was `111e9c826ad18c24547218b463b733ec8b961a39`; that context
alone is not proof of compiled plugin identity. Browser probe SHA256 was
`edf2414bd6fc2025c33616831314110bdc076a11f107ecf63510113db4a42443`.
Screenshots under `build/acceptance/` show the most recent run. Artifacts are ignored.

Earlier qinda-top checks observed live Settings1 theme publication and desktop scales
1.25 and 1.7. Their zero-scan QtTest text used the IME path and did not prove ordinary
keyboard delivery; the remote native gates above establish that behavior. Laptop
builds/runtime tests are now paused for media playback.

Remaining: the desktop owner's consolidated **Portage-owned installed SDK** and exact
minimum package revision, shared interactive preview/confirm/cancel selector,
content accessibility relationships, IME, clipboard/drag-and-drop, prompts, downloads,
profile persistence and crash recovery. GPU acceleration and physical pen/touch have
not been qualified. The local four-side picker accepts native keyboard and pointer
controls; the richer compositor selector is still being developed.

The browser package candidate requires the public SDK and patched Qt. It remains an
explicit `.ebuild.in` template until the SDK-owning desktop minimum is provided and
installed-package acceptance passes. No development library is copied into a release.
