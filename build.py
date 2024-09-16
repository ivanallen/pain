#!/bin/python3
import argparse
import subprocess

def run_in_shell(cmd):
    results = subprocess.run(cmd, shell=True, universal_newlines=True, check=True)
    if results.stdout:
        print(results.stdout)

def format_project():
    run_in_shell('find src -iname "*.h" -o -iname "*.cc" | xargs clang-format -i')
    run_in_shell('git diff')

def main(args):
    if args.subparser_name == 'format':
        format_project()
    elif args.subparser_name == 'build':
        pass

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Description of your program')
    parser.add_argument('-g', '--global')
    subparsers = parser.add_subparsers(dest="subparser_name")
    format_parser = subparsers.add_parser('format')
    build_parser = subparsers.add_parser('build')
    args = parser.parse_args()
    main(args)