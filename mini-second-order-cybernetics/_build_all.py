
import os, sys

BASE = os.path.dirname(os.path.abspath(__file__))

def w(relpath, content):
    path = os.path.join(BASE, relpath)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    lines = content.count("\n")
    print(f"  {relpath}: {lines} lines")
    return lines

total = 0
