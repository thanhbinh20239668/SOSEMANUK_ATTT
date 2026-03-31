# 🛡️ SOSEMANUK Stream Cipher Implementation (C Language)

Dự án triển khai thuật toán mã hóa dòng **SOSEMANUK**, một trong những thuật toán chiến thắng trong dự án **eSTREAM** (Profile 1 - Software-oriented). Thuật toán kết hợp thiết kế giữa hệ thống thanh ghi dịch phản hồi tuyến tính (LFSR) và máy trạng thái hữu hạn (FSM).

---

## 🏗️ Kiến trúc thuật toán

Hệ thống mã hóa SOSEMANUK trong code được xây dựng dựa trên 3 thành phần cốt lõi:

### 1. LFSR (Linear Feedback Shift Register)
* Gồm 10 thanh ghi 32-bit ($s_0$ đến $s_9$).
* Hoạt động trên trường hữu hạn $GF(2^{32})$.
* Đóng vai trò cung cấp tính chất thống kê và chu kỳ dài cho dòng khóa.

### 2. FSM (Finite State Machine)
* Sử dụng 2 biến trạng thái nội bộ $R1, R2$.
* Kết hợp với đầu ra của LFSR để tạo ra giá trị phi tuyến $f_t$.
* Sử dụng hàm `Trans` (phép nhân và xoay bit `ROTL32`) để khuếch tán dữ liệu.

### 3. Serpent Components
* **S-Box:** Sử dụng bảng tra (Lookup Table) từ thuật toán Serpent để khởi tạo khóa.
* **Bitslice S-Box 2:** Được tối ưu hóa bằng các cổng logic (XOR, AND, OR, NOT) để xử lý 4 words dữ liệu cùng lúc trong hàm `Sosemanuk_SBox2`.
* **Linear Transformation (LT):** Tăng cường độ xáo trộn giữa các bit trong quá trình Setup IV.

---

## 🚀 Tính năng chương trình

Chương trình cung cấp một giao diện Console trực quan với các chức năng:
* **Mã hóa file:** Đọc `input.txt` -> Xuất `encrypted.bin`.
* **Giải mã file:** Đọc `encrypted.bin` -> Khôi phục `decrypted.txt`.
* **Ghi đè Input:** Nhập văn bản trực tiếp để thử nghiệm nhanh.
* **Benchmark:** Đo tốc độ mã hóa thực tế (MB/s) của thuật toán trên phần cứng hiện tại.
* **Hex Dump:** Xem dữ liệu dưới dạng mã hexa để kiểm tra cấu trúc file nhị phân.

---

## 💻 Hướng dẫn sử dụng

### Biên dịch
Sử dụng trình biên dịch GCC hoặc Clang:
```bash
gcc main.c -o sosemanuk
