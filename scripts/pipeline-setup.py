#!/usr/bin/env python3

import subprocess
import sys

def run_command(cmd):
    process = subprocess.Popen(cmd, shell=True, text=True)
    process.communicate()
    if process.returncode != 0:
        print(f"Command '{cmd}' failed with exit code {process.returncode}")
        sys.exit(process.returncode)

commands = [
    "pip3 install -U west",
    "west init -l .",
    "west update",
    "pip3 install -r ../zephyr/scripts/requirements-base.txt"
]

for cmd in commands:
    run_command(cmd)