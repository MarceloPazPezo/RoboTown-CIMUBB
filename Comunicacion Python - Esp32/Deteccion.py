#
#  * Autor: Marcelo Paz
#  * Fecha: 14/01/2024
#  * Descripcion: Codigo para el control de un robot utilizando comunicación websocket y control por ArUco 99.
#  * Firma:
#   elnube      _..----.._    _
#             .'  .--.    "-.(0)_
# '-.__.-'"'=:|   ,  _)_ \__ . c\'-..
#              '''------'---''---'-"
#
import numpy as np
import websocket
import time
import threading
import numpy as np
import keyboard
import cv2
from cv2 import aruco
from collections import deque
from queue import Queue

IP = "10.3.10.154"
puerto = 81

global ruta
ruta = deque()
global cola_mensajes
cola_mensajes = Queue()


# global aruco_id_fijo
# aruco_id_fijo = 1  # ID del aruco fijo
global aruco_id_movil
aruco_id_movil = 99  # ID del aruco móvil


global umbral_distancia
umbral_distancia = 150 #su unidad es pixeles

# --------------------------------------------------------------------------------

def mensajeEntrante(ws, message):
    print("Recibido: " + message)
    instrucciones = message.split("-")
    for i in instrucciones:
        print(i)
        if i.isdigit():
            ruta.append(i)
    print(ruta)
    
def enviarMensajes(ws):
    while True:
        # Esperar a que haya un mensaje en la cola
        msg = cola_mensajes.get()
        if msg == "salir":
            finConexion(ws, 0, "Conexión cerrada de forma segura")
            break
        else:
            ws.send(msg)
            time.sleep(1)
            
def error(ws, error):
    print("Error: " + str(error))

def finConexion(ws, close_status_code=0, close_msg="Conexion cerrada"):
    if close_status_code == 0:
        print(f"[INFO] {close_msg}, code: {close_status_code}")
        ws.close()
    
def inicioConexion(ws):
    print("[INFO] Conexión abierta")
    print("\n")
    ws.send("Iniciando comunicación...")
    
    # Crear e iniciar el hilo de envío de mensajes
    t = threading.Thread(target=enviarMensajes, args=(ws,))
    t.start()

def comunicacionWebSocket():
    ws = websocket.WebSocketApp(f"ws://{IP}:{puerto}/",
                              on_message = mensajeEntrante,
                              on_error = error,
                              on_close = finConexion)
    ws.on_open = inicioConexion
    ws.run_forever()

# --------------------------------------------------------------------------------

def controlMedianteArUco():
    time.sleep(1)
    print("[INFO] Inicializando control", end="")
    for i in range(3):
        print(".", end="")
        time.sleep(1)
    print("\n")
    
    while True:
        if keyboard.is_pressed('q'):
            exit()
        while len(ruta) > 0:
            detectarAruco(ruta)

