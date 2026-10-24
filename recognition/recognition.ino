#include <MPU9250_WE.h>
#include <Wire.h>
#define MPU9250_ADDR 0x68
#define BUFFER_SIZE 50

MPU9250_WE sensor = MPU9250_WE(MPU9250_ADDR);

struct Frame {
  float ax, ay, az;
  float gx, gy, gz;
};

float pattern_gyro[20] = {
  24.08,
  61.26,
  108.92,
  177.83,
  253.45,
  254.27,
  254.36,
  254.7,
  255.9,
  257.83,
  258.1,
  256.39,
  182.79,
  113.5,
  64,
  29.17,
  25.16,
  96,
  185.1,
  249
};

Frame data_buffer[BUFFER_SIZE];
int head;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  sensor.init();                            //инициализация датчика. ОБЯЗАТЕЛЬНА

  // Serial.println("Position you MPU9250 flat and don't move it - calibrating...");
  delay(1000);
  sensor.autoOffsets();
  // Serial.println("Done");
  sensor.setSampleRateDivider(0);             //частота опроса датчка. чем меньше - тем чаще
  sensor.enableGyrDLPF();                     //включение фильтра низких частот для гироскопа
  sensor.enableAccDLPF(true);                 //включение фильтра низких частот для акселерометра
  sensor.setGyrDLPF(MPU9250_DLPF_2);          //менять для изменения уровня фильтра гироскопа (от 1 до 6)
  sensor.setAccDLPF(MPU9250_DLPF_2);          //менять для изменения уровня фильтра акселерометра (от 1 до 6)
  sensor.setAccRange(MPU9250_ACC_RANGE_2G);   //менять для изменения чувствительности
  Serial.println("Ready");
}

void loop() {
  xyzFloat acc = sensor.getGValues(); //данные с акселерометра
  xyzFloat gyr = sensor.getGyrValues();
  //float resultantG = sensor.getErsultantG(acc); // получение результирующего вектора акселерометра
  // Serial.print(acc.x, 4);
  // Serial.print(",");
  // Serial.print(acc.y, 4);
  // Serial.print(",");
  // Serial.print(acc.z, 4);                         // в неподвижном состоянии по z дожна быть +-"1"
  // Serial.print(",");                              // вывод данных в Serial
  // Serial.print(gyr.x, 3);
  // Serial.print(",");
  // Serial.print(gyr.y, 3);
  // Serial.print(",");
  // Serial.println(gyr.z, 3);

  data_buffer[head] = {acc.x,acc.y,acc.z,gyr.x,gyr.y,gyr.z};
  head = (head + 1) % BUFFER_SIZE;

  if (head % 10 == 0) {
    analyze();
  } 

  delay(20);
}

void analyze() {
  float avgZ = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) avgZ += data_buffer[i].az;
  avgZ /= BUFFER_SIZE;

  if (avgZ > 0.65) checkPattern();
}

void checkPattern() {
  float diff = 0;
  for(int i = 0; i < 20; i++) {
    int idx = (head - 20 + i + BUFFER_SIZE) % BUFFER_SIZE;
    
    float currentResGyr = sqrt(pow(data_buffer[idx].gx,2) + pow(data_buffer[idx].gy,2) + pow(data_buffer[idx].gz,2));

    diff += abs(currentResGyr - pattern_gyro[i]);
  }

  if (diff < 1000.0) {                 // порог сходства паттерна
    Serial.println("Action");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(400);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

