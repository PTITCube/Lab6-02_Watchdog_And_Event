/**
 * VÍ DỤ 8: EVENT-DRIVEN (HƯỚNG SỰ KIỆN) & TIẾT KIỆM PIN
 * 
 * Mô tả:
 * Thay vì gửi LoRa liên tục mỗi 5 giây gây tốn pin, vệ tinh trong ví dụ này 
 * sẽ "im lặng". Nó chỉ phát sóng LoRa về mặt đất KHI VÀ CHỈ KHI phát hiện
 * các sự kiện bất thường (Nhiệt độ quá cao, hoặc Phát hiện đang Rơi Tự Do).
 */

#include <Arduino.h>
#include <PTITCube.h>

PTIT_Sensor sensor;
PTIT_COM lora;
PTIT_Scheduler scheduler;

// Ngưỡng cảnh báo
const float TEMP_CRITICAL = 45.0; // Độ C
const float FREEFALL_G    = 1.0;  // m/s^2 (gia tốc gần 0)

void anomalyDetectionTask() {
    if (PTIT_Scheduler::lock(PTIT_BUS_I2C, 100)) {
        sensor.update();
        PTIT_Scheduler::unlock(PTIT_BUS_I2C);
    }

    float t = sensor.getTemperature();
    float ax = abs(sensor.getAccX());

    bool isEmergency = false;
    String alertMsg = "EMERGENCY: ";

    // 1. Kiểm tra nhiệt hoả hoạn
    if (t > TEMP_CRITICAL) {
        isEmergency = true;
        alertMsg += "OVERHEAT (" + String(t, 1) + "C)! ";
    }

    // 2. Kiểm tra rơi
    if (ax < FREEFALL_G) {
        isEmergency = true;
        alertMsg += "FREEFALL DETECTED! ";
    }

    // NẾU CÓ SỰ CỐ -> GỬI LORA NGAY LẬP TỨC
    if (isEmergency) {
        PTIT_Scheduler::println("\n[SỰ CỐ] " + alertMsg);
        
        if (PTIT_Scheduler::lock(PTIT_BUS_LORA, 200)) {
            lora.sendMessage(alertMsg);
            PTIT_Scheduler::unlock(PTIT_BUS_LORA);
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println("\n[EXAMPLE] Event-Driven Satellite Mode Started.");
    
    sensor.init();
    lora.init();

    // Quét liên tục mỗi 200ms để không bỏ lỡ khoảnh khắc rơi
    scheduler.addTask("AnomalyScanner", anomalyDetectionTask, 200, 1, 2);

    scheduler.start();
}

void loop() {
    // Để trống
}
