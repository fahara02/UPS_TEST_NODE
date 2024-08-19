#include "SwitchTest.h"
#include "driver/gpio.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern volatile bool mains_triggered;
extern volatile bool ups_triggered;
extern TaskHandle_t ISR1;
extern TaskHandle_t ISR2;

// Initialize static members
SwitchTest* SwitchTest::instance = nullptr;

// Private Constructor
SwitchTest::SwitchTest()
    : _testRunning(false),
      _time_capture_running(false),
      _time_capture_ok(false),
      _currentTest(0),
      _testDuration(_config.testduration_ms) {
  _taskparams.init();
}

SwitchTest::~SwitchTest() {
  // Cleanup actions (if needed)
  if (instance == this) {

    if (switchTestTaskHandle != NULL) {
      vTaskDelete(switchTestTaskHandle);
      switchTestTaskHandle = NULL;
    }
    // Delete the ISR tasks
    if (ISR1 != NULL) {
      vTaskDelete(ISR1);
      ISR1 = NULL;
    }
    if (ISR2 != NULL) {
      vTaskDelete(ISR2);
      ISR2 = NULL;
    }

    // Clear the singleton instance
    instance = nullptr;
  }
}

// Singleton accessor
SwitchTest* SwitchTest::getInstance() {
  if (instance == nullptr) {
    instance = new SwitchTest();
  }
  return instance;
}

void SwitchTest::deleteInstance() {
  if (instance != nullptr) {
    delete instance;
    instance = nullptr;
  }
}

void SwitchTest::init() {

  if (_initialized) {
    return;  // Already initialized, do nothing
  }

  // Perform initialization
  for (auto& test : _data.switchTest) {
    test.testNo = 0;
    test.testTimestamp = 0;
    test.valid_data = false;
    test.switchtime = 0;
    test.starttime = 0;
    test.endtime = 0;
    test.load_percentage = LoadPercentage::LOAD_0P;
  };
  setupPins();
  createMainTasks();
  createISRTasks();

  _initialized = true;  // Mark as initialized
}
void SwitchTest::createMainTasks() {

  xTaskCreatePinnedToCore(instance->switchTestTask, "SwitchTestTask",
                          instance->_tasksetting.switchtaskStack, &_taskparams,
                          instance->_tasksetting.switchtaskPr,
                          &switchTestTaskHandle,
                          instance->_tasksetting.switchtaskCore);
}

void SwitchTest::createISRTasks() {
  xTaskCreatePinnedToCore(instance->onMainsPowerLossTask, "MainslossTask",
                          instance->_tasksetting.mainsISRtaskStack, NULL,
                          instance->_tasksetting.mainsISRtaskPr,
                          &ISR_MAINS_POWER_LOSS,
                          instance->_tasksetting.mainsISRtaskCore);
  xTaskCreatePinnedToCore(instance->onUPSPowerGainTask, "UPSgainTask",
                          instance->_tasksetting.upsISRtaskStack, NULL,
                          instance->_tasksetting.upsISRtaskPr,
                          &ISR_UPS_POWER_GAIN,
                          instance->_tasksetting.upsISRtaskCore);
}

