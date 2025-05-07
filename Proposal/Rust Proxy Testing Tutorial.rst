.. _rust-proxy-testing-tutorial:

Rust Proxy Testing Tutorial
===========================

This tutorial provides a detailed guide on applying a proxy test framework in Rust to enable comprehensive unit and integration testing, including data flow analysis and fault injection, without modifying the original codebase. The framework leverages traits, proxy wrappers, and a factory pattern to instrument code for testing while preserving production integrity. The tutorial is structured with extensive descriptions and includes example code formatted for Sphinx documentation, ensuring that code blocks can be easily copied and used.

The provided framework is designed to address common testing challenges, such as verifying execution paths, tracking data propagation, and simulating failures, all while maintaining a clear separation between production and test code. This tutorial assumes familiarity with Rust basics (structs, traits, and testing) and focuses on practical application with detailed examples.

Overview of the Framework
-------------------------

The proxy test framework enables non-invasive testing by wrapping existing code with instrumentation that logs execution details and supports fault injection. It achieves the following objectives:

- **Execution Flow Tracking**: Logs method entry and exit points to verify the call stack.
- **Data Flow Analysis**: Captures input and output values at each step for validation.
- **Fault Injection**: Simulates failures in specific methods to test error handling.
- **Non-Invasive Design**: Preserves original code by using proxy wrappers and runtime switching.
- **Flexible Testing**: Supports both unit and integration tests with minimal setup.

Key components of the framework include:

- **Traits** (e.g., ``Execute``, ``Transform``, ``Process``): Define interfaces for structs, enabling polymorphism between original and instrumented implementations.
- **Original Structs** (e.g., ``Class1``, ``Class2``, ``Class3``): Contain the core business logic, untouched by testing instrumentation.
- **TestTracker**: A thread-safe utility for logging call stacks and storing input/output values.
- **TestProxy**: A generic wrapper that intercepts method calls to log data and inject faults.
- **Instrumented Structs** (e.g., ``InstrumentedClass1``): Proxy-wrapped versions of original structs for testing.
- **Factory** (e.g., ``Class1Factory``): Dynamically creates either original or instrumented instances based on context.

The framework is particularly useful for complex systems where understanding data flow and ensuring robust error handling are critical. By using a factory pattern, it ensures that production code remains unaffected while tests gain deep visibility into execution.

Step-by-Step Guide to Apply the Framework
-----------------------------------------

The following steps detail how to integrate the proxy test framework into an existing Rust codebase. Each step includes a thorough explanation and example code, with considerations for practical implementation.

1. Define Traits for Existing Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable testing without modifying original structs, define traits that capture the public interface of each struct. Traits serve as a contract, allowing both the original and instrumented implementations to be used interchangeably via trait objects.

**Why Traits?**

- They decouple the interface from the implementation, enabling the factory pattern.
- They allow instrumented structs to mimic the original behavior while adding test-specific functionality.
- They support polymorphism, making it easy to switch implementations at runtime.

**Implementation Steps**:

- Identify the public methods of the struct that need testing.
- Define a trait with method signatures matching those of the struct.
- Implement the trait for the original struct.

**Example** (for a hypothetical struct ``MyClass``):

.. code-block:: rust
   :caption: src/lib.rs
   :name: define-trait

   /// Trait defining the interface for MyClass.
   pub trait MyClassInterface {
       /// Processes an input value and returns a result.
       fn my_method(&self, x: i32) -> i32;
   }

   /// Original struct with core logic.
   pub struct MyClass;

   impl MyClassInterface for MyClass {
       fn my_method(&self, x: i32) -> i32 {
           x + 10 // Original logic: adds 10 to input
       }
   }

**Notes**:

- Ensure method signatures in the trait match the original struct exactly.
- Use descriptive trait names (e.g., ``MyClassInterface``) to avoid confusion.
- In the provided framework, traits like ``Execute``, ``Transform``, and ``Process`` are defined similarly for ``Class1``, ``Class2``, and ``Class3``.

