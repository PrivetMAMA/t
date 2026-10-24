#include <MPU9250_WE.h>
#include <Wire.h>

#define MPU9250_ADDR 0x68
#define BUFFER_SIZE 50
const int PATTERN_SIZE = 17;

MPU9250_WE sensor = MPU9250_WE(MPU9250_ADDR);

struct Frame {
  float ax, ay, az;
  float gx, gy, gz;
};

Frame data_buffer[BUFFER_SIZE];
int head = 0;

// БАЗА ЭТАЛОНОВ
float pattern_TopRight[PATTERN_SIZE] = {
  20.1, 47.3, 102.3, 140.9, 153.2, 141.3, 110.9, 80.4, 49.9, 
  62.9, 122.6, 160.8, 161.7, 136.1, 97.1, 62.6, 32.9
};

float pattern_TopLeft[PATTERN_SIZE] = {
  62.1, 66.6, 71.1, 101.0, 131.5, 126.8, 104.3, 72.1, 39.3, 
  71.0, 100.2, 111.8, 114.6, 99.2, 87.5, 75.9, 65.0
};


// === СЧЕТЧИКИ ДВИЖЕНИЙ ===
int count_TopRight = 0;
int count_TopLeft = 0;

// Таймер блокировки (чтобы не засчитать один взмах дважды на скользящем окне)
unsigned long lockTimer = 0; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  sensor.init();
  delay(1000);
  sensor.autoOffsets();
  
  sensor.setSampleRateDivider(0);
  sensor.enableGyrDLPF();
  sensor.enableAccDLPF(true);
  sensor.setGyrDLPF(2);
  sensor.setAccDLPF(2);
  sensor.setAccRange(MPU9250_ACC_RANGE_2G);
  
  Serial.println("Ready");
}

void loop() {
  xyzFloat acc = sensor.getGValues();
  xyzFloat gyr = sensor.getGyrValues();

  // Запись в кольцевой буфер
  data_buffer[head] = {acc.x, acc.y, acc.z, gyr.x, gyr.y, gyr.z};
  head = (head + 1) % BUFFER_SIZE;

  // Шаг окна — 2 кадра (40 мс). Опрашиваем часто, так как окно стало коротким
  if (head % 2 == 0) {
    analyze();
  } 

  delay(20);
}

void analyze() {
  // 1. Вычисляем среднее положение щетки по X и Z за последнюю секунду
  float avgX = 0;
  float avgZ = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    avgX += data_buffer[i].ax;
    avgZ += data_buffer[i].az;
  }
  avgX /= BUFFER_SIZE;
  avgZ /= BUFFER_SIZE;

  // Если сработал таймер блокировки после успешного жеста — выходим
  if (millis() < lockTimer) return;

  // БАЗОВЫЙ ФИЛЬТР: Мы на верхней челюсти (Z в минусе для обеих зон)
  if (avgZ < -0.40) { 
    
    // РАЗВЕЛКА 1: ПРАВАЯ СТОРОНА (AccX уходит в минус, в среднем -0.35)
    if (avgX < 0.0) {
      // Вызываем проверку с правым эталоном (17 чисел, которые мы нашли ранее)
      if (checkPattern(pattern_TopRight, 750.0)) {
        count_TopRight++;
        Serial.print("ВВЕРХ-ПРАВО: ");
        Serial.println(count_TopRight);
        triggerLED();
      }
    } 
    
    // РАЗВЕЛКА 2: ЛЕВАЯ СТОРОНА (AccX уходит в плюс, в среднем +0.62)
    else if (avgX > 0.40) {
      // Вызываем проверку с НОВЫМ левым эталоном
      if (checkPattern(pattern_TopLeft, 400.0)) {
        count_TopLeft++;
        Serial.print("ВВЕРХ-ЛЕВО: ");
        Serial.println(count_TopLeft);
        triggerLED();
      }
    }
  }
}

// Универсальная функция проверки: принимает указатель на массив-эталон и индивидуальный порог diff
bool checkPattern(float* target_pattern, float max_diff) {
  float diff = 0;
  
  for(int i = 0; i < PATTERN_SIZE; i++) {
    // Отматываем кольцевой буфер назад строго на PATTERN_SIZE кадров
    int idx = (head - PATTERN_SIZE + i + BUFFER_SIZE) % BUFFER_SIZE;
    
    float currentResGyr = sqrt(pow(data_buffer[idx].gx, 2) + pow(data_buffer[idx].gy, 2) + pow(data_buffer[idx].gz, 2));
    
    diff += abs(currentResGyr - target_pattern[i]);
  }

  // Если график совпал с эталоном, возвращаем true
  if (diff < max_diff) {
    return true;
  }
  return false;
}

void triggerLED() {
  digitalWrite(LED_BUILTIN, HIGH);
  lockTimer = millis() + 350; 
  delay(150); 
  digitalWrite(LED_BUILTIN, LOW);
}
