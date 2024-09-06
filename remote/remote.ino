#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRutils.h>
#include <IRsend.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>

const uint16_t kRecvPin = 13;//định chân cho IR Receiver (KY-022)
const uint16_t kIrLed = 14;//định chân led hồng ngoại
int SEND_BUTTON_PIN = 16;//định chân nút 1
int SEND_BUTTON_PIN_2 = 4;//định chân nút 2
int MODE_BUTTON_PIN = 26;//định chân nút chuyển mode
int lcdColumns = 16;//giá trị cột lcd
int lcdRows = 2;//giá trị hàng lcd
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);//cấu hình lcd 1602
const uint16_t kCaptureBufferSize = 1024;
uint16_t *rawcode;//biến lưu dữ liệu tín hiệu 1
uint16_t len;//biến lưu độ dài tín hiệu 1
uint16_t *rawcode2;//biến lưu dữ liệu tín hiệu 2
uint16_t len2;//biến lưu độ dài tín hiệu 2
int isSendMode = 0;//biến trạng thái để chuyển chế độ
int state = 0;//biến trạng thái dùng cho chế độ 2
decode_type_t protocol;//biến protocol để lưu dữ liệu model
int i = 1;

#if DECODE_AC
const uint8_t kTimeout = 50;
#else
const uint8_t kTimeout = 15;
#endif
const uint8_t kTolerancePercentage = kTolerance;
const uint16_t kMinUnknownSize = 12;

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
IRsend irsend(kIrLed);
IRac ir(kIrLed);
decode_results results;
Preferences preferences;

void setup() {
    pinMode(SEND_BUTTON_PIN, INPUT_PULLUP);//gán chân 16 làm input pullup
    pinMode(SEND_BUTTON_PIN_2, INPUT_PULLUP);//gán chân 4 làm input pullup
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);//gán chân 26 làm input pullup
    lcd.init();//khởi tạo lcd
#if DECODE_HASH
    irrecv.setUnknownThreshold(kMinUnknownSize);
#endif
    irrecv.setTolerance(kTolerancePercentage);
    irrecv.enableIRIn();//bắt đầu hàm thu tín hiệu
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Library Mode");
    lcd.setCursor(0, 1);
    lcd.print("Ready to send");
    irsend.begin();//bắt đầu hàm gửi tín hiệu
    loadData();//load dữ liệu từ flash memory
}

void loop() {
bool tSendButtonIsActive = (digitalRead(SEND_BUTTON_PIN) == LOW);
bool tSendButton2IsActive = (digitalRead(SEND_BUTTON_PIN_2) == LOW);
bool tModeButtonIsActive = (digitalRead(MODE_BUTTON_PIN) == LOW);
 if (tModeButtonIsActive) {
   isSendMode = (isSendMode + 1) % 4; // nhấn mode button để chuyển đổi theo thứ tự 4 chế độ: 0 (lib mode), 1 (lib mode setting), 2 (receive mode), và 3 (send mode)
   if (isSendMode == 3) {//send mode
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Send Mode");
     lcd.setCursor(0, 1);
     lcd.print("Ready to send");
     irrecv.pause(); // Dừng thu tín hiệu khi vào send mode
    } else if (isSendMode == 2) {//receive mode
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Receive mode");
     lcd.setCursor(0, 1);
     lcd.print("Ready to receive");      
     irrecv.resume(); // Bắt đầu thu tín hiệu khi vào receive mode
    } else if (isSendMode == 0) {//Lib mode
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Library Mode");
     lcd.setCursor(0, 1);
     lcd.print("Ready to send");      
     irrecv.pause(); // Dừng thu tín hiệu khi vào Lib mode
    }
    else if (isSendMode == 1) {//lib mode setting
     i = 1;  
     state = 0;
     irrecv.pause(); // Dừng thu tín hiệu khi vào Lib mode setting
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Chose your model");
     lcd.setCursor(0, 1);
     lcd.print(typeToString((decode_type_t)i));
    }
  }
  if (isSendMode == 3) {
    // Send mode
   if (tSendButtonIsActive) {//nếu nhấn nút 1
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Send Button 1");
      irsend.sendRaw(rawcode, len, 38);//truyền tín hiệu 1
    } else if (tSendButton2IsActive) {//nếu nhấn nút 2
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Send Button 2");
      irsend.sendRaw(rawcode2, len2, 38);//truyền tín hiệu 2
    }
  } else if (isSendMode == 2) {
    // Receive mode
    if (irrecv.decode(&results)) {//nếu có tín hiệu hồng ngoại
      if (tSendButtonIsActive) {//nếu nhấn nút 1
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Received");
        storeCode();
        saveData();//lưu vào flash memory
        irrecv.resume();//tiếp tục thu tín hiệu
      } else if (tSendButton2IsActive) {//nếu nhấn nút 2
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Received");
        storeCode2();
        saveData();//lưu vào flash memory
        irrecv.resume();//tiếp tục thu tín hiệu
      }
    }
  } else if (isSendMode == 0 || (isSendMode == 1 && state == 1)) {
      // Lib mode
     initIR();//nạp thông số ban đầu
     ir.next.protocol = protocol;  // Set model hãng cho tín hiệu tiếp theo.
     if (tSendButtonIsActive) {//nếu nhấn nút 1
        ir.next.power = true;  // tín hiệu tiếp theo sẽ là on.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(typeToString(protocol));
        lcd.setCursor(0, 1);
        lcd.print("Turn on");
        ir.sendAc();  //gửi tín hiệu on.
      } else if (tSendButton2IsActive) {//nếu nhấn nút 2
        ir.next.power = false;  // tín hiệu tiếp theo sẽ là off.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(typeToString(protocol));
        lcd.setCursor(0, 1);
        lcd.print("Turn off");
        ir.sendAc();  // gửi tín hiệu off.
      }
  } else if (isSendMode == 1 && state == 0) {//Lib mode setting
    if (tSendButtonIsActive) {//nếu nhấn nút 1 thì hiển thị model tiếp theo trong danh sách model
     i++;
     if (i > kLastDecodeType) {
          i = 1; // Reset i về 1 nếu i đến cuối danh sách model
     }
     lcd.setCursor(0, 1);
     lcd.print("                ");
     lcd.setCursor(0, 1);
     lcd.print(typeToString((decode_type_t)i)); 
     } else if (tSendButton2IsActive) {//nếu nhấn nút 2 thì select model đó
     state = 1;
     protocol = (decode_type_t)i;
     saveData(); 
     lcd.clear();
     lcd.setCursor(0, 0);
     lcd.print("Selected");
     lcd.setCursor(0, 1);
     lcd.print(typeToString((decode_type_t)i));
     } 
  }
  delay(100);    
}