2. Create Instrumented Structs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Instrumented structs wrap the original struct’s methods with a ``TestProxy`` to enable tracking and fault injection. These structs implement the same trait as the original, ensuring compatibility, but delegate to the original logic via the proxy.

**Purpose**:

- Add test-specific behavior (logging, fault injection) without altering the original code.
- Maintain the same interface for seamless integration with existing code.
- Enable fine-grained control over method execution during tests.

**Implementation Steps**:

- Define a new struct (e.g., ``InstrumentedMyClass``) with a ``TestProxy`` field parameterized by the original struct type.
- Implement a ``new`` method that accepts a ``TestTracker`` and initializes the proxy.
- Implement the trait, using ``proxy.wrap`` to intercept method calls, log data, and execute the original logic.

**Example** (for ``MyClass``):

.. code-block:: rust
   :caption: src/lib.rs
   :name: instrumented-struct

   /// Instrumented version of MyClass for testing.
   pub struct InstrumentedMyClass {
       proxy: TestProxy<MyClass>,
   }

   impl InstrumentedMyClass {
       /// Creates a new instrumented instance with a tracker.
       pub fn new(tracker: TestTracker) -> Self {
           InstrumentedMyClass {
               proxy: TestProxy::new(tracker),
           }
       }
   }

   impl MyClassInterface for InstrumentedMyClass {
       fn my_method(&self, x: i32) -> i32 {
           self.proxy.wrap(|x| {
               let original = MyClass;
               original.my_method(x)
           }, x, "MyClass::my_method")
       }
   }

**Key Points**:

- The ``TestProxy`` requires a phantom type (``MyClass``) to associate it with the original struct, even though it doesn’t store an instance.
- The ``wrap`` method takes a closure that executes the original logic, the input value, and a unique method identifier (e.g., ``"MyClass::my_method"``).
- In the framework, ``InstrumentedClass1``, ``InstrumentedClass2``, and ``InstrumentedClass3`` are implemented similarly, each wrapping their respective methods (``execute``, ``transform``, ``process``).

3. Use the TestTracker
~~~~~~~~~~~~~~~~~~~~~

The ``TestTracker`` is a central component for logging execution details and data values. It uses thread-safe ``Arc<Mutex<>>`` to store a call stack (method entry/exit) and a key-value map (input/output values).

**Purpose**:

- Provides visibility into the execution flow for integration tests.
- Stores input and output values for data flow analysis.
- Supports test isolation by allowing state reset.

**Implementation Steps**:

- Initialize a ``TestTracker`` in your test setup.
- Pass the tracker (or its clone) to instrumented structs.
- Use tracker methods to verify test outcomes.

**Example**:

.. code-block:: rust
   :caption: tests/integration.rs
   :name: use-tracker

   #[test]
   fn test_setup() {
       let tracker = TestTracker::new();
       let my_class = InstrumentedMyClass::new(tracker.clone());
       let result = my_class.my_method(5);
       assert_eq!(result, 15); // 5 + 10
   }

**Key Methods**:

- ``new()``: Creates a new tracker with empty call stack and value map.
- ``push_call(&self, call: &str)``: Adds a call event (e.g., ``"Enter MyClass::my_method"``).
- ``insert_value(&self, key: &str, value: i32)``: Stores an input or output value.
- ``get_call_stack(&self) -> Vec<String>``: Returns the recorded call stack.
- ``get_values(&self) -> HashMap<String, i32>``: Returns the stored values.
- ``reset(&self)``: Clears the call stack and values for test isolation.

**Notes**:

- Clone the tracker (``tracker.clone()``) when passing to multiple structs, as it uses ``Arc`` for shared ownership.
- Use ``reset`` in test setup to ensure clean state between tests.
- In the framework, ``TestTracker`` is used extensively in tests to verify call order and data propagation.

4. Create a Factory for Runtime Switching
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The factory pattern enables runtime selection between original and instrumented implementations, ensuring that production code uses the original structs while tests use the instrumented ones.

