import httpx
import asyncio

async def request_reverse_location(email: str, location: tuple[float:'latitude', float:'longitude']) -> dict:
    latitude, longitude = location
    url = f"https://nominatim.openstreetmap.org/reverse?lat={latitude}&lon={longitude}&format=jsonv2"
    headers = {
        'Referer': f'https://www.ics.uci.edu/~thornton/icsh32/ProjectGuide/Project3/{email}'
    }
    await asyncio.sleep(1)
    async with httpx.AsyncClient(timeout=10) as client:
        try:
            response = await client.get(url, headers=headers)
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