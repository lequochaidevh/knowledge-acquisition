# 📘 Derivative Notes — Bảng Công Thức Đạo Hàm

```sh
ctrl+shift+V
```

## 🧩 I. CÁC ĐẠO HÀM CƠ BẢN

| Hàm số f(x) | Đạo hàm f'(x) |
|--------------|----------------|
| c (hằng số) | 0 |
| x | 1 |
| x^n | n·x^(n−1) |
| √x = x^(1/2) | 1 / (2√x) |
| 1/x = x^(−1) | −1 / x² |
| a^x | a^x·ln(a) |
| e^x | e^x |
| ln(x) | 1 / x |
| logₐ(x) | 1 / (x·ln(a)) |

---

## 📐 II. HÀM LƯỢNG GIÁC

| Hàm | Đạo hàm |
|------|----------|
| sin(x) | cos(x) |
| cos(x) | −sin(x) |
| tan(x) | 1 / cos²(x) = sec²(x) |
| cot(x) | −1 / sin²(x) = −csc²(x) |
| sec(x) = 1/cos(x) | sec(x)·tan(x) |
| csc(x) = 1/sin(x) | −csc(x)·cot(x) |

---

## 🔁 III. HÀM NGHỊCH ĐẢO LƯỢNG GIÁC

| Hàm | Đạo hàm |
|------|----------|
| arcsin(x) | 1 / √(1 − x²) |
| arccos(x) | −1 / √(1 − x²) |
| arctan(x) | 1 / (1 + x²) |
| arccot(x) | −1 / (1 + x²) |
| arcsec(x) | 1 / (\|x\|√(x² − 1)) |
| arccsc(x) | −1 / (\|x\|√(x² − 1)) |

---

## ⚙️ IV. QUY TẮC ĐẠO HÀM (TỔ HỢP)

| Dạng | Công thức |
|-------|------------|
| (u ± v)' | u' ± v' |
| (u·v)' | u'v + uv' |
| (u/v)' | (u'v − uv') / v² |
| (f(g(x)))' | f'(g(x))·g'(x) |
| (uⁿ)' | n·u^(n−1)·u' |
| (ln(u))' | u'/u |
| (aᵘ)' | aᵘ·ln(a)·u' |
| (eᵘ)' | eᵘ·u' |

---

## 💫 V. MỘT SỐ HÀM ĐẶC BIỆT

| Hàm | Đạo hàm |
|------|----------|
| \|x\| | x/\|x\| (với x ≠ 0) |
| ln\|x\| | 1/x |
| x^x | x^x (ln(x) + 1) |
| a^(x²) | 2x·a^(x²)·ln(a) |
| sin²(x) | 2·sin(x)·cos(x) = sin(2x) |
| e^(kx) | k·e^(kx) |

---

## 📊 VI. XẤP XỈ ĐẠO HÀM SỐ (NUMERICAL DERIVATIVE)

Sử dụng **vi phân trung tâm**:

\[
f'(x) ≈ (f(x + h) − f(x − h)) / (2h)
\]

Với `h` nhỏ, ví dụ `1e-5`.

---

🧠 **Mẹo học nhanh:**
- Nhớ quy tắc *chuỗi* (chain rule): đạo hàm hàm hợp.
- Nhớ rằng *đạo hàm là tốc độ biến thiên*.
- Khi đạo hàm bằng 0 → có thể là cực trị hoặc điểm uốn (kiểm tra thêm đạo hàm bậc 2).

---


📅 Phiên bản: 2025-10-17  
📄 Dành cho học & lập trình biểu đồ cực trị C++