// Function for SwitchTest task
void SwitchTest::switchTestTask(void* pvParameters) {
  if (instance) {
    while (true) {
      SwitchTestTaskParams* params = (SwitchTestTaskParams*)pvParameters;
      Serial.print("Switchtask VA rating is: ");
      Serial.println(params->testVARating);
      Serial.print("Switchtask duration is: ");
      Serial.println(params->testDuration);

      instance->run(params->testVARating, params->testDuration);
      Serial.print("Switch Stack High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage
      vTaskDelay(pdMS_TO_TICKS(100));  // Delay to prevent tight loop
    }
  }
  vTaskDelete(NULL);
}

// ISR for mains power loss
void SwitchTest::onMainsPowerLossTask(void* pvParameters) {
  while (true) {

    if (mains_triggered) {
      if (instance) {
        instance->_time_capture_running = true;
        instance->startTimeCapture();
        Serial.print("\033[31m");  // Start red color
        Serial.print("mains Powerloss triggered...");
        Serial.print("\033[0m");  // Reset color
        mains_triggered = false;
      }
      Serial.print("mains task High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage

      vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
      vTaskSuspend(NULL);
    }
  }
  vTaskDelete(NULL);
}

// ISR for UPS power gain
void SwitchTest::onUPSPowerGainTask(void* pvParameters) {

  while (true) {
    if (ups_triggered) {

      if (instance) {
        instance->stopTimeCapture();
        Serial.print("\033[31m");  // Start red color
        Serial.print("UPS Powerloss triggered...");
        Serial.print("\033[0m");  // Reset color
        ups_triggered = false;
      }
      Serial.print("UPS High Water Mark: ");
      Serial.println(uxTaskGetStackHighWaterMark(NULL));  // Monitor stack usage

      vTaskDelay(pdMS_TO_TICKS(100));  // Task delay
      vTaskSuspend(NULL);
    }
  }
  vTaskDelete(NULL);
}

SwithTestData& SwitchTest::data() { return _data; }
// Pin setup
void SwitchTest::setupPins() {

  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(UPS_POWER_CUT_PIN, OUTPUT);  // Set power cut pin as output
  pinMode(TEST_END_INT_PIN, OUTPUT);
  pinMode(LOAD_PWM_PIN, OUTPUT);
  pinMode(LOAD25P_ON_PIN, OUTPUT);
  pinMode(LOAD50P_ON_PIN, OUTPUT);
  pinMode(LOAD75P_ON_PIN, OUTPUT);
  pinMode(LOAD_FULL_ON_PIN, OUTPUT);
  pinMode(TEST_END_INT_PIN, OUTPUT);

  ledcSetup(_config.pwmchannelNo, _config.pwm_frequecy,
            _config.pwmResolusion_bits);
  ledcWrite(_config.pwmchannelNo, 0);
  ledcAttachPin(LOAD_PWM_PIN, 0);
  configureInterrupts();
  Serial.println("After configuring interrupts:");

  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
}

// Interrupt configuration
void SwitchTest::configureInterrupts() {

  gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);

  Serial.print("configuring button\n");
  gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);

  gpio_num_t upspowerPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_PIN);
  gpio_set_intr_type(upspowerPin, GPIO_INTR_POSEDGE);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
  gpio_isr_handler_add(upspowerPin, keyISR2, NULL);

  Serial.print("config complete\n");
}

// Start the test
void SwitchTest::startTimeCapture() {
  if (_time_capture_running) {
    _data.switchTest[_currentTest].starttime = millis();
    _time_capture_ok = false;
    Serial.println("time capture started...");
    Serial.print("start time:");
    Serial.println(_data.switchTest[_currentTest].starttime);
  }

  else {
    Serial.println("Debounce...");
  }
}

// Stop the test
void SwitchTest::stopTimeCapture() {
  if (_time_capture_running) {
    _data.switchTest[_currentTest].endtime = millis();
    Serial.println("time capture done...");
    Serial.print("stop time:");
    Serial.println(_data.switchTest[_currentTest].endtime);
    Serial.print("switch time:");
    Serial.println(_data.switchTest[_currentTest].endtime
                   - _data.switchTest[_currentTest].starttime);
    _time_capture_running = false;
    _time_capture_ok = true;  // Set flag to process timing data

    if (_time_capture_ok) {
      Serial.println("time capture ok recorded");
    }
  }
}

void SwitchTest::settestDuration(unsigned long duration) {
  _testDuration = duration;
}
void SwitchTest::setLoad(uint16_t testVARating) {
  uint16_t singlebankVA = maxVARating / 4;
  uint16_t dualbankVA = (maxVARating / 4) * 2;
  uint16_t triplebankVA = (maxVARating / 4) * 3;
  uint16_t reqbankNumbers = 0;
  uint16_t duty = 0;
  uint16_t pwmValue = 0;
  uint16_t adjustpwm = 0;

  if (testVARating <= singlebankVA) {
    reqbankNumbers = 1;
    duty = (testVARating * 100) / singlebankVA;
    pwmValue = map(testVARating, 0, singlebankVA, 0, 255);
    adjustpwm = _config.adjust_pwm_25P;
  }
  if (testVARating > singlebankVA && testVARating <= dualbankVA) {
    reqbankNumbers = 2;
    duty = (testVARating * 100) / dualbankVA;
    pwmValue = map(testVARating, 0, dualbankVA, 0, 255);
    adjustpwm = _config.adjust_pwm_50P;
  }
  if (testVARating > dualbankVA && testVARating <= triplebankVA) {
    reqbankNumbers = 3;
    duty = (testVARating * 100) / triplebankVA;
    pwmValue = map(testVARating, 0, triplebankVA, 0, 255);
    adjustpwm = _config.adjust_pwm_75P;
  }
  if (testVARating > triplebankVA && testVARating <= maxVARating) {
    reqbankNumbers = 4;
    duty = (testVARating * 100) / maxVARating;
    pwmValue = map(testVARating, 0, maxVARating, 0, 255);
    adjustpwm = _config.adjust_pwm_100P;
  }
  _config.pwmduty_set = pwmValue;
  uint16_t set_pwmValue = pwmValue + adjustpwm;
  analogWrite(LOAD_PWM_PIN, set_pwmValue);
  LoadPercentage loadPercentage = static_cast<LoadPercentage>(duty);
  _data.switchTest[_currentTest].load_percentage = loadPercentage;

  selectLoadBank(reqbankNumbers);
}

