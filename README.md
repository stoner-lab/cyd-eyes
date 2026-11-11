# CYD eyes

Đôi mắt biểu cảm trên board `ESP32-2432S028` (CYD bản hai cổng USB, chip CH340C).
Màn 2.8" 320×240, cảm ứng điện trở XPT2046, microSD, LED RGB, LDR, không loa.

## Đang có gì

| Thành phần | Mô tả |
|---|---|
| `lib/eye` | Engine biểu cảm: 11 emotion, mắt pill bo góc, mí trên `lidDrop`, mắt nhắm cong, lông mày cong, catchlight, blink, gaze |
| `lib/touch` | Driver XPT2046 bit-bang (bus riêng 25/32/33/36/39), hiệu chỉnh thực đo, lọc nhiễu 3 mẫu |
| `lib/gallery` | Xem ảnh JPEG từ `/cyd` trên microSD, giải mã từ RAM (16–33 KB/ảnh), không dùng File handle |
| `lib/game2048` | 2048 4×4, không animation, 32 byte bàn cờ, điểm + kỷ lục |
| `lib/pomodoro` | Pomodoro 25/5, tiếng Anh, đếm bằng millis (không cần RTC) |
| `lib/settings` | Độ sáng, mốc tắt màn, màu mắt — lưu NVS namespace `cfg` |
| `lib/uploader` | Nhận file qua serial (`PUT`/`LIST`/`SHOW`/`TEST`) vì máy không có đầu đọc thẻ |
| `src/eyes_main.cpp` | Vòng lặp 40 fps, cử chỉ, giảm sáng, chuyển màn |

## Cử chỉ

| Ngữ cảnh | Thao tác | Kết quả |
|---|---|---|
| Màn mắt | Chạm | Chớp mắt |
| Màn mắt | Vuốt lên ≥ 90 px và ≥ 675 px/s | Ảnh ngẫu nhiên |
| Màn mắt | Vuốt phải ≥ 160 px và ≥ 520 px/s | Mở 2048 |
| Màn mắt | Vuốt quanh vùng giữa hai mắt | Nhắm mắt tận hưởng (`CONTENT`) |
| Màn ảnh | Vuốt lên ≥ 90 px | Ảnh khác |
| Màn ảnh | Vuốt xuống ≥ 100 px | Về màn mắt |
| 2048 | Vuốt 4 hướng ≥ 60 px | Đi nước |
| 2048 | Chạm nút `<` góc trên-trái | Về màn mắt (giữ ván) |
| 2048 | Giữ yên 1,5 s | Về màn mắt (dự phòng) |
| 2048 | Game over → chạm | Ván mới |
| Màn mắt | Vuốt xuống ≥ 90 px và ≥ 675 px/s | Mở Pomodoro |
| Màn mắt | Vuốt trái ≥ 160 px và ≥ 520 px/s | Mở Settings |
| Pomodoro | Chạm | Chạy / tạm dừng |
| Pomodoro | Vuốt xuống / lên ≥ 60 px | Reset / về màn mắt |
| Settings | Chạm nút | Đổi và lưu |
| Settings | Vuốt phải ≥ 60 px | Về màn mắt |

Ngưỡng tốc độ chỉ áp dụng ở màn mắt, nơi cử chỉ tranh chấp với việc liếc mắt theo
ngón tay. Kéo chậm chỉ làm mắt liếc. Trong game và màn ảnh, ngưỡng thấp vì không có
gì để nhầm.

Tự động: `DAZED` sau 45 s, `SLEEPY` sau 3 phút, `ASLEEP` sau 5 phút; giảm đèn nền
còn ~23% sau 15 s không chạm.

## Lệnh

```powershell
$pio  = 'C:\Python314\python.exe'
$proj = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet'

& $pio -m platformio run -e cyd -t upload --project-dir $proj   # nạp
& $pio -m platformio run -e cyd_probe -t upload --project-dir $proj
```

Cổng USB của board này chập chờn: dùng `tools\flash_retry.ps1` (chờ cổng rồi nạp,
thử nhiều lượt) thay vì `upload` trực tiếp. Nạp bị huỷ giữa chừng sẽ để lại firmware dở.

## Ảnh

Ảnh nguồn ở `C:\Users\duy\Desktop\cyd`. Gửi sang thẻ:

```powershell
& 'C:\Python314\python.exe' 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet\tools\send_photos.py' COM3
```

Script crop 4:3, thu nhỏ hai bước về 320×240, JPEG q92 **4:4:4** (4:2:0 làm nhoè chi tiết).

## Số liệu thật của board

| Hạng mục | Giá trị |
|---|---|
| Panel | ILI9341, cần `-DTFT_INVERSION_ON=1` |
| Hiệu chỉnh cảm ứng | X 360–1700, Y 320–1670, không hoán trục |
| Điểm mù | sát biên trả toạ độ kẹp về 0 — nút bấm phải tính cả vùng 0..70 |
| RAM tĩnh | 26.780 B / 327.680 B |
| Flash | 422 KB / 3 MB |
| fps màn mắt | 36–38 |
| LDR | sáng = 0, che tay = tăng; `dark = ldr > 1500` |

## Chưa làm

- OTA qua WiFi đã thử và **gỡ bỏ**: board nằm ở mạng khác PC (192.168.0.x so với
  192.168.5.x), nên nạp bằng cáp qua `tools\flash_retry.ps1` là đường chính thức.
  `include\secrets.h` còn trên đĩa nhưng không còn được biên dịch.
- Kỷ lục 2048 chỉ nằm trong RAM, reset là mất (thêm 4 byte NVS là xong).
- Chưa xác nhận đèn nền có nhấp nháy khi PWM ở mức 23% không.
- Không có loa nên chưa có âm thanh.
