# SOSEMANUK Stream Cipher — Giải thích & Đánh giá mã nguồn C

# SOSEMANUK Stream Cipher

## 1. SOSEMANUK là gì?

SOSEMANUK là một **stream cipher** (mã dòng) hướng phần mềm, do nhóm tác giả Pháp (Berbain, Billet, Canteaut, Chevallier‑Mames, Gilbert...) thiết kế, lọt vào vòng chung kết eSTREAM (profile phần mềm). Nó lai giữa hai ý tưởng:

- **Kiến trúc LFSR + FSM** kiểu SNOW 2.0: một thanh ghi dịch phản hồi tuyến tính 10×32-bit kết hợp với một máy trạng thái hữu hạn phi tuyến 2×32-bit để sinh keystream.
- **Serpent** (rút gọn còn 24 vòng, không hoán vị cuối) dùng làm hàm khởi tạo: từ Key sinh 100 subkey, rồi dùng các subkey đó "chạy" IV qua Serpent để nạp giá trị khởi tạo cho LFSR và FSM.

## 2. Luồng xử lý tổng thể trong code

1. `Sosemanuk_KeySetup`: đệm khoá 128-bit lên 256-bit, khai triển 132 từ nháp bằng hằng số vàng `PHI`, sau đó "nghiền" qua các S-box Serpent (chạy lùi S3→S0→S7...) để ra 100 subkey.
2. `Sosemanuk_IVSetup`: đưa IV qua 24 vòng Serpent dùng 100 subkey trên, trích xuất trạng thái ở vòng 12 và vòng 18 để nạp cho `s[]`, phần còn lại nạp cho `s[5], s[6], r1, r2`.
3. `Sosemanuk_GenerateKeystreamBlock`: mỗi lần sinh 4 từ (16 byte) keystream bằng cách chạy FSM 4 bước, dịch LFSR 4 bước, đưa 4 giá trị `f_t` qua `Sosemanuk_SBox2` (S-box số 2 của Serpent, dạng bitslice), rồi XOR với trạng thái LFSR cũ.
4. `Sosemanuk_ProcessData`: XOR keystream với dữ liệu — encrypt và decrypt dùng chung một hàm (đặc trưng của stream cipher).

## 3. Biên dịch & chạy

```bash
gcc -O2 -o sosemanuk sosemanuk.c
./sosemanuk
```

Chương trình cung cấp menu: nhập văn bản, mã hoá `input.txt` → `encrypted.bin`, giải mã ngược lại, xem hex dump, và benchmark tốc độ (MB/s).

## 4. Giải thích nội dung trong ảnh và cơ sở toán học của `mul_table`/`div_table`

Ảnh gốc ghi: *"SOSEMANUK, giải thích bằng Mul_table và Div_table làm rõ cách tìm giá trị alpha"*. Đây là phần toán trên trường hữu hạn dùng để tính phản hồi LFSR (`Calculate_LFSR_Feedback`).

### 4.1 Trường nền `GF(2⁸)`

Gọi `β` là nghiệm của đa thức nguyên thủy trên `F₂[X]`:

```
Q(X) = X^8 + X^7 + X^5 + X^3 + 1
```

`GF(2⁸) = F₂[X]/Q(X)`, mỗi phần tử viết theo cơ sở `(β⁷,...,β,1)` và đồng nhất với số nguyên 8-bit qua `φ(Σxᵢβⁱ) = Σxᵢ2ⁱ`. Vì `Q` nguyên thủy nên `β` sinh toàn bộ nhóm nhân bậc **255**: mọi phần tử khác 0 viết được dưới dạng `βᵏ` (0 ≤ k ≤ 254). Đặc tả gốc cho `φ(β²³) = 0xE1` — dùng làm mốc đối chiếu ở bước kiểm chứng bên dưới.

### 4.2 Trường mở rộng `GF(2³²)` và α

`α` là nghiệm của đa thức nguyên thủy trên `GF(2⁸)[X]`:

```
P(X) = X^4 + β^23·X^3 + β^245·X^2 + β^48·X + β^239
```

`GF(2³²) = GF(2⁸)[X]/P(X)`. Một từ 32-bit `c` là đa thức bậc ≤3, hệ số 1 byte mỗi bậc:

```
c = c3·α³ + c2·α² + c1·α + c0        (c3,c2,c1,c0 ∈ GF(2⁸))
```

### 4.3 Suy ra công thức nhân với α — nguồn gốc thật của `mul_table`

Vì `α` là nghiệm của `P`, ta có quan hệ rút gọn:

```
α⁴ = β²³α³ + β²⁴⁵α² + β⁴⁸α + β²³⁹
```

