```rst
.. _proxy_testing_tutorial:

Proxy-Based Testing Tutorial
============================

This tutorial explains how to use the C++ proxy-based testing system for unit and integration testing, focusing on control and data flow tracking, fault injection, mocking, and CI integration. The system is designed to test C++ classes non-invasively, with runtime switching between production and instrumented code.

.. need:: Setup Project
   :id: SETUP_PROJECT
   :tags: setup
   :status: open

   **Objective**: Set up the project structure and dependencies.

   **Steps**:

   1. **Create Directory Structure**:
      - Create a project directory with:
        - ``test_dataflow.cpp``: Main test file (see code above).
        - ``CMakeLists.txt``: Build configuration.
        - ``.github/workflows/ci.yml``: GitHub Actions workflow.

      .. code-block:: bash

         mkdir proxy_test
         cd proxy_test
         mkdir .github/workflows

   2. **Install Dependencies**:
      - Install CMake, Google Test, and Google Mock:
        .. code-block:: bash

           sudo apt-get update
           sudo apt-get install -y cmake libgtest-dev googletest

   3. **Save Files**:
      - Save the provided ``test_dataflow.cpp``, ``CMakeLists.txt``, and ``ci.yml`` in the appropriate directories.

   4. **Build Project**:
      - Configure and build:
        .. code-block:: bash

           mkdir build && cd build
           cmake ..
           make

   **Purpose**: Establishes the foundation for running tests locally and in CI.

.. need:: Write Unit Tests
   :id: WRITE_UNIT_TESTS
   :tags: testing
   :status: open

   **Objective**: Write unit tests for individual methods.

   **Steps**:

   1. **Create Test Case**:
      - Add a test to ``DataFlowTest`` fixture in ``test_dataflow.cpp``.
      - Example: Test ``Class3::validate``:
        .. code-block:: cpp

           TEST_F(DataFlowTest, TestValidate) {
               InstrumentedClass3NoExecuteNoSummarize c3(tracker);
               EXPECT_TRUE(c3.validate(3, 5));
               std::vector<std::string> expected = {
                   "Enter Class3::validate",
                   "Exit Class3::validate"
               };
               ASSERT_EQ(tracker.call_stack, expected);
               EXPECT_EQ(tracker.values["Class3::validate_output"], "true");
           }

   2. **Verify Control Flow**:
      - Check ``tracker.call_stack`` for method entry/exit.

   3. **Verify Data Flow**:
      - Check ``tracker.values`` for input/output values.

   **Purpose**: Ensures individual methods work correctly in isolation.

.. need:: Write Integration Tests
   :id: WRITE_INTEGRATION_TESTS
   :tags: testing
   :status: open

   **Objective**: Test interactions between classes.

   **Steps**:

   1. **Create Test Case**:
      - Use ``Class1Factory`` to create an instrumented ``Class1``:
        .. code-block:: cpp

           TEST_F(DataFlowTest, IntegrationTest) {
               auto c1 = Class1Factory::create(true, tracker);
               EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 3);
               std::vector<std::string> expected = {
                   "Enter Class1::execute",
                   "Enter Class2::transform",
                   "Enter Class3::process",
                   "Exit Class3::process",
                   "Exit Class2::transform",
                   "Exit Class1::execute"
               };
               ASSERT_EQ(tracker.call_stack, expected);
           }

   2. **Verify Full Chain**:
      - Check call stack and values to ensure correct method interactions.

   **Purpose**: Validates the entire computation chain.

.. need:: Configure Fault Injection
   :id: CONFIGURE_FAULT_INJECTION
   :tags: fault_injection
   :status: open

   **Objective**: Simulate failures to test robustness.

   **Steps**:

   1. **Exception Fault**:
      - Configure ``TestProxy`` to throw an exception:
        .. code-block:: cpp

           TEST_F(DataFlowTest, FaultException) {
               InstrumentedClass3NoExecuteNoSummarize c3(tracker);
               c3.proxy_.set_fault(FaultType::Exception, "Class3::process");
               auto c1 = Class1Factory::create(true, tracker);
               EXPECT_THROW(c1->execute(2), std::runtime_error);
           }

   2. **Custom Return Fault**:
      - Return a specific value:
        .. code-block:: cpp

           TEST_F(DataFlowTest, FaultCustomReturn) {
               InstrumentedClass2NoSummarize c2(tracker);
               c2.proxy_.set_fault(FaultType::CustomReturn, "Class2::transform", 0, 100);
               auto c1 = Class1Factory::create(true, tracker);
               EXPECT_EQ(c1->execute(2), 100 * 3);
           }

   3. **Delay Fault**:
      - Simulate latency:
        .. code-block:: cpp

           TEST_F(DataFlowTest, FaultDelay) {
               InstrumentedClass3NoExecuteNoSummarize c3(tracker);
               c3.proxy_.set_fault(FaultType::Delay, "Class3::validate", 100);
               auto start = std::chrono::steady_clock::now();
               c3.validate(3, 5);
               auto end = std::chrono::steady_clock::now();
               auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
               EXPECT_GE(duration, 100);
           }

   **Purpose**: Tests system behavior under various failure conditions.

.. need:: Use Mocking
   :id: USE_MOCKING
   :tags: mocking
   :status: open

   **Objective**: Test classes in isolation using mocks.

   **Steps**:

   1. **Create Mock**:
      - Use ``MockClass2`` or ``MockClass3``:
        .. code-block:: cpp

           TEST_F(DataFlowTest, MockTest) {
               MockClass2 mock_c2;
               EXPECT_CALL(mock_c2, transform(2)).WillOnce(testing::Return(10));
               auto c1 = Class1Factory::create(true, tracker);
               int result = mock_c2.transform(2) * c1->multiplier;
               EXPECT_EQ(result, 30);
           }

   2. **Configure Expectations**:
      - Use ``EXPECT_CALL`` to define mock behavior.

   3. **Verify Interaction**:
      - Ensure mock is called as expected.

   **Purpose**: Isolates dependencies for focused testing.

.. need:: Test Member Variables
   :id: TEST_MEMBER_VARIABLES
   :tags: testing
   :status: open

   **Objective**: Verify behavior with custom configurations.

   **Steps**:

   1. **Create Test Case**:
      - Use ``Class1Factory`` with custom multiplier:
        .. code-block:: cpp

           TEST_F(DataFlowTest, CustomMultiplier) {
               auto c1 = Class1Factory::create(true, tracker, 4);
               EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 4);
           }

   2. **Verify Impact**:
      - Check output reflects member variable.

   **Purpose**: Ensures configuration affects behavior correctly.

.. need:: Run Tests Without Instrumentation
   :id: RUN_WITHOUT_INSTRUMENTATION
   :tags: testing
   :status: open

   **Objective**: Verify production code behavior.

   **Steps**:

   1. **Create Test Case**:
      - Use ``Class1Factory`` with ``use_instrumented=false``:
        .. code-block:: cpp

           TEST_F(DataFlowTest, NoInstrumentation) {
               auto c1 = Class1Factory::create(false, tracker);
               EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 3);
               EXPECT_TRUE(tracker.call_stack.empty());
           }

   **Purpose**: Ensures original code runs without test overhead.

.. need:: Integrate with CI
   :id: INTEGRATE_CI
   :tags: ci
   :status: open

   **Objective**: Automate testing in CI.

   **Steps**:

   1. **Push to GitHub**:
      - Create a repository and push the project.

   2. **Verify Workflow**:
      - Ensure ``.github/workflows/ci.yml`` is present.
      - Check GitHub Actions runs on push/pull requests.

   3. **Monitor Results**:
      - View test output in GitHub Actions logs.

   **Purpose**: Ensures consistent testing across environments.

.. need:: Extend the System
   :id: EXTEND_SYSTEM
   :tags: extension
   :status: open

   **Objective**: Add new methods or classes to the test system.

   **Steps**:

   1. **Add New Method**:
      - Define method in original class (e.g., ``Class3::newMethod``).
      - Add corresponding ``wrap_`` function in ``TestProxy`` (e.g., ``wrap_new_type``).
      - Create a new ``PROXY_METHOD_`` macro.
      - Update ``DEFINE_INSTRUMENTED_CLASS`` to include the method.

   2. **Add New Class**:
      - Define new class (e.g., ``Class4``).
      - Use ``DEFINE_INSTRUMENTED_CLASS`` to create instrumented version.
      - Update tests to cover new class.

   **Purpose**: Makes the system adaptable to new requirements.
```