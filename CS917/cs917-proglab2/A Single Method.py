import math

def find_min_spread_from_file(filename, label_col_index, val1_col_index, val2_col_index, use_absolute_diff=True):
    """
    一個可重用的方法，用於從 CSV 檔案中找出兩欄之間差異最小的標籤。
    
    """
    min_spread = math.inf
    result_label = ""
    
    # 計算所需的最小欄位數，以安全地存取索引
    max_index = max(label_col_index, val1_col_index, val2_col_index)

    try:
        with open(filename, 'r') as file:
            header_skipped = False
            for line in file:
                if not header_skipped:
                    header_skipped = True
                    continue
                
                parts = line.strip().split(',')
                
                if len(parts) > max_index:
                    try:
                        label = parts[label_col_index]
                        val1 = float(parts[val1_col_index])
                        val2 = float(parts[val2_col_index])
                        
                        if use_absolute_diff:
                            spread = abs(val1 - val2)
                        else:
                            spread = val1 - val2 # 假設 val1 總是 >= val2
                        
                        if spread < min_spread:
                            min_spread = spread
                            result_label = label
                            
                    except (ValueError, IndexError):
                        print(f"Skipping malformed row: {line.strip()}")
                        
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        return None

    return result_label

# --- 使用這個單一方法解決兩個問題 ---

# 解決第 1 部分：Football 
# 假設：Team(0), Goals(5), Goals Allowed(6)
football_team = find_min_spread_from_file(
    filename='football.csv',
    label_col_index=0,
    val1_col_index=5,
    val2_col_index=6,
    use_absolute_diff=True
)
if football_team:
    print(f"Section 1 Result: {football_team}")

# 解決第 2 部分：Weather 
# 索引：Day(0), MaxTemp(1), MinTemp(2)
weather_day = find_min_spread_from_file(
    filename='weather.csv',
    label_col_index=0,
    val1_col_index=1,
    val2_col_index=2,
    use_absolute_diff=False # Max temp - Min temp
)
if weather_day:
    print(f"Section 2 Result: {weather_day}")