
.. _proxy_testing_tutorial:

Proxy-Based Testing Tutorial
============================

This tutorial demonstrates how to use the enhanced C++ proxy-based testing system for unit and integration testing, covering all functionalities with one positive and one negative test per feature. The system supports non-invasive testing, control/data flow tracking, fault injection, mocking, and CI integration, ensuring robust testing without modifying production code.

.. need:: Setup Project
   :id: SETUP_PROJECT
   :tags: setup
   :status: open

   **Objective**: Set up the project structure and dependencies.

   **Steps**:
   - Create a directory with ``test_dataflow.cpp``, ``CMakeLists.txt``, and ``.github/workflows/ci.yml`` (see provided code).
   - Install dependencies: ``sudo apt-get install -y cmake libgtest-dev googletest``.
   - Build: ``mkdir build && cd build && cmake .. && make``.

   **Purpose**: Establishes the environment for running tests locally and in CI.

.. need:: Test Non-Invasive Testing
   :id: TEST_NON_INVASIVE
   :tags: testing, non_invasive
   :status: open

   **Objective**: Verify testing without modifying original classes.

   **Positive Test**:
   - Use original ``Class1`` to ensure production code runs unchanged.
     .. code-block:: cpp

        TEST_F(DataFlowTest, NonInvasivePositive) {
            auto c1 = Class1Factory::create(false, tracker);
            EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 3); // Expect 15
        }

   **Negative Test**:
   - Verify no tracking occurs with original ``Class1`` (unexpected tracking).
     .. code-block:: cpp

        TEST_F(DataFlowTest, NonInvasiveNegative) {
            auto c1 = Class1Factory::create(false, tracker);
            c1->execute(2);
            EXPECT_TRUE(tracker.call_stack.empty()); // No tracking should occur
        }

   **Purpose**: Ensures production code integrity and test isolation.

.. need:: Test Runtime Switching
   :id: TEST_RUNTIME_SWITCHING
   :tags: testing, runtime_switching
   :status: open

   **Objective**: Toggle between original and instrumented implementations.

   **Positive Test**:
   - Use instrumented ``Class1`` to verify tracking.
     .. code-block:: cpp

        TEST_F(DataFlowTest, RuntimeSwitchingPositive) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            EXPECT_FALSE(tracker.call_stack.empty()); // Tracking occurs
        }

   **Negative Test**:
   - Use original ``Class1`` expecting no tracking (unexpected tracking).
     .. code-block:: cpp

        TEST_F(DataFlowTest, RuntimeSwitchingNegative) {
            auto c1 = Class1Factory::create(false, tracker);
            c1->execute(2);
            EXPECT_TRUE(tracker.call_stack.empty()); // No tracking
        }

   **Purpose**: Confirms ability to switch contexts without code changes.

.. need:: Test Control Flow Tracking
   :id: TEST_CONTROL_FLOW
   :tags: testing, control_flow
   :status: open

   **Objective**: Verify correct sequence of method calls.

   **Positive Test**:
   - Check call stack for ``execute`` chain.
     .. code-block:: cpp

        TEST_F(DataFlowTest, ControlFlowPositive) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            std::vector<std::string> expected = {
                "Enter Class1::execute", "Enter Class2::transform", "Enter Class3::process",
                "Exit Class3::process", "Exit Class2::transform", "Exit Class1::execute"
            };
            ASSERT_EQ(tracker.call_stack, expected);
        }

   **Negative Test**:
   - Check for incorrect call sequence (unexpected order).
     .. code-block:: cpp

        TEST_F(DataFlowTest, ControlFlowNegative) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            std::vector<std::string> wrong = {
                "Enter Class3::process", "Enter Class2::transform", "Enter Class1::execute"
            };
            EXPECT_NE(tracker.call_stack, wrong); // Wrong order
        }

   **Purpose**: Ensures methods are called in the expected order.

.. need:: Test Data Flow Tracking
   :id: TEST_DATA_FLOW
   :tags: testing, data_flow
   :status: open

   **Objective**: Validate input/output value propagation.

   **Positive Test**:
   - Verify data flow through ``execute`` chain.
     .. code-block:: cpp

        TEST_F(DataFlowTest, DataFlowPositive) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            EXPECT_EQ(tracker.values["Class3::process_output"], "4");
            EXPECT_EQ(tracker.values["Class1::execute_output"], "15");
        }

   **Negative Test**:
   - Check for incorrect output (unexpected value).
     .. code-block:: cpp

        TEST_F(DataFlowTest, DataFlowNegative) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            EXPECT_NE(tracker.values["Class1::execute_output"], "10"); // Wrong output
        }

   **Purpose**: Confirms correct data transformations across methods.

.. need:: Test Dynamic Fault Injection
   :id: TEST_FAULT_INJECTION
   :tags: fault_injection
   :status: open

   **Objective**: Simulate failures to test robustness.

   **Positive Test**:
   - Inject exception in ``Class3::process`` and verify error handling.
     .. code-block:: cpp

        TEST_F(DataFlowTest, FaultInjectionPositive) {
            InstrumentedClass3NoExecuteNoSummarize c3(tracker);
            c3.proxy_.set_fault(FaultType::Exception, "Class3::process");
            auto c1 = Class1Factory::create(true, tracker);
            EXPECT_THROW(c1->execute(2), std::runtime_error);
        }

   **Negative Test**:
   - Expect no exception without fault (unexpected exception).
     .. code-block:: cpp

        TEST_F(DataFlowTest, FaultInjectionNegative) {
            auto c1 = Class1Factory::create(true, tracker);
            EXPECT_NO_THROW(c1->execute(2)); // No fault injected
        }

   **Purpose**: Tests system behavior under failure conditions.

