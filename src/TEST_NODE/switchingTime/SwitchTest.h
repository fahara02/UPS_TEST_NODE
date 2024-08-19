#ifndef SWITCH_TEST_H
#define SWITCH_TEST_H

#include "Testmanager.h"
#include "UPSTest.h"

struct SwithTestData {

  struct TestData {
    uint8_t testNo;
    unsigned long testTimestamp;
    unsigned long switchtime;
    unsigned long starttime;
    unsigned long endtime;
    LoadPercentage load_percentage : 7;  // Adjusted to cover all possible
    bool valid_data : 1;
  } switchTest[5];
  struct TestSettings {
    uint8_t pwmchannelNo = 0;
    uint8_t pwmResolusion_bits = 8;
    uint16_t pwmduty_set = 0;
    uint16_t adjust_pwm_25P = 0;
    uint16_t adjust_pwm_50P = 0;
    uint16_t adjust_pwm_75P = 0;
    uint16_t adjust_pwm_100P = 0;
    uint32_t pwm_frequecy = 3000;
    unsigned long ToleranceSwitchTime_ms = 50;
    unsigned long min_valid_switch_time_ms = 0;
    unsigned long max_valid_switch_time_ms = 10000;
    unsigned long testduration_ms = 10000;
    uint8_t max_retest = 3;
  } testsettings;

  struct TaskSettings {
    BaseType_t switchtaskCore = 0;
    BaseType_t mainsISRtaskCore = ARDUINO_RUNNING_CORE;
    BaseType_t upsISRtaskCore = ARDUINO_RUNNING_CORE;

    UBaseType_t switchtaskPr = 2;
    UBaseType_t mainsISRtaskPr = 1;
    UBaseType_t upsISRtaskPr = 1;
    uint32_t switchtaskStack = 12000;
    uint32_t mainsISRtaskStack = 12000;
    uint32_t upsISRtaskStack = 12000;
  } tasksettings;
};
class SwitchTest {
public:
  static SwitchTest* getInstance();  // Singleton accessor
  static void deleteInstance();
  void init();
  SwithTestData& data();
  TestResult run(uint16_t testVARating = 4000,
                 unsigned long testduration = 10000);

private:
  friend class TestManager;
  SwitchTest();  // Private Constructor
  ~SwitchTest();
  static SwitchTest* instance;  // Static pointer to hold the instance

  SwithTestData _data;
  SwithTestData::TestSettings _config;
  SwithTestData::TaskSettings _tasksetting;
  SwitchTestTaskParams _taskparams;
  bool _initialized = false;
  bool _testRunning;
  bool _time_capture_running;
  bool _time_capture_ok;
  uint8_t _currentTest;
  unsigned long _testDuration;

  void setupPins();
  void configureInterrupts();
  void createMainTasks();
  void createISRTasks();

  static void switchTestTask(void* pvParameters);
  static void onMainsPowerLossTask(void* pvParameters);
  static void onUPSPowerGainTask(void* pvParameters);

  void simulatePowerCut();
  void simulatePowerRestore();
  void sendEndSignal();
  void settestDuration(unsigned long duration);
  void setLoad(uint16_t testVARating);
  void selectLoadBank(uint16_t bankNumbers);

  void startTimeCapture();
  void stopTimeCapture();
  bool process_time_capture();
  bool checkTimerange(unsigned long switchtime);
};

#endif  // SWITCH_TEST_H
