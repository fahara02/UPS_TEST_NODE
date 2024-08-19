#include "UPSTest.h"
extern UPSTesterSetup* TesterSetup;
// Private Constructor
template <typename T, typename U, TestType testype>
UPSTest<T, U, testype>::UPSTest()
    : _data(),
      _cfgSpec(TesterSetup->specSetup()),
      _cfgTest(TesterSetup->testSetup()),
      _cfgTask(TesterSetup->taskSetup()),
      _cfgTaskParam(TesterSetup->paramSetup()),
      _cfgHardware(TesterSetup->hardwareSetup()),
      _config(_data.testsettings),
      _taskSetting(_data.tasksettings),
      _initialized(false),
      _testRunning(false),
      _dataCaptureRunning(false),
      _dataCaptureOk(false),
      _currentTest(0),
      _testDuration(_config.testduration_ms) {}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::setupPins() {
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

  ledcSetup(_cfgHardware.pwmchannelNo, _cfgHardware.pwm_frequency,
            _cfgHardware.pwmResolusion_bits);
  ledcWrite(_cfgHardware.pwmchannelNo, 0);
  ledcAttachPin(LOAD_PWM_PIN, 0);
  configureInterrupts();
  Serial.println("After configuring interrupts:");
  pinMode(SENSE_MAINS_POWER_PIN, INPUT_PULLDOWN);
  pinMode(SENSE_UPS_POWER_PIN, INPUT_PULLDOWN);
}

// Interrupt configuration
template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::configureInterrupts() {
  gpio_num_t mainpowerPin = static_cast<gpio_num_t>(SENSE_MAINS_POWER_PIN);
  gpio_num_t upspowerPin = static_cast<gpio_num_t>(SENSE_UPS_POWER_PIN);
  gpio_install_isr_service(0);
  switch (testype) {
    case TestType::SwitchTest:
      gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);
      gpio_set_intr_type(upspowerPin, GPIO_INTR_POSEDGE);
      break;

    case TestType::BackupTimeTest:
      gpio_set_intr_type(mainpowerPin, GPIO_INTR_NEGEDGE);
      gpio_set_intr_type(upspowerPin, GPIO_INTR_NEGEDGE);
      break;

      // Add other cases as needed
  }

  gpio_isr_handler_add(mainpowerPin, keyISR1, NULL);
  gpio_isr_handler_add(upspowerPin, keyISR2, NULL);

  Serial.print("Interrupt configuration complete\n");
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::setTestDuration(unsigned long duration) {
  _testDuration = duration;
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::setLoad(uint16_t testVARating) {
  const uint16_t maxVARating
      = _cfgSpec.Rating_va;  // Assuming maxVA rating is in TestSettings
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
    adjustpwm = _cfgHardware.adjust_pwm_25P;
  } else if (testVARating > singlebankVA && testVARating <= dualbankVA) {
    reqbankNumbers = 2;
    duty = (testVARating * 100) / dualbankVA;
    pwmValue = map(testVARating, 0, dualbankVA, 0, 255);
    adjustpwm = _cfgHardware.adjust_pwm_50P;
  } else if (testVARating > dualbankVA && testVARating <= triplebankVA) {
    reqbankNumbers = 3;
    duty = (testVARating * 100) / triplebankVA;
    pwmValue = map(testVARating, 0, triplebankVA, 0, 255);
    adjustpwm = _cfgHardware.adjust_pwm_75P;
  } else if (testVARating > triplebankVA && testVARating <= maxVARating) {
    reqbankNumbers = 4;
    duty = (testVARating * 100) / maxVARating;
    pwmValue = map(testVARating, 0, maxVARating, 0, 255);
    adjustpwm = _cfgHardware.adjust_pwm_100P;
  }

  uint16_t set_pwmValue = pwmValue + adjustpwm;
  analogWrite(LOAD_PWM_PIN, set_pwmValue);
  // LoadPercentage loadPercentage = static_cast<LoadPercentage>(duty);
  // _data.switchTest[_currentTest].load_percentage = loadPercentage;

  selectLoadBank(reqbankNumbers);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::selectLoadBank(uint16_t bankNumbers) {
  digitalWrite(LOAD25P_ON_PIN, bankNumbers >= 1 ? HIGH : LOW);
  digitalWrite(LOAD50P_ON_PIN, bankNumbers >= 2 ? HIGH : LOW);
  digitalWrite(LOAD75P_ON_PIN, bankNumbers >= 3 ? HIGH : LOW);
  digitalWrite(LOAD_FULL_ON_PIN, bankNumbers >= 4 ? HIGH : LOW);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::sendEndSignal() {
  digitalWrite(TEST_END_INT_PIN, HIGH);
  vTaskDelay(pdMS_TO_TICKS(10));
  digitalWrite(TEST_END_INT_PIN, LOW);
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::simulatePowerCut() {
  digitalWrite(UPS_POWER_CUT_PIN, HIGH);  // Simulate power cut
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::simulatePowerRestore() {
  digitalWrite(UPS_POWER_CUT_PIN, LOW);  // Simulate Mains Power In
}

template <typename T, typename U, TestType testype>
void UPSTest<T, U, testype>::processTest(T& test) {
  // Placeholder for processing the test; type-specific implementations should
  // override processTestImpl
  processTestImpl(test);
}
