import time

filename = "bench/dummy_test.log"
data = "x" * 10_000_000  # 10 MB

# --- Write benchmark ---
start = time.time()
with open(filename, "w") as f:
    f.write(data)
print("[Python] WriteAll:", time.time() - start)

# --- Read benchmark ---
start = time.time()
with open(filename, "r") as f:
    content = f.read()
print("[Python] ReadAll:", time.time() - start)