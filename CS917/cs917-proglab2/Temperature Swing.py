import math

def find_smallest_temp_spread():
    """
    讀取 weather.csv 並找出溫度範圍最小的日期編號。
    
    """
    filename = 'weather.csv'
    min_spread = math.inf
    day_with_min_spread = ""

    try:
        with open(filename, 'r') as file:
            header_skipped = False
            for line in file:
                if not header_skipped:
                    header_skipped = True
                    continue
                
                parts = line.strip().split(',')
                
                # 確保該行有足夠的欄位
                if len(parts) >= 3:
                    try:
                        # 根據文件描述獲取欄位 
                        day_number = parts[0]      # 第 1 欄 (索引 0)
                        max_temp = float(parts[1]) # 第 2 欄 (索引 1)
                        min_temp = float(parts[2]) # 第 3 欄 (索引 2)
                        
                        spread = max_temp - min_temp
                        
                        if spread < min_spread:
                            min_spread = spread
                            day_with_min_spread = day_number
                            
                    except ValueError:
                        # 處理非數字資料的行
                        print(f"Skipping row with invalid data: {line.strip()}")

    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        return

    print(f"The day with the smallest temperature spread is: {day_with_min_spread}")

# 執行第 2 部分
find_smallest_temp_spread()