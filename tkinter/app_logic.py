import math

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

def parse_time(time: str) -> str:
    time_adjustment = 0
    if ' ' in time: #from standard time
        ind = time.find(' ')
        meridiem = time[ind + 1:]
        time = time[:ind]
        if meridiem == 'PM':
            time_adjustment = 12
    time_str = time.split(':')
    hour, minute = map(int, time_str)
    hour += time_adjustment
    if hour >= 24:
        hour = 0
    return hour, minute

def delay_second(ctk_obj) -> None:
    ctk_obj.configure(state="disabled")
    ctk_obj.after(1000, lambda: ctk_obj.configure(state="normal"))