**Purpose**:

- Isolates test instrumentation from production code.
- Simplifies switching between implementations using a single interface.
- Supports dependency injection via trait objects.

**Implementation Steps**:

- Define a factory struct (e.g., ``MyClassFactory``).
- Implement a ``create`` method that takes a boolean flag (``use_instrumented``) and an optional ``TestTracker``.
- Return a ``Box<dyn Trait>`` to abstract the implementation.

**Example**:

.. code-block:: rust
   :caption: src/lib.rs
   :name: factory

   /// Factory for creating MyClass instances.
   pub struct MyClassFactory;

   impl MyClassFactory {
       /// Creates either an original or instrumented MyClass instance.
       pub fn create(use_instrumented: bool, tracker: Option<TestTracker>) -> Box<dyn MyClassInterface> {
           if use_instrumented {
               Box::new(InstrumentedMyClass::new(tracker.unwrap_or_default()))
           } else {
               Box::new(MyClass)
           }
       }
   }

**Notes**:

- Use ``unwrap_or_default`` to provide a default ``TestTracker`` if none is supplied, ensuring flexibility.
- The returned ``Box<dyn MyClassInterface>`` allows the caller to interact with either implementation via the trait.
- In the framework, ``Class1Factory`` creates instances of ``Class1`` or ``InstrumentedClass1`` for the ``Execute`` trait.

5. Write Tests
~~~~~~~~~~~~~~

The framework supports a range of tests, including unit tests for individual components, integration tests for the entire workflow, data flow tests for value propagation, and fault injection tests for error scenarios. Tests leverage the ``TestTracker`` to validate behavior.

**General Setup**:

- Create a ``TestTracker`` and reset it between tests.
- Use the factory to create instrumented instances (``use_instrumented: true``).
- Verify results, call stack, and data values using tracker methods.

Unit Tests
^^^^^^^^^^

Unit tests focus on individual structs, verifying their behavior in isolation.

**Example**:

.. code-block:: rust
   :caption: tests/unit.rs
   :name: unit-test

   #[test]
   fn test_my_class() {
       let tracker = TestTracker::new();
       let my_class = InstrumentedMyClass::new(tracker.clone());
       assert_eq!(my_class.my_method(5), 15); // 5 + 10
       assert_eq!(
           tracker.get_call_stack(),
           vec!["Enter MyClass::my_method", "Exit MyClass::my_method"]
       );
       let values = tracker.get_values();
       assert_eq!(values.get("MyClass::my_method_input"), Some(&5));
       assert_eq!(values.get("MyClass::my_method_output"), Some(&15));
   }

**Description**:

- Tests the ``my_method`` behavior of ``InstrumentedMyClass``.
- Verifies the result (``5 + 10 = 15``).
- Checks the call stack for correct entry and exit events.
- Validates input and output values stored by the tracker.

Integration Tests
^^^^^^^^^^^^^^^^

Integration tests verify the interaction between multiple structs, using the factory to create the top-level component.

**Example** (adapted from the framework):

.. code-block:: rust
   :caption: tests/integration.rs
   :name: integration-test

   #[test]
   fn normal_flow() {
       let tracker = TestTracker::new();
       let c1 = Class1Factory::create(true, Some(tracker.clone()));
       assert_eq!(c1.execute(2), ((2 * 2) + 1) * 3); // (2*2=4) + 1=5 * 3=15
       let expected = vec![
           "Enter Class1::execute",
           "Enter Class2::transform",
           "Enter Class3::process",
           "Exit Class3::process",
           "Exit Class2::transform",
           "Exit Class1::execute",
       ];
       assert_eq!(tracker.get_call_stack(), expected);
       let values = tracker.get_values();
       assert_eq!(values.get("Class1::execute_output"), Some(&15));
   }

**Description**:

- Tests the full workflow of ``Class1`` calling ``Class2`` and ``Class3``.
- Verifies the final result (``15``).
- Checks the call stack to ensure all methods are called in the correct order.
- Validates the output value at the top level.

