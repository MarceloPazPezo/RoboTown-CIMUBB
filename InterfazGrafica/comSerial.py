import requests

esp32_ip = "192.168.124.210"  # Reemplaza con la dirección IP del ESP32 en tu red
url = f"http://{esp32_ip}/"

response = requests.get(url)

if response.status_code == 200:
    console_output = response.text
    print("Salida de la consola Serial del ESP32:")
    print(console_output)
else:
    print("Error al comunicarse con el ESP32")