void SwitchTest::selectLoadBank(uint16_t bankNumbers) {
  digitalWrite(LOAD25P_ON_PIN, bankNumbers >= 1 ? HIGH : LOW);
  digitalWrite(LOAD50P_ON_PIN, bankNumbers >= 2 ? HIGH : LOW);
  digitalWrite(LOAD75P_ON_PIN, bankNumbers >= 3 ? HIGH : LOW);
  digitalWrite(LOAD_FULL_ON_PIN, bankNumbers >= 4 ? HIGH : LOW);
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
bool SwitchTest::checkTimerange(unsigned long switchtime) {
  if (switchtime >= _config.min_valid_switch_time_ms
      && switchtime <= _config.max_valid_switch_time_ms) {
    return true;
  }
  return false;
}

bool SwitchTest::process_time_capture() {
  unsigned long endtime = _data.switchTest[_currentTest].endtime;
  unsigned long starttime = _data.switchTest[_currentTest].starttime;
  unsigned long switchTime = endtime - starttime;

  if (starttime > 0 && endtime > 0) {
    if (checkTimerange(switchTime)) {
      _data.switchTest[_currentTest].valid_data = true;
      _data.switchTest[_currentTest].testNo = _currentTest + 1;
      _data.switchTest[_currentTest].testTimestamp = millis();
      _data.switchTest[_currentTest].switchtime = switchTime;
      return true;
    }
  }
  return false;
}

TestResult SwitchTest::run(uint16_t testVARating, unsigned long testduration) {
  uint8_t retries = 0;
  setLoad(testVARating);                   // Set the load
  unsigned long testStartTime = millis();  // Overall start time
  bool valid_data = false;                 // Initially assume data is not valid
  bool testInProgress = false;
  unsigned long currentTestStartTime = 0;
  uint8_t maximum_retest_number = _config.max_retest;
  unsigned long testDurationWithRetest = 0;
  if (testduration == _testDuration) {

    testDurationWithRetest = maximum_retest_number * _testDuration;
  } else {
    _testDuration = testduration;
    testDurationWithRetest = maximum_retest_number * _testDuration;
  }

  while (millis() - testStartTime < testDurationWithRetest) {
    if (!testInProgress) {
      Serial.print("Starting test attempt ");
      Serial.println(retries + 1);
      simulatePowerCut();
      currentTestStartTime = millis();
      testInProgress = true;
    }

    unsigned long elapsedTime = millis() - currentTestStartTime;
    long remainingTime = _testDuration - elapsedTime;
    Serial.print("test duration set is:");
    Serial.println(_testDuration);

    Serial.print("Remaining single test time: ");
    Serial.println(remainingTime);

    if (elapsedTime >= _testDuration) {
      Serial.println("Ending switch test...");
      simulatePowerRestore();
      testInProgress = false;

      vTaskDelay(pdMS_TO_TICKS(100));  // Small delay before processing

      if (_time_capture_ok) {
        Serial.println("Processing time capture...");

        if (process_time_capture()) {
          Serial.print("Switching Time: ");
          Serial.print(_data.switchTest[_currentTest].switchtime);
          Serial.println(" ms");
          sendEndSignal();
          valid_data = true;
          break;  // Exit the loop as the test was successful
        }

      } else {
        _data.switchTest[_currentTest].valid_data = false;
        Serial.println("Invalid timing data, retrying...");
        retries++;

        if (retries >= maximum_retest_number) {
          Serial.println("Max retries reached. Test failed.");
          break;
        }
      }

      _time_capture_ok = false;  // Reset for the next iteration
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Delay to avoid busy-waiting
  }

  if (!valid_data) {
    Serial.println("Test duration elapsed or test failed.");
    return TEST_FAILED;
  } else {
    Serial.println("Test completed successfully.");
    return TEST_SUCESSFUL;
  }
}
