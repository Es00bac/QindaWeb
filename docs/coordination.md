# Related repository coordination

Updated 2026-09-29. Browser owns `QindaWeb feature/native-browser`; no toolkit or
WM implementation paths are modified. Source authority is qinda's hubs.

| Owner | Revision / direction | Browser contract |
| --- | --- | --- |
| Desktop manager, thread `01a0ee84-c193-7a70-a2e6-b79266b436f3` | `container-wm feature/native-application-placement`, candidate `2f067b142b18beafb7a2497e9eddd58b744440c9` | Public `WindowPlacementClient`, same requesting Qt Wayland connection; Tab and four split directions. Development prefix on qinda: `container-wm.worktrees/application-placement/.cache/sdk-stage/usr`. |
| Desktop manager | Shared semantic commands, captured target contexts and fractional placement; gathered-window visibility, lock screen and keyring work active | Interactive preview/confirm/cancel should reuse keyboard/mouse/touch/pen Hybrid interactions. V1 lacks the entrypoint. Browser's four-side picker is interim, not the future shared chooser. |
| Toolkit agent, thread `01a0eeb7-bc25-7422-9347-bc36efa21829` | Integrated main `2d5e308`, SDK code pin `1d14371469f6684005846b74db306c0201bd1d6e`; Portage r13 installed on both hosts | Existing AppWindow, QST and commands unchanged. Public `QindaTK::agent` target: AgentProvider/Host/Server, stable IDs, typed actions, opaque live revisions and Off/Read/Edit grants. Available for a later browser provider; no competing external agent protocol. |

Both owners know the native Qt chrome/private Gecko Wayland host direction.
Gecko's internal surfaces never enter the desktop SDK: Qt owns the outer windows.

Both owners received the dismissible-UI/global-menu requirement. Toolkit advice:
keep commands alive independently of visible controls and menus; export semantic
models rather than rendered MenuItems. Browser owns its CommandRegistry MenuSource,
public dbusmenu exporter and KWayland native announcement. No Qt-private API used.

Appearance now uses the public asynchronous SettingsClient, validated owner/revision
snapshots, ThemeCatalog and TokenFacade, then the QindaTK.QindaQt bridge. The toolkit
owner was told about partial personal theme directories shadowing the installed
catalog; exact selected IDs are resolved through ordered user/system directories.

2026-09-29 resource steering: user is watching anime on the laptop. All owners
agreed to move heavy compilation and runtime qualification to qinda. The toolkit
owner stopped its local Office build and reported available laptop RAM recovering
to 7.7 GiB. Browser laptop native tests are paused; only light, low-priority checks
and editing/synchronization remain there. Configured MAKEOPTS are unchanged.

Desktop coordination note on qinda:
`container-wm/ops/team/messages/plasma-free-recovery/20260929T214007Z-qindaweb-placement-coordination.md`.
Reconfirm owner, current revision and shared contracts before cross-repository
changes/native scene runs. Send exact browser commits and evidence at milestones.

Packaging owns only new `www-client/qindaweb` and a verbatim Qt 6.11.1 compositor
compatibility recipe on `QindaGentoo feature/qindaweb-packaging`. The latter avoids
an unrelated Qt upgrade on qinda after Gentoo removed that version. Installs use
Portage with existing machine job/load configuration.
