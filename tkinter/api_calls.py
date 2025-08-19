import httpx

_BASE_URL = 'http://127.0.0.1:8000'

def request_geocode(email: str, location: str) -> dict:
    url = f'{_BASE_URL}/geocode/'
    params = {
        'email': email,
        'location': location
    }
    with httpx.Client(timeout=10) as client:
        response = client.get(url=url, params=params)
        data = response.json()
        return data

def request_weather(email: str, latitude: float, longitude: float) -> dict:
    url = f'{_BASE_URL}/weather/'
    params = {
        'email': email,
        'latitude': latitude,
        'longitude': longitude
    }
    with httpx.Client(timeout=10) as client:
        response = client.get(url=url, params=params)
        data = response.json()
        return data
    
def request_time(key: str, latitude: float, longitude: float) -> dict:
    url = f'{_BASE_URL}/time/'
    params = {
        'key': key,
        'latitude': latitude,
        'longitude': longitude
    }
    with httpx.Client(timeout=10) as client:
        response = client.get(url=url, params=params)
        data = response.json()
        return data