// Força a inclusão dos arquivos .cpp
#include "../../src/States/InitState.cpp"
#include "../../src/States/ConnectState.cpp"
#include "../../src/States/RunState.cpp"

#include <Arduino.h>
#include <unity.h>
#include "States/State.h"
#include "States/InitState.h"
#include "States/ConnectState.h"
#include "States/RunState.h"

void test_init_to_connect() {
    InitState* init = new InitState();
    init->enter();
    State* next = init->update();
    TEST_ASSERT_NOT_NULL(next);
    TEST_ASSERT_EQUAL_STRING("ConnectState", next->getName());
    init->exit();
    delete init;
    delete next;
}

void test_connect_to_run() {
    ConnectState* connect = new ConnectState();
    connect->enter();
    State* next = connect->update();
    TEST_ASSERT_NOT_NULL(next);
    TEST_ASSERT_EQUAL_STRING("RunState", next->getName());
    connect->exit();
    delete connect;
    delete next;
}

void test_run_state_static() {
    RunState* run = new RunState();
    run->enter();
    State* same = run->update();
    TEST_ASSERT_EQUAL(run, same);
    run->exit();
    delete run;
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_init_to_connect);
    RUN_TEST(test_connect_to_run);
    RUN_TEST(test_run_state_static);
    UNITY_END();
}

void loop() {}
