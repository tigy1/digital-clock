import asyncio
import urllib.parse
import httpx

async def request_nominatim_data(email: str, location: str) -> dict:
    parsed_location = urllib.parse.quote_plus(location)
    url = f"https://nominatim.openstreetmap.org/search?q={parsed_location}&format=jsonv2"
    headers = {
        'Referer': f'https://www.ics.uci.edu/~thornton/icsh32/ProjectGuide/Project3/{email}'
    }
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