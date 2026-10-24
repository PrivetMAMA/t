#include <EEPROM.h>
#include <MPU9250_WE.h>
#include <Wire.h>

#define MPU9250_ADDR 0x68
MPU9250_WE sensor = MPU9250_WE(MPU9250_ADDR);

// Структура, которая будет целиком записываться в EEPROM (весит 28 байт)
struct CalibrationData {
  uint32_t magicKey;       // Маркер валидности данных
  xyzFloat accOffset;      // Оффсеты акселерометра
  xyzFloat gyrOffset;      // Оффсеты гироскопа
};

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // if (!sensor.init()) {
  //   Serial.println("Ошибка: датчик MPU9250 не найден!");
  //   while (1); // Останавливаем выполнение
  // }

  sensor.init();

  Serial.println("=== СЕРВИСНАЯ КАЛИБРОВКА MPU9250 ===");
  Serial.println("1. Положите устройство строго горизонтально и не трогайте.");
  Serial.println("2. Отправьте английскую букву 'c' в консоль для старта.");

  // Ждем команду от пользователя
  while (true) {
    if (Serial.available() > 0) {
      char cmd = Serial.read();
      if (cmd == 'c' || cmd == 'C') {
        break;
      }
    }
  }

  Serial.println("Идет калибровка... (не дышать на плату)");
  delay(1000); // Даем время на стабилизацию после нажатия
  
  // Библиотека сама производит замеры и вычисляет смещения
  sensor.autoOffsets();
  
  // Упаковываем полученные данные в структуру
  CalibrationData data;
  data.magicKey = 0xDEADBEEF; // Уникальный шестнадцатеричный ключ (маркер)
  data.accOffset = sensor.getAccOffsets();
  data.gyrOffset = sensor.getGyrOffsets();

  // Пишем структуру в память, начиная с 0-го байта
  EEPROM.put(0, data);

  Serial.println("УСПЕХ! Данные зашиты в EEPROM.");
  Serial.print("Gyro X/Y/Z: ");
  Serial.print(data.gyrOffset.x); Serial.print(" / ");
  Serial.print(data.gyrOffset.y); Serial.print(" / ");
  Serial.println(data.gyrOffset.z);
  
  Serial.println("Теперь можно прошивать основной код продукта.");
}

void loop() {
  // Пусто. Калибровка делается один раз в setup.
}