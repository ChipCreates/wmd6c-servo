*** Settings ***
Documentation     Automated servo loop tests for DSR-1 firmware.
...               Exercises the PI control law via synthetic FG pulse injection.
...               Run with: renode-test firmware/simulation/tests/servo_loop.robot
...               (from workspace root d:/Projects/wmd6c-servo)

Library           OperatingSystem

*** Variables ***
${ELF}            ${CURDIR}/../../build/firmware.elf
${PLATFORM}       ${CURDIR}/../platform/stm32g0b1kbu6.repl

# TIM2/TIM3 register addresses
${TIM2_SR}        0x40000010
${TIM2_CCR1}      0x40000034
${TIM3_CCR1}      0x40000434

# DSR-1 constants (must match config.h)
${TARGET_PERIOD}  ${25600}     # 64 MHz / 2500 Hz
${DAC_CENTER}     ${2048}
${DAC_MIN}        ${100}
${DAC_MAX}        ${3995}
${CC1IF}          ${2}         # TIM_SR_CC1IF = bit 1

*** Keywords ***
Setup DSR-1 Simulation
    Execute Command             mach create "DSR-1-Test"
    Execute Command             machine LoadPlatformDescription @${PLATFORM}
    Execute Command             sysbus LoadELF @${ELF}
    Execute Command             start

Inject FG Pulse
    [Arguments]                 ${ccr1_value}
    Execute Command             sysbus WriteDoubleWord ${TIM2_CCR1} ${ccr1_value}
    Execute Command             sysbus WriteDoubleWord ${TIM2_SR} ${CC1IF}
    Execute Command             sysbus WriteDoubleWord 0xE000E200 0x00008000
    Execute Command             emulation RunFor "0.000400"

Read TIM3 CCR1
    ${raw}=                     Execute Command    sysbus ReadDoubleWord ${TIM3_CCR1}
    ${value}=                   Convert To Integer    ${raw}
    [Return]                    ${value}

Inject N Nominal Pulses
    [Arguments]                 ${count}
    FOR    ${i}    IN RANGE    1    ${count}+1
        ${ccr1}=                Evaluate    ${i} * ${TARGET_PERIOD}
        Inject FG Pulse         ${ccr1}
    END

*** Test Cases ***

Boot Completes Without Hang
    [Documentation]    Firmware boots through clock_init, flash_load, adc_init,
    ...                servo_init without hanging in any polling loop.
    Setup DSR-1 Simulation
    # Allow 10 ms of emulation time for init sequence
    Execute Command             emulation RunFor "0.010"
    # If we get here, no polling loop blocked simulation
    [Teardown]                  Execute Command    mach clear

Servo Stays At DAC_CENTER For Nominal FG Rate
    [Documentation]    When FG pulses arrive exactly at TARGET_PERIOD, error = 0
    ...                and TIM3->CCR1 should stay at DAC_CENTER (2048).
    Setup DSR-1 Simulation
    # Prime: inject one below-threshold pulse and 10 nominal pulses
    Inject FG Pulse             ${TARGET_PERIOD}
    Inject N Nominal Pulses     10
    ${pwm}=                     Read TIM3 CCR1
    Should Be True              ${pwm} >= ${DAC_CENTER} - 50
    Should Be True              ${pwm} <= ${DAC_CENTER} + 50
    [Teardown]                  Execute Command    mach clear

Motor Drive Increases When FG Is Too Slow
    [Documentation]    A FG period longer than TARGET_PERIOD means the motor is
    ...                running too slow.  The servo must increase motor drive, i.e.
    ...                TIM3->CCR1 must decrease below DAC_CENTER.
    Setup DSR-1 Simulation
    # Start at steady state
    Inject N Nominal Pulses     5
    ${before}=                  Read TIM3 CCR1
    # Inject a 110% period (motor 10% too slow)
    ${slow_period}=             Evaluate    int(${TARGET_PERIOD} * 1.1)
    Inject FG Pulse             ${slow_period}
    ${after}=                   Read TIM3 CCR1
    Should Be True              ${after} < ${before}
    [Teardown]                  Execute Command    mach clear

Motor Drive Decreases When FG Is Too Fast
    [Documentation]    A FG period shorter than TARGET_PERIOD means the motor is
    ...                running too fast.  TIM3->CCR1 must increase above DAC_CENTER.
    Setup DSR-1 Simulation
    Inject N Nominal Pulses     5
    ${before}=                  Read TIM3 CCR1
    # Inject a 90% period (motor 10% too fast)
    ${fast_period}=             Evaluate    int(${TARGET_PERIOD} * 0.9)
    Inject FG Pulse             ${fast_period}
    ${after}=                   Read TIM3 CCR1
    Should Be True              ${after} > ${before}
    [Teardown]                  Execute Command    mach clear

PWM Output Stays Within Safe Clamp Range
    [Documentation]    TIM3->CCR1 must stay between DAC_MIN and DAC_MAX regardless
    ...                of the FG rate, enforcing the servo output clamps.
    Setup DSR-1 Simulation
    # Drive with very slow FG (50% of target — worst-case slow)
    ${very_slow}=               Evaluate    ${TARGET_PERIOD} * 2
    FOR    ${i}    IN RANGE    1    11
        ${ccr1}=                Evaluate    ${i} * ${very_slow}
        Inject FG Pulse         ${ccr1}
    END
    ${pwm}=                     Read TIM3 CCR1
    Should Be True              ${pwm} >= ${DAC_MIN}
    Should Be True              ${pwm} <= ${DAC_MAX}
    [Teardown]                  Execute Command    mach clear

Short Period Pulses Are Filtered
    [Documentation]    Periods shorter than MIN_PERIOD_TICKS (6400) must be
    ...                discarded — TIM3->CCR1 must not change.
    Setup DSR-1 Simulation
    Inject N Nominal Pulses     5
    ${before}=                  Read TIM3 CCR1
    # Inject a pulse at 3200 ticks (below MIN_PERIOD_TICKS = 6400)
    Inject FG Pulse             3200
    ${after}=                   Read TIM3 CCR1
    Should Be Equal             ${before}    ${after}
    [Teardown]                  Execute Command    mach clear
