import csv, time, calendar

"""
    Part D
    Please provide definitions for the following class and functions
"""

# Class Investment:
# Instance variables
#	start date
#	end date
#	data 
#Functions
#	highest_price(data, start_date, end_date) -> float
#	lowest_price(data, start_date, end_date) -> float
#	max_volume(data, start_date, end_date) -> float
#	best_avg_price(data, start_date, end_date) -> float
#	moving_average(data, start_date, end_date) -> float

class MyException(Exception):
    def __init__(self, value):
        self.parameter = value
    def __str__(self):
        return str(self.parameter)
    
def str_to_date(date_str):
         t = time.strptime(date_str, "%d/%m/%Y")
         return calendar.timegm(t)


class Investment:
    def __init__(self, start_date, end_date, data):
        self.start_date = start_date
        self.end_date = end_date
        self.data = data
        self.validate_dates()
    
    def validate_dates(self):
       
            start_ts = str_to_date(self.start_date)
            end_ts = str_to_date(self.end_date)
            if end_ts < start_ts:
                print("Error: end date must be larger than start date")

        
    def filter_data(self, start_date, end_date):
        start_ts = str_to_date(start_date)
        end_ts = str_to_date(end_date)
        filtered = [row for row in self.data 
                    if start_ts <= int(row['time']) <= end_ts]
        return filtered
    
    def highest_price(self,start_date = None, end_date = None):
        try:
            s_date = start_date or self.start_date
            e_date = end_date or self.end_date
            filtered = self.filter_data(s_date, e_date)
            if not filtered:
                return None
            max_price = max(float(row['high']) for row in filtered)
            return max_price
        except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"
        except MyException as e:
            return str(e)
    
    def lowest_price(self, start_date = None, end_date = None):
        try:
            s_date = start_date or self.start_date
            e_date = end_date or self.end_date
            filtered = self.filter_data(s_date, e_date)
            if not filtered:
                return None
            min_price = round(min(float(row['low']) for row in filtered), 2)
            return min_price
        except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"
        except MyException as e:
            return str(e)
    
    def max_volume(self, start_date = None, end_date = None):
        try:
            s_date = start_date or self.start_date
            e_date = end_date or self.end_date
            filtered = self.filter_data(s_date, e_date)
            if not filtered:
                return None
            max_vol = max(float(row['volumefrom']) for row in filtered)
            return max_vol
        except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"
        except MyException as e:
            return str(e)
    
    def best_avg_price(self, start_date = None, end_date = None):
        try:
            s_date = start_date or self.start_date
            e_date = end_date or self.end_date
            filtered = self.filter_data(s_date, e_date)
            daily_avgs = [float(row['volumeto']) / float(row['volumefrom'])
                          for row in filtered if float(row['volumefrom']) > 0]
            best_avg_price = max(daily_avgs) if daily_avgs else None 
            return best_avg_price
        except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"
        except MyException as e:
            return str(e)
    
    def moving_average(self, start_date = None, end_date = None):
        try:
            s_date = start_date or self.start_date
            e_date = end_date or self.end_date
            filtered = self.filter_data(s_date, e_date)
            daily_avgs = [float(row['volumeto']) / float(row['volumefrom'])
                          for row in filtered if  float(row['volumefrom']) > 0]
            if not daily_avgs:
                return None
            mov_avg = round(sum(daily_avgs) / len(daily_avgs), 2)
            return mov_avg
        except (KeyError, ValueError):
            return "Error: requested column is missing from dataset"
        except MyException as e:
            return str(e)

def cal_linear_regression(x_val, y_val):
    if not x_val or not y_val or len(x_val) != len(y_val) or len(x_val) < 2:
        return 0, 0
    n = len(x_val)
    x_mean = sum(x_val) / n
    y_mean = sum(y_val) / n
    num = sum((x - x_mean) * (y - y_mean) for x, y in zip(x_val, y_val))
    den = sum((x - x_mean) ** 2 for x in x_val)

    if den == 0:
        return 0, y_mean
    
    m = num / den
    b = y_mean - m * x_mean
    return m, b

# predict_next_average(investment) -> float
# investment: Investment type
def	predict_next_average(investment):
    try:
       filtered = investment.filter_data(investment.start_date, investment.end_date)
       valid_data = [row for row in filtered if float(row.get('volumefrom', 0)) > 0]

       if len(valid_data) < 2: return None
       
       timestamps = [int(row['time']) for row in valid_data]
       daily_avgs = [float(row['volumeto']) / float(row['volumefrom'])
                      for row in valid_data]
       m, b = cal_linear_regression(timestamps, daily_avgs)
       next_day_ts = str_to_date(investment.end_date) + 86400
       predicted_avg = m * next_day_ts + b
       return predicted_avg
    except (MyException, KeyError, ValueError):
		#replace None with an appropriate return value
        return None


# classify_trend(investment) -> str
# investment: Investment type
def classify_trend(investment):
    try:
        filtered = investment.filter_data(investment.start_date, investment.end_date)
        
        if len(filtered) < 2:
            return "other"
        
        timestamps = [int(row['time']) for row in filtered]
        highs = [float(row['high']) for row in filtered]
        lows = [float(row['low']) for row in filtered]
        m_high, _ = cal_linear_regression(timestamps, highs)
        m_low, _ = cal_linear_regression(timestamps, lows)

        if m_high > 0 and m_low > 0:
            return "increasing"
        elif m_high < 0 and m_low < 0:
            return "decreasing"
        elif m_high > 0 and m_low < 0:
            return "volatile"
        else:
            return "other"
    except (MyException, KeyError, ValueError):
        #replace None with an appropriate return value
        return 'other'
	

# Replace the body of this main function for your testing purposes
if __name__ == "__main__":
    # Start the program
    data = []

    try:
        with open("cryptocompare_btc.csv", "r") as f:
            reader = csv.DictReader(f)
            data = list(reader)
    except FileNotFoundError:   
        print("Error: cryptocompare_btc.csv not found.")
        exit()
    
    start = input("Enter start date (between 28/04/2015 and 18/10/2020): ")
    end = input("Enter end date (between 28/04/2015 and 18/10/2020): ")

    print("1. Creating Investment object for:") 
    print(f"   Start Date: {start}\n   End Date: {end}\n")

    try:
        inv = Investment(start, end, data)
        print("2. Highest Price:")
        print("  ",inv.highest_price(),"\n")
        print("3. Lowest Price:")
        print("  ",inv.lowest_price(),"\n")

        pred = predict_next_average(inv)
        print("4. Predicted Next Day Average Price (for 01/04/2020):ss")
        if pred is not None:
            print("   ",f"{pred}\n")
        else:
            print("   Error in prediction.\n")

        trend = classify_trend(inv)
        print("5. Classifying the trend for the period:")
        print("  ",trend,"\n")

    except MyException as e:
        print(f"Error creating Investment object: {e}")
