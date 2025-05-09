.. _fault-injection-tutorial:

Fault Injection Testing Tutorial
================================

This tutorial explains how to implement fault injection testing for C++ code using a proxy layer, focusing on a sequential test approach to verify data flow transitions without modifying the original code. It describes the process used to solve a specific testing problem, provides UML diagrams in PlantUML notation, and offers a step-by-step guide to apply the concept to other codebases.

Introduction
------------

Testing fault tolerance in software systems often requires injecting faults into specific methods to verify how the system handles errors. In the given problem, we needed to test a C++ call chain involving three classes (`Class1`, `Class2`, `Class3`) with methods like `execute`, `transform`, and `process`, without modifying their source code. The challenge was to instrument the call chain to track execution and inject faults, ensuring that transitions between classes (e.g., `Class1::execute` to `Class2::transform`) were correctly verified.

The solution uses a **proxy layer** (`InstrumentedClass1`, `InstrumentedClass2`, `InstrumentedClass3`) to wrap the original methods, combined with a **sequential test approach** to test each transition independently. Fault injection is achieved by configuring the proxy layer to throw exceptions at specific points. This tutorial details the approach, provides UML diagrams, and explains how to generalize it to other codebases.

Problem Description
-------------------

The original code consists of three classes forming a call chain:

- `Class1::execute(int x)` calls `Class2::transform(x)`.
- `Class2::transform(x)` calls `Class3::process(x)`.
- Additional methods like `Class1::compute`, `Class2::combine`, and `Class3::scale` form other call chains.

The requirements were:
- **Instrument the call chain** to track method calls and input/output values using a `TestTracker`.
- **Inject faults** to simulate errors in specific methods (e.g., `Class3::process`, `Class2::transform`).
- **Do not modify** the original classes (`Class1`, `Class2`, `Class3`), which have non-virtual methods.
- **Verify transitions** between classes (e.g., `Class1` to `Class2`) to ensure correct data flow.

The initial issue was that the non-virtual methods prevented overriding in derived classes, and direct calls to regular methods bypassed the proxy layer, causing fault injection tests to fail.

Solution Overview
-----------------

The solution uses a **proxy layer** to instrument method calls and a **sequential test approach** to test each transition independently. Key components include:

1. **Proxy Classes**:
   - `InstrumentedClass1`, `InstrumentedClass2`, and `InstrumentedClass3` inherit from `Class1`, `Class2`, and `Class3`.
   - Each defines `proxy_XXX` methods that wrap original methods using `TestProxy<T>::wrap`, which tracks calls and injects faults.

2. **Factory**:
   - A `Factory` class creates either `ClassX` or `InstrumentedClassX` instances based on a `use_instrumented` flag, allowing runtime switching.

3. **TestTracker**:
   - Tracks method calls (`call_stack`) and input/output values (`values`).

4. **Sequential Tests**:
   - Tests instrument the call chain up to a transition point (e.g., `Class1::execute`), call the regular method (e.g., `Class2::transform`), and verify the result.
   - Fault injection tests call `proxy_XXX` methods with fault settings to throw exceptions.

5. **Fault Injection**:
   - Configured via `TestProxy<T>::inject_fault` and `fault_target` static members, checked in `TestProxy<T>::wrap` to throw `std::runtime_error`.

UML Diagrams
------------

Below are PlantUML diagrams illustrating the class structure and test flow.

Class Diagram
~~~~~~~~~~~~~

The class diagram shows the relationships between original classes, proxy classes, factory, and test components.

