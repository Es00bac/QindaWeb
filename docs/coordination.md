# Related repository coordination

Updated 2026-09-29. Browser owns `QindaWeb feature/native-browser`; no toolkit or
WM implementation paths are modified. Source authority is qinda's hubs.

| Owner | Revision / direction | Browser contract |
| --- | --- | --- |
| Desktop manager, thread `01a0ee84-c193-7a70-a2e6-b79266b436f3` | `container-wm feature/native-application-placement`, candidate `2f067b142b18beafb7a2497e9eddd58b744440c9` | Public `WindowPlacementClient`, same requesting Qt Wayland connection; Tab and four split directions. Development prefix on qinda: `container-wm.worktrees/application-placement/.cache/sdk-stage/usr`. |
| Desktop manager | Shared semantic commands, captured target contexts and fractional placement; gathered-window visibility, lock screen and keyring work active | Interactive preview/confirm/cancel should reuse keyboard/mouse/touch/pen Hybrid interactions. V1 lacks the entrypoint. Browser's four-side picker is interim, not the future shared chooser. |
| Toolkit agent, thread `01a0eeb7-bc25-7422-9347-bc36efa21829` | Baseline `becfc7d`, `feature/agent-host` published at `e4768cc`; Calc integration in progress | Existing AppWindow, QST and commands unchanged. New `QindaTK::agent` target: AgentProvider/Host/Server, stable IDs, typed actions, opaque live revisions and Off/Read/Edit grants. Consume after integration/install agreement; no competing external protocol. |

Both owners know the native Qt chrome/private Gecko Wayland host direction.
Gecko's internal surfaces never enter the desktop SDK: Qt owns the outer windows.

Desktop coordination note on qinda:
`container-wm/ops/team/messages/plasma-free-recovery/20260929T214007Z-qindaweb-placement-coordination.md`.
Reconfirm owner, current revision and shared contracts before cross-repository
changes/native scene runs. Send exact browser commits and evidence at milestones.

Packaging owns only new `www-client/qindaweb` and a verbatim Qt 6.11.1 compositor
compatibility recipe on `QindaGentoo feature/qindaweb-packaging`. The latter avoids
an unrelated Qt upgrade on qinda after Gentoo removed that version. Installs use
Portage with existing machine job/load configuration.
