.. Detailed Description: Functionality and Configuration
   ====================================================

.. need:: Requirement
   :id: REQ_DETAILED
   :title: Detailed System Description
   :status: open
   :tags: functionality; configuration
   :links: REQ_OVERVIEW
   :description: A comprehensive description of the Rust Proxy Demonstrator’s functionality, steps, and configurations, including PlantUML diagrams.

System Overview
---------------

The Rust Proxy Demonstrator is a testing framework for Rust classes (`Class1`, `Class2`, `Class3`) using a proxy pattern. It includes 39 unit tests (`src/lib.rs`) and 12 integration tests (`tests/integration.rs`) to verify functionality and fault injection, leveraging `rstest` for parameterized testing and `mockall` for mocking.

.. uml::
   :caption: Class Relationships

   @startuml
   class Class1 {
     +offset: i32
     +counter: i32
     +c3: Class3
     +execute(x: i32): i32
     +compute(factor: f64, count: i32): f64
     +get_counter(): i32
   }
   class Class2 {
     +offset: i32
     +name: String
     +c3: Class3
     +transform(x: i32): i32
     +combine(x: i32, label: &str): String
     +get_name(): String
   }
   class Class3 {
     +offset: i32
     +name: String
     +process(x: i32): i32
     +scale(factor: f64, count: i32): f64
     +describe(): String
   }
   Class1 --> Class3
   Class2 --> Class3
   class TestTracker {
     +call_stack: Vec<String>
     +values: HashMap<String, String>
     +inject_fault: Option<String>
     +push_call(call: &str)
     +insert_value(key: &str, value: T)
     +set_inject_fault(target: &str)
     +inject_fault(target: &str): bool
   }
   class InstrumentedClass1 {
     +inner: Class1
     +tracker: Option<TestTracker>
     +execute(x: i32): i32
     +compute(factor: f64, count: i32): f64
     +get_counter(): i32
   }
   class TestClass1 {
     +inner: Class1
     +instrumented: InstrumentedClass1
     +counter: i32
     +execute(x: i32): i32
     +compute(factor: f64, count: i32): f64
     +get_counter(): i32
     +proxy_execute(x: i32): i32
     +proxy_compute(factor: f64, count: i32): f64
     +proxy_get_counter(): i32
   }
   InstrumentedClass1 --> TestTracker
   TestClass1 --> InstrumentedClass1
   TestClass1 --> Class1
   @enduml

.. note:: To view the PlantUML diagram, copy the code between `@startuml` and `@enduml` into a PlantUML viewer like https://www.plantuml.com/plantuml/ or a VS Code PlantUML extension.

Functionality
-------------

.. need:: Specification
   :id: SPEC_FUNCTIONALITY
   :title: System Functionality
   :status: implemented
   :tags: functionality
   :links: REQ_DETAILED
   :description: Describes the core functionality of the Rust Proxy Demonstrator.

The system provides a proxy-based testing framework for three classes:

1. **Class1**:
   - Implements `execute` for integer computations and `compute` for floating-point operations, delegating to `Class3::process` and `Class3::scale`.
   - Example: `execute(x)` computes `((x * 2 + 5) * 2) * 3`.

2. **Class2**:
   - Implements `transform` for integer transformations and `combine` for labeled outputs, delegating to `Class3::process`.
   - Example: `transform(x)` computes `(x * 2 + 5) * 2`.

3. **Class3**:
   - Core processing unit with `process` (integer transformation), `scale` (floating-point scaling), and `describe` (description).
   - Example: `process(x)` computes `x * 2 + 5`.

**Fault Injection**:
- The `TestTracker` class enables fault injection by setting `inject_fault` for specific methods (e.g., `Class1::execute`).
- `InstrumentedClass*` wrappers log calls and trigger panics when faults are injected.

**Testing**:
- **Unit Tests**: 39 tests in `src/lib.rs` verify individual methods and fault injection scenarios.
- **Integration Tests**: 12 tests in `tests/integration.rs` validate class interactions, e.g., `execute_to_transform` tests `Class1::execute` followed by `Class2::transform`.

Configuration Steps
-------------------

.. need:: Configuration
   :id: CONFIG_STEPS
   :title: Configuration Steps
   :status: implemented
   :tags: configuration
   :links: REQ_DETAILED
   :description: Details the configuration steps for setting up the Rust Proxy Demonstrator.

1. **Cargo.toml**:
   - Defines dependencies and dev-dependencies:
     .. code-block:: toml

        [package]
        name = "rust_proxy"
        version = "0.1.0"
        edition = "2021"

        [dependencies]
        mockall = "0.13.0"

        [dev-dependencies]
        rstest = "0.18.0"
   - Fetch dependencies:
     .. code-block:: powershell

        cargo build

