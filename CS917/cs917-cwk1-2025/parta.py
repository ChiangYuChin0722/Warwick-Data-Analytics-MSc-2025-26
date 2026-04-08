import csv ,time, calendar

"""
    Part A
    Please provide definitions for the following functions
"""

def str_to_date(date_str):
    t = time.strptime(date_str, "%d/%m/%Y")
    time_UTC = calendar.timegm(t)
    return time_UTC


def filter_by_date(data, start_date, end_date):
    start = str_to_date(start_date)
    end = str_to_date(end_date)
    filtered_data = []
    for row in data:
        row_date = (int(row['time']))
        if start <= row_date <= end:
            filtered_data.append(row)
    return filtered_data


# highest_price(data, start_date, end_date) -> float
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  highest_price(data, start_date, end_date):
    filtered_data = filter_by_date(data, start_date, end_date)
    if not filtered_data: 
          return None
    max_price = max(float(row['high']) for row in filtered_data)
	#replace None with an appropriate return value
    return max_price


# lowest_price(data, start_date, end_date) -> float
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  lowest_price(data, start_date, end_date):
    filtered_data = filter_by_date(data, start_date, end_date)
    if not filtered_data:
        return None
    min_price = round(min(float(row['low']) for row in filtered_data),2)
	 
	#replace None with an appropriate return value
    return min_price



# max_volume(data, start_date, end_date) -> float
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  max_volume(data, start_date, end_date):
    filtered_data = filter_by_date(data, start_date, end_date)
    if not filtered_data:
        return None
    max_vol = max(float(row['volumefrom']) for row in filtered_data)
    #replace None with an appropriate return value
    return max_vol	 




# best_avg_price(data, start_date, end_date) -> float
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  best_avg_price(data, start_date, end_date):
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





# moving_average(data, start_date, end_date) -> float
# data: the data from a csv file
# start_date: string in "dd/mm/yyyy" format
# end_date: string in "dd/mm/yyyy" format
def  moving_average(data, start_date, end_date):
      filtered_data = filter_by_date(data, start_date, end_date)
      daily_avgs = [float(row['volumeto']) / float(row['volumefrom']) 
                    for row in filtered_data if float(row['volumefrom']) > 0]
      if daily_avgs:
          mov_avgs = round(sum(daily_avgs) / len(daily_avgs), 2)
          return mov_avgs 
      else:
            #replace None with an appropriate return value
            return None

# Replace the body of this main function for your testing purposes
if __name__ == "__main__":
    # Start the program

    # Example variable initialization
    # data is always the cryptocompare_btc.csv read in using a DictReader
    
    data = []
    with open("cryptocompare_btc.csv", "r") as f:
        reader = csv.DictReader(f)
        data = [r for r in reader]
    
    # access individual rows from data using list indices
    first_row = data[0]
    # to access row values, use relevant column heading in csv
    print(f"timestamp = {first_row['time']}")
    print(f"daily high = {first_row['high']}")
    print(f"volume in BTC = {first_row['volumefrom']}")

    start_date_str = input("Enter start date (between 28/04/2015 and 18/10/2020): ")
    end_date_str = input("Enter end date (between 28/04/2015 and 18/10/2020): ")

    
    print(f"Test date range: {start_date_str} to {end_date_str}\n")
    
    print(f"Highest Price: {highest_price(data, start_date_str, end_date_str)}")
    print(f"Lowest Price: {lowest_price(data, start_date_str, end_date_str)}")
    print(f"Max Volume: {max_volume(data, start_date_str, end_date_str)}")
    print(f"Best Average Price: {best_avg_price(data, start_date_str, end_date_str)}")
    print(f"Moving Average: {moving_average(data, start_date_str, end_date_str)}")



    pass

   


