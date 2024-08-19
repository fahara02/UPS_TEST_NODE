#ifndef UPS_TEST_H
#define UPS_TEST_H

#include "Arduino.h"
#include "HardwareConfig.h"
#include "TestManager.h"
#include "UPSTesterSetup.h"

extern void IRAM_ATTR keyISR1(void* pvParameters);
extern void IRAM_ATTR keyISR2(void* pvParameters);
extern volatile bool mains_triggered;
extern volatile bool ups_triggered;
extern UPSTesterSetup* TesterSetup;
using namespace Node_Core;

enum TestResult { TEST_FAILED = 0, TEST_SUCESSFUL = 1 };
enum LoadPercentage {
  LOAD_0P = 0,
  LOAD_25P = 25,
  LOAD_50P = 50,
  LOAD_75P = 75,
  LOAD_100P = 100
};

enum class TestType {
  SwitchTest,
  BackupTimeTest,
  EfficiencyTest,
  InputVoltageTest,
  WaveformTest,
  TunePWMTest,
};
static const char* testTypeToString(TestType type) {
  switch (type) {
    case TestType::SwitchTest:
      return "SwitchTest";
    case TestType::BackupTimeTest:
      return "BackupTimeTest";
    case TestType::EfficiencyTest:
      return "EfficiencyTest";
    case TestType::InputVoltageTest:
      return "InputVoltageTest";
    case TestType::WaveformTest:
      return "WaveformTest";
    case TestType::TunePWMTest:
      return "TunePWMTest";
    default:
      return "UnknownTest";
  }
}

template <typename T, typename U, TestType testype>
class UPSTest {
public:
  static constexpr TestType test_type = testype;
  static T* getInstance() {
    if (!instance) {
      instance = new T();
    }
    return instance;
  }

  static void deleteInstance() {
    if (instance) {
      delete instance;
      instance = nullptr;
    }
  }

  virtual void init() = 0;

  U& data() { return _data; }
  void updateSettings() {
    if (TesterSetup) {
      _cfgSpec = TesterSetup->specSetup();
      _cfgTest = TesterSetup->testSetup();
      _cfgTask = TesterSetup->taskSetup();
      _cfgTaskParam = TesterSetup->paramSetup();
      _cfgHardware = TesterSetup->hardwareSetup();
    };
  }
  static void (*taskFunctionPointerToFunction(void (T::*taskFunction)(void*)))(
      void*) {
    // The return type is a function pointer: void(*)(void*)
    return [taskFunction](void* param) {
      T* obj = static_cast<T*>(param);
      (obj->*taskFunction)(param);
    };
  }
  virtual TestResult run(uint16_t testVARating = 4000,
                         unsigned long testDuration = 10000)
      = 0;
  TaskHandle_t createTask();
  void startTest();
  void stopTest();

protected:
  UPSTest();
  virtual ~UPSTest() = default;

  void setupPins();
  void configureInterrupts();
  void setTestDuration(unsigned long duration);
  void setLoad(uint16_t testVARating);
  void selectLoadBank(uint16_t bankNumbers);
  void simulatePowerCut();
  void simulatePowerRestore();
  void sendEndSignal();
  void processTest(T& test);

  TaskHandle_t createTask(void (T::*taskFunction)(void*), const char* taskName);
  virtual void createISRTasks() = 0;

  virtual void MainTestTask(void* pvParameters) = 0;
  virtual void onMainsPowerISRTask(void* pvParameters) = 0;
  virtual void onUPSPowerISRTask(void* pvParameters) = 0;

  virtual void startTestCapture() = 0;
  virtual void stopTestCapture() = 0;
  virtual void checkRange() = 0;
  virtual void processTestImpl(T& test) = 0;

private:
  friend class TestManager;
  TaskHandle_t getTaskhandle();
  static T* instance;
  U _data;
  SetupSpec _cfgSpec;
  SetupTest _cfgTest;
  SetupTask _cfgTask;
  SetupTaskParams _cfgTaskParam;
  SetupHardware _cfgHardware;

  bool _initialized;
  bool _testRunning;
  bool _dataCaptureRunning;
  bool _dataCaptureOk;
  uint8_t _currentTest;
  unsigned long _testDuration;

  // Prevent copying
  UPSTest(const UPSTest&) = delete;
  UPSTest& operator=(const UPSTest&) = delete;
};

template <typename T, typename U, TestType testype>
T* UPSTest<T, U, testype>::instance = nullptr;

#endif  // UPS_TEST_H
