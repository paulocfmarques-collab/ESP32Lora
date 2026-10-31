#!/usr/bin/env python3
"""Read-only UDP client for the ESP32 LoRa gateway (Python standard library only)."""
import argparse
import json
import socket
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", help="Transmitter IP shown on OLED page 3")
    parser.add_argument("commands", nargs="*", default=["help"])
    parser.add_argument("--port", type=int, default=4210)
    parser.add_argument("--timeout", type=float, default=None, help="Seconds: default 3 for local queries, 75 for command:RX01")
    parser.add_argument("--raw", action="store_true", help="Do not pretty-print JSON")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535 or (args.timeout is not None and args.timeout <= 0):
        parser.error("Use a valid port and a positive timeout.")
    try:
        address = socket.gethostbyname(args.host)
    except OSError as exc:
        parser.error(str(exc))
    failures = 0
    for command in args.commands or ["help"]:
        base, separator, target = command.partition(":")
        command = base.strip().lower() + (separator + target.strip() if separator else "")
        try:
            payload = command.encode("ascii")
            if not 1 <= len(payload) <= 255:
                raise ValueError("Commands must contain 1 to 255 ASCII bytes.")
            # A separate socket prevents a delayed response to an earlier query
            # from being mistaken for the response to the next command.
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
                udp.settimeout(args.timeout if args.timeout is not None else (75.0 if ":" in command else 3.0))
                udp.connect((address, args.port))
                udp.send(payload)
                response = udp.recv(4096).decode("utf-8", errors="replace")
            print(f"[{command}]")
            if response.lstrip().startswith("ERRO:"):
                failures += 1
            base_command = command.split(":", 1)[0].strip().lower()
            if not args.raw and base_command in {"status_json", "info_json"}:
                try:
                    print(json.dumps(json.loads(response), indent=2, ensure_ascii=False))
                except json.JSONDecodeError:
                    print(response.rstrip())
            else:
                print(response.rstrip())
        except (OSError, ValueError, UnicodeError) as exc:
            print(f"[{command}] {exc}", file=sys.stderr)
            failures += 1
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
