# Related repository coordination

Updated 2026-09-30 UTC. Browser owns `QindaWeb feature/native-browser`; no toolkit or
WM implementation paths are modified. Source authority is qinda's hubs.

| Owner | Revision / direction | Browser contract |
| --- | --- | --- |
| Desktop manager, thread `01a0ee84-c193-7a70-a2e6-b79266b436f3` | Accepted SDK implementation `6db8864bf1ce4eb59f87153a1478af42eebe8297`, integrated main initially `deba819`, native fixture context now `111e9c8` | Public `WindowPlacementClient`, same requesting Qt Wayland connection; Tab and four split directions. Targets `QindaQt::ApplicationWindowClient` / `ApplicationWindowProtocol`. Development prefix: `container-wm.worktrees/application-placement/.cache/sdk-stage/usr`. |
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
The owner's preferred future composition is the public
`ApplicationAppearanceController::publishTokens(TokenFacade&, error)` with
`appearanceChanged`; import `QindaQt.Tokens` before obtaining its singleton. This
module is not in the current installed desktop SDK, so no private Welcome helper
is imported or copied. Current publication already uses public complete themes.

2026-09-29 resource steering: user is watching anime on the laptop. All owners
agreed to move heavy compilation and runtime qualification to qinda. The toolkit
owner stopped its local Office build and reported available laptop RAM recovering
to 7.7 GiB. Browser laptop native tests are paused; only light, low-priority checks
and editing/synchronization remain there. Configured MAKEOPTS are unchanged.

Manager allocation: browser direct compilation uses at most eight jobs/load 24,
inside qinda's configured maximum 24/24; retain at least 4 GiB available. The manager
reserved an exclusive configured Portage 24/24 window for QtWayland r2 and received
completion/release promptly. Toolkit/Office builds and delivery were completed and
their remote slot was idle. No laptop compiler or native browser job was restarted.

Desktop coordination note on qinda:
`container-wm/ops/team/messages/plasma-free-recovery/20260929T214007Z-qindaweb-placement-coordination.md`.
The manager's durable reply is
`container-wm/ops/team/messages/plasma-free-recovery/20260929T224800Z-qindaweb-sdk-capacity.md`;
its outbound TUI messaging was unavailable after restart, so this board is monitored.
Reconfirm owner, current revision and shared contracts before cross-repository
changes/native scene runs. Send exact browser commits and evidence at milestones.

Packaging owns only new `www-client/qindaweb` and Qt 6.11.1 compatibility/fix recipes
on `QindaGentoo feature/qindaweb-packaging`. QtWayland r2 commit `f35ce16` fans keyboard
state out to every same-client listener without public ABI or object-layout changes.
Toolkit owner reviewed it read-only and identified the missing explicit modifier
path, preserved as a separate r2 candidate. Portage installed r2 on qinda; the raw
wire test and full native suites at 100%/150% passed. See [acceptance](acceptance.md).

The native browser recipe pins qualified code `82b091a`, exact patched Qt 6.11.1-r2,
QindaTK r13 and requires the public desktop SDK. The desktop owner explicitly holds
consolidated packaging until source qualification and an external installed-SDK
consumer passes. Its exact minimum revision is pending: the recipe is `.ebuild.in`
with that one placeholder, not a fallback delivery. No shared overlay main/delivery
record, active desktop checkout or live session was modified. Office's published
overlay master `109ddbf` and toolkit r13 were preserved as ancestors of our branch.
