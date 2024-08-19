#include "HardwareConfig.h"
#include "SwitchTest.h"
#include "driver/gpio.h"


// Initialize static members
SwitchTest* SwitchTest::instance = nullptr;

extern SemaphoreHandle_t interruptSemaphore1;  // Create semaphore handle
extern SemaphoreHandle_t interruptSemaphore2;
static portMUX_TYPE powerSpinlock1 = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE powerSpinlock2 = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR keyISR1() {
  BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;
  portENTER_CRITICAL_ISR(&powerSpinlock1);
  xSemaphoreGiveFromISR(interruptSemaphore1, &xHigherPriorityTaskWoken1);
  portEXIT_CRITICAL_ISR(&powerSpinlock1);
  if (xHigherPriorityTaskWoken1)
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken1);
}
void IRAM_ATTR keyISR2() {
  BaseType_t xHigherPriorityTaskWoken2 = pdFALSE;
  portENTER_CRITICAL_ISR(&powerSpinlock2);
  xSemaphoreGiveFromISR(interruptSemaphore2, NULL);
  portEXIT_CRITICAL_ISR(&powerSpinlock2);
  if (xHigherPriorityTaskWoken2)
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken2);
}

// Singleton accessor
SwitchTest* SwitchTest::getInstance() {
  if (instance == nullptr) {
    instance = new SwitchTest();
  }
  return instance;
}

