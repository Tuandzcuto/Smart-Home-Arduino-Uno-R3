# SMART HOME IoT

**Smart Home** là dự án mô hình nhà thông minh đa chức năng sử dụng Arduino Uno R3, được thiết kế nhằm tự động hóa các hoạt động kiểm soát an ninh, cảnh báo an toàn và tiện ích sinh hoạt thông qua bàn phím mật khẩu, hệ thống cảm biến, màn hình LCD 1602 và các cơ cấu chấp hành.

## 1. Tổng quan dự án

Hệ thống được xây dựng dựa trên vi điều khiển Arduino Uno R3, tích hợp đồng bộ các cảm biến và thiết bị điện tử theo cơ chế xử lý song song không trễ (`Non-blocking` với hàm `millis()`), giúp mô hình phản hồi tức thì trước thao tác người dùng và thay đổi của môi trường.

Các thành phần chính:

- Bộ điều khiển: Arduino Uno R3.
- Cảm biến & thiết bị đầu vào: Bàn phím ma trận 4x4, cảm biến khói/khí gas MQ-2, cảm biến nước mưa và cảm biến khoảng cách siêu âm HC-SR04.
- Hiển thị: Màn hình LCD 1602 giao tiếp I2C.
- Thiết bị đầu ra: Còi Buzzer 5V, đèn LED, module Relay 5V điều khiển quạt hút và 2 động cơ Servo SG90.

## 2. Chức năng chính

- Kiểm soát đóng/mở cửa tự động bằng mật khẩu qua bàn phím 4x4 và động cơ Servo SG90.
- Hỗ trợ đổi mật khẩu trực tiếp bằng phím `A` và kích hoạt báo động còi kêu, đèn LED nhấp nháy khi nhập sai 5 lần liên tiếp.
- Tự động thu giàn phơi vào mái che khi cảm biến nước mưa phát hiện trời mưa và đẩy ra lại khi khô ráo.
- Phát hiện khói/khí gas bằng cảm biến MQ-2 để kích hoạt Relay bật quạt hút và hú còi cảnh báo.
- Tự động bật đèn LED chiếu sáng trong 5 giây khi cảm biến siêu âm HC-SR04 phát hiện có người lại gần dưới 20 cm.
- Hiển thị trạng thái trực quan trên màn hình LCD 1602 và phát âm thanh phản hồi phân biệt qua còi Buzzer.

## 3. Công nghệ sử dụng

| Thành phần | Công nghệ |
| :--- | :--- |
| Vi điều khiển | Arduino Uno R3 |
| Ngôn ngữ lập trình | Arduino C/C++ |
| Nhập liệu bảo mật | Bàn phím ma trận Keypad 4x4 |
| Cảm biến khói / gas | MQ-2 (Analog) |
| Cảm biến mưa | Cảm biến nước mưa (Analog) |
| Cảm biến khoảng cách | HC-SR04 |
| Màn hình | LCD 1602 I2C |
| Động cơ | Servo SG90 × 2 |
| Đóng ngắt & làm mát | Module Relay 5V + Quạt DC 5V |
| Âm thanh & chiếu sáng | Còi Buzzer 5V, Đèn LED |
| Giao tiếp | I2C, Digital I/O, Analog, PWM |
| Phần mềm | Arduino IDE |

## 4. Hình ảnh dự án

### 4.1. Poster

<img width="1084" height="1451" alt="posterr" src="https://github.com/user-attachments/assets/4b67abe2-2fc5-4af4-92fb-960ad56c18a7" />



### 4.2. Mô hình Smart Home
<img width="1536" height="1024" alt="mohinh" src="https://github.com/user-attachments/assets/67d3a251-5849-45da-8ff2-998951cc5041" />




### 4.3. Sơ đồ hệ thống

<img width="2400" height="1792" alt="Gemini_Generated_Image_kw1hbekw1hbekw1h" src="https://github.com/user-attachments/assets/37183d15-7175-43c4-a3b0-e038ac27207f" />


### 4.4. Các linh kiện sử dụng

<img width="2400" height="1792" alt="linhkien" src="https://github.com/user-attachments/assets/bb86110d-4ca3-4cbf-a22f-9f14a1616812" />


## 5. Nguyên lý hoạt động

1. Bàn phím 4x4 tiếp nhận thao tác nhập và đổi mật khẩu của người dùng.
2. Các cảm biến MQ-2, cảm biến mưa và HC-SR04 liên tục đo nồng độ khói/gas, độ ẩm nước mưa và khoảng cách vật thể phía trước.
3. Arduino Uno R3 tiếp nhận và xử lý tín hiệu từ các cảm biến theo cơ chế đa nhiệm `millis()`.
4. Hệ thống điều khiển 2 Servo SG90, Relay quạt hút, đèn LED và còi Buzzer để phản hồi tương ứng theo từng tình huống.
5. Trạng thái hoạt động và cảnh báo được hiển thị trực quan trên màn hình LCD 1602.

## 6. Hướng phát triển

- Tích hợp kết nối WiFi (ESP8266 / ESP32) để giám sát thông số và điều khiển thiết bị từ xa qua ứng dụng điện thoại (IoT).
- Bổ sung tính năng điều khiển nhà thông minh bằng giọng nói.
- Nâng cấp lưu trữ mật khẩu mới vào bộ nhớ EEPROM để không bị đặt lại mặc định khi ngắt nguồn điện.
- Bổ sung cảm biến nhiệt độ, độ ẩm (DHT11/DHT22) để hiển thị thông tin môi trường lên màn hình LCD.
- Cải tiến thiết kế vỏ mô hình để tăng tính thẩm mỹ và tiện dụng.

## 7. Thông tin dự án

- Sinh viên thực hiện: Nguyễn Xuân Tuấn, Trần Đình Nguyên
- Nền tảng: Arduino Uno R3
- Ngôn ngữ: Arduino C/C++
- Công cụ phát triển: Arduino IDE

---

**SMART HOME IoT – Your Smart Living Solution**
