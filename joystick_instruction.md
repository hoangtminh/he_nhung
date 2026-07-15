# Hướng Dẫn Kết Nối Joystick Và Cấu Hình ADC

Tài liệu này mô tả phần input joystick của game Space Invaders trên STM32F429I-Discovery.
Dự án dùng 2 joystick analog, mỗi joystick có 2 trục X/Y và 1 nút bấm riêng.

## 1. Sơ Đồ Đấu Nối

| Chức năng | Chân STM32 | ADC/GPIO | Ghi chú |
| :--- | :--- | :--- | :--- |
| Joystick 1 VRx | PA5 | ADC1_IN5 | Trục X người chơi đầu tiên |
| Joystick 1 VRy | PA7 | ADC1_IN7 | Trục Y người chơi đầu tiên |
| Nút người chơi 1 | PG3 | GPIO input pull-up | Active-low, nhấn nút sẽ kéo xuống GND |
| Joystick 2 VRx | PC5 | ADC2_IN15 | Trục X người chơi thứ hai |
| Joystick 2 VRy | PA0 | ADC2_IN0 | Trục Y người chơi thứ hai |
| Nút người chơi 2 | PG2 | GPIO input pull-up | Active-low, nhấn nút sẽ kéo xuống GND |

Mỗi module joystick cần đấu thêm:

| Chân joystick | Kết nối |
| :--- | :--- |
| VCC | 3V3 |
| GND | GND |
| VRx | Chân ADC trục X tương ứng |
| VRy | Chân ADC trục Y tương ứng |
| SW | Chân nút tương ứng, đầu còn lại về GND |

Không cấp joystick bằng 5V vì ngõ vào ADC của STM32F429 chỉ làm việc an toàn trong mức 0..3.3V.

## 2. Cấu Hình ADC Thủ Công

ADC không được bật trong CubeMX. Code cấu hình ADC nằm trong `Core/Src/main.c` và ghi trực tiếp thanh ghi:

- `MX_ADC1_Init()` cấu hình ADC1 để đọc PA5/PA7.
- `MX_ADC2_Init()` cấu hình ADC2 để đọc PC5/PA0.
- GPIO được đặt ở chế độ analog, không pull-up/pull-down.
- `ADC->CCR` chọn prescaler PCLK2 / 4.
- `SMPR1/SMPR2` tăng thời gian lấy mẫu cho các kênh joystick.
- `SQR1` đặt regular sequence dài 1 conversion.
- `SQR3` được đổi trước mỗi lần đọc để chọn kênh X hoặc Y.
- `SWSTART` bắt đầu chuyển đổi bằng phần mềm.
- `EOC` được polling để biết khi nào giá trị mới sẵn sàng.
- `DR` trả về giá trị ADC 12-bit trong khoảng 0..4095.

## 3. Luồng Xử Lý Input

Task `StartTask03()` đọc 4 trục joystick mỗi 50 ms:

- ADC1 channel 5: joystick 1 X.
- ADC1 channel 7: joystick 1 Y.
- ADC2 channel 15: joystick 2 X.
- ADC2 channel 0: joystick 2 Y.

Giá trị đọc được đổi thành lệnh ngắn:

| Điều kiện ADC | Lệnh |
| :--- | :--- |
| Giá trị < 1000 | Trái hoặc lên |
| Giá trị > 3000 | Phải hoặc xuống |
| 1000..3000 | Trung lập |

Lệnh khác trung lập được đẩy vào queue FreeRTOS:

- `Joystick1QueueHandle` cho người chơi đầu tiên.
- `Joystick2QueueHandle` cho người chơi thứ hai.

TouchGFX đọc queue trong `handleTickEvent()` của từng màn hình để điều khiển menu và tàu người chơi.

## 4. Nút Bấm

Hai nút ngoài dùng pull-up nội bộ:

- Không nhấn: GPIO đọc `GPIO_PIN_SET`.
- Nhấn: GPIO đọc `GPIO_PIN_RESET`.

Trong màn menu, nút nào cũng có thể xác nhận chế độ đang chọn.
Trong màn chơi, nút của từng người chơi dùng để bắn đạn; khi game over, nút được dùng để restart.
