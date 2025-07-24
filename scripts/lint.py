#!/usr/bin/env python3
from argparse import ArgumentParser
from pathlib import Path
from subprocess import check_output, check_call, CalledProcessError, STDOUT

RED = "\x1b[31m"
GREEN = "\x1b[32m"
BOLD = "\x1b[1m"
RESET = "\x1b[0m"

PRJ_ROOT = Path(__file__).resolve().parent.parent


def get_all_files():
    all_files = check_output(["git", "-C", PRJ_ROOT, "ls-files"], text=True).split("\n")
    return all_files


def get_staged_files():
    # Get staged files but exclude deleted files
    staged_with_status = (
        check_output(["git", "-C", PRJ_ROOT, "diff", "--cached", "--name-status"], text=True).strip().split("\n")
    )

    # Filter out deleted files (those starting with 'D')
    staged_files = []
    for line in staged_with_status:
        if line and not line.startswith("D"):
            # Extract just the filename (after the status letter and tab)
            parts = line.split("\t")
            if len(parts) > 1:
                staged_files.append(parts[-1])

    return staged_files


def files_to_check(check_all=False):
    exclude_folders = []

    files = get_all_files() if check_all else get_staged_files()

    return [
        f
        for f in files
        if (f.endswith(".c") or f.endswith(".h")) and not any(f.startswith(folder) for folder in exclude_folders)
    ]


def main():
    parser = ArgumentParser(
        description="Check and reformat code using clang-format",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--check",
        "-c",
        action="store_true",
        help="show formatting diff without applying changes",
    )
    group.add_argument(
        "--apply",
        "-a",
        action="store_true",
        help="apply formatting changes to files",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="check all files instead of only staged files",
    )
    args = parser.parse_args()

    files = files_to_check(check_all=args.all)

    if not len(files):
        file_scope = "files" if args.all else "staged files"
        print(f"{RED}{BOLD}No {file_scope} to format{RESET}")
        exit(0)
    try:
        check_call(
            ["python", PRJ_ROOT / "scripts" / "run-clang-format.py"] + (["-i"] if args.apply else []) + files,
            stderr=STDOUT,
        )
    except CalledProcessError as e:
        print(f"{RED}{BOLD}clang-format found issues. Run with --apply (-a) to fix automatically{RESET}")
        exit(e.returncode)


if __name__ == "__main__":
    main()
