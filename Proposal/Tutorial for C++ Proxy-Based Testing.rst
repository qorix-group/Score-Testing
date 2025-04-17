.. _cpp-proxy-testing-tutorial:

Tutorial: C++ Proxy-Based Testing Framework
==========================================

This tutorial explains how to use a C++ proxy-based testing framework to perform unit and integration testing, with a focus on control and data flow analysis. The framework uses a proxy layer to instrument method calls, enabling runtime tracking, fault injection, and switching between test and original implementations without modifying the original source code. The tutorial is based on a practical example of an "Order Processing System" and provides step-by-step instructions for adapting the framework to your own project.

The framework uses Google Test for testing and CMake for build management. It is designed to be non-invasive, flexible, and reusable, making it suitable for complex C++ applications.

.. contents:: Table of Contents
   :depth: 2
   :local:

Overview
--------

The proxy-based testing framework provides the following features:

- **Unit Testing**: Test individual components in isolation.
- **Integration Testing**: Verify interactions between components, focusing on control and data flow.
- **Control Flow Tracking**: Log the sequence of method calls.
- **Data Flow Tracking**: Track input and output values across methods.
- **Fault Injection**: Simulate failures to test system robustness.
- **Runtime Switching**: Toggle between instrumented (test) and original implementations at runtime.
- **Non-invasive**: Original source code remains unmodified.

The example system is an "Order Processing System" with three classes:

- **OrderProcessor**: Coordinates order processing by invoking payment and inventory operations.
- **PaymentService**: Processes payments based on the provided amount.
- **InventoryManager**: Updates stock levels for items.

The framework instruments these classes using a proxy layer (`TestProxy`) and tests them with Google Test, ensuring comprehensive verification of behavior.

Prerequisites
-------------

To follow this tutorial, you need:

- A C++17-compatible compiler (e.g., GCC, Clang).
- Google Test library installed.
- CMake 3.10 or higher.
- Basic knowledge of C++, object-oriented programming, and unit testing.
- (Optional) Sphinx for generating documentation.

Step-by-Step Guide
------------------

This section outlines the steps to adapt the proxy-based testing framework to your C++ project, using the Order Processing System as a reference.

Step 1: Understand the System Under Test
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Identify the components and their interactions in your application.

**Tasks**:

- Map out the classes, their methods, and dependencies.
- Determine the control flow (sequence of method calls) and data flow (input/output transformations).
- Identify key interfaces to instrument for testing.

**Example**:

In the Order Processing System, the classes are:

- ``OrderProcessor``: Calls ``PaymentService::processPayment`` and ``InventoryManager::updateStock``.
- ``PaymentService``: Validates payment amounts.
- ``InventoryManager``: Checks item IDs and quantities.

The control flow for ``OrderProcessor::processOrder`` is:

1. Call ``PaymentService::processPayment(amount)``.
2. If payment succeeds, call ``InventoryManager::updateStock(itemId, quantity)``.
3. Return the result.

The data flow involves:

- Input: ``orderId``, ``amount``, ``itemId``.
- Output: Boolean indicating success or failure.

**Code**:

.. code-block:: cpp

   // src/order_processor.h
   #pragma once

   #include "payment_service.h"
   #include "inventory_manager.h"

   class OrderProcessor {
   public:
       bool processOrder(int orderId, double amount, int itemId) const {
           PaymentService payment;
           InventoryManager inventory;
           if (payment.processPayment(amount)) {
               return inventory.updateStock(itemId, 1);
           }
           return false;
       }
   };

   // src/payment_service.h
   #pragma once

   class PaymentService {
   public:
       bool processPayment(double amount) const {
           return amount > 0; // Simplified: accepts positive amounts
       }
   };

   // src/inventory_manager.h
   #pragma once

   class InventoryManager {
   public:
       bool updateStock(int itemId, int quantity) const {
           return itemId >= 0 && quantity > 0; // Simplified: valid item and quantity
       }
   };