void storeCode() {
    // lưu dữ liệu và độ dài tín hiệu 1 đã giải mã
        rawcode = resultToRawArray(&results);
        len = getCorrectedRawLength(&results);
}

void storeCode2() {
    // lưu dữ liệu và độ dài tín hiệu 2 đã giải mã
        rawcode2 = resultToRawArray(&results);
        len2 = getCorrectedRawLength(&results);
}

void saveData() { //Hàm lưu dữ liệu vào flash memory
  preferences.begin("ir-codes", false);
  preferences.putBytes("rawcode", (uint8_t*)rawcode, len * sizeof(uint16_t));
  preferences.putUInt("len", len);
  preferences.putBytes("rawcode2", (uint8_t*)rawcode2, len2 * sizeof(uint16_t));
  preferences.putUInt("len2", len2);
  preferences.putUInt("protocol", (uint16_t)protocol);
  preferences.end();
}

void loadData() { //Hàm load dữ liệu từ flash memory
  preferences.begin("ir-codes", true);
  size_t size = preferences.getBytesLength("rawcode");
  if (size > 0) {
    rawcode = (uint16_t*)malloc(size);
    preferences.getBytes("rawcode", (uint8_t*)rawcode, size);
    len = preferences.getUInt("len", 0);
  }
  size = preferences.getBytesLength("rawcode2");
  if (size > 0) {
    rawcode2 = (uint16_t*)malloc(size);
    preferences.getBytes("rawcode2", (uint8_t*)rawcode2, size);
    len2 = preferences.getUInt("len2", 0);
  }
  protocol = (decode_type_t)preferences.getUInt("protocol", 0);
  preferences.end();
}

void initIR() {//Hàm khởi tạo giá trị ban đầu cho Lib mode và Lib setting mode
  ir.next.model = 1;
  ir.next.mode = stdAc::opmode_t::kCool;  // Chạy ở chế độ làm lạnh
  ir.next.celsius = true;  // Sử dụng độ C. False = Fahrenheit
  ir.next.degrees = 25;  // 25 độ C.
  ir.next.fanspeed = stdAc::fanspeed_t::kHigh;  // Tốc độ quạt cao.
  ir.next.swingv = stdAc::swingv_t::kOff;  // Không xoay quạt lên xuống.
  ir.next.swingh = stdAc::swingh_t::kOff;  // Không xoay quạt trái phải.
  ir.next.light = false;  // Tắt bất kỳ LED/Lights/Display.
  ir.next.beep = false;  // Tắt bất kỳ beep từ máy lạnh.
  ir.next.econo = false;  // Tắt bất kỳ chế độ economy.
  ir.next.filter = false;  // Tắt bất kỳ bộ lọc Ion/Mold/Health.
  ir.next.turbo = false;  // Không dùng chế độ turbo/powerful/etc.
  ir.next.quiet = false;  // Không dùng chế độ quiet/silent/etc.
  ir.next.sleep = -1;  // Không đặt sleep time.
  ir.next.clean = false;  // Tắt bất kỳ cài đặt Cleaning.
  ir.next.clock = -1;  // Không đặt thời gian hiện tại.
  ir.next.power = false;  // Khởi tạo với tín hiệu off.
}