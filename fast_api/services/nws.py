import asyncio
import httpx

async def request_nws_data(email: str, location: tuple[float:'latitude', float:'longitude']) -> dict:
    latitude, longitude = location
    url = f"https://api.weather.gov/points/{latitude},{longitude}"
    headers = {
        "User-Agent": "(email)",
        "Accept": "application/geo+json"
    }
    async with httpx.AsyncClient(timeout=10,follow_redirects=True) as client:
        try:
            response = await client.get(url, headers=headers)
            response.raise_for_status()
            grid_forecast = response.json()
            asyncio.sleep(1)
            hourly_data_url = grid_forecast['properties']['forecastHourly']
            hourly_response = await client.get(hourly_data_url, headers=headers)
            hourly_response.raise_for_status()
            data = hourly_response.json()
            return data
        except ValueError as e:
            print("JSON FAILED")
            print(f'JSON message: {e}')
            return {}
        except httpx.HTTPStatusError as e:
            print("HTTP ERROR")
            print(f'status: {e.response.status_code}: {e.response.reason_phrase}, {url}')
            return {}
        except httpx.RequestError as e:
            print("URL/CONNECTION ERROR")
            print(f'{e}, {url}')
            return {}