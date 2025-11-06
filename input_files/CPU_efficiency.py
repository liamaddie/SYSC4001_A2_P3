cpu_time = 0
overhead_time = 0

with open('execution.txt') as f:
    for line in f:
        parts = line.strip().split(', ')
        duration = int(parts[1])
        label = parts[2]
        if "CPU" in label:
            cpu_time += duration
        else:
            overhead_time += duration

print("CPU work:", cpu_time, "ms")
print("Overhead:", overhead_time, "ms")
print("CPU efficiency:", (cpu_time / (cpu_time + overhead_time)) * 100, "%")
