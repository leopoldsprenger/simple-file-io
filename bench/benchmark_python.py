import time

filename = "bench_test_py.txt"
data_str = "A" * 10_000_000  # 10 MB
data_bytes = b"A" * 10_000_000
lines = ["This is a line {}\n".format(i) for i in range(1_000_000)]

def timed(func):
    start = time.perf_counter()
    func()
    end = time.perf_counter()
    return (end - start) * 1000  # ms

results = {}

# Write/read string
results["writeString"] = timed(lambda: open(filename,"w").write(data_str))
results["readString"]  = timed(lambda: open(filename,"r").read())

# Write/read bytes
results["writeBytes"] = timed(lambda: open(filename,"wb").write(data_bytes))
results["readBytes"]  = timed(lambda: open(filename,"rb").read())

# Write/read line
results["writeLine"] = timed(lambda: open(filename,"w").write("Single line test\n"))
results["readLine"]  = timed(lambda: open(filename,"r").readline())

# Write/read lines
results["writeLines"] = timed(lambda: open(filename,"w").writelines(lines))
results["readLines"]  = timed(lambda: open(filename,"r").readlines())

# Output in format C++ can parse
for k,v in results.items():
    print(f"{k}:{v:.3f}")