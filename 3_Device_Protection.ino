void deviceProtection() {

  // Battery overvoltage
  if (voltageOutput > 15.0) {
    pwmDuty = 0;
  }

  // Overcurrent
  if (currentOutput > activeBat.maxCurrent) {
    pwmDuty -= 1.0;
  }

  // PV dropout
  if (voltageInput < voltageOutput + 1.0) {
    pwmDuty = 0;
  }

  if (pwmDuty < 0) pwmDuty = 0;
}
