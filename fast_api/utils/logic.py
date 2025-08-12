import math

# in fahrenheit
# heat index when the air temperature is at least 68°F
# wind chill index when the air temperature is no more than 50°F & wind speed is over three miles per hour
def feels_like_temp(temp: float, humidity: float, wind_spd: float) -> float: 
    if temp >= 68: # heat index
        return -42.379 + 2.04901523*temp + 10.14333127*humidity + -0.22475541*temp*humidity + -0.00683783*temp*temp + -0.05481717*humidity*humidity + 0.00122874*temp*temp*humidity + 0.00085282*temp*humidity*humidity + -0.00000199*temp*temp*humidity*humidity
    elif temp <= 50 and wind_spd > 3: # wind chill
        return 35.74 + 0.6215*temp + -35.75*math.pow(wind_spd, 0.16) + 0.4275*temp*math.pow(wind_spd, 0.16)
    else: # air temp
        return temp    

#temperature conversion ----->
def fahrenheit_to_celsius(temp: float) -> float:
    return (temp - 32) * (5.0/9)
assert math.isclose(fahrenheit_to_celsius(32), 0)
assert math.isclose(fahrenheit_to_celsius(212.9), 100.5, rel_tol=1e-5)
assert math.isclose(fahrenheit_to_celsius(-49), -45)

def celsius_to_fahrenheit(temp: float) -> float:
    return (temp) * (9.0/5) + 32
assert math.isclose(celsius_to_fahrenheit(0), 32)
assert math.isclose(celsius_to_fahrenheit(100.5), 212.9, rel_tol=1e-5)
assert math.isclose(celsius_to_fahrenheit(-45), -49)

#processes weather queries and prints them out
def get_hourly_data(nws_data: dict) -> dict: 
    hourly_data = nws_data['properties']['periods'][0] #data of first hour block
    return hourly_data

#coords[0] = long, coords[1] = lat
def average_coords(coords: list[list]) -> tuple[float:'latitude', float:'longitude']:
    long_sum = 0
    lat_sum = 0
    coord_set = set(map(tuple, coords))
    for coord in coord_set:
        long_sum += coord[0]
        lat_sum += coord[1]
    return (lat_sum/len(coord_set), long_sum/len(coord_set))

#parse time from timezonedb
def parse_timestamp(timestamp: str) -> tuple[int, int]:
    date_time_seperator_index = timestamp.index(' ')
    time_str = timestamp[date_time_seperator_index + 1:].split(':')
    hour_str, minute_str, second_str = time_str
    return int(hour_str), int(minute_str)