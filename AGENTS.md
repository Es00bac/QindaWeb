# QindaWeb agent contract

QindaWeb is a native QindaTK browser for QindaQt, with one browsing view per Qt toplevel. QindaQt owns container pages, splits, geometry and placement. Gecko is the intended production engine; do not represent another development backend as Gecko.

The source of truth is qinda:~/git/QindaWeb.git; qinda checkout ~/work_SPaC3/QindaWeb. Fetch before working on the laptop, preserve unrelated work and push completed milestones and unready branches to qinda.

Keep browser changes in this repository. Coordinate toolkit changes with its AgentHost owner and desktop changes with the native application placement owner. Record exact revisions, direction and follow-ups in docs/coordination.md. Never edit an active owner's paths without agreement.

Use QindaTK as Tk, semantic theme roles, native accessible names, stable view/action IDs and C++ models. Reuse the toolkit AgentHost protocol when published; no competing external agent protocol. Consume the public desktop placement SDK; do not include compositor-private headers or handle foreign Wayland surfaces.

Install all software and runtime dependencies through Portage and QindaGentoo. Keep secrets in user configuration. Never override configured MAKEOPTS; direct builds must preserve the machine's configured job/load limits.

Keep the laptop responsive for media playback: heavy builds, native runtime tests and package compilation run on qinda at low CPU/disk priority. Laptop work is limited to editing, synchronization and brief low-priority checks unless the user changes this preference. Coordinate resource use with the desktop and toolkit owners.

Keep hand-written source files below 500 nonblank lines. Document ownership, lifecycle, threading and error behavior at public boundaries. Verify actual browsing and input, engine identity, window lifecycle and placement; fixtures or backend labels alone are not evidence that Gecko works. Native/physical claims require direct evidence. Push milestone commits after the appropriate checks pass.
