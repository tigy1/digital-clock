import asyncio
import httpx
    
async def request_time(key: str, location: tuple[float, float:]) -> dict:
    latitude, longitude = location
    format = 'json'
    url = f'http://api.timezonedb.com/v2.1/get-time-zone?key={key}&format={format}&by=position&lat={latitude}&lng={longitude}'
    async with httpx.AsyncClient(timeout=10) as client:
        try:
            response = await client.get(url)
            response.raise_for_status()
            data = response.json()
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