.. uml::

   @startuml
   class Class1 {
     -factor_: int
     -counter_: int
     +execute(x: int): int
     +compute(value: double, count: int): double
     +get_counter(): int
   }

   class Class2 {
     -multiplier_: int
     -name_: string
     +transform(x: int): int
     +combine(x: int, label: string): string
     +get_name(): string
   }

   class Class3 {
     -offset_: int
     -name_: string
     +process(x: int): int
     +scale(factor: double, count: int): double
     +describe(): string
   }

   class InstrumentedClass1 {
     +proxy_execute(x: int): int
     +proxy_compute(value: double, count: int): double
     +proxy_get_counter(): int
   }

   class InstrumentedClass2 {
     +proxy_transform(x: int): int
     +proxy_combine(x: int, label: string): string
     +proxy_get_name(): string
   }

   class InstrumentedClass3 {
     +proxy_process(x: int): int
     +proxy_scale(factor: double, count: int): double
     +proxy_describe(): string
   }

   class Factory {
     +use_instrumented: bool
     +create_class1(): unique_ptr<Class1>
     +create_class2(): unique_ptr<Class2>
     +create_class3(): unique_ptr<Class3>
   }

   class TestTracker {
     +call_stack: vector<string>
     +values: map<string, string>
     +reset()
     +push_call(call: string)
     +insert_value(key: string, value: int)
     +insert_value(key: string, value: double)
     +insert_value(key: string, value: string)
   }

   class TestProxy<T> {
     +inject_fault: bool
     +fault_target: string
     +wrap(method, obj, name, args...): auto
   }

   InstrumentedClass1 --|> Class1
   InstrumentedClass2 --|> Class2
   InstrumentedClass3 --|> Class3
   Factory --> Class1
   Factory --> InstrumentedClass1
   Factory --> Class2
   Factory --> InstrumentedClass2
   Factory --> Class3
   Factory --> InstrumentedClass3
   TestProxy <|.. InstrumentedClass1
   TestProxy <|.. InstrumentedClass2
   TestProxy <|.. InstrumentedClass3
   TestTracker <|.. TestProxy
   @enduml

Sequence Diagram: Fault Injection Test
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The sequence diagram illustrates the `FaultInjectionClass3` test, showing how fault injection is triggered.

.. uml::

   @startuml
   actor Tester
   participant "Test: FaultInjectionClass3" as Test
   participant "Factory" as Factory
   participant "InstrumentedClass2" as IC2
   participant "InstrumentedClass3" as IC3
   participant "TestProxy<Class2>" as Proxy2
   participant "TestProxy<Class3>" as Proxy3
   participant "TestTracker" as Tracker

   Tester -> Test: Run FaultInjectionClass3
   Test -> Factory: create_class2()
   Factory -> IC2: new InstrumentedClass2
   Test -> Factory: create_class3()
   Factory -> IC3: new InstrumentedClass3
   Test -> IC2: proxy_transform(2)
   IC2 -> Proxy2: wrap(&Class2::transform, this, "Class2::transform", 2)
   Proxy2 -> Tracker: push_call("Enter Class2::transform")
   Proxy2 -> Tracker: insert_value("Class2::transform_input", 2)
   Proxy2 -> IC2: Class2::transform(2)
   IC2 -> Tracker: push_call("Exit Class2::transform")
   IC2 -> Tracker: insert_value("Class2::transform_output", 18)
   Test -> Proxy3: inject_fault = true
   Test -> Proxy3: fault_target = "Class3::process"
   Test -> IC3: proxy_process(2)
   IC3 -> Proxy3: wrap(&Class3::process, this, "Class3::process", 2)
   Proxy3 -> Tracker: push_call("Enter Class3::process")
   Proxy3 -> Tracker: insert_value("Class3::process_input", 2)
   Proxy3 -> Tracker: push_call("FAULT INJECTED")
   Proxy3 -> Test: throw std::runtime_error("Fault injected in Class3::process")
   Test -> Test: Verify tracker.call_stack and values
   @enduml

Solution Details
----------------

The solution addresses the challenge of testing a non-virtual call chain without modifying the original code. Below are the key components and how they work together.

Proxy Layer
~~~~~~~~~~~

The proxy layer consists of `InstrumentedClassX` classes that inherit from `ClassX` and define `proxy_XXX` methods. These methods wrap original methods using `TestProxy<T>::wrap`, which:

- Logs entry/exit to `TestTracker::call_stack`.
- Stores input/output values in `TestTracker::values`.
- Checks `inject_fault` and `fault_target` to throw `std::runtime_error` for fault injection.

For example, in `InstrumentedClass3`:

.. code-block:: cpp

   PROXY_METHOD(Class3, process, int, (int x), x)

Expands to:

.. code-block:: cpp

   int proxy_process(int x) const {
       return TestProxy<Class3>::wrap(&Class3::process, this, "Class3::process", x);
   }

The `TestProxy<T>::wrap` function:

.. code-block:: cpp

   template<typename Method, typename... Args>
   static auto wrap(Method method, const T* obj, const std::string& name, Args... args) {
       global_tracker->push_call("Enter " + name);
       if constexpr (sizeof...(args) > 0) {
           std::apply([&](auto first, auto...) { global_tracker->insert_value(name + "_input", first); }, std::tuple(args...));
       }
       if (inject_fault && fault_target == name) {
           global_tracker->push_call("FAULT INJECTED");
           throw std::runtime_error("Fault injected in " + name);
       }
       auto result = (obj->*method)(args...);
       global_tracker->push_call("Exit " + name);
       global_tracker->insert_value(name + "_output", result);
       return result;
   }

