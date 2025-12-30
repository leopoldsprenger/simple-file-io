import time

FILENAME = "dummy_test.txt"
DATA = "x" * 10_000_000          # 10 MB
BYTES = DATA.encode("utf-8")
LINES = ["x"] * 1_000_000        # 1 million lines

def benchmark_write_string():
    start = time.time()
    with open(FILENAME, "w") as f:
        f.write(DATA)
    print(f"[Python] writeString: {time.time() - start:.6f} s")

def benchmark_read_string():
    start = time.time()
    with open(FILENAME, "r") as f:
        content = f.read()
    print(f"[Python] readString: {time.time() - start:.6f} s")

def benchmark_write_bytes():
    start = time.time()
    with open(FILENAME, "wb") as f:
        f.write(BYTES)
    print(f"[Python] writeBytes: {time.time() - start:.6f} s")

def benchmark_read_bytes():
    start = time.time()
    with open(FILENAME, "rb") as f:
        content = f.read()
    print(f"[Python] readBytes: {time.time() - start:.6f} s")

def benchmark_write_lines():
    start = time.time()
    with open(FILENAME, "w") as f:
        f.writelines(line + "\n" for line in LINES)
    print(f"[Python] writeLines: {time.time() - start:.6f} s")

def benchmark_read_lines():
    start = time.time()
    with open(FILENAME, "r") as f:
        read_lines = f.readlines()
    print(f"[Python] readLines: {time.time() - start:.6f} s")

if __name__ == "__main__":
    print("--- Python Benchmarks ---")
    benchmark_write_string()
    benchmark_read_string()
    benchmark_write_bytes()
    benchmark_read_bytes()
    benchmark_write_lines()
    benchmark_read_lines()