2. **VS Code Setup**:
   - The `.vscode/` directory includes `launch.json` and `tasks.json` for debugging:
     .. code-block:: json

        // launch.json
        {
            "version": "0.2.0",
            "configurations": [
                {
                    "type": "lldb",
                    "request": "launch",
                    "name": "Debug unit tests",
                    "cargo": {
                        "args": ["test", "--lib", "--no-run"],
                        "filter": { "name": "rust_proxy", "kind": "lib" }
                    },
                    "args": [],
                    "cwd": "${workspaceFolder}"
                }
            ]
        }
   - Install the Rust-Analyzer extension in VS Code for code completion and debugging.

3. **Project Structure**:
   - Ensure the structure matches:
     .. code-block:: text

        rust_proxy/
        ├── Cargo.toml
        ├── src/
        │   ├── lib.rs
        │   ├── main.rs
        │   ├── class1.rs
        │   ├── class2.rs
        │   ├── class3.rs
        │   ├── test_tracker.rs
        │   ├── test_class1.rs
        │   ├── test_class2.rs
        │   ├── test_class3.rs
        ├── tests/
        │   ├── integration.rs
        ├── .vscode/
        │   ├── launch.json
        │   ├── tasks.json
   - Verify:
     .. code-block:: powershell

        dir
        dir src
        dir tests
        dir .vscode

.. uml::
   :caption: Configuration Workflow

   @startuml
   actor User
   participant "Cargo" as Cargo
   participant "VS Code" as VSCode
   participant "Project Files" as Files

   User -> Cargo: cargo build
   Cargo -> Files: Compile src/*.rs
   Files --> Cargo: Build artifacts
   Cargo --> User: Build complete
   User -> VSCode: Open project
   VSCode -> Files: Load src/, tests/
   User -> VSCode: Install Rust-Analyzer
   VSCode --> User: Code completion, debugging
   User -> Cargo: cargo test --all
   Cargo -> Files: Run tests
   Files --> Cargo: Test results
   Cargo --> User: Test results
   @enduml

Step-by-Step Functionality
--------------------------

.. need:: Specification
   :id: SPEC_STEPS
   :title: Step-by-Step Functionality
   :status: implemented
   :tags: functionality
   :links: REQ_DETAILED
   :description: Explains each step of the system’s functionality in detail.

1. **Initialization**:
   - Classes (`Class1`, `Class2`, `Class3`) are instantiated with default values (e.g., `offset = 5`).
   - `TestClass*` and `InstrumentedClass*` wrap the classes for testing and fault injection.

2. **Test Execution**:
   - Unit tests in `src/lib.rs` use `rstest` for parameterized testing, verifying method outputs and fault injection.
   - Integration tests in `tests/integration.rs` validate class interactions, e.g., `execute_to_transform` tests `Class1::execute` followed by `Class2::transform`.

3. **Fault Injection**:
   - `TestTracker::set_inject_fault` specifies a method to fail (e.g., `Class1::execute`).
   - `InstrumentedClass*` methods check for faults and panic if triggered, simulating failures.

4. **Result Verification**:
   - Assertions verify outputs (e.g., `assert_eq!(result, 226)` in `execute_to_transform`).
   - Fault injection tests confirm panics using `std::panic::catch_unwind`.

Applying to Other Sources
-------------------------

.. need:: Specification
   :id: SPEC_APPLY_DETAILED
   :title: Detailed Application to Other Sources
   :status: implemented
   :tags: functionality; adaptation
   :links: REQ_DETAILED; SPEC_APPLY
   :description: Detailed explanation of adapting the system to other sources.

To adapt the system to other sources:

1. **Modify Core Logic**:
   - Update `Class*.rs` to integrate new data sources or logic. For example, modify `Class3::process` to use a database:
     .. code-block:: rust

        fn process(&mut self, x: i32) -> i32 {
            let db_value = query_database(); // Hypothetical
            x.wrapping_mul(db_value).wrapping_add(self.offset)
        }
   - Adjust `InstrumentedClass*` in `src/test_class*.rs` to log new inputs.

2. **Extend Tests**:
   - Add tests in `src/lib.rs` or `tests/integration.rs` to cover new logic.
   - Example: New unit test for modified `Class3::process`:
     .. code-block:: rust

        #[rstest]
        fn unit_new_process(tracker: TestTracker) {
            let c3 = class3::Class3::new();
            let mut tc3 = test_class3::TestClass3::new(
                c3.clone(),
                test_class3::InstrumentedClass3::new(c3.clone(), Some(tracker.clone())),
            );
            let result = tc3.process(10);
            assert_eq!(result, /* expected value */);
        }

3. **Integrate into New Projects**:
   - Copy `src/` and `tests/` to a new project:
     .. code-block:: powershell

        cargo new new_project
        cd new_project
        Copy-Item -Recurse -Path D:\Temp\Qorix\Proxy Test\RUST\rust_proxy\src -Destination src
        Copy-Item -Recurse -Path D:\Temp\Qorix\Proxy Test\RUST\rust_proxy\tests -Destination tests
   - Update `Cargo.toml` and adapt files.
   - Run tests:
     .. code-block:: powershell

        cargo test --all