# --------------------------------------------------------------------------------
def detectarAruco(ruta):
    instruccion = ruta.popleft()
    instruccion=int(instruccion)
    print(f'Procesando: {instruccion}')
    
    dictionary = aruco.Dictionary_get(aruco.DICT_4X4_100)
    parameters = aruco.DetectorParameters_create()

    cap = cv2.VideoCapture(0)  # Puedes ajustar el parámetro según tu configuración
    
    markerLength_af = 0.04755 
    markerLength_am = 0.1 #medida del aruco movil
    movil_en_movimiento = False
    aruco_fijo_detectado = False
    
    # Asume que tienes la matriz de la cámara y los coeficientes de distorsión
    camera_matrix = np.array([[239.51027567, 0, 314.1138314 ],[  0, 238.47458721, 284.9145289 ],[0, 0, 1]])
    dist_coeffs = np.array([-0.10410461, 0.29094547, -0.00622743, -0.00109586, 0.99861016])
    
    # camera_matrix = np.array([[fx, 0, cx], [0, fy, cy], [0, 0, 1]])
    # dist_coeffs = np.array([k1, k2, p1, p2, k3])  # Ajusta estos valores según tu cámara

    instruccion_procesada = False
    while not instruccion_procesada:
        ret, frame = cap.read()
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)  # Convierte la imagen a escala de grises
        
        # Detecta los arucos en la imagen
        corners, ids, rejectedImgPoints = aruco.detectMarkers(gray, dictionary, parameters=parameters)
        
        # Comprueba si el aruco móvil está en movimiento
        if ids is not None and aruco_id_movil in ids:
            movil_en_movimiento = True
        else:
            movil_en_movimiento = False

        if ids is not None and instruccion in ids:
            print(f"Buscando...{instruccion}")
            aruco_fijo_detectado = True
            
            aruco_fijo_index = np.where(ids == instruccion)[0][0]  # Obtén el índice del aruco fijo en los IDs detectados
            corners_fijo = corners[aruco_fijo_index]  # Obtén las esquinas del aruco fijo
            
            # Comprueba si el aruco móvil y el aruco fijo están suficientemente cerca
            if movil_en_movimiento:
                aruco_movil_index = np.where(ids == aruco_id_movil)[0][0]  # Obtén el índice del aruco móvil en los IDs detectados
                corners_movil = corners[aruco_movil_index]  # Obtén las esquinas del aruco móvil
                
                # Calcula la distancia entre el aruco móvil y el aruco fijo
                distancia = np.linalg.norm(corners_movil - corners_fijo)
                print(distancia)
                
                # Estima la pose del aruco fijo
                rvecs_fijo, tvecs_fijo, _ = aruco.estimatePoseSingleMarkers(corners_fijo, markerLength_af, camera_matrix, dist_coeffs)

                # Estima la pose del aruco móvil
                rvecs_movil, tvecs_movil, _ = aruco.estimatePoseSingleMarkers(corners_movil, markerLength_am, camera_matrix, dist_coeffs)

                # Calcula el ángulo de rotación del aruco móvil con respecto al aruco fijo
                angle = np.arctan2(tvecs_movil[0][0][1] - tvecs_fijo[0][0][1], tvecs_movil[0][0][0] - tvecs_fijo[0][0][0])

                angle = np.degrees(angle)  # Convierte el ángulo a grados
                print("Ángulo de rotación: ", angle)

                if distancia < umbral_distancia:  # Ajusta el umbral de distancia según tu configuración
                    print("Aruco móvil llegó al aruco fijo")
                    cola_mensajes.put("avanzar 500")#determinar cuando avanzar una vez se acerca
                    time.sleep(1)
                    cola_mensajes.put("detener")#luego de avanzar detenerse
                    time.sleep(1) 
                    instruccion_procesada = True
                else:
                    print("Aruco móvil en movimiento")
                    if angle < 25 and angle > -25:
                        cola_mensajes.put("avanzar 1000")
                        time.sleep(1)
                    elif angle >= 25:
                        cola_mensajes.put("izquierda 1000")
                        time.sleep(1)
                    elif angle <= -25:
                        cola_mensajes.put("derecha 1000")
                        time.sleep(1)
        else:
            aruco_fijo_detectado = False
            print("Aruco fijo no detectado")
            cola_mensajes.put("detener")
            time.sleep(1)
            instruccion_procesada = True      
        
        frame_markers = aruco.drawDetectedMarkers(frame, corners, ids)
        
        cv2.imshow('Detección de arucos', frame_markers)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
# --------------------------------------------------------------------------------

if __name__ == '__main__':
    
    # Crear los hilos
    t1 = threading.Thread(target=comunicacionWebSocket)
    t2 = threading.Thread(target=controlMedianteArUco)

    # Iniciar los hilos
    t1.start()
    t2.start()
    
    while True:
        if keyboard.is_pressed('q'):
            # Establecer los eventos para detener los hilos
            print("[INFO] Deteniendo hilos...")
            time.sleep(1)
            cola_mensajes.put("salir")
            break
    # Esperar a que ambos hilos terminen
    t1.join()
    t2.join()