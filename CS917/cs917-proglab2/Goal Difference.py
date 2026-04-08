import math

def find_smallest_goal_difference():
    """
    讀取 football.csv 並找出淨球數差異最小的球隊
    [cite: 8, 9, 10]
    """
    filename = 'football.csv'
    min_difference = math.inf
    team_with_min_diff = ""

    try:
        with open(filename, 'r') as file:
            header_skipped = False
            for line in file:
                if not header_skipped:
                    header_skipped = True
                    continue
                
                # 使用 split 方法將每一行分割成欄位 
                parts = line.strip().split(',')
                
                # 確保該行有足夠的欄位
                if len(parts) >= 7:
                    try:
                        # 假設球隊名稱在索引 0 
                        team_name = parts[0]
                        
                        # 假設 'Goals' 在索引 5, 'Goals Allowed' 在索引 6 
                        goals_for = int(parts[5])
                        goals_allowed = int(parts[6])
                        
                        difference = abs(goals_for - goals_allowed)
                        
                        if difference < min_difference:
                            min_difference = difference
                            team_with_min_diff = team_name
                            
                    except ValueError:
                        # 處理非數字資料的行
                        print(f"Skipping row with invalid data: {line.strip()}")
                        
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        return

    print(f"The team with the smallest goal difference is: {team_with_min_diff}")

# 執行第 1 部分
find_smallest_goal_difference()