Step 2: Define the Proxy Layer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Create a proxy layer to instrument method calls for tracking and fault injection.

**Tasks**:

- Implement a ``TestTracker`` class to store control flow (call stack) and data flow (input/output values).
- Implement a ``TestProxy`` template class to wrap method calls, logging entry/exit, inputs/outputs, and injecting faults.
- Define macros to simplify proxy method creation for different signatures.

**Example**:

The ``TestTracker`` stores a call stack and key-value pairs for data tracking. The ``TestProxy`` template provides ``wrap`` methods for each method signature in the system, handling fault injection and logging.

**Code**:

.. code-block:: cpp

   // proxy/test_proxy.h
   #pragma once

   #include <vector>
   #include <map>
   #include <string>
   #include <stdexcept>

   class TestTracker {
   public:
       std::vector<std::string> call_stack;
       std::map<std::string, std::string> values;
       void reset() {
           call_stack.clear();
           values.clear();
       }
   };

   template<typename T>
   class TestProxy {
   public:
       static TestTracker* tracker;
       static bool inject_fault;
       static std::string fault_target;

       // Wrap for methods returning bool with (double) parameter
       template<typename Method>
       static bool wrap_bool_double(Method method, const T* obj, double x, const std::string& name) {
           tracker->call_stack.push_back("Enter " + name);
           tracker->values[name + "_input"] = std::to_string(x);

           if (inject_fault && fault_target == name) {
               tracker->call_stack.push_back("FAULT INJECTED");
               throw std::runtime_error("Fault injected in " + name);
           }

           bool result = (obj->*method)(x);
           tracker->call_stack.push_back("Exit " + name);
           tracker->values[name + "_output"] = result ? "true" : "false";
           return result;
       }

       // Wrap for methods returning bool with (int, int) parameters
       template<typename Method>
       static bool wrap_bool_int_int(Method method, const T* obj, int x, int y, const std::string& name) {
           tracker->call_stack.push_back("Enter " + name);
           tracker->values[name + "_input_1"] = std::to_string(x);
           tracker->values[name + "_input_2"] = std::to_string(y);

           if (inject_fault && fault_target == name) {
               tracker->call_stack.push_back("FAULT INJECTED");
               throw std::runtime_error("Fault injected in " + name);
           }

           bool result = (obj->*method)(x, y);
           tracker->call_stack.push_back("Exit " + name);
           tracker->values[name + "_output"] = result ? "true" : "false";
           return result;
       }

       // Wrap for methods returning bool with (int, double, int) parameters
       template<typename Method>
       static bool wrap_bool_int_double_int(Method method, const T* obj, int x, double y, int z, const std::string& name) {
           tracker->call_stack.push_back("Enter " + name);
           tracker->values[name + "_input_1"] = std::to_string(x);
           tracker->values[name + "_input_2"] = std::to_string(y);
           tracker->values[name + "_input_3"] = std::to_string(z);

           if (inject_fault && fault_target == name) {
               tracker->call_stack.push_back("FAULT INJECTED");
               throw std::runtime_error("Fault injected in " + name);
           }

           bool result = (obj->*method)(x, y, z);
           tracker->call_stack.push_back("Exit " + name);
           tracker->values[name + "_output"] = result ? "true" : "false";
           return result;
       }
   };

   template<typename T> TestTracker* TestProxy<T>::tracker = nullptr;
   template<typename T> bool TestProxy<T>::inject_fault = false;
   template<typename T> std::string TestProxy<T>::fault_target = "";

**Notes**:

- ``TestTracker`` uses ``std::vector`` for call sequences and ``std::map`` for key-value pairs.
- ``TestProxy`` is a template to support different classes (e.g., ``OrderProcessor``, ``PaymentService``).
- Each ``wrap`` method logs entry/exit, inputs, outputs, and checks for fault injection.
- Static members (``tracker``, ``inject_fault``, ``fault_target``) allow global configuration.

Step 3: Create Instrumented Classes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Define subclasses that inherit from the original classes to add proxy instrumentation.

