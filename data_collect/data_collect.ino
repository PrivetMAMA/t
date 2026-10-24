#include <EEPROM.h>
#include <MPU9250_WE.h>
#include <Wire.h>

#define MPU9250_ADDR 0x68
MPU9250_WE sensor = MPU9250_WE(MPU9250_ADDR);

// Та самая структура калибровки, которая лежит в EEPROM
struct CalibrationData {
  uint32_t magicKey;
  xyzFloat accOffset;
  xyzFloat gyrOffset;
};

bool recording = false;
unsigned long startTime = 0;
const unsigned long RECORD_DURATION = 10000; // 10 секунд записи

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  sensor.init();

  // --- ЧТЕНИЕ КАЛИБРОВКИ ИЗ ПАМЯТИ ---
  CalibrationData storedData;
  EEPROM.get(0, storedData);

  if (storedData.magicKey == 0xDEADBEEF) {
    sensor.setAccOffsets(storedData.accOffset);
    sensor.setGyrOffsets(storedData.gyrOffset);
  } else {
    // Резервный вариант, если забыл прошить калибратор
    Serial.println("EEPROM пуста! Делаю autoOffsets...");
    delay(1000);
    sensor.autoOffsets(); 
  }

  // --- НАСТРОЙКА ЖЕЛЕЗА ---
  sensor.setSampleRateDivider(0);
  sensor.enableGyrDLPF();
  sensor.enableAccDLPF(true);
  sensor.setGyrDLPF(2); // Стабильные 92 Гц
  sensor.setAccDLPF(2);
  sensor.setAccRange(MPU9250_ACC_RANGE_2G);

  Serial.println("--- СБОРЩИК ГОТОВ ---");
  Serial.println("Отправь 's' в консоль для старта записи (10 сек).");
  Serial.println("Время,AccX,AccY,AccZ,MagGyro"); // Шапка для Excel
}

void loop() {
  // Ждем команду от пользователя
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 's' || cmd == 'S') {
      recording = true;
      startTime = millis();
    }
  }

  // Основной цикл записи данных
  if (recording) {
    if (millis() - startTime < RECORD_DURATION) {
      xyzFloat acc = sensor.getGValues();
      xyzFloat gyr = sensor.getGyrValues();

      // Магнитуда гироскопа для поиска пиков (рабочего хода)
      float magGyr = sqrt(pow(gyr.x, 2) + pow(gyr.y, 2) + pow(gyr.z, 2));

      // Чистый вывод для CSV, разделенный запятыми
      Serial.print(millis() - startTime);
      Serial.print(",");
      Serial.print(acc.x, 4);
      Serial.print(",");
      Serial.print(acc.y, 4); // ДОБАВЛЕНА ОСЬ Y
      Serial.print(",");
      Serial.print(acc.z, 4);
      Serial.print(",");
      Serial.println(magGyr, 2);
      
      delay(20); // Строго 50 Гц (шаг в 20 мс)
    } else {
      recording = false;
      Serial.println("Запись окончена. Жду 's' для записи следующей зоны.");
    }
  }
}