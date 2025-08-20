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

def parse_time(hour: int, minute: int, time_format: bool) -> str:
    #time_format: True → 12h with AM/PM, False → 24h
    if time_format:
        meridiem = "AM"
        display_hour = hour
        if hour == 0:
            display_hour = 12
        elif hour >= 12:
            meridiem = "PM"
            if hour > 12:
                display_hour = hour - 12
        return f"{display_hour:02d}:{minute:02d} {meridiem}"
    else:  # 24-hour
        return f"{hour:02d}:{minute:02d}"

def delay_second(ctk_obj) -> None:
    ctk_obj.configure(state="disabled")
    ctk_obj.after(1000, lambda: ctk_obj.configure(state="normal"))