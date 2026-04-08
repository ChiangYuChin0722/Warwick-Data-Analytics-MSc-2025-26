import csv
import time
import calendar

class MyException(Exception):
    def __init__(self, value):
        self.parameter = value
    def __str__(self):
        return self.parameter

def str_to_date(date_str):
        t = time.strptime(date_str, "%d/%m/%Y")
        time_UTC = calendar.timegm(t)
        return time_UTC


def get_date_range(dataset):
    try:
        timestamps = []
        for entry in dataset:
            timestamps.append(int(entry['time']))
        return min(timestamps), max(timestamps)
    except (ValueError, TypeError):
        return None, None

def filter_by_date(data, start_date, end_date):
    start = str_to_date(start_date)
    end = str_to_date(end_date)

    if not data or not isinstance(data, list):
        return "Error: dataset not found"

    if end < start:
        return "Error: end date must be larger than start date"

    min_data_ts, max_data_ts = get_date_range(data)

    if min_data_ts is not None and (start < min_data_ts or end > max_data_ts):
        return "Error: date range is out of range"

    filtered_data = []
    for row in data:
        row_date = (int(row['time']))# No error checking done... I'll find out when I encounter it XD
        if start <= row_date <= end:
            filtered_data.append(row)
    return filtered_data


def highest_price(data, start_date, end_date):
    try:
        if not data or not isinstance(data, list):  
            return "Error: dataset not found"

        filtered = filter_by_date(data, start_date, end_date)
        if isinstance(filtered, str):
            return filtered
        if not filtered:
            return None
        # one line
        max_price = max(float(row['high']) for row in filtered)
        return max_price
    
    except (KeyError, ValueError):
        return "Error: requested column is missing from dataset"



def lowest_price(data, start_date, end_date):
    try:
        if not isinstance(data, list) or data is None:
            return "Error: dataset not found" 

        filtered = filter_by_date(data, start_date, end_date)
        if not filtered:
            return None
        min_price = round(min(float(row['low']) for row in filtered), 2)
        return min_price
    
    except (KeyError, ValueError):
        return "Error: requested column is missing from dataset"



def max_volume(data, start_date, end_date):
    try:
        if data is None or not isinstance(data, list):
            return "Error: dataset not found"

        windowed_data = filter_by_date(data, start_date, end_date)
        if not windowed_data:
            return None

        max_vol = max([float(row['volumefrom']) for row in windowed_data])
        return max_vol
    
    except (KeyError, ValueError):
        return "Error: requested column is missing from dataset"


def best_avg_price(data, start_date, end_date):
    try:
        if not data or type(data) is not list:
            return "Error: dataset not found"

        filtered_data = filter_by_date(data, start_date, end_date)
        if not filtered_data:
            return None
        daily_avgs = [float(row['volumeto']) / float(row['volumefrom']) 
                        for row in filtered_data if float(row['volumefrom']) > 0]
        if not daily_avgs:
            return None
        best_avg = max(daily_avgs)
        #replace None with an appropriate return value
        return best_avg
    except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"


def moving_average(data, start_date, end_date):
    try:
        if not data or not isinstance(data, list):
            return "Error: dataset not found"

        filtered_data = filter_by_date(data, start_date, end_date)
        daily_avgs = [float(row['volumeto']) / float(row['volumefrom']) 
                        for row in filtered_data if float(row['volumefrom']) > 0]
        if daily_avgs:
            mov_avgs = round(sum(daily_avgs) / len(daily_avgs), 2)
            return mov_avgs 
        else:
                #replace None with an appropriate return value
                return None

    except (KeyError, ValueError):
        return "Error: requested column is missing from dataset"



if __name__ == "__main__":

    
    data = []
    try:
        with open("cryptocompare_btc.csv", "r") as f:
            rdr = csv.DictReader(f)
            for line in rdr:
                data.append(line)
    except FileNotFoundError:
        print("Error: file not found")
        data = None

    print("--- Testing All Normal  ---\n")

    if data:
        print(f"timestamp = {data[0]['time']}")
        print(f"daily high = {data[0]['high']}")
        print(f"volume in BTC = {data[0]['volumefrom']}")

    start_date_str = "01/01/2020"
    end_date_str = "31/01/2020"
    
    print(f"Test date range: {start_date_str} to {end_date_str}\n")
    
    print(f"Highest Price: {highest_price(data, start_date_str, end_date_str)}")
    print(f"Lowest Price: {lowest_price(data, start_date_str, end_date_str)}")
    print(f"Max Volume: {max_volume(data, start_date_str, end_date_str)}")
    print(f"Best Average Price: {best_avg_price(data, start_date_str, end_date_str)}")
    print(f"Moving Average: {moving_average(data, start_date_str, end_date_str)}")

    print("\n--- Testing All Exception Cases ---\n")

    print("1. Testing 'dataset not found':")
    print(f"  Result: {highest_price(None, '01/01/2020', '31/01/2020')}\n")

    print("2. Testing 'requested column is missing from dataset':")
    bad_data = [{'time': '1577836800', 'low': '7000'}] 
    print(f"  Result: {highest_price(bad_data, '01/01/2020', '01/01/2020')}\n")

    print("3. Testing 'invalid date value':")
    print(f"  Result: {highest_price(data, '32/01/2020', '31/01/2020')}\n")

    print("4. Testing 'date value is out of range':")
    print(f"  Result: {highest_price(data, '01/01/2010', '31/01/2010')}\n")

    print("5. Testing 'end date must be larger than start date':")
    print(f"  Result: {highest_price(data, '01/02/2020', '31/01/2020')}\n")