Data Flow Tests
^^^^^^^^^^^^^^^

Data flow tests verify how values propagate through the system, checking inputs and outputs at each step.

**Example** (from the framework):

.. code-block:: rust
   :caption: tests/integration.rs
   :name: data-flow-test

   #[test]
   fn value_propagation() {
       let tracker = TestTracker::new();
       let c1 = Class1Factory::create(true, Some(tracker.clone()));
       c1.execute(5);
       let values = tracker.get_values();
       assert_eq!(values.get("Class3::process_input"), Some(&5));
       assert_eq!(values.get("Class3::process_output"), Some(&10)); // 5*2
       assert_eq!(values.get("Class2::transform_input"), Some(&5));
       assert_eq!(values.get("Class2::transform_output"), Some(&11)); // 10+1
       assert_eq!(values.get("Class1::execute_input"), Some(&5));
       assert_eq!(values.get("Class1::execute_output"), Some(&33)); // 11*3
   }

**Description**:

- Tests the propagation of the input value ``5`` through ``Class3``, ``Class2``, and ``Class1``.
- Verifies intermediate and final values:
  - ``Class3::process``: ``5 * 2 = 10``
  - ``Class2::transform``: ``10 + 1 = 11``
  - ``Class1::execute``: ``11 * 3 = 33``
- Ensures inputs are correctly passed between methods.

Fault Injection Tests
^^^^^^^^^^^^^^^^^^^^^

Fault injection tests simulate failures by triggering panics in specific methods, allowing you to test error handling or partial execution.

**Example** (from the framework):

.. code-block:: rust
   :caption: tests/integration.rs
   :name: fault-injection-test

   #[test]
   #[should_panic(expected = "Fault injected in Class3::process")]
   fn fault_injection_class3() {
       let tracker = TestTracker::new();
       let mut c3 = InstrumentedClass3::new(tracker.clone());
       c3.proxy.set_fault(true, "Class3::process");
       let c1 = InstrumentedClass1::new(tracker.clone());
       c1.execute(2);
       let expected = vec![
           "Enter Class1::execute",
           "Enter Class2::transform",
           "Enter Class3::process",
           "FAULT INJECTED",
       ];
       assert_eq!(tracker.get_call_stack(), expected);
   }

**Description**:

- Enables fault injection in ``Class3::process`` using ``set_fault``.
- Triggers a panic when ``Class3::process`` is called.
- Verifies that the call stack reflects the execution up to the fault point.
- Ensures the test fails with the expected panic message.

6. Integrate with Existing Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To apply the framework to an existing codebase, follow these steps to ensure seamless integration while maintaining production stability.

**Integration Steps**:

1. **Identify Key Structs and Methods**: Determine which components require testing (e.g., core business logic, critical workflows).
2. **Define Traits**: Create traits for each struct’s public interface, as shown in Step 1.
3. **Implement Instrumented Structs**: Wrap methods with ``TestProxy``, as shown in Step 2.
4. **Create Factories**: Add factories for runtime switching, as shown in Step 4.
5. **Update Production Code**: Use the factory with ``use_instrumented: false`` to ensure original behavior.
6. **Write Tests**: Use instrumented structs and factories for unit, integration, data flow, and fault injection tests.

**Example (Production Code)**:

.. code-block:: rust
   :caption: src/main.rs
   :name: production-code

   fn main() {
       let my_class = MyClassFactory::create(false, None); // Original implementation
       let result = my_class.my_method(5);
       println!("Result: {}", result); // Outputs: Result: 15
   }

**Example (Test Code)**:

.. code-block:: rust
   :caption: tests/integration.rs
   :name: test-code

   #[test]
   fn test_my_class_integration() {
       let tracker = TestTracker::new();
       let my_class = MyClassFactory::create(true, Some(tracker.clone())); // Instrumented
       let result = my_class.my_method(5);
       assert_eq!(result, 15);
       assert_eq!(
           tracker.get_call_stack(),
           vec!["Enter MyClass::my_method", "Exit MyClass::my_method"]
       );
   }