Sequential Test Approach
~~~~~~~~~~~~~~~~~~~~~~~~

The sequential test approach tests each transition in the call chain independently:

- **Transition Tests**: For each transition (e.g., `Class1::execute` to `Class2::transform`):
  - Instrument up to the transition point using `proxy_XXX` (e.g., `proxy_execute`).
  - Call the regular method (e.g., `Class2::transform`) to test the original code.
  - Verify the result and `tracker.call_stack`/`values`.
- **Fault Injection Tests**: For specific methods (e.g., `Class3::process`):
  - Instrument up to the prior point (e.g., `proxy_transform`).
  - Set `TestProxy<ClassX>::inject_fault = true` and `fault_target` (e.g., `"Class3::process"`).
  - Call the proxy method (e.g., `proxy_process`) to trigger the fault.
  - Verify the exception and `tracker.call_stack`.

Example: `FaultInjectionClass3` test:

.. code-block:: cpp

   TEST_F(DataFlowTest, FaultInjectionClass3) {
       auto c2 = Factory::create_class2();
       auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
       auto c3 = Factory::create_class3();
       auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
       ic2->proxy_transform(2); // Instrument up to Class2::transform
       TestProxy<Class3>::inject_fault = true;
       TestProxy<Class3>::fault_target = "Class3::process";
       EXPECT_THROW(ic3->proxy_process(2), std::runtime_error);
       std::vector<std::string> expected = {
           "Enter Class2::transform",
           "Exit Class2::transform",
           "Enter Class3::process",
           "FAULT INJECTED"
       };
       EXPECT_EQ(tracker.call_stack, expected);
   }

Factory and Runtime Switching
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `Factory` class enables runtime switching between original and instrumented classes:

.. code-block:: cpp

   class Factory {
   public:
       static bool use_instrumented;
       static std::unique_ptr<Class1> create_class1() {
           if (use_instrumented) return std::make_unique<InstrumentedClass1>();
           return std::make_unique<Class1>();
       }
       // Similar for create_class2, create_class3
   };

Set to `true` in `main()` for instrumented tests and `false` for `NonInstrumentedExecution`.

Applying the Concept to Other Code
----------------------------------

To apply this fault injection testing approach to another C++ codebase, follow these steps:

1. **Identify the Call Chain**:
   - Analyze the codebase to identify the classes and methods forming the call chain (e.g., `ClassA::method1` calls `ClassB::method2`).
   - Note whether methods are virtual or non-virtual and any constraints (e.g., cannot modify original code).

2. **Create Proxy Classes**:
   - Define proxy classes (e.g., `InstrumentedClassA`, `InstrumentedClassB`) inheriting from the original classes.
   - Add `proxy_XXX` methods for each method to be tested, using a macro like `PROXY_METHOD`:
     .. code-block:: cpp

        #define PROXY_METHOD(Class, Method, ReturnType, Params, ...) \
            ReturnType proxy_##Method Params const { \
                return TestProxy<Class>::wrap(&Class::Method, this, #Class "::" #Method, ##__VA_ARGS__); \
            }

3. **Implement TestProxy**:
   - Create a `TestProxy<T>` template to handle tracking and fault injection:
     .. code-block:: cpp

        template<typename T>
        class TestProxy {
        public:
            static bool inject_fault;
            static std::string fault_target;
            template<typename Method, typename... Args>
            static auto wrap(Method method, const T* obj, const std::string& name, Args... args) {
                global_tracker->push_call("Enter " + name);
                if constexpr (sizeof...(args) > 0) {
                    std::apply([&](auto first, auto...) { global_tracker->insert_value(name + "_input", first); }, std::tuple(args...));
                }
                if (inject_fault && fault_target == name) {
                    global_tracker->push_call("FAULT INJECTED");
                    throw std::runtime_error("Fault injected in " + name);
                }
                auto result = (obj->*method)(args...);
                global_tracker->push_call("Exit " + name);
                global_tracker->insert_value(name + "_output", result);
                return result;
            }
        };
        template<typename T> bool TestProxy<T>::inject_fault = false;
        template<typename T> std::string TestProxy<T>::fault_target = "";

