"""Summarize a private read-only reflection capture without publishing it."""
import argparse
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from ammo_replication_inspection import inspect_ammo_replication


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    result = inspect_ammo_replication(json.loads(args.capture.read_text(encoding='utf-8')))
    print(json.dumps(result, indent=2))
    return 0 if result['condition_recognized'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
