#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Runs solely as the private compositor's exit-with-session child."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def main():
    desktop = Path(os.environ["QINDAWEB_DESKTOP_CHECKOUT"])
    sys.path.insert(0, str(desktop / "tests/session/shade_visibility"))
    from shade_control import connect
    control = connect()
    output = Path(os.environ["QINDAWEB_NATIVE_OUTPUT"])
    mapped = sorted({line.split()[-1] for line in Path(f"/proc/{os.getppid()}/maps").read_text().splitlines() if "qindaqt_compositor.so" in line})
    env = dict(os.environ, QINDAWEB_NATIVE_PLACEMENT="1", QT_QUICK_BACKEND="software")
    evidence = dict(mappedLibraries=mapped, snapshots=[])
    titles = {name: "Native QindaWeb " + name + " — QindaWeb" for name in ("source", "tab", "split", "grouped tab")}
    with (output / "probe.log").open("w") as log:
        child = subprocess.Popen([os.environ["QINDAWEB_NATIVE_PROBE"], "-v1"], env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 80
            while child.poll() is None and time.monotonic() < deadline:
                snapshot = dict(hybrid=control.hybrid(), windows=control.windows())
                evidence["snapshots"].append(snapshot)
                trace = (output / "probe.log").read_text()
                # Preserve a live scene after each completion; the final
                # surface-teardown snapshot cannot prove placement geometry.
                if "QINDAWEB_NATIVE_TAB_READY" in trace and "QINDAWEB_NATIVE_SPLIT_PENDING" not in trace and {titles["source"], titles["tab"]}.issubset(snapshot["windows"]):
                    evidence["tabSnapshot"] = snapshot
                if "QINDAWEB_NATIVE_SPLIT_READY" in trace and "QINDAWEB_NATIVE_GROUP_TAB_PENDING" not in trace and {titles[name] for name in ("source", "tab", "split")}.issubset(snapshot["windows"]):
                    evidence["splitSnapshot"] = snapshot
                if "QINDAWEB_NATIVE_GROUP_TAB_READY" in trace and set(titles.values()).issubset(snapshot["windows"]):
                    evidence["groupedTabSnapshot"] = snapshot
                time.sleep(0.05)
            if child.poll() is None:
                child.terminate()
            evidence["probeExit"] = child.wait(timeout=5)
        finally:
            if child.poll() is None:
                child.kill()
                child.wait(timeout=5)
    (output / "evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    return evidence["probeExit"]


if __name__ == "__main__":
    sys.exit(main())