4. **Create a Factory**:
   - Implement a factory to create either original or proxy instances:
     .. code-block:: cpp

        class Factory {
        public:
            static bool use_instrumented;
            static std::unique_ptr<ClassA> create_classA() {
                if (use_instrumented) return std::make_unique<InstrumentedClassA>();
                return std::make_unique<ClassA>();
            }
            // Add for other classes
        };
        bool Factory::use_instrumented = false;

5. **Define TestTracker**:
   - Create a `TestTracker` to track calls and values:
     .. code-block:: cpp

        class TestTracker {
        public:
            std::vector<std::string> call_stack;
            std::map<std::string, std::string> values;
            void reset();
            void push_call(const std::string& call);
            void insert_value(const std::string& key, int value);
            // Add overloads for other types
        };

6. **Write Sequential Tests**:
   - Create tests for each transition (e.g., `ClassA::method1` to `ClassB::method2`):
     - Use `proxy_XXX` to instrument up to the transition.
     - Call the regular method to test the original code.
     - Verify results and `tracker.call_stack`/`values`.
   - For fault injection, set `TestProxy<ClassX>::inject_fault` and `fault_target` before calling `proxy_XXX`:
     .. code-block:: cpp

        TEST_F(MyTest, FaultInjectionMethod2) {
            auto ca = Factory::create_classA();
            auto ia = static_cast<InstrumentedClassA*>(ca.get());
            auto cb = Factory::create_classB();
            auto ib = static_cast<InstrumentedClassB*>(cb.get());
            ia->proxy_method1(input); // Instrument up to method1
            TestProxy<ClassB>::inject_fault = true;
            TestProxy<ClassB>::fault_target = "ClassB::method2";
            EXPECT_THROW(ib->proxy_method2(input), std::runtime_error);
            // Verify tracker.call_stack and values
        }

7. **Set Up Test Fixture**:
   - Use Google Test to create a test fixture:
     .. code-block:: cpp

        class MyTest : public ::testing::Test {
        protected:
            TestTracker tracker;
            void SetUp() override {
                Factory::use_instrumented = true;
                global_tracker = &tracker;
            }
            void TearDown() override {
                TestProxy<ClassA>::inject_fault = false;
                TestProxy<ClassB>::inject_fault = false;
                TestProxy<ClassA>::fault_target = "";
                TestProxy<ClassB>::fault_target = "";
                Factory::use_instrumented = false;
                global_tracker = nullptr;
                tracker.reset();
            }
        };

8. **Integrate with Build System**:
   - Ensure the test code links with Google Test and compiles with C++17 or later.
   - Example compilation command:
     .. code-block:: bash

        g++ -std=c++17 -I/usr/include -DGTEST_LINKED_AS_SHARED_LIBRARY=1 test.cpp -L/usr/lib -lgtest -lgtest_main -lgmock -o test_exe

Troubleshooting Common Issues
-----------------------------

1. **Fault Injection Not Triggering**:
   - **Symptom**: `proxy_XXX` does not throw `std::runtime_error`.
   - **Fix**: Ensure `TestProxy<ClassX>` is used for `inject_fault` and `fault_target`, matching the `PROXY_METHOD` instantiation. Check debug output for `inject_fault` and `fault_target` values.

2. **Proxy Methods Bypassed**:
   - **Symptom**: Regular methods are called instead of `proxy_XXX`.
   - **Fix**: Verify that `Factory::use_instrumented = true` and objects are cast to `InstrumentedClassX` using `static_cast`.

3. **Test Isolation Issues**:
   - **Symptom**: Fault injection settings from one test affect another.
   - **Fix**: Ensure `TearDown` resets `TestProxy<ClassX>::inject_fault` and `fault_target`. Set fault injection settings just before proxy calls.

Conclusion
----------

This tutorial detailed a fault injection testing approach using a proxy layer and sequential tests to verify data flow transitions in a C++ call chain without modifying the original code. The UML diagrams illustrated the class structure and test flow, and the step-by-step guide explained how to apply the concept to other codebases. By following this approach, developers can instrument and test complex call chains, ensuring robust fault tolerance and correct data flow.

For further assistance, refer to the provided code and diagrams, or contact the documentation team for additional examples or clarifications.

.. note::

   Ensure that the codebase uses a compatible C++ standard (e.g., C++17) and that Google Test is properly integrated for running the tests.