#! /usr/bin/env python3

import sys

from sys import argv
from pathlib import Path

from delta_debugger import run_target
from delta_debugger.delta import delta_debug


def exist_check(file):
    if not Path(file).exists():
        print(f"{file} not found", sys.stderr)
        return 1
    return 0


def main(*args) -> int:
    if len(args) < 2:
        print(f"usage: {sys.argv[0]} [target] [crashing input file]")
        return 1
    target, input_file = args[0], args[1]
    if not Path(target).exists():
        print(f"{target} not found", file=sys.stderr)
        return 1
    if not Path(input_file).exists():
        print(f"{input_file} not found", file=sys.stderr)
        return 1

    with open(input_file, "rb") as fp:
        file_input = fp.read()
        if not run_target(target=target, input=file_input):
            print(
                "Sanity check failed: the program does not crash with the initial input",
                file=sys.stderr,
            )
            return 1

    delta_debugging_result = delta_debug(target=target, program_input=file_input)

    print(
        f"Original Input Size: {len(file_input)}",
        f"Minimized Input Size: {len(delta_debugging_result)}",
        sep="\n",
    )
    with open(f"{input_file}.delta", "wb") as fp:
        fp.write(delta_debugging_result)
    return 0


if __name__ == "__main__":
    """
    usage: delta-debug [target] [crashing input file]
    """
    sys.exit(main(*sys.argv[1:]))
