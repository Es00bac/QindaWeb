#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run browser placement against an agreed, disposable QindaQt compositor."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("desktop-checkout", "kwin", "fork-lib", "fork-plugin-root", "probe", "scenario", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    desktop = args.desktop_checkout.resolve()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / "evidence.json").unlink(missing_ok=True)
    sys.path.insert(0, str(desktop / "tests/session"))
    from nested_session_scenario import (
        isolated_environment, load_virtual_spec, running_private_session_bus,
        write_virtual_output_config,
    )
    spec = load_virtual_spec(args.scenario)
    plugin = desktop / "build/dev/plugins/qindaqt-kwin/plugins/qindaqt_compositor.so"
    with tempfile.TemporaryDirectory(prefix="qindaweb-native-", dir="/tmp") as private:
        root = Path(private)
        env = isolated_environment(root)
        env.update(
            QINDAWEB_DESKTOP_CHECKOUT=str(desktop),
            QINDAWEB_NATIVE_OUTPUT=str(output),
            QINDAWEB_NATIVE_PROBE=str(args.probe.resolve()),
            QINDAWEB_EXPECTED_PLUGIN=str(plugin.resolve()),
            PYTHONDONTWRITEBYTECODE="1",
            LD_LIBRARY_PATH=str(args.fork_lib.resolve()),
            QT_PLUGIN_PATH=str(args.fork_plugin_root.resolve()) + ":" + str(args.fork_lib.resolve() / "qt6/plugins"),
        )
        # Removing capabilities is required for a virtual, unprivileged fixture.
        # Retain the fork basename because Qt uses it for plugin discovery.
        runtime = output / "runtime" / args.kwin.name
        runtime.parent.mkdir(exist_ok=True)
        shutil.copyfile(args.kwin, runtime)
        runtime.chmod(0o755)
        write_virtual_output_config(Path(env["XDG_CONFIG_HOME"]), spec)
        command = [
            str(desktop / "build/dev/src/session/qindaqt-wm"),
            "--plugin-root", str(desktop / "build/dev/plugins"),
            "--kwin", str(runtime), "--virtual", "--width", str(spec.logical_width),
            "--height", str(spec.logical_height), "--scale", str(spec.scale),
            "--output-count", str(spec.output_count), "--test-scenario", str(args.scenario.resolve()),
            "--no-lockscreen", "--no-global-shortcuts", "--session",
            str(Path(__file__).with_name("driver.py")),
        ]
        with running_private_session_bus(root, Path(shutil.which("dbus-daemon")), env):
            session = subprocess.Popen(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, start_new_session=True)
            try:
                trace, _ = session.communicate(timeout=100)
            except subprocess.TimeoutExpired as error:
                os.killpg(session.pid, signal.SIGTERM)
                try:
                    trace, _ = session.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(session.pid, signal.SIGKILL)
                    trace, _ = session.communicate(timeout=5)
                (output / "session.log").write_text(trace)
                raise RuntimeError("Private browser fixture exceeded its deadline") from error
        (output / "session.log").write_text(trace)
    evidence_file = output / "evidence.json"
    if not evidence_file.exists():
        raise RuntimeError("No browser scene evidence; inspect session.log")
    evidence = json.loads(evidence_file.read_text())
    evidence["desktopRevision"] = subprocess.check_output(["git", "-C", str(desktop), "rev-parse", "HEAD"], text=True).strip()
    evidence["probeSha256"] = hashlib.sha256(args.probe.read_bytes()).hexdigest()
    evidence["scenario"] = spec.scenario_id
    tab = evidence.get("tabSnapshot", {}).get("windows", {})
    split = evidence.get("splitSnapshot", {}).get("windows", {})
    source, target = (tab.get("Native QindaWeb " + name + " — QindaWeb", {}) for name in ("source", "tab"))
    tab_ok = bool(source.get("containerId")) and source.get("containerId") == target.get("containerId") and source.get("minimized") is True and target.get("minimized") is False
    members = [split.get("Native QindaWeb " + name + " — QindaWeb", {}) for name in ("source", "tab", "split")]
    owners = {member.get("containerId") for member in members}
    same_group = len(owners) == 1 and bool(next(iter(owners)))
    left, right = members[1].get("geometry", {}), members[2].get("geometry", {})
    geometry = bool(left) and bool(right) and left["x"] + left["width"] <= right["x"] and abs(left["y"] - right["y"]) <= 1 and abs(left["height"] - right["height"]) <= 1
    visibility = members[0].get("minimized") is True and all(member.get("minimized") is False for member in members[1:])
    grouped_scene = evidence.get("groupedTabSnapshot", {})
    grouped_members = [grouped_scene.get("windows", {}).get("Native QindaWeb " + name + " — QindaWeb", {}) for name in ("source", "tab", "split", "grouped tab")]
    grouped_owners = {member.get("containerId") for member in grouped_members}
    grouped_ok = len(grouped_owners) == 1 and grouped_owners == owners and bool(next(iter(owners))) and all(member.get("minimized") is True for member in grouped_members[:3]) and grouped_members[3].get("minimized") is False and grouped_scene.get("hybrid", {}).get("containerCount") == 1
    passed = evidence.get("probeExit") == 0 and str(plugin.resolve()) in evidence["mappedLibraries"] and tab_ok and same_group and geometry and visibility and grouped_ok
    evidence["checks"] = dict(tabAcceptedInScene=tab_ok, sameContainer=same_group, splitFrames=geometry, activePageVisibility=visibility, existingContainerTab=grouped_ok, passed=passed)
    evidence_file.write_text(json.dumps(evidence, indent=2) + "\n")
    print(json.dumps(dict(scenario=spec.scenario_id, **evidence["checks"], output=str(output)), sort_keys=True))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
