import serial
from vpython import *
import math

# Настройки COM-порта
serial_port = 'COM8'  # ЗАМЕНИТЕ НА ВАШ (например, /dev/ttyUSB0 для Linux)
baud_rate = 115200

try:
    ser = serial.Serial(serial_port, baud_rate, timeout=1)
except Exception as e:
    print(f"Ошибка подключения: {e}")
    exit()

# Создание 3D сцены
scene = canvas(title='MPU9250 Real-time Orientation', width=800, height=600)
scene.range = 2
# Создаем модель датчика (серый прямоугольник)
sensor_model = box(length=1, height=0.1, width=0.6, color=color.gray(0.5))
# Оси для наглядности (X - красная, Y - зеленая, Z - синяя)
x_axis = arrow(axis=vector(1, 0, 0), color=color.red, length=0.5, shaftwidth=0.05)
y_axis = arrow(axis=vector(0, 1, 0), color=color.green, length=0.5, shaftwidth=0.05)

def get_orientation(ax, ay, az):
    """Вычисляет углы наклона по акселерометру"""
    # Roll (крен) и Pitch (тангаж) в радианах
    roll = math.atan2(ay, az)
    pitch = math.atan2(-ax, math.sqrt(ay*ay + az*az))
    return roll, pitch

print("Принимаю данные...")

while True:
    rate(100) # Ограничение частоты обновления визуализации
    if ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            data = line.split(',')
            
            if len(data) == 3:
                ax = float(data[0])
                ay = float(data[1])
                az = float(data[2])

                roll, pitch = get_orientation(ax, ay, az)

                # Создаем вектор направления (взгляда вверх) на основе углов
                # В VPython up — это вектор нормали к поверхности
                up_vec = vector(0, 1, 0)
                
                # Применяем вращение к модели
                sensor_model.up = vector(math.sin(roll), math.cos(roll), 0)
                sensor_model.axis = vector(math.cos(pitch), 0, -math.sin(pitch))

        except Exception as e:
            print(f"Ошибка обработки данных: {e}")
