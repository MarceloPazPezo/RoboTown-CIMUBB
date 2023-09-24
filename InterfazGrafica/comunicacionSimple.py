import requests

esp32_ip = "192.168.124.210"  # Reemplaza con la dirección IP del ESP32 en tu red
url = f"http://{esp32_ip}/"

response = requests.get(url)

if response.status_code == 200:
    data = response.text
    print(f"Respuesta del ESP32: {data}")
else:
    print("Error al comunicarse con el ESP32")