.. need:: Test Thread Safety
   :id: TEST_THREAD_SAFETY
   :tags: thread_safety
   :status: open

   **Objective**: Ensure safe parallel test execution.

   **Positive Test**:
   - Run multiple threads calling ``execute`` and verify no data races.
     .. code-block:: cpp

        TEST_F(DataFlowTest, ThreadSafetyPositive) {
            auto c1 = Class1Factory::create(true, tracker);
            std::vector<std::thread> threads;
            for (int i = 0; i < 3; ++i) {
                threads.emplace_back([&c1]() { c1->execute(2); });
            }
            for (auto& t : threads) t.join();
            EXPECT_FALSE(tracker.call_stack.empty()); // Tracking occurred
        }

   **Negative Test**:
   - Verify no corruption with single-threaded misuse (unexpected empty stack).
     .. code-block:: cpp

        TEST_F(DataFlowTest, ThreadSafetyNegative) {
            auto c1 = Class1Factory::create(true, tracker);
            c1->execute(2);
            EXPECT_FALSE(tracker.call_stack.empty()); // Stack should not be empty
        }

   **Purpose**: Ensures reliability in multi-threaded environments.

.. need:: Test Generalized Method Signatures
   :id: TEST_METHOD_SIGNATURES
   :tags: signatures
   :status: open

   **Objective**: Support multiple method signatures.

   **Positive Test**:
   - Test ``Class3::validate`` (bool method).
     .. code-block:: cpp

        TEST_F(DataFlowTest, MethodSignaturesPositive) {
            InstrumentedClass3NoExecuteNoSummarize c3(tracker);
            EXPECT_TRUE(c3.validate(3, 5));
            EXPECT_EQ(tracker.values["Class3::validate_output"], "true");
        }

   **Negative Test**:
   - Test for incorrect boolean output (unexpected result).
     .. code-block:: cpp

        TEST_F(DataFlowTest, MethodSignaturesNegative) {
            InstrumentedClass3NoExecuteNoSummarize c3(tracker);
            EXPECT_FALSE(c3.validate(1, 5)); // 1*2 < 5
        }

   **Purpose**: Verifies flexibility for diverse method types.

.. need:: Test Automated Instrumentation
   :id: TEST_AUTOMATION
   :tags: automation
   :status: open

   **Objective**: Use macro to generate instrumented classes.

   **Positive Test**:
   - Use ``InstrumentedClass2NoSummarize`` to verify transform.
     .. code-block:: cpp

        TEST_F(DataFlowTest, AutomationPositive) {
            InstrumentedClass2NoSummarize c2(tracker);
            EXPECT_EQ(c2.transform(2), (2 * 2) + 1); // Expect 5
        }

   **Negative Test**:
   - Verify incorrect transform output (unexpected result).
     .. code-block:: cpp

        TEST_F(DataFlowTest, AutomationNegative) {
            InstrumentedClass2NoSummarize c2(tracker);
            EXPECT_NE(c2.transform(2), 10); // Wrong output
        }

   **Purpose**: Simplifies adding new classes for testing.

.. need:: Test Advanced Mocking
   :id: TEST_MOCKING
   :tags: mocking
   :status: open

   **Objective**: Isolate dependencies with Google Mock.

   **Positive Test**:
   - Mock ``Class2::transform`` to return 10.
     .. code-block:: cpp

        TEST_F(DataFlowTest, MockingPositive) {
            MockClass2 mock_c2;
            EXPECT_CALL(mock_c2, transform(2)).WillOnce(testing::Return(10));
            auto c1 = Class1Factory::create(true, tracker);
            EXPECT_EQ(mock_c2.transform(2) * c1->multiplier, 30);
        }

   **Negative Test**:
   - Expect mock not called with wrong input (unexpected call).
     .. code-block:: cpp

        TEST_F(DataFlowTest, MockingNegative) {
            MockClass2 mock_c2;
            EXPECT_CALL(mock_c2, transform(3)).Times(0);
            mock_c2.transform(2); // Should not call transform(3)
        }

   **Purpose**: Ensures accurate dependency testing.

.. need:: Test CI Integration
   :id: TEST_CI
   :tags: ci
   :status: open

   **Objective**: Automate testing in CI.

   **Positive Test**:
   - Verify tests pass in CI (manual check in GitHub Actions).
     .. code-block:: bash

        # Push to GitHub and check Actions tab for successful run
        git push origin main

   **Negative Test**:
   - Introduce failing test and verify CI failure.
     .. code-block:: cpp

        TEST_F(DataFlowTest, CIFail) {
            auto c1 = Class1Factory::create(true, tracker);
            EXPECT_EQ(c1->execute(2), 0); // Fails (expect 15)
        }
        # Push and check CI failure in Actions

   **Purpose**: Ensures consistent test execution across environments.
```
