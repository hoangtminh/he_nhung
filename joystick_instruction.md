# Hướng Dẫn Cấu Hình, Sử Dụng và Kết Nối Joystick

Tài liệu này hướng dẫn chi tiết cách cấu hình phần cứng, nguyên lý hoạt động của mã nguồn và cách đấu nối chân cho Joystick trong dự án game đua xe **Simple Racing** trên kit phát triển **STM32F429I-Discovery**.

---

## 1. Cấu hình Phần cứng & Khai báo trong Mã nguồn (Configuration & Declaration)

Các tài nguyên phần cứng liên quan đến Joystick được cấu hình thông qua **STM32CubeMX** và sinh code trong file [main.c](file:///c:/Users/HP/Downloads/SimpleRacingNew/SimpleRacingNew/SimpleRacingNew/Core/Src/main.c):

### A. Khai báo ADC1 (Đọc giá trị Analog từ Joystick)
* **Bộ điều khiển ADC:** Được khai báo ở dòng 65:
  ```c
  ADC_HandleTypeDef hadc1;
  ```
* **Cấu hình Pin:** Pin **PC3** được cấu hình làm đầu vào Analog cho kênh `ADC_CHANNEL_13` (trong hàm `MX_GPIO_Init()`):
  ```c
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  ```
* **Cấu hình ADC1:** Trong hàm `MX_ADC1_Init()`, ADC1 hoạt động với độ phân giải **12-bit** (giá trị trả về từ `0` đến `4095`), chế độ chuyển đổi đơn lẻ (Single Conversion) kích hoạt bằng phần mềm (Software Trigger).

### B. Khai báo RTOS Message Queue (Truyền nhận lệnh di chuyển)
* **Khai báo hàng đợi:** Bộ đệm hàng đợi nhận lệnh điều khiển xe được khai báo toàn cục:
  ```c
  osMessageQueueId_t Queue1Handle;
  ```
* **Khởi tạo hàng đợi:** Hàng đợi được tạo ra trong hàm `main()` sử dụng CMSIS-RTOS V2 API:
  ```c
  Queue1Handle = osMessageQueueNew(8, sizeof(uint8_t), NULL);
  ```

---

## 2. Nguyên lý Hoạt động & Cách Sử dụng (Usage & Logic)

Hệ thống hoạt động theo mô hình **Producer-Consumer** (Nhà sản xuất - Nhà tiêu thụ) thông qua FreeRTOS Queue để đảm bảo giao diện đồ họa TouchGFX hoạt động mượt mà không bị nghẽn (non-blocking).

### A. Luồng sản xuất lệnh (Producer - Trong main.c)
Một Task FreeRTOS tên là `StartTask03` chuyên trách việc đọc Joystick định kỳ mỗi **50ms**:
1. Khởi động chuyển đổi ADC và đọc giá trị điện áp trục X từ chân **PC3**:
   ```c
   HAL_ADC_Start(&hadc1);
   if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
   {
       adc_x = HAL_ADC_GetValue(&hadc1);
   }
   ```
2. Phân tích giá trị ADC (12-bit nên điểm giữa tĩnh là ~2048):
   * **`adc_x > 3000`**: Joystick đẩy sang **Phải (Right)** $\rightarrow$ Gán lệnh `'R'`.
   * **`adc_x < 1000`**: Joystick đẩy sang **Trái (Left)** $\rightarrow$ Gán lệnh `'L'`.
   * Ngược lại: Không đổi hướng $\rightarrow$ Lệnh `'N'` (Neutral - Trung lập).
3. Gửi lệnh điều khiển vào `Queue1Handle` nếu lệnh khác `'N'`:
   ```c
   if(command != 'N')
   {
       osMessageQueuePut(Queue1Handle, &command, 0, 0);
   }
   ```

### B. Luồng tiêu thụ lệnh (Consumer - Trong TouchGFX)
Mã nguồn C++ của TouchGFX sẽ lấy lệnh từ hàng đợi trong hàm `handleTickEvent()` (chạy định kỳ khoảng 60Hz tương ứng với tần số quét màn hình):

* **Tại Màn hình chính (Screen1):** ([Screen1View.cpp](file:///c:/Users/HP/Downloads/SimpleRacingNew/SimpleRacingNew/SimpleRacingNew/TouchGFX/gui/src/screen1_screen/Screen1View.cpp))
  * Khi đẩy Joystick sang phải (`'R'`), giao diện sẽ tự chuyển sang màn hình chơi game (`Screen2`):
    ```cpp
    if (osMessageQueueGet(Queue1Handle, &cmd, NULL, 0) == osOK)
    {
        if (cmd == 'R')
        {
            application().gotoScreen2ScreenCoverTransitionEast();
        }
    }
    ```

* **Tại Màn hình chơi game (Screen2):** ([Screen2View.cpp](file:///c:/Users/HP/Downloads/SimpleRacingNew/SimpleRacingNew/SimpleRacingNew/TouchGFX/gui/src/screen2_screen/Screen2View.cpp))
  * Nhận lệnh từ hàng đợi. Nếu nhận được lệnh `'R'`, dịch chuyển xe sang phải 10 pixel. Nếu là `'L'`, dịch chuyển xe sang trái 10 pixel:
    ```cpp
    if (osMessageQueueGet(Queue1Handle, &cmd, NULL, 0) == osOK)
    {
        image1.invalidate();
        if (cmd == 'R')
        {
            if (image1.getX() < 180)
            {
                image1.setX(image1.getX() + 10);
            }
        }
        else if (cmd == 'L')
        {
            if (image1.getX() > 0)
            {
                image1.setX(image1.getX() - 10);
            }
        }
        image1.invalidate();
    }
    ```

---

## 3. Hướng dẫn Mắc nối Phần cứng (Hardware Wiring)

Bạn cần chuẩn bị 1 mô-đun **Joystick Analog 2 Trục** (ví dụ: mô-đun Joystick PS2) và thực hiện kết nối dây vào kit phát triển **STM32F429I-Discovery** theo sơ đồ sau:

| Chân trên Joystick | Chân trên STM32F429I-Discovery | Ghi chú |
| :--- | :--- | :--- |
| **GND** | **GND** | Chân đất chung. |
| **VCC** (hoặc `+5V`/`3V3`) | **3V3** | **LƯU Ý:** Nối vào nguồn **3.3V** của Kit để điện áp ra từ Joystick khớp với điện áp tham chiếu ADC (0 - 3.3V). Không nên dùng nguồn 5V vì có thể làm hỏng chân ADC của STM32 hoặc cho sai lệch giá trị. |
| **VRx** (hoặc `X-out`) | **PC3** | Trục điều khiển Trái / Phải của xe. |
| **VRy** (hoặc `Y-out`) | *Để trống* | Trục Lên / Xuống (Game hiện tại không sử dụng trục này). |
| **SW** (hoặc `Key`) | *Để trống* | Nút nhấn tích hợp trên Joystick (Không sử dụng). |

> **Mẹo nhỏ:** Nếu sau khi kết nối mà thao tác gạt Joystick bị ngược chiều di chuyển của xe (ví dụ: gạt sang trái xe lại chạy sang phải), bạn chỉ cần đổi chân đấu nối từ **VRx** sang **VRy** hoặc đảo ngược điều kiện so sánh ADC trong hàm `StartTask03` của file [main.c](file:///c:/Users/HP/Downloads/SimpleRacingNew/SimpleRacingNew/SimpleRacingNew/Core/Src/main.c).
