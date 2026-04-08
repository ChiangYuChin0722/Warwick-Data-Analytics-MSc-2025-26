import timeit

def read_data(filename):
    data = []
    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            line = line.replace(",", " ").replace(";", " ")
            parts = line.split()
            if len(parts) < 2:
                continue
            name = parts[0].strip()
            try:
                value = int(parts[1])
            except ValueError:
                continue
            data.append((name, value))
    return data

def write_data(filename, data):
    with open(filename, "w") as f:
        for name, value in data:
            f.write(f"{name},{value}\n")

def bubble_sort(data):
    data = data.copy()
    n = len(data)
    for i in range(n):
        for j in range(0, n - i - 1):
            if data[j][1] > data[j + 1][1]:
                data[j], data[j + 1] = data[j + 1], data[j]
    return data

def merge_sort(data):
    if len(data) <= 1:
        return data
    mid = len(data) // 2
    left = merge_sort(data[:mid])
    right = merge_sort(data[mid:])
    return merge(left, right)

def merge(left, right):
    result = []
    i = j = 0
    while i < len(left) and j < len(right):
        if left[i][1] <= right[j][1]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    return result + left[i:] + right[j:]

def main():
    input_file = "namePriorities.txt"
    data = read_data(input_file)

    if not data:
        print("⚠️ 沒有讀到任何資料，請確認 namePriorities.txt 是否在同一資料夾。")
        return

    # 為了安全測試，只取前5000筆
    if len(data) > 5000:
        data = data[:5000]
        print(f"⚠️ 測試時僅取前 5000 筆資料 (總資料 {len(data)} 筆)\n")

    # Bubble Sort (repeat 10 times取平均)
    start = timeit.default_timer()
    for _ in range(10):
        bubble_sort(data)
    bubble_time = (timeit.default_timer() - start) / 10
    bubble_sorted = bubble_sort(data)
    write_data("bubble_sorted.txt", bubble_sorted)

    # Merge Sort (repeat 10 times取平均)
    start = timeit.default_timer()
    for _ in range(10):
        merge_sort(data)
    merge_time = (timeit.default_timer() - start) / 10
    merge_sorted = merge_sort(data)
    write_data("merge_sorted.txt", merge_sorted)

    print("✅ Sorting complete!\n")
    print(f"Bubble Sort Avg Time: {bubble_time:.6f} seconds")
    print(f"Merge Sort Avg Time:  {merge_time:.6f} seconds")
    print("\nOutput written to:")
    print(" - bubble_sorted.txt")
    print(" - merge_sorted.txt")

if __name__ == "__main__":
    main()
