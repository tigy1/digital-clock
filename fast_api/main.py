from fastapi import FastAPI
import uvicorn

from services.nominatim import request_nominatim_data
from services.nws import request_nws_data
from services.nominatim_rev import request_reverse_location
from services.timezonedb import request_time
from utils import logic

# Uvicorn running on http://127.0.0.1:8000

app = FastAPI()

@app.get('/geocode/')
async def nominatim(location: str) -> dict:
    data = await request_nominatim_data(location)
    if len(data) != 0:
        location_coords = (float(data[0]["lat"]), float(data[0]["lon"]))
        return {
            'status': 'SUCCESS',
            'location_coords': location_coords
        }
    return {'status': 'DATA ERROR'}

#in fahrenheit
@app.get('/weather/')
async def nws(latitude: float, longitude: float) -> dict:
    location = latitude, longitude
    nws_data = await request_nws_data(location)
    if len(nws_data) != 0:
        first_hour_data = nws_data['properties']['periods'][0]
        air_temp = first_hour_data['temperature']
        humidity = first_hour_data['relativeHumidity']['value']
        wind_spd = int(first_hour_data['windSpeed'].replace(' mph', ''))
        feels_temp = logic.feels_like_temp(air_temp, humidity, wind_spd)

        coords: list[list] = nws_data['geometry']['coordinates'][0]
        coordinate_average = logic.average_coords(coords)
        reverse_data = await request_reverse_location(coordinate_average)
        if len(reverse_data) != 0:
            location_description = reverse_data['display_name']
            return {
                'status': 'SUCCESS',
                'air_temp': air_temp,
                'feels_temp': feels_temp,
                'humidity': humidity,
                'wind_spd': wind_spd,
                'weather_location': location_description
            }
    return {'status': 'DATA ERROR'}

@app.get('/time/')
async def time(key: str, latitude: float, longitude: float) -> dict:
    coords = latitude, longitude
    data = await request_time(key, coords)
    if data is not None and len(data) != 0:
        timestamp: str = data['formatted']
        time_tuple = logic.parse_timestamp(timestamp) #hour, minute
        return {
            'status': 'SUCCESS',
            'hour': time_tuple[0],
            'minute': time_tuple[1]
        }
    else:
        return {'status': 'DATA ERROR'}

if __name__ == '__main__':
    print("    **Time Zone data from TimeZoneDB at https://timezonedb.com")
    print("    **Forward geocoding data from OpenStreetMap")
    print("    **Reverse geocoding data from OpenStreetMap")
    print("    **Real-time weather data from National Weather Service, United States Department of Commerce")
    uvicorn.run(app, host='127.0.0.1', port=8000)