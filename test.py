import glob
import sys

files = []
files += glob.glob(f"{sys.argv[1]}/*.hpp")
files += glob.glob(f"{sys.argv[1]}/*.cpp")
files = sorted(files)

for path in files:
    with open(path) as f:
        text = f.read()
    if text[:5].strip().startswith("/*") and text[-5:].strip().endswith("*/"):
        print(f"skip {path}")
        continue
    print(f"fixing {path}")
    with open(path, "w") as f:
        f.write("/*\n")
        f.write(text)
        f.write("\n*/")
    print(f"fixing {path} done")
