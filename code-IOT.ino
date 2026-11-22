#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// ================= 1. KHAI BÁO CHÂN KẾT NỐI =================
const byte SO_HANG = 4;
const byte SO_COT  = 4;
byte chanHang[SO_HANG] = {2, 3, 4, 5}; // R1 -> R4
byte chanCot[SO_COT]   = {6, 7, 8, 9}; // C1 -> C4

const int PIN_DEN_LED       = 1;  // Chân D1 điều khiển LED (không dùng Serial)
const int PIN_SERVO_CUA     = 10;
const int PIN_SERVO_PHOI    = 11;
const int PIN_RELAY_QUAT    = 12;
const int PIN_COI           = 13; // Còi Buzzer

const int PIN_CAM_BIEN_KHOI = A0;
const int PIN_CAM_BIEN_MUA  = A1;
const int PIN_TRIG          = A2; // HC-SR04 Trig
const int PIN_ECHO          = A3; // HC-SR04 Echo

// ================= 2. THÔNG SỐ CÀI ĐẶT =================
String matKhauDung                    = "1234"; // Mật khẩu mở cửa (đổi bằng phím A)
const int GIOI_HAN_SAI_MK             = 5;      // Sai 5 lần sẽ báo động
const unsigned long THOI_GIAN_KHOA_MK = 10000;  // Thời gian còi kêu & đèn nháy (10 giây)

const int NGUONG_KHOI                 = 400;    // MQ-2 > 400 là có khói/gas
const int NGUONG_CO_MUA               = 850;    // ADC < 850 (có nước đọng) -> Thu giàn phơi
const int NGUONG_HET_MUA              = 930;    // ADC > 930 (lau khô hẳn) -> Đưa giàn phơi ra

const int KHOANG_CACH_BAT_DEN         = 20;     // Dưới 20cm thì bật đèn LED (cm)
const unsigned long THOI_GIAN_GIU_DEN = 5000;   // Giữ đèn sáng 5 giây

// Góc quay Servo
const int GOC_CUA_DONG  = 0;
const int GOC_CUA_MO    = 90;
const int GOC_PHOI_RA   = 0;
const int GOC_THU_VAO   = 90;

// Mức kích Relay (Nếu quạt chạy ngược, đảo LOW <-> HIGH ở 2 dòng này)
const int RELAY_BAT = HIGH;
const int RELAY_TAT = LOW;

// ================= 3. KHỞI TẠO ĐỐI TƯỢNG & BIẾN =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoCua;
Servo servoPhoi;