**Tasks**:

- Create instrumented classes (e.g., ``InstrumentedOrderProcessor``) that inherit from the original classes.
- Use macros to override methods, routing calls through ``TestProxy::wrap``.
- Ensure the instrumented classes maintain the same interface as the originals.

**Example**:

Instrumented classes override the methods of ``OrderProcessor``, ``PaymentService``, and ``InventoryManager`` using proxy macros.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   #include "order_processor.h"
   #include "test_proxy.h"

   // Proxied Methods
   #define PROXY_METHOD_BOOL_DOUBLE(Class, Method) \
       bool Method(double x) const { \
           return TestProxy<Class>::wrap_bool_double(&Class::Method, this, x, #Class "::" #Method); \
       }

   #define PROXY_METHOD_BOOL_INT_INT(Class, Method) \
       bool Method(int x, int y) const { \
           return TestProxy<Class>::wrap_bool_int_int(&Class::Method, this, x, y, #Class "::" #Method); \
       }

   #define PROXY_METHOD_BOOL_INT_DOUBLE_INT(Class, Method) \
       bool Method(int x, double y, int z) const { \
           return TestProxy<Class>::wrap_bool_int_double_int(&Class::Method, this, x, y, z, #Class "::" #Method); \
       }

   // Instrumented Classes
   class InstrumentedOrderProcessor : public OrderProcessor {
   public:
       PROXY_METHOD_BOOL_INT_DOUBLE_INT(OrderProcessor, processOrder)
   };

   class InstrumentedPaymentService : public PaymentService {
   public:
       PROXY_METHOD_BOOL_DOUBLE(PaymentService, processPayment)
   };

   class InstrumentedInventoryManager : public InventoryManager {
   public:
       PROXY_METHOD_BOOL_INT_INT(InventoryManager, updateStock)
   };

**Notes**:

- The ``PROXY_METHOD_*`` macros reduce boilerplate by generating method overrides that call ``TestProxy::wrap``.
- Each instrumented class inherits from its original counterpart, ensuring compatibility.

Step 4: Implement Runtime Switching
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Enable switching between original and instrumented classes at runtime.

**Tasks**:

- Create a factory class (e.g., ``OrderProcessorFactory``) that returns either an original or instrumented instance based on a flag.
- Use smart pointers (e.g., ``std::unique_ptr``) for polymorphic behavior.
- Allow configuration via environment variables, command-line arguments, or a boolean flag.

**Example**:

The ``OrderProcessorFactory`` creates either an ``OrderProcessor`` or ``InstrumentedOrderProcessor`` instance.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   #include <memory>

   // Factory for Runtime Switching
   class OrderProcessorFactory {
   public:
       static std::unique_ptr<OrderProcessor> create(bool use_instrumented) {
           return use_instrumented ? std::make_unique<InstrumentedOrderProcessor>()
                                  : std::make_unique<OrderProcessor>();
       }
   };

**Notes**:

- The factory uses a boolean flag (``use_instrumented``) to select the implementation.
- In a real application, you could read an environment variable (e.g., ``TEST_MODE=1``) to set this flag.

Step 5: Set Up the Test Infrastructure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Create a test fixture to manage the test environment and tracker.

**Tasks**:

- Define a Google Test fixture (e.g., ``OrderSystemTest``) that initializes the ``TestTracker``.
- Set up ``TestProxy`` static members in ``SetUp`` and clean up in ``TearDown``.
- Ensure each test starts with a clean state.

**Example**:

The ``OrderSystemTest`` fixture configures the ``TestProxy`` trackers and resets the state after each test.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   #include <gtest/gtest.h>

   // Test Fixture
   class OrderSystemTest : public ::testing::Test {
   protected:
       TestTracker tracker;
       void SetUp() override {
           TestProxy<InstrumentedOrderProcessor>::tracker = &tracker;
           TestProxy<InstrumentedPaymentService>::tracker = &tracker;
           TestProxy<InstrumentedInventoryManager>::tracker = &tracker;
       }

       void TearDown() override {
           TestProxy<InstrumentedOrderProcessor>::inject_fault = false;
           TestProxy<InstrumentedPaymentService>::inject_fault = false;
           TestProxy<InstrumentedInventoryManager>::inject_fault = false;
           tracker.reset();
       }
   };

**Notes**:

- ``SetUp`` assigns the ``tracker`` to all ``TestProxy`` instances.
- ``TearDown`` disables fault injection and clears the tracker, ensuring test isolation.

Step 6: Write Unit Tests
~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Test individual components in isolation.

**Tasks**:

- Create tests for each class's methods using instrumented classes.
- Verify control flow (method calls) and data flow (input/output values) using ``TestTracker``.
- Use Google Test assertions (e.g., ``EXPECT_TRUE``, ``ASSERT_EQ``).

**Example**:

Unit tests verify ``PaymentService::processPayment`` and ``InventoryManager::updateStock`` in isolation.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   TEST_F(OrderSystemTest, PaymentServiceValidPayment) {
       InstrumentedPaymentService payment;
       EXPECT_TRUE(payment.processPayment(100.0));
       std::vector<std::string> expected = {
           "Enter PaymentService::processPayment",
           "Exit PaymentService::processPayment"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["PaymentService::processPayment_input"], "100");
       EXPECT_EQ(tracker.values["PaymentService::processPayment_output"], "true");
   }

   TEST_F(OrderSystemTest, InventoryManagerValidStockUpdate) {
       InstrumentedInventoryManager inventory;
       EXPECT_TRUE(inventory.updateStock(1, 2));
       std::vector<std::string> expected = {
           "Enter InventoryManager::updateStock",
           "Exit InventoryManager::updateStock"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_input_1"], "1");
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_input_2"], "2");
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_output"], "true");
   }

**Notes**:

- Each test creates an instrumented instance and verifies the method's behavior.
- The ``call_stack`` confirms the method was called, and ``values`` checks inputs/outputs.

Step 7: Write Integration Tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Test interactions between components, focusing on control and data flow.

**Tasks**:

- Use the factory to create an instrumented ``OrderProcessor``.
- Execute a complete workflow (e.g., ``processOrder``).
- Verify the call sequence and data propagation using ``TestTracker``.

**Example**:

The integration test verifies the full order processing workflow.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   TEST_F(OrderSystemTest, ProcessOrderSuccess) {
       auto processor = OrderProcessorFactory::create(true);
       EXPECT_TRUE(processor->processOrder(101, 50.0, 1));
       std::vector<std::string> expected = {
           "Enter OrderProcessor::processOrder",
           "Enter PaymentService::processPayment",
           "Exit PaymentService::processPayment",
           "Enter InventoryManager::updateStock",
           "Exit InventoryManager::updateStock",
           "Exit OrderProcessor::processOrder"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_1"], "101");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_2"], "50");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_3"], "1");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_output"], "true");
   }

**Notes**:

- The test checks the entire call sequence from ``processOrder`` to ``updateStock``.
- Input/output values are verified to ensure correct data flow.

Step 8: Add Fault Injection
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Simulate failures to test system robustness.

**Tasks**:

- Configure ``TestProxy::inject_fault`` and ``fault_target`` to throw exceptions in specific methods.
- Write tests to verify partial execution and error handling.
- Use ``EXPECT_THROW`` to catch exceptions.

**Example**:

The fault injection test simulates a payment failure.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   TEST_F(OrderSystemTest, ProcessOrderPaymentFailure) {
       TestProxy<InstrumentedPaymentService>::inject_fault = true;
       TestProxy<InstrumentedPaymentService>::fault_target = "PaymentService::processPayment";
       auto processor = OrderProcessorFactory::create(true);
       EXPECT_THROW(processor->processOrder(101, 50.0, 1), std::runtime_error);
       std::vector<std::string> expected = {
           "Enter OrderProcessor::processOrder",
           "Enter PaymentService::processPayment",
           "FAULT INJECTED"
       };
       ASSERT_EQ(tracker.call_stack, expected);
   }

**Notes**:

- The test enables fault injection for ``PaymentService::processPayment``.
- It verifies that the exception stops execution and logs the correct partial call stack.

Step 9: Test the Reference Case
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Verify that the original implementation runs without instrumentation.

**Tasks**:

- Use the factory to create an original (non-instrumented) instance.
- Execute the workflow and confirm no tracking occurs.
- Ensure the behavior matches the instrumented case.

**Example**:

The reference case test uses the original ``OrderProcessor``.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp (partial)
   TEST_F(OrderSystemTest, OriginalProcessorNoTracking) {
       auto processor = OrderProcessorFactory::create(false);
       EXPECT_TRUE(processor->processOrder(101, 50.0, 1));
       EXPECT_TRUE(tracker.call_stack.empty()); // No tracking
   }

**Notes**:

- The ``use_instrumented=false`` flag ensures the original ``OrderProcessor`` is used.
- An empty ``call_stack`` confirms no instrumentation overhead.

Step 10: Integrate with Build System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Set up CMake to build and run tests.

**Tasks**:

- Create a ``CMakeLists.txt`` file to manage production and test builds.
- Enable tests conditionally with an ``ENABLE_TESTS`` option.
- Link against Google Test libraries.
- Run tests with ``ctest``.

**Example**:

The CMake configuration includes the application and test targets.

**Code**:

.. code-block:: cmake

   # CMakeLists.txt
   cmake_minimum_required(VERSION 3.10)
   project(OrderSystemTest)

   set(CMAKE_CXX_STANDARD 17)
   option(ENABLE_TESTS "Enable testing" ON)

   add_executable(app src/order_processor.h src/payment_service.h src/inventory_manager.h)

   if(ENABLE_TESTS)
       find_package(GTest REQUIRED)
       include_directories(${GTEST_INCLUDE_DIRS} proxy)
       add_executable(test_order_system tests/test_order_system.cpp)
       target_link_libraries(test_order_system ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
       enable_testing()
       add_test(NAME OrderSystemTest COMMAND test_order_system)
   endif()

**Notes**:

- The ``app`` target builds the production code (though no ``main`` is shown here).
- The ``test_order_system`` target builds and links the tests.
- Tests can be run with ``ctest`` or ``./test_order_system``.

Step 11: Best Practices
~~~~~~~~~~~~~~~~~~~~~~~

**Objective**: Ensure the framework is robust and maintainable.

**Tasks**:

- **Isolation**: Reset the ``TestTracker`` in ``TearDown`` to prevent test interference.
- **Thread Safety**: If tests run concurrently, consider thread-local storage for ``TestProxy`` static members (e.g., ``thread_local TestTracker* tracker``).
- **Documentation**: Document each instrumented class, test case, and proxy method.
- **CI/CD**: Integrate tests into a CI pipeline (e.g., GitHub Actions, Jenkins).
- **Extensibility**: Add ``wrap`` methods for new method signatures as needed.
- **Mocking**: Consider using a mocking library (e.g., Google Mock) for dependencies.

**Example**:

To make ``TestProxy`` thread-safe, modify the static members:

.. code-block:: cpp

   // proxy/test_proxy.h (partial)
   template<typename T>
   class TestProxy {
   public:
       static thread_local TestTracker* tracker;
       static thread_local bool inject_fault;
       static thread_local std::string fault_target;
       // ... rest unchanged
   };

   template<typename T> thread_local TestTracker* TestProxy<T>::tracker = nullptr;
   template<typename T> thread_local bool TestProxy<T>::inject_fault = false;
   template<typename T> thread_local std::string TestProxy<T>::fault_target = "";

Complete Example Code
--------------------

Below is the complete test file for the Order Processing System, combining all components.

**Code**:

.. code-block:: cpp

   // tests/test_order_system.cpp
   #include <gtest/gtest.h>
   #include <memory>
   #include "order_processor.h"
   #include "test_proxy.h"

   // Proxied Methods
   #define PROXY_METHOD_BOOL_DOUBLE(Class, Method) \
       bool Method(double x) const { \
           return TestProxy<Class>::wrap_bool_double(&Class::Method, this, x, #Class "::" #Method); \
       }

   #define PROXY_METHOD_BOOL_INT_INT(Class, Method) \
       bool Method(int x, int y) const { \
           return TestProxy<Class>::wrap_bool_int_int(&Class::Method, this, x, y, #Class "::" #Method); \
       }

   #define PROXY_METHOD_BOOL_INT_DOUBLE_INT(Class, Method) \
       bool Method(int x, double y, int z) const { \
           return TestProxy<Class>::wrap_bool_int_double_int(&Class::Method, this, x, y, z, #Class "::" #Method); \
       }

   // Instrumented Classes
   class InstrumentedOrderProcessor : public OrderProcessor {
   public:
       PROXY_METHOD_BOOL_INT_DOUBLE_INT(OrderProcessor, processOrder)
   };

   class InstrumentedPaymentService : public PaymentService {
   public:
       PROXY_METHOD_BOOL_DOUBLE(PaymentService, processPayment)
   };

   class InstrumentedInventoryManager : public InventoryManager {
   public:
       PROXY_METHOD_BOOL_INT_INT(InventoryManager, updateStock)
   };

   // Factory for Runtime Switching
   class OrderProcessorFactory {
   public:
       static std::unique_ptr<OrderProcessor> create(bool use_instrumented) {
           return use_instrumented ? std::make_unique<InstrumentedOrderProcessor>()
                                  : std::make_unique<OrderProcessor>();
       }
   };

   // Test Fixture
   class OrderSystemTest : public ::testing::Test {
   protected:
       TestTracker tracker;
       void SetUp() override {
           TestProxy<InstrumentedOrderProcessor>::tracker = &tracker;
           TestProxy<InstrumentedPaymentService>::tracker = &tracker;
           TestProxy<InstrumentedInventoryManager>::tracker = &tracker;
       }

       void TearDown() override {
           TestProxy<InstrumentedOrderProcessor>::inject_fault = false;
           TestProxy<InstrumentedPaymentService>::inject_fault = false;
           TestProxy<InstrumentedInventoryManager>::inject_fault = false;
           tracker.reset();
       }
   };

   // Unit Tests
   TEST_F(OrderSystemTest, PaymentServiceValidPayment) {
       InstrumentedPaymentService payment;
       EXPECT_TRUE(payment.processPayment(100.0));
       std::vector<std::string> expected = {
           "Enter PaymentService::processPayment",
           "Exit PaymentService::processPayment"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["PaymentService::processPayment_input"], "100");
       EXPECT_EQ(tracker.values["PaymentService::processPayment_output"], "true");
   }

   TEST_F(OrderSystemTest, InventoryManagerValidStockUpdate) {
       InstrumentedInventoryManager inventory;
       EXPECT_TRUE(inventory.updateStock(1, 2));
       std::vector<std::string> expected = {
           "Enter InventoryManager::updateStock",
           "Exit InventoryManager::updateStock"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_input_1"], "1");
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_input_2"], "2");
       EXPECT_EQ(tracker.values["InventoryManager::updateStock_output"], "true");
   }

   // Integration Tests
   TEST_F(OrderSystemTest, ProcessOrderSuccess) {
       auto processor = OrderProcessorFactory::create(true);
       EXPECT_TRUE(processor->processOrder(101, 50.0, 1));
       std::vector<std::string> expected = {
           "Enter OrderProcessor::processOrder",
           "Enter PaymentService::processPayment",
           "Exit PaymentService::processPayment",
           "Enter InventoryManager::updateStock",
           "Exit InventoryManager::updateStock",
           "Exit OrderProcessor::processOrder"
       };
       ASSERT_EQ(tracker.call_stack, expected);
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_1"], "101");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_2"], "50");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_input_3"], "1");
       EXPECT_EQ(tracker.values["OrderProcessor::processOrder_output"], "true");
   }

   TEST_F(OrderSystemTest, ProcessOrderPaymentFailure) {
       TestProxy<InstrumentedPaymentService>::inject_fault = true;
       TestProxy<InstrumentedPaymentService>::fault_target = "PaymentService::processPayment";
       auto processor = OrderProcessorFactory::create(true);
       EXPECT_THROW(processor->processOrder(101, 50.0, 1), std::runtime_error);
       std::vector<std::string> expected = {
           "Enter OrderProcessor::processOrder",
           "Enter PaymentService::processPayment",
           "FAULT INJECTED"
       };
       ASSERT_EQ(tracker.call_stack, expected);
   }

   // Reference Case
   TEST_F(OrderSystemTest, OriginalProcessorNoTracking) {
       auto processor = OrderProcessorFactory::create(false);
       EXPECT_TRUE(processor->processOrder(101, 50.0, 1));
       EXPECT_TRUE(tracker.call_stack.empty()); // No tracking
   }

   int main(int argc, char** argv) {
       ::testing::InitGoogleTest(&argc, argv);
       return RUN_ALL_TESTS();
   }

Building and Running Tests
--------------------------

To build and run the tests:

1. **Organize the Project**:

   Create the following directory structure:

   .. code-block:: text

      project/
      ├── src/
      │   ├── order_processor.h
      │   ├── payment_service.h
      │   ├── inventory_manager.h
      ├── tests/
      │   ├── test_order_system.cpp
      ├── proxy/
      │   ├── test_proxy.h
      ├── CMakeLists.txt

2. **Build with CMake**:

   Run the following commands:

   .. code-block:: bash

      mkdir build
      cd build
      cmake ..
      make

3. **Run Tests**:

   Execute the tests:

   .. code-block:: bash

      ctest
      # Or directly: ./test_order_system

   All tests should pass, with fault injection tests catching exceptions as expected.

Extending the Framework
-----------------------

To adapt the framework for more complex systems, consider the following:

- **Additional Method Signatures**: Add ``wrap`` methods in ``TestProxy`` for new parameter/return types (e.g., ``wrap_string`` for ``std::string`` returns).
- **Mocking**: Use Google Mock to create mock dependencies for isolated unit testing.
- **Thread Safety**: Implement thread-local storage for ``TestProxy`` static members to support concurrent tests.
- **Custom Fault Injection**: Extend ``TestProxy`` to simulate specific failure modes (e.g., returning ``false`` instead of throwing).
- **Automation**: Use code generation tools to create instrumented classes and macros for large codebases.

Troubleshooting
---------------

- **Linker Errors**: Ensure Google Test is installed and linked correctly in ``CMakeLists.txt``.
- **Test Failures**: Verify that ``SetUp`` and ``TearDown`` correctly initialize and reset the ``TestTracker``. Check that fault injection is disabled after each test.
- **Thread Safety Issues**: If tests run concurrently and fail, use ``thread_local`` for ``TestProxy`` static members.
- **Performance Overhead**: Ensure the factory uses original classes in production (``use_instrumented=false``) to avoid instrumentation overhead.

Conclusion
----------

This tutorial demonstrates how to use a C++ proxy-based testing framework to test an Order Processing System, covering unit and integration testing, control and data flow tracking, fault injection, and runtime switching. The framework is non-invasive, flexible, and extensible, making it suitable for real-world C++ applications. By following the steps outlined, you can adapt the framework to your own project, ensuring robust and maintainable tests.

For further details, refer to the source code and experiment with the provided example. To generate this documentation with Sphinx, save this file as ``index.rst`` and configure your Sphinx project accordingly.

.. note::

   The example code is simplified for clarity. In a production system, consider adding error handling, logging, and dependency injection for greater robustness.