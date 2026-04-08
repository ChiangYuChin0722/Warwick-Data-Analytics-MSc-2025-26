import csv
import math

def find_min_spread_with_csv_lib(filename, label_key, val1_key, val2_key, use_absolute_diff=True):
    """
    使用 csv.DictReader 函式庫重構的通用方法。
    
    """
    min_spread = math.inf
    result_label = ""
    
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            # 使用 DictReader 將每一行讀取為一個字典
            reader = csv.DictReader(file)
            
            for row in reader:
                try:
                    # 使用鍵（標頭名稱）而不是索引來存取資料
                    label = row[label_key]
                    val1 = float(row[val1_key])
                    val2 = float(row[val2_key])
                    
                    if use_absolute_diff:
                        spread = abs(val1 - val2)
                    else:
                        spread = val1 - val2
                    
                    if spread < min_spread:
                        min_spread = spread
                        result_label = label
                        
                except (ValueError, KeyError):
                    # KeyError 處理標頭名稱不匹配
                    # ValueError 處理非數字資料
                    print(f"Skipping row with invalid data or missing keys: {row}")

    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
        return None
    except Exception as e:
        print(f"An error occurred: {e}")
        return None

    return result_label

# --- 使用 CSV 函式庫執行 ---

# 解決第 1 部分：Football
# 標頭名稱來自文件描述 
football_team_bonus = find_min_spread_with_csv_lib(
    filename='football.csv',
    label_key='Team',           # 假設標頭是 'Team'
    val1_key='Goals',         # 標頭 
    val2_key='Goals Allowed', # 標頭 
    use_absolute_diff=True
)
if football_team_bonus:
    print(f"Bonus Section 1 Result: {football_team_bonus}")

# 解決第 2 部分：Weather
# 假設標頭是 'Day', 'MxT', 'MnT' (基於描述 )
# **您可能需要根據您的 weather.csv 檔案修改 'MxT' 和 'MnT'**
weather_day_bonus = find_min_spread_with_csv_lib(
    filename='weather.csv',
    label_key='Day',            # 假設標頭是 'Day' 
    val1_key='MxT',           # 假設標頭是 'MxT' 
    val2_key='MnT',           # 假設標頭是 'MnT' 
    use_absolute_diff=False
)
if weather_day_bonus:
    print(f"Bonus Section 2 Result: {weather_day_bonus}")