// ISR for mains power loss
void SwitchTest::onMainsPowerLossISR(void* pvParameters) {
  Serial.println("Triggered");
  if (instance) {
    while (true) {
      if (xSemaphoreTake(interruptSemaphore1, portMAX_DELAY) == pdTRUE) {
        instance->startTimeCapture();
      }
      Serial.print("MainPowerLoss ISR High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage
      vTaskDelay(pdMS_TO_TICKS(10));  // Delay to prevent tight loop
    }
  }
  vTaskDelete(NULL);
}

// ISR for UPS power gain
void SwitchTest::onUPSPowerGainISR(void* pvParameters) {
  if (instance) {
    while (true) {
      if (xSemaphoreTake(interruptSemaphore2, portMAX_DELAY) == pdTRUE) {
        instance->stopTimeCapture();
      }
      Serial.print("UPSPowergain ISR High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage
      vTaskDelay(pdMS_TO_TICKS(10));  // Delay to prevent tight loop
    }
  }
  vTaskDelete(NULL);
}

// Private Constructor
SwitchTest::SwitchTest()
    : testRunning(false),
      time_capture_running(false),
      time_capture_ok(false),
      current_test(0),
      max_retest(3),
      testDuration(0),
      min_valid_switch_time_ms(1),
      max_valid_switch_time_ms(3000) {
  // Initialize each SwitchTest entry in the _data.switchTest array
  for (auto& test : _data.switchTest) {
    test.testNo = 0;
    test.testTimestamp = 0;
    test.valid_data = false;
    test.switchtime = 0;
    test.starttime = 0;
    test.endtime = 0;
    test.load_percentage = LoadPercentage::LOAD_0P;
  }
}
SwitchTest::~SwitchTest() {
  // Cleanup actions (if needed)
  if (instance == this) {
    instance = nullptr;  // Clear the singleton instance
  }
}

TestData& SwitchTest::data() { return _data; }
// Pin setup
void SwitchTest::setupPins() {
  // pinMode(SENSE_MAINS_POWER_PIN, INPUT);
  pinMode(SENSE_UPS_POWER_PIN, INPUT);

  pinMode(UPS_POWER_CUT_PIN, OUTPUT);  // Set power cut pin as output
  pinMode(TEST_END_INT_PIN, OUTPUT);
  pinMode(LOAD_PWM_PIN, OUTPUT);
  pinMode(LOAD25P_ON_PIN, OUTPUT);
  pinMode(LOAD50P_ON_PIN, OUTPUT);
  pinMode(LOAD75P_ON_PIN, OUTPUT);
  pinMode(LOAD_FULL_ON_PIN, OUTPUT);
  pinMode(TEST_END_INT_PIN, OUTPUT);

  ledcSetup(0, 400, 8);
  ledcWrite(0, 0);
  ledcAttachPin(LOAD_PWM_PIN, 0);
  configureInterrupts();
  Serial.println("After configuring interrupts:");

  pinMode(SENSE_MAINS_POWER_PIN, INPUT);
  pinMode(SENSE_UPS_POWER_PIN, INPUT);
}

// Interrupt configuration
void SwitchTest::configureInterrupts() {

  // Serial.println("configuring interrupts");
  // if (interruptSemaphore1 != NULL) {
  //   Serial.println("Semaphore1 available");
  //   attachInterrupt(digitalPinToInterrupt(SENSE_MAINS_POWER_PIN), keyISR1,
  //                   FALLING);
  // }
  // if (interruptSemaphore2 != NULL) {
  //   Serial.println("Semaphore2 available");
  //   attachInterrupt(digitalPinToInterrupt(SENSE_UPS_POWER_PIN), keyISR2,
  //                   RISING);
  // }
  gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);
  gpio_install_isr_service(0);
  Serial.print("configuring button\n");
  gpio_reset_pin(mainpowerPin);
  gpio_set_direction(mainpowerPin, GPIO_MODE_INPUT);
  // gpio_pullup_en(BTN_RED);
  gpio_set_intr_type(mainpowerPin, GPIO_INTR_POSEDGE);
  gpio_isr_handler_add(mainpowerPin, onMainsPowerLossISR, NULL);
  Serial.print("config complete\n");
}

// Start the test
void SwitchTest::startTimeCapture() {
  _data.switchTest[current_test].starttime = millis();
  time_capture_running = true;
  time_capture_ok = false;
}

// Stop the test
void SwitchTest::stopTimeCapture() {
  if (time_capture_running) {
    _data.switchTest[current_test].endtime = millis();
    time_capture_running = false;
    time_capture_ok = true;  // Set flag to process timing data
  }
}

// Set the valid switch time range
void SwitchTest::setValidSwitchTimeRange(unsigned long minTime,
                                         unsigned long maxTime) {
  min_valid_switch_time_ms = minTime;
  max_valid_switch_time_ms = maxTime;
}

void SwitchTest::settestDuration(unsigned long duration) {
  testDuration = duration;
}
void SwitchTest::setLoad(uint16_t testVARating, uint16_t maxVARating) {
  uint16_t pwmValue = map(testVARating, 0, maxVARating, 0, 255);
  analogWrite(LOAD_PWM_PIN, pwmValue);
  LoadPercentage loadPercentage
      = static_cast<LoadPercentage>((testVARating * 100) / maxVARating);
  _data.switchTest[current_test].load_percentage = loadPercentage;

  selectLoadBank(loadPercentage);
}

void SwitchTest::selectLoadBank(LoadPercentage loadPercentage) {
  digitalWrite(LOAD25P_ON_PIN, loadPercentage >= LOAD_25P ? HIGH : LOW);
  digitalWrite(LOAD50P_ON_PIN, loadPercentage >= LOAD_50P ? HIGH : LOW);
  digitalWrite(LOAD75P_ON_PIN, loadPercentage >= LOAD_75P ? HIGH : LOW);
  digitalWrite(LOAD_FULL_ON_PIN, loadPercentage >= LOAD_100P ? HIGH : LOW);
}

bool SwitchTest::checkTimerange(unsigned long switchtime) {
  if (switchtime >= min_valid_switch_time_ms
      && switchtime <= max_valid_switch_time_ms) {
    return true;
  }
  return false;
}

void SwitchTest::sendEndSignal() {
  digitalWrite(TEST_END_INT_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  digitalWrite(TEST_END_INT_PIN, LOW);
}

void SwitchTest::simulatePowerCut() {
  digitalWrite(UPS_POWER_CUT_PIN, HIGH);  // Simulate power cut
}

void SwitchTest::simulatePowerRestore() {
  digitalWrite(UPS_POWER_CUT_PIN, LOW);  // Simulate Mains Power In
}

void SwitchTest::process_time_capture() {
  unsigned long endtime = _data.switchTest[current_test].endtime;
  unsigned long starttime = _data.switchTest[current_test].starttime;
  unsigned long switchTime = endtime - starttime;

  if (starttime > 0 && endtime > 0) {
    if (checkTimerange(switchTime)) {
      _data.switchTest[current_test].valid_data = true;
      _data.switchTest[current_test].testNo = current_test + 1;
      _data.switchTest[current_test].testTimestamp = millis();
      _data.switchTest[current_test].switchtime = switchTime;
    }
  }
}

// Main task function

void SwitchTest::run_SwitchTest(uint16_t testVARating) {

  uint8_t retries = 0;
  setLoad(4000, 4000);
  unsigned long testStartTime = millis();
  unsigned long currentTestStartTime;
  bool testInProgress = false;

  while (millis() - testStartTime < testDuration) {
    if (!_data.switchTest[current_test].valid_data && retries < max_retest) {
      if (!testInProgress) {
        Serial.print("Starting test... ");
        Serial.println(current_test + 1);
        currentTestStartTime = millis();
        simulatePowerCut();
        testInProgress = true;
      }
      if (millis() - currentTestStartTime >= testDuration) {
        simulatePowerRestore();
        testInProgress = false;
        vTaskDelay(pdMS_TO_TICKS(100));

        if (time_capture_ok) {
          process_time_capture();
          Serial.print("Switching Time: ");
          Serial.print(_data.switchTest[current_test].switchtime);
          Serial.println(" ms");
          sendEndSignal();
          break;
        } else {
          _data.switchTest[current_test].valid_data = false;
          Serial.println("Invalid timing data, retrying...");
          retries++;
        }
      }
      time_capture_ok = false;
    } else {
      if (retries >= max_retest) {
        Serial.println("Max retries reached. Test failed.");
        break;
      } else if (_data.switchTest[current_test].valid_data) {
        Serial.println("Test completed successfully.");
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  Serial.println("Test duration elapsed.");
}
