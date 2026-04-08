import csv, time, calendar
from datetime import datetime

"""
    Part C
    Please provide definitions for the following functions
"""

def str_to_date(date_str):
    t = time.strptime(date_str, "%d/%m/%Y")
    time_UTC = calendar.timegm(t)
    return time_UTC

def ts_date_str(timestamp):
    utc_time = time.gmtime(timestamp)
    return time.strftime("%d/%m/%Y", utc_time)

def cal_daily_avgs(data):
	daily_avgs = {int(row['time']): float(row['volumeto']) / float(row['volumefrom'])
			   for row in data if float(row.get('volumefrom', 0)) > 0}
	return daily_avgs

def cal_mov_avg(data, window_size, start_date, end_date):
    daily_avgs = cal_daily_avgs(data)
    all_ts = sorted(daily_avgs.keys())
    start = str_to_date(start_date) 
    end = str_to_date(end_date)
    
    mov_avg_dict = {}
    current_ts = start
    while current_ts <= end:
        history = [ts for ts in all_ts if ts <= current_ts] 
        window_ts = history[-window_size:] 
        
        window_values = [daily_avgs[ts] for ts in window_ts if ts in daily_avgs]
        if window_values:
            mov_avg_dict[ts_date_str(current_ts)] = sum(window_values) / len(window_values)
        current_ts += 86400 
    return mov_avg_dict

def find_signals(short_avg_dict, long_avg_dict, condition_func):
    signals = {}
    common_dates = sorted(short_avg_dict.keys() & long_avg_dict.keys()
                          , key=lambda d: datetime.strptime(d, "%d/%m/%Y"))
    
    for i, curr_date in enumerate(common_dates):
        if i == 0 or common_dates[i-1] not in long_avg_dict:
            signals[curr_date] = 0
            continue

        prev_date = common_dates[i-1]
        prev_s = short_avg_dict[prev_date]
        prev_l = long_avg_dict[prev_date]
        curr_s = short_avg_dict[curr_date]
        curr_l = long_avg_dict[curr_date]

        signals[curr_date] = 1 if condition_func(prev_s, prev_l, curr_s, curr_l) else 0
    return signals

# moving_avg_short(data, start_date, end_date) -> dict
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  moving_avg_short(data, start_date, end_date):
    mov_avg_s = cal_mov_avg(data, 3, start_date, end_date)
	 #replace None with an appropriate return value
    return mov_avg_s


# moving_avg_long(data, start_date, end_date) -> dict
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  moving_avg_long(data, start_date, end_date):
    mov_avg_l = cal_mov_avg(data, 10, start_date, end_date)
	 #replace None with an appropriate return value
    return mov_avg_l


# find_buy_list(short_avg_dict, long_avg_dict) -> dict
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  find_buy_list(short_avg_dict, long_avg_dict):
    buy_condition = lambda prev_s, prev_l, curr_s, curr_l: prev_s <= prev_l and curr_s > curr_l
    buy_list = find_signals(short_avg_dict, long_avg_dict, buy_condition)
    #replace None with an appropriate return value
    return buy_list


# find_sell_list(short_avg_dict, long_avg_dict) -> dict
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  find_sell_list(short_avg_dict, long_avg_dict):
    sell_condition = lambda prev_s, prev_l, curr_s, curr_l: prev_s >= prev_l and curr_s < curr_l
    sell_list = find_signals(short_avg_dict, long_avg_dict, sell_condition)
	#replace None with an appropriate return value
    return sell_list



# crossover_method(data, start_date, end_date) -> [buy_list, sell_list]
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  crossover_method(data, start_date, end_date):
    short_avg_dict = moving_avg_short(data, start_date, end_date)
    long_avg_dict = moving_avg_long(data, start_date, end_date)
    buy_singals = find_buy_list(short_avg_dict, long_avg_dict)
    sell_signals = find_sell_list(short_avg_dict, long_avg_dict)
    buy_list = [date for date, signal in buy_singals.items() if signal == 1]
    sell_list = [date for date, signal in sell_signals.items() if signal == 1]
    cross_meth = [buy_list, sell_list]
    #replace None with an appropriate return value
    return cross_meth


# Replace the body of this main function for your testing purposes
if __name__ == "__main__":
    # Start the program

    # Example variable initialization
    # data is always the cryptocompare_btc.csv read in using a DictReader
    
    data = []
    try:
        with open("cryptocompare_btc.csv", "r") as f:
            reader = csv.DictReader(f)
            data = list(reader)
    except FileNotFoundError:   
        print("CSV file not found. Please ensure 'cryptocompare_btc.csv' is in the working directory.")
        exit()

    start_date = input("Enter start date (between 28/04/2015 and 18/10/2020): ")
    end_date = input("Enter end date (between 28/04/2015 and 18/10/2020): ")

    print("-----Cross Strategy Testing-----")
    print(f"Analysis period: {start_date} to {end_date}\n")

    buy_date, sell_date = crossover_method(data, start_date, end_date)

    print("Buy signal detected: \n")
    print(f"{', '.join(buy_date) if buy_date else 'none'}\n")
    print("Sell signal detected: \n")
    print(f"{', '.join(sell_date) if sell_date else 'none'}\n")