**Notes**:

- In production, always use the factory with ``use_instrumented: false`` to avoid the overhead of instrumentation.
- Ensure tests cover all critical paths, including edge cases and failure scenarios.
- Use separate test modules (e.g., ``tests/unit.rs``, ``tests/integration.rs``) to organize unit and integration tests.

7. Best Practices
~~~~~~~~~~~~~~~~~

To maximize the effectiveness of the framework, adhere to the following best practices:

- **Preserve Original Code**: Always use the factory to instantiate original structs in production to avoid test-related overhead.
- **Isolate Tests**: Call ``tracker.reset()`` in test setup to clear state and prevent interference between tests.
- **Use Unique Method Identifiers**: Ensure the method names passed to ``proxy.wrap`` (e.g., ``"MyClass::my_method"``) are unique to avoid tracking conflicts.
- **Test Edge Cases**: Include tests for boundary values, null inputs, and fault injection scenarios.
- **Validate Extensively**: Use ``get_call_stack`` and ``get_values`` to verify both execution flow and data correctness.
- **Document Assumptions**: Clearly document any assumptions about input ranges or method behavior in both code and tests.
- **Optimize for Readability**: Structure tests with clear assertions and descriptive names to facilitate maintenance.

Example Application to a New Struct
----------------------------------

To illustrate the framework’s flexibility, consider applying it to a new struct, ``Calculator``, which performs basic arithmetic.

**Original Struct**:

.. code-block:: rust
   :caption: src/lib.rs
   :name: original-calculator

   /// A simple calculator struct.
   pub struct Calculator;

   impl Calculator {
       /// Adds two numbers.
       pub fn add(&self, x: i32, y: i32) -> i32 {
           x + y
       }
   }

**Apply the Framework**:

1. **Define a Trait**:

.. code-block:: rust
   :caption: src/lib.rs
   :name: calculator-trait

   /// Trait for calculator operations.
   pub trait Calculate {
       /// Adds two numbers.
       fn add(&self, x: i32, y: i32) -> i32;
   }

   impl Calculate for Calculator {
       fn add(&self, x: i32, y: i32) -> i32 {
           self.add(x, y)
       }
   }

**Description**:

- Defines the ``Calculate`` trait with the ``add`` method.
- Implements the trait for ``Calculator`` to delegate to the original method.

2. **Create an Instrumented Struct**:

.. code-block:: rust
   :caption: src/lib.rs
   :name: instrumented-calculator

   /// Instrumented version of Calculator for testing.
   pub struct InstrumentedCalculator {
       proxy: TestProxy<Calculator>,
   }

   impl InstrumentedCalculator {
       /// Creates a new instrumented calculator.
       pub fn new(tracker: TestTracker) -> Self {
           InstrumentedCalculator {
               proxy: TestProxy::new(tracker),
           }
       }
   }

   impl Calculate for InstrumentedCalculator {
       fn add(&self, x: i32, y: i32) -> i32 {
           self.proxy.wrap(|_| {
               let calc = Calculator;
               calc.add(x, y)
           }, x, "Calculator::add") // Tracks only x as input
       }
   }

**Description**:

- Wraps the ``add`` method with ``proxy.wrap``.
- Note that ``wrap`` currently tracks only the first input (``x``). To track both ``x`` and ``y``, modify ``TestProxy`` to handle multiple inputs (see Limitations).

3. **Create a Factory**:

.. code-block:: rust
   :caption: src/lib.rs
   :name: calculator-factory

   /// Factory for creating Calculator instances.
   pub struct CalculatorFactory;

   impl CalculatorFactory {
       /// Creates either an original or instrumented Calculator.
       pub fn create(use_instrumented: bool, tracker: Option<TestTracker>) -> Box<dyn Calculate> {
           if use_instrumented {
               Box::new(InstrumentedCalculator::new(tracker.unwrap_or_default()))
           } else {
               Box::new(Calculator)
           }
       }
   }