char phim[SO_HANG][SO_COT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
Keypad banPhim = Keypad(makeKeymap(phim), chanHang, chanCot, SO_HANG, SO_COT);

String matKhauNhap       = "";
String matKhauMoiTam     = "";
int cheDoDoiMK           = 0; // 0: bình thường | 1: nhập MK cũ | 2: nhập MK mới | 3: xác nhận MK mới
int soLanSaiMK           = 0;

bool cuaDangMo           = false;
bool denDangBat          = false;
bool dangCoKhoi          = false;
bool dangCoMua           = false;
bool dangBaoDongSaiMK    = false;
bool dangHienThongBaoTam = false;

bool trangThaiCoiChay    = false;
bool trangThaiNhaySaiMK  = false;

// Biến điều khiển kiểu kêu còi bàn phím không gây treo (Non-blocking)
int kieuKeuCoi            = 0; // 0: tắt, 1: bấm phím, 2: MK đúng, 3: MK sai
int buocKeuCoi            = 0;
unsigned long thoiDiemCoi = 0;

unsigned long thoiDiemMoCua       = 0;
unsigned long thoiDiemBatDen      = 0;
unsigned long thoiDiemDocCamBien  = 0;
unsigned long thoiDiemNhayCoiChay = 0;
unsigned long thoiDiemThongBaoLCD = 0;
unsigned long thoiDiemBaoDongSai  = 0;
unsigned long thoiDiemNhaySaiMK   = 0;

// ================= 4. SETUP =================
void setup() {
  pinMode(PIN_DEN_LED, OUTPUT);
  pinMode(PIN_RELAY_QUAT, OUTPUT);
  pinMode(PIN_COI, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  digitalWrite(PIN_DEN_LED, LOW);
  digitalWrite(PIN_RELAY_QUAT, RELAY_TAT);
  digitalWrite(PIN_COI, LOW);
  digitalWrite(PIN_TRIG, LOW);

  servoCua.attach(PIN_SERVO_CUA);
  servoPhoi.attach(PIN_SERVO_PHOI);
  servoCua.write(GOC_CUA_DONG);
  servoPhoi.write(GOC_PHOI_RA);

  lcd.init();
  lcd.backlight();
  capNhatManHinhChinh();
}

// ================= 5. LOOP (CHẠY SONG SONG NON-BLOCKING) =================
void loop() {
  unsigned long hienTai = millis();

  // 1. Quét bàn phím & điều khiển nhịp kêu còi bàn phím liên tục
  xuLyBanPhim();
  xuLyCoiBanPhim(hienTai);

  // 2. Xử lý báo động khi nhập sai mật khẩu quá 5 lần (10 giây)
  if (dangBaoDongSaiMK) {
    if (hienTai - thoiDiemBaoDongSai >= THOI_GIAN_KHOA_MK) {
      // Hết 10 giây báo động -> Tắt còi/đèn và mở khóa bàn phím
      dangBaoDongSaiMK = false;
      soLanSaiMK = 0;
      trangThaiNhaySaiMK = false;
      denDangBat = false;
      digitalWrite(PIN_COI, LOW);
      digitalWrite(PIN_DEN_LED, LOW);
      capNhatManHinhChinh();
    } else if (!dangCoKhoi && (hienTai - thoiDiemNhaySaiMK >= 250)) {
      // Nháy đảo pha mỗi 250ms: Còi kêu thì Đèn tắt, Còi tắt thì Đèn sáng (chống sụt áp)
      thoiDiemNhaySaiMK = hienTai;
      trangThaiNhaySaiMK = !trangThaiNhaySaiMK;
      digitalWrite(PIN_COI, trangThaiNhaySaiMK ? HIGH : LOW);
      digitalWrite(PIN_DEN_LED, trangThaiNhaySaiMK ? LOW : HIGH);
    }
  }

  // 3. Tự động đóng cửa sau 3 giây mở
  if (cuaDangMo && (hienTai - thoiDiemMoCua >= 3000)) {
    servoCua.write(GOC_CUA_DONG);
    cuaDangMo = false;
    capNhatManHinhChinh();
  }

  // 4. Tự động tắt đèn LED sau 5 giây không còn người
  if (denDangBat && !dangBaoDongSaiMK && (hienTai - thoiDiemBatDen >= THOI_GIAN_GIU_DEN)) {
    digitalWrite(PIN_DEN_LED, LOW);
    denDangBat = false;
  }

  // 5. Tắt thông báo tạm trên LCD sau 1.5 giây
  if (dangHienThongBaoTam && (hienTai - thoiDiemThongBaoLCD >= 1500)) {
    dangHienThongBaoTam = false;
    capNhatManHinhChinh();
  }

  // 6. Đọc cảm biến khói, mưa & siêu âm mỗi 250ms
  if (hienTai - thoiDiemDocCamBien >= 250) {
    thoiDiemDocCamBien = hienTai;
    kiemTraCamBienKhoi();
    kiemTraCamBienMua();
    kiemTraSieuAmBatDen();
  }

  // 7. Kêu còi Buzzer báo cháy ngắt quãng mỗi 250ms (ưu tiên cao nhất)
  if (dangCoKhoi && (hienTai - thoiDiemNhayCoiChay >= 250)) {
    thoiDiemNhayCoiChay = hienTai;
    trangThaiCoiChay = !trangThaiCoiChay;
    digitalWrite(PIN_COI, trangThaiCoiChay);
  }
}

// ================= 6. CÁC HÀM ĐIỀU KHIỂN CÒI BÀN PHÍM =================
void kichHoatKieuCoi(int kieu) {
  if (dangCoKhoi || dangBaoDongSaiMK) return;
  kieuKeuCoi = kieu;
  buocKeuCoi = 0;
  thoiDiemCoi = millis();
  digitalWrite(PIN_COI, HIGH);
}

void xuLyCoiBanPhim(unsigned long hienTai) {
  if (dangCoKhoi || dangBaoDongSaiMK || kieuKeuCoi == 0) return;

  unsigned long daTroiQua = hienTai - thoiDiemCoi;

  if (kieuKeuCoi == 1) {
    // Kiểu 1: Bấm từng phím -> Kêu 1 tiếng ngắn 50ms
    if (daTroiQua >= 50) {
      digitalWrite(PIN_COI, LOW);
      kieuKeuCoi = 0;
    }
  } 
  else if (kieuKeuCoi == 2) {
    // Kiểu 2: Mật khẩu ĐÚNG / Đổi MK thành công -> Kêu 2 tiếng "bíp - bíp"
    if (buocKeuCoi == 0 && daTroiQua >= 100) {
      digitalWrite(PIN_COI, LOW);
      buocKeuCoi = 1;
      thoiDiemCoi = hienTai;
    } else if (buocKeuCoi == 1 && daTroiQua >= 80) {
      digitalWrite(PIN_COI, HIGH);
      buocKeuCoi = 2;
      thoiDiemCoi = hienTai;
    } else if (buocKeuCoi == 2 && daTroiQua >= 120) {
      digitalWrite(PIN_COI, LOW);
      kieuKeuCoi = 0;
    }
  } 
  else if (kieuKeuCoi == 3) {
    // Kiểu 3: Mật khẩu SAI -> Kêu 1 tiếng kéo dài 600ms "bííííp"
    if (daTroiQua >= 600) {
      digitalWrite(PIN_COI, LOW);
      kieuKeuCoi = 0;
    }
  }
}

// ================= 7. XỬ LÝ BÀN PHÍM, SAI 5 LẦN & ĐỔI MẬT KHẨU =================
void hienThongBaoNgan(String dong2) {
  dangHienThongBaoTam = true;
  thoiDiemThongBaoLCD = millis();
  lcd.setCursor(0, 1);
  lcd.print(dong2);
}

void hienThiSaoDangNhap() {
  lcd.setCursor(0, 1);
  lcd.print("MK: ");
  for (unsigned int i = 0; i < matKhauNhap.length(); i++) {
    lcd.print("*");
  }
  for (unsigned int i = matKhauNhap.length(); i < 12; i++) {
    lcd.print(" ");
  }
}

void xuLyBanPhim() {
  // Đang trong 10 giây báo động sai 5 lần -> Tạm khóa bàn phím
  if (dangBaoDongSaiMK) return;

  char kyTu = banPhim.getKey();
  if (!kyTu) return;

  // --- Bấm phím 'A' ở chế độ bình thường để bắt đầu ĐỔI MẬT KHẨU ---
  if (kyTu == 'A' && cheDoDoiMK == 0) {
    kichHoatKieuCoi(1);
    cheDoDoiMK = 1;
    matKhauNhap = "";
    matKhauMoiTam = "";
    dangHienThongBaoTam = false;
    capNhatManHinhChinh();
    return;
  }

  // --- Bấm phím '*' để XÓA số đang nhập hoặc HỦY chế độ đổi mật khẩu ---
  if (kyTu == '*') {
    kichHoatKieuCoi(1);
    if (matKhauNhap.length() > 0) {
      matKhauNhap = "";
      if (cheDoDoiMK > 0) {
        hienThiSaoDangNhap();
      } else {
        capNhatManHinhChinh();
      }
    } else if (cheDoDoiMK > 0) {
      cheDoDoiMK = 0;
      matKhauMoiTam = "";
      capNhatManHinhChinh();
    }
    return;
  }

  // --- Bấm phím '#' để XÁC NHẬN theo từng chế độ ---
  if (kyTu == '#') {
    // 1. CHẾ ĐỘ MỞ CỬA BÌNH THƯỜNG
    if (cheDoDoiMK == 0) {
      if (matKhauNhap == matKhauDung) {
        soLanSaiMK = 0;
        kichHoatKieuCoi(2);
        servoCua.write(GOC_CUA_MO);
        cuaDangMo = true;
        dangHienThongBaoTam = false;
        thoiDiemMoCua = millis();

        lcd.setCursor(0, 1);
        lcd.print("DUNG -> MO CUA! ");
      } else {
        soLanSaiMK++;
        if (soLanSaiMK >= GIOI_HAN_SAI_MK) {
          // Sai đủ 5 lần -> Kích hoạt báo động ngay lập tức (Pha 1: Còi bật, Đèn tắt)
          dangBaoDongSaiMK = true;
          thoiDiemBaoDongSai = millis();
          thoiDiemNhaySaiMK = millis();
          trangThaiNhaySaiMK = true;
          kieuKeuCoi = 0;
          dangHienThongBaoTam = false;
          digitalWrite(PIN_COI, HIGH);
          digitalWrite(PIN_DEN_LED, LOW);
          capNhatManHinhChinh();
        } else {
          kichHoatKieuCoi(3);
          hienThongBaoNgan("SAI MAT KHAU!   ");
        }
      }
      matKhauNhap = "";
    }
    // 2. ĐỔI MK - BƯỚC 1: KIỂM TRA MẬT KHẨU CŨ
    else if (cheDoDoiMK == 1) {
      if (matKhauNhap == matKhauDung) {
        kichHoatKieuCoi(1);
        cheDoDoiMK = 2;
        matKhauNhap = "";
        capNhatManHinhChinh();
      } else {
        kichHoatKieuCoi(3);
        cheDoDoiMK = 0;
        matKhauNhap = "";
        capNhatManHinhChinh();
        hienThongBaoNgan("SAI MK CU!      ");
      }
    }
    // 3. ĐỔI MK - BƯỚC 2: NHẬP MẬT KHẨU MỚI
    else if (cheDoDoiMK == 2) {
      if (matKhauNhap.length() == 0) {
        kichHoatKieuCoi(3);
        hienThongBaoNgan("MK KHONG HOP LE!");
      } else {
        kichHoatKieuCoi(1);
        matKhauMoiTam = matKhauNhap;
        matKhauNhap = "";
        cheDoDoiMK = 3;
        capNhatManHinhChinh();
      }
    }
    // 4. ĐỔI MK - BƯỚC 3: XÁC NHẬN LẠI MẬT KHẨU MỚI
    else if (cheDoDoiMK == 3) {
      if (matKhauNhap == matKhauMoiTam && matKhauNhap.length() > 0) {
        matKhauDung = matKhauMoiTam;
        soLanSaiMK = 0;
        kichHoatKieuCoi(2);
        cheDoDoiMK = 0;
        matKhauNhap = "";
        matKhauMoiTam = "";
        capNhatManHinhChinh();
        hienThongBaoNgan("DOI MK THANH CONG");
      } else {
        kichHoatKieuCoi(3);
        cheDoDoiMK = 0;
        matKhauNhap = "";
        matKhauMoiTam = "";
        capNhatManHinhChinh();
        hienThongBaoNgan("MK KHONG KHOP!  ");
      }
    }
    return;
  }

  // --- Các phím số/ký tự còn lại (tối đa 6 ký tự) ---
  if (kyTu != 'A' && matKhauNhap.length() < 6) {
    kichHoatKieuCoi(1);
    dangHienThongBaoTam = false;
    matKhauNhap += kyTu;
    hienThiSaoDangNhap();
  }
}

// ================= 8. CÁC HÀM CẢM BIẾN & HIỂN THỊ LCD =================
void kiemTraSieuAmBatDen() {
  if (dangBaoDongSaiMK) return;

  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long thoiGian = pulseIn(PIN_ECHO, HIGH, 15000);
  if (thoiGian == 0) return;

  int khoangCach = thoiGian * 0.034 / 2;

  if (khoangCach > 0 && khoangCach < KHOANG_CACH_BAT_DEN) {
    thoiDiemBatDen = millis();
    if (!denDangBat) {
      denDangBat = true;
      digitalWrite(PIN_DEN_LED, HIGH);
    }
  }
}

void kiemTraCamBienKhoi() {
  int giaTriKhoi = analogRead(PIN_CAM_BIEN_KHOI);
  bool phatHienKhoiMoi = (giaTriKhoi > NGUONG_KHOI);

  if (phatHienKhoiMoi != dangCoKhoi) {
    dangCoKhoi = phatHienKhoiMoi;
    if (dangCoKhoi) {
      kieuKeuCoi = 0;
      digitalWrite(PIN_RELAY_QUAT, RELAY_BAT);
    } else {
      digitalWrite(PIN_RELAY_QUAT, RELAY_TAT);
      digitalWrite(PIN_COI, LOW);
      trangThaiCoiChay = false;
    }
    capNhatManHinhChinh();
  }
}

void kiemTraCamBienMua() {
  int giaTriMua = analogRead(PIN_CAM_BIEN_MUA);

  if (!dangCoMua && giaTriMua < NGUONG_CO_MUA) {
    dangCoMua = true;
    servoPhoi.write(GOC_THU_VAO);
  } else if (dangCoMua && giaTriMua > NGUONG_HET_MUA) {
    dangCoMua = false;
    servoPhoi.write(GOC_PHOI_RA);
  }
}

void capNhatManHinhChinh() {
  lcd.setCursor(0, 0);
  if (cheDoDoiMK == 1) {
    lcd.print("NHAP MK CU:     ");
  } else if (cheDoDoiMK == 2) {
    lcd.print("NHAP MK MOI:    ");
  } else if (cheDoDoiMK == 3) {
    lcd.print("XAC NHAN MK MOI:");
  } else {
    lcd.print("   SMART HOME   ");
  }

  lcd.setCursor(0, 1);
  if (dangCoKhoi) {
    lcd.print("CANH BAO KHOI!  ");
  } else if (dangBaoDongSaiMK) {
    lcd.print("CANH BAO! SAI 5L");
  } else if (cuaDangMo) {
    lcd.print("CUA DANG MO...  ");
  } else if (cheDoDoiMK > 0) {
    hienThiSaoDangNhap();
  } else if (matKhauNhap.length() == 0 && !dangHienThongBaoTam) {
    lcd.print("                ");
  }
}