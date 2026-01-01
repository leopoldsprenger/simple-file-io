import time
import os
import sys
import ctypes

# ---------------- Arguments ----------------
filename = sys.argv[1] if len(sys.argv) > 1 else "bench_test.log"
DATA_SIZE = int(sys.argv[2]) if len(sys.argv) > 2 else 10_000_000  # 10 MB

# ---------------- Data preparation ----------------
data_str   = "A" * DATA_SIZE
data_bytes = b"A" * DATA_SIZE

# Build lines ~1 KB each to reach ~10 MB total
line_size = 1024
lines = ["A" * (line_size - 1) + "\n" for _ in range(DATA_SIZE // line_size)]

# ---------------- Cache advisory ----------------
libc = None
try:
    libc = ctypes.CDLL(None)
    POSIX_FADV_DONTNEED = 4
except Exception:
    libc = None

def drop_cache(path):
    if libc is None:
        return
    try:
        fd = os.open(path, os.O_RDONLY)
        libc.posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED)
        os.close(fd)
    except Exception:
        pass

# ---------------- Timing ----------------
def timed_median(func, runs=30, setup=None):
    times = []
    for _ in range(runs):
        if setup:
            setup()
        start = time.perf_counter()
        result = func()
        end = time.perf_counter()
        # Fully materialize result
        if isinstance(result, (str, bytes)):
            _ = len(result)
        elif isinstance(result, list):
            _ = sum(len(x) for x in result)
        times.append((end - start) * 1000.0)
    times.sort()
    return times[len(times) // 2]

# ---------------- Write functions ----------------
def write_string():
    with open(filename, "w") as f:
        f.write(data_str)
        f.flush()
        os.fsync(f.fileno())

def write_bytes():
    with open(filename, "wb") as f:
        f.write(data_bytes)
        f.flush()
        os.fsync(f.fileno())

def write_lines():
    with open(filename, "w") as f:
        f.writelines(lines)
        f.flush()
        os.fsync(f.fileno())

def write_line():
    with open(filename, "w") as f:
        for l in lines:
            f.write(l)
        f.flush()
        os.fsync(f.fileno())

# ---------------- Read functions ----------------
def read_string():
    with open(filename, "r") as f:
        return f.read()

def read_bytes():
    with open(filename, "rb") as f:
        return f.read()

def read_lines():
    with open(filename, "r") as f:
        return f.readlines()

def read_line():
    out = []
    with open(filename, "r") as f:
        while True:
            line = f.readline()
            if not line:
                break
            out.append(line)
    return out

# ---------------- Run benchmarks ----------------
results = {}
results["writeString"] = timed_median(write_string)
results["writeBytes"]  = timed_median(write_bytes)
results["writeLines"]  = timed_median(write_lines)
results["writeLine"]   = timed_median(write_line)

results["readString"] = timed_median(read_string, setup=lambda: drop_cache(filename))
results["readBytes"]  = timed_median(read_bytes,  setup=lambda: drop_cache(filename))
results["readLines"]  = timed_median(read_lines,  setup=lambda: drop_cache(filename))
results["readLine"]   = timed_median(read_line,   setup=lambda: drop_cache(filename))

# ---------------- Output ----------------
for k, v in results.items():
    print(f"{k}:{v:.3f}")