**Description**:

- Provides runtime switching between ``Calculator`` and ``InstrumentedCalculator``.
- Returns a ``Box<dyn Calculate>`` for polymorphic usage.

4. **Write a Test**:

.. code-block:: rust
   :caption: tests/integration.rs
   :name: calculator-test

   #[test]
   fn test_calculator() {
       let tracker = TestTracker::new();
       let calc = CalculatorFactory::create(true, Some(tracker.clone()));
       assert_eq!(calc.add(3, 4), 7);
       assert_eq!(
           tracker.get_call_stack(),
           vec!["Enter Calculator::add", "Exit Calculator::add"]
       );
       let values = tracker.get_values();
       assert_eq!(values.get("Calculator::add_input"), Some(&3));
       assert_eq!(values.get("Calculator::add_output"), Some(&7));
   }

**Description**:

- Tests the ``add`` method of an instrumented ``Calculator``.
- Verifies the result (``3 + 4 = 7``).
- Checks the call stack and stored values.
- Note that only ``x`` (``3``) is tracked as input due to the current ``TestProxy`` design.

Limitations and Considerations
-----------------------------

While the framework is powerful, it has some limitations and considerations to keep in mind:

- **Performance Overhead**:

  - The ``TestProxy`` and ``TestTracker`` introduce overhead due to mutex locking and data copying.
  - **Solution**: Use original structs in production via the factory to eliminate this overhead.

- **Single Input Tracking**:

  - The ``TestProxy::wrap`` method tracks only one input parameter (``x``), which may not suffice for methods with multiple inputs (e.g., ``Calculator::add(x, y)``).
  - **Solution**: Modify ``TestProxy`` to accept a tuple or custom struct for multiple inputs. For example:

    .. code-block:: rust
       :caption: src/lib.rs
       :name: modified-proxy

       impl<T> TestProxy<T> {
           pub fn wrap<F, R>(&self, method: F, inputs: (i32, i32), name: &str) -> R
           where
               F: Fn(i32, i32) -> R,
               R: Copy + Into<i32>,
           {
               self.tracker.push_call(&format!("Enter {}", name));
               self.tracker.insert_value(&format!("{}_input_x", name), inputs.0);
               self.tracker.insert_value(&format!("{}_input_y", name), inputs.1);
               let result = method(inputs.0, inputs.1);
               self.tracker.push_call(&format!("Exit {}", name));
               self.tracker.insert_value(&format!("{}_output", name), result.into());
               result
           }
       }

- **Thread Safety**:

  - The ``TestTracker`` uses ``Arc<Mutex<>>`` for thread safety, but concurrent tests may still require careful synchronization.
  - **Solution**: Ensure tests run sequentially or use separate trackers for concurrent tests.

- **Fault Injection**:

  - Faults are simulated via panics, which may not cover all failure modes (e.g., returning errors).
  - **Solution**: Extend ``TestProxy::wrap`` to return a ``Result`` or custom error type for more flexible fault simulation.

- **Method Identifier Uniqueness**:

  - The string passed to ``proxy.wrap`` (e.g., ``"MyClass::my_method"``) must be unique to avoid tracking conflicts.
  - **Solution**: Use a consistent naming convention, such as ``"StructName::method_name"``.

Conclusion
----------

The proxy test framework provides a robust solution for testing Rust code without modifying the original implementation. By leveraging traits, instrumented structs, and a factory pattern, it enables detailed execution tracking, data flow analysis, and fault injection. This tutorial has demonstrated how to apply the framework to both existing and new code, with comprehensive examples formatted for Sphinx documentation.

To use this framework effectively:

- Define clear traits for all testable structs.
- Implement instrumented structs with proxy wrappers.
- Use factories to isolate test instrumentation from production.
- Write thorough tests covering unit, integration, data flow, and fault scenarios.
- Address limitations (e.g., multiple input tracking) as needed for your use case.

By following these steps and best practices, you can achieve robust testing while maintaining a clean and maintainable codebase.