Nhân `c` với `α`:

```
c·α = c3α⁴ + c2α³ + c1α² + c0α
    = c3(β²³α³+β²⁴⁵α²+β⁴⁸α+β²³⁹) + c2α³ + c1α² + c0α
    = (c3β²³ ⊕ c2)α³ + (c3β²⁴⁵ ⊕ c1)α² + (c3β⁴⁸ ⊕ c0)α + c3β²³⁹
```

(`⊗` = nhân trong `GF(2⁸)`; cộng trong `GF(2⁸)` = XOR vì đặc số 2). Tách thành 4 byte `(d3,d2,d1,d0)`:

```
d3 = c2 ⊕ (c3⊗β²³)      d2 = c1 ⊕ (c3⊗β²⁴⁵)
d1 = c0 ⊕ (c3⊗β⁴⁸)      d0 = 0 ⊕ (c3⊗β²³⁹)
```

Vế `(c2,c1,c0,0)` là "dịch trái một byte"; vế còn lại chỉ phụ thuộc byte `c3`, nên được tính sẵn cho cả 256 giá trị:

```
mul_table[b] = (b⊗β²³) || (b⊗β²⁴⁵) || (b⊗β⁴⁸) || (b⊗β²³⁹)     (ghép 4 byte MSB→LSB)
```

Đây là bản chất toán học của bảng: **kết quả rút gọn `α⁴ mod P(X)` nhân với hệ số byte tràn**, tra bảng thay vì tính GF(2⁸) mỗi lần.

### 4.4 Suy ra công thức chia cho α — nguồn gốc thật của `div_table`

Cần `e = c·α⁻¹`, tức `c = e·α`. Áp công thức 4.3 với `e` thay `c` rồi giải ngược:

```
c0 = e3⊗β²³⁹  ⇒  e3 = c0 ⊗ β⁻²³⁹ = c0 ⊗ β¹⁶     (vì β²³⁹⊗β¹⁶ = β²⁵⁵ = 1, nhóm nhân bậc 255)
e2 = c3 ⊕ (e3⊗β²³)
e1 = c2 ⊕ (e3⊗β²⁴⁵)
e0 = c1 ⊕ (e3⊗β⁴⁸)
```

Vế `(0,c3,c2,c1)` là "dịch phải một byte"; phần còn lại chỉ phụ thuộc byte `c0`, nên:

```
div_table[b] = (b⊗β¹⁶) || (b⊗β¹⁶⊗β²³) || (b⊗β¹⁶⊗β²⁴⁵) || (b⊗β¹⁶⊗β⁴⁸)
```

Cấu trúc **đối xứng với `mul_table`**, chỉ khác việc nhân thêm `β¹⁶` (nghịch đảo nhân của `β²³⁹`) trước khi tra hệ số.

### 4.5 Kiểm chứng số liệu — không dừng lại ở lý thuyết suông

Đã dựng lại `GF(2⁸)` bằng chính `Q(X)` ở trên, tính `β²³, β²⁴⁵, β⁴⁸, β²³⁹, β¹⁶`, rồi tái tạo `mul_table`/`div_table` hoàn toàn từ công thức 4.3–4.4 và đối chiếu với mảng hằng số trong code:

| Vị trí | Tính lại từ toán học | Trong code | Khớp? |
|---|---|---|---|
| `mul_table[0]` | `0x00000000` | `0x00000000` | ✅ |
| `mul_table[1]` | `0xE19FCF13` | `0xE19FCF13` | ✅ |
| `mul_table[255]` | `0x3F53B5EB` | `0x3F53B5EB` | ✅ |
| `div_table[1]` | `0x180F40CD` | `0x180F40CD` | ✅ |
| `div_table[255]` | `0xB6F3A5E2` | `0xB6F3A5E2` | ✅ |

Ngoài ra `φ(β²³)` tính lại ra đúng `0xE1` như đặc tả gốc nêu, xác nhận cả `Q(X)` lẫn cách dựng `β` đều đúng.

**Kết luận và giới hạn của kết luận này:** hai bảng `mul_table`/`div_table` là chính xác theo đặc tả toán học gốc của SOSEMANUK — không phải hằng số tự chế hay sai lệch. Đây **chỉ** xác nhận đúng phần số học trường hữu hạn `GF(2³²)`; nó *không* chứng minh toàn bộ cài đặt đúng, vì các phần khác (thứ tự S-box trong `KeySetup`, hằng số dịch trong `Serpent_LT`, điểm trích xuất LFSR ở vòng 12/18 trong `IVSetup`) là những khối độc lập, cần được đối chiếu test vector riêng.