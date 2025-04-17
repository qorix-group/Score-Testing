```cpp
// Include Google Test for unit testing framework
#include <gtest/gtest.h>
// Include Google Mock for creating mock objects
#include <gmock/gmock.h>
// Include standard library containers for tracking calls and values
#include <vector>
#include <string>
#include <map>
// Include smart pointers for factory-based runtime switching
#include <memory>
// Include exception handling for fault injection
#include <stdexcept>
// Include threading utilities for thread safety and delays
#include <mutex>
#include <thread>
#include <chrono>

// --- Original Classes (Unchanged) ---

// Class3: Performs innermost computation with configurable scaling
// Purpose: Represents the lowest-level component in the computation chain
class Class3 {
public:
    // Member variable: scale_factor determines multiplication factor
    // Purpose: Allows configuration of process method behavior
    const int scale_factor;

    // Constructor: Initializes scale_factor with a default value
    // Purpose: Enables flexible configuration of Class3 instances
    explicit Class3(int scale = 2) : scale_factor(scale) {}

    // process: Multiplies input x by scale_factor
    // Purpose: Core computation for Class3, used by Class2
    int process(int x) const { return x * scale_factor; } // Step 3: x * scale_factor

    // describe: Returns a string description of the processed value
    // Purpose: Provides an alternative output format for testing
    std::string describe(int x) const {
        return "Class3 processed: " + std::to_string(x * scale_factor);
    }

    // validate: Checks if x * scale_factor meets a threshold
    // Purpose: Adds a boolean method with multiple parameters for testing
    bool validate(int x, int threshold) const {
        return x * scale_factor >= threshold;
    }
};

// Class2: Depends on Class3, adds an offset to the result
// Purpose: Intermediate component in the computation chain
class Class2 {
public:
    // Member variable: offset to add in transform
    // Purpose: Configures the transform method's behavior
    const int offset;

    // Constructor: Initializes offset with a default value
    // Purpose: Allows flexible configuration of Class2
    explicit Class2(int offset = 1) : offset(offset) {}

    // transform: Adds offset to Class3::process result
    // Purpose: Combines Class3's output with local configuration
    int transform(int x) const {
        Class3 c3; // Create Class3 with default scale_factor=2
        return c3.process(x) + offset; // Step 2: (x*scale_factor) + offset
    }
};

// Class1: Depends on Class2, applies a multiplier
// Purpose: Top-level component orchestrating the computation
class Class1 {
public:
    // Member variable: multiplier for execute
    // Purpose: Configures the execute method's scaling
    const int multiplier;

    // Constructor: Initializes multiplier with a default value
    // Purpose: Enables flexible configuration of Class1
    explicit Class1(int multiplier = 3) : multiplier(multiplier) {}

    // execute: Multiplies Class2::transform result by multiplier
    // Purpose: Final computation step in the chain
    int execute(int x) const {
        Class2 c2; // Create Class2 with default offset=1
        return c2.transform(x) * multiplier; // Step 1: ((x*scale_factor)+offset)*multiplier
    }

    // summarize: Returns a string summary via Class2 and Class3
    // Purpose: Tests string-based method interactions
    std::string summarize(int x) const {
        Class2 c2;
        Class3 c3;
        return c3.describe(c2.transform(x));
    }
};

// --- Test Infrastructure ---

// TestTracker: Tracks control and data flow, thread-safe
// Purpose: Central logging mechanism for test verification
class TestTracker {
public:
    // call_stack: Stores sequence of method calls (entry, exit, faults)
    // Purpose: Tracks control flow for test assertions
    std::vector<std::string> call_stack;

    // values: Stores input/output values as strings for flexibility
    // Purpose: Tracks data flow across methods
    std::map<std::string, std::string> values;

    // mutex: Ensures thread-safe access to call_stack and values
    // Purpose: Supports parallel test execution
    std::mutex mutex;

    // reset: Clears call_stack and values
    // Purpose: Ensures clean state for each test
    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        call_stack.clear();
        values.clear();
    }

    // push_call: Adds a call to call_stack
    // Purpose: Logs method entry, exit, or fault events
    void push_call(const std::string& call) {
        std::lock_guard<std::mutex> lock(mutex);
        call_stack.push_back(call);
    }

    // insert_value: Stores a key-value pair
    // Purpose: Records input/output data for verification
    void insert_value(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex);
        values[key] = value;
    }
};

// --- Proxy Layer ---

// FaultType: Enum for different fault injection types
// Purpose: Enables flexible fault simulation (exceptions, custom returns, delays)
enum class FaultType {
    None,        // No fault
    Exception,   // Throw an exception
    CustomReturn,// Return a custom value
    Delay        // Introduce a delay
};

// TestProxy: Instruments method calls for a given type T
// Purpose: Intercepts method calls to log, inject faults, and control execution
template<typename T>
class TestProxy {
public:
    // tracker: Reference to TestTracker for logging
    // Purpose: Links proxy to the tracking system
    TestTracker& tracker;

    // fault_type: Type of fault to inject
    // Purpose: Determines fault behavior
    FaultType fault_type;

    // fault_target: Method to inject fault into
    // Purpose: Specifies which method to target
    std::string fault_target;

    // custom_return_int: Custom return value for int methods
    // Purpose: Allows simulating specific int outputs
    int custom_return_int;

    // custom_return_string: Custom return value for string methods
    // Purpose: Allows simulating specific string outputs
    std::string custom_return_string;

    // custom_return_bool: Custom return value for bool methods
    // Purpose: Allows simulating specific bool outputs
    bool custom_return_bool;

    // delay_ms: Delay duration in milliseconds
    // Purpose: Simulates latency for delay faults
    int delay_ms;

    // Constructor: Initializes with TestTracker
    // Purpose: Sets up proxy with default fault settings
    explicit TestProxy(TestTracker& t)
        : tracker(t), fault_type(FaultType::None), fault_target(""),
          custom_return_int(0), custom_return_string(""), custom_return_bool(false), delay_ms(0) {}

    // set_fault: Configures fault injection
    // Purpose: Allows dynamic fault setup for tests
    void set_fault(FaultType type, const std::string& target, int delay = 0,
                   int ret_int = 0, const std::string& ret_string = "", bool ret_bool = false) {
        fault_type = type;
        fault_target = target;
        delay_ms = delay;
        custom_return_int = ret_int;
        custom_return_string = ret_string;
        custom_return_bool = ret_bool;
    }

    // wrap_int: Wraps int-returning methods with one int parameter
    // Purpose: Instruments int methods for logging and fault injection
    template<typename Method>
    int wrap_int(Method method, const T* obj, int x, const std::string& name) {
        // Log method entry and input
        tracker.push_call("Enter " + name);
        tracker.insert_value(name + "_input", std::to_string(x));

        // Handle fault injection
        if (fault_type != FaultType::None && fault_target == name) {
            // Log fault event with fault type
            tracker.push_call("FAULT INJECTED: " + std::to_string(static_cast<int>(fault_type)));
            if (fault_type == FaultType::Exception) {
                // Throw exception to simulate failure
                throw std::runtime_error("Fault injected in " + name);
            } else if (fault_type == FaultType::Delay) {
                // Simulate latency
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            } else if (fault_type == FaultType::CustomReturn) {
                // Return custom value
                tracker.insert_value(name + "_output", std::to_string(custom_return_int));
                return custom_return_int;
            }
        }

        // Execute original method
        int result = (obj->*method)(x);

        // Log exit and output
        tracker.push_call("Exit " + name);
        tracker.insert_value(name + "_output", std::to_string(result));
        return result;
    }

    // wrap_string: Wraps string-returning methods with one int parameter
    // Purpose: Instruments string methods for logging and fault injection
    template<typename Method>
    std::string wrap_string(Method method, const T* obj, int x, const std::string& name) {
        // Log method entry and input
        tracker.push_call("Enter " + name);
        tracker.insert_value(name + "_input", std::to_string(x));

        // Handle fault injection
        if (fault_type != FaultType::None && fault_target == name) {
            // Log fault event
            tracker.push_call("FAULT INJECTED: " + std::to_string(static_cast<int>(fault_type)));
            if (fault_type == FaultType::Exception) {
                // Throw exception
                throw std::runtime_error("Fault injected in " + name);
            } else if (fault_type == FaultType::Delay) {
                // Simulate latency
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            } else if (fault_type == FaultType::CustomReturn) {
                // Return custom value
                tracker.insert_value(name + "_output", custom_return_string);
                return custom_return_string;
            }
        }

        // Execute original method
        std::string result = (obj->*method)(x);

        // Log exit and output
        tracker.push_call("Exit " + name);
        tracker.insert_value(name + "_output", result);
        return result;
    }

    // wrap_bool: Wraps bool-returning methods with two int parameters
    // Purpose: Instruments bool methods for logging and fault injection
    template<typename Method>
    bool wrap_bool(Method method, const T* obj, int x, int y, const std::string& name) {
        // Log entry and inputs
        tracker.push_call("Enter " + name);
        tracker.insert_value(name + "_input_1", std::to_string(x));
        tracker.insert_value(name + "_input_2", std::to_string(y));

        // Handle fault injection
        if (fault_type != FaultType::None && fault_target == name) {
            // Log fault event
            tracker.push_call("FAULT INJECTED: " + std::to_string(static_cast<int>(fault_type)));
            if (fault_type == FaultType::Exception) {
                // Throw exception
                throw std::runtime_error("Fault injected in " + name);
            } else if (fault_type == FaultType::Delay) {
                // Simulate latency
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            } else if (fault_type == FaultType::CustomReturn) {
                // Return custom value
                tracker.insert_value(name + "_output", custom_return_bool ? "true" : "false");
                return custom_return_bool;
            }
        }

        // Execute original method
        bool result = (obj->*method)(x, y);

        // Log exit and output
        tracker.push_call("Exit " + name);
        tracker.insert_value(name + "_output", result ? "true" : "false");
        return result;
    }
};

// --- Proxied Methods ---

// PROXY_METHOD_INT: Macro for int-returning methods
// Purpose: Simplifies instrumentation of int methods
#define PROXY_METHOD_INT(Class, Method) \
    int Method(int x) const { \
        return proxy_.wrap_int(&Class::Method, this, x, #Class "::" #Method); \
    }

// PROXY_METHOD_STRING: Macro for string-returning methods
// Purpose: Simplifies instrumentation of string methods
#define PROXY_METHOD_STRING(Class, Method) \
    std::string Method(int x) const { \
        return proxy_.wrap_string(&Class::Method, this, x, #Class "::" #Method); \
    }

// PROXY_METHOD_BOOL: Macro for bool-returning methods with two parameters
// Purpose: Simplifies instrumentation of bool methods
#define PROXY_METHOD_BOOL(Class, Method) \
    bool Method(int x, int y) const { \
        return proxy_.wrap_bool(&Class::Method, this, x, y, #Class "::" #Method); \
    }

// --- Instrumented Classes ---

// DEFINE_INSTRUMENTED_CLASS: Macro to generate instrumented classes
// Purpose: Automates creation of instrumented classes to reduce boilerplate
#define DEFINE_INSTRUMENTED_CLASS(BaseClass, ...) \
class Instrumented##BaseClass : public BaseClass { \
public: \
    TestProxy<Instrumented##BaseClass> proxy_; \
    explicit Instrumented##BaseClass(TestTracker& tracker, ##__VA_ARGS__) \
        : BaseClass(__VA_ARGS__), proxy_(tracker) {} \
    PROXY_METHOD_INT(BaseClass, execute) \
    PROXY_METHOD_STRING(BaseClass, summarize) \
}; \
class Instrumented##BaseClass##NoSummarize : public BaseClass { \
public: \
    TestProxy<Instrumented##BaseClass##NoSummarize> proxy_; \
    explicit Instrumented##BaseClass##NoSummarize(TestTracker& tracker, ##__VA_ARGS__) \
        : BaseClass(__VA_ARGS__), proxy_(tracker) {} \
    PROXY_METHOD_INT(BaseClass, transform) \
}; \
class Instrumented##BaseClass##NoExecuteNoSummarize : public BaseClass { \
public: \
    TestProxy<Instrumented##BaseClass##NoExecuteNoSummarize> proxy_; \
    explicit Instrumented##BaseClass##NoExecuteNoSummarize(TestTracker& tracker, ##__VA_ARGS__) \
        : BaseClass(__VA_ARGS__), proxy_(tracker) {} \
    PROXY_METHOD_INT(BaseClass, process) \
    PROXY_METHOD_STRING(BaseClass, describe) \
    PROXY_METHOD_BOOL(BaseClass, validate) \
};

// Define instrumented classes for Class1, Class2, Class3
// Purpose: Creates instrumented versions for testing
DEFINE_INSTRUMENTED_CLASS(Class1, int multiplier = 3)
DEFINE_INSTRUMENTED_CLASS(Class2, int offset = 1)
DEFINE_INSTRUMENTED_CLASS(Class3, int scale = 2)

// --- Factory for Runtime Switching ---

// Class1Factory: Creates original or instrumented Class1 instances
// Purpose: Enables runtime switching between production and test code
class Class1Factory {
public:
    // create: Returns Class1 or InstrumentedClass1
    // Purpose: Supports toggling instrumentation with custom configuration
    static std::unique_ptr<Class1> create(bool use_instrumented, TestTracker& tracker, int multiplier = 3) {
        return use_instrumented ? std::make_unique<InstrumentedClass1>(tracker, multiplier)
                               : std::make_unique<Class1>(multiplier);
    }
};

// --- Mock Classes with Google Mock ---

// MockClass2: Mocks Class2 interface
// Purpose: Allows isolated testing of Class1
class MockClass2 : public Class2 {
public:
    // Constructor: Sets default offset
    MockClass2() : Class2(1) {}
    // MOCK_METHOD: Defines mock for transform
    MOCK_CONST_METHOD1(transform, int(int));
};

// MockClass3: Mocks Class3 interface
// Purpose: Allows isolated testing of Class2
class MockClass3 : public Class3 {
public:
    // Constructor: Sets default scale_factor
    MockClass3() : Class3(2) {}
    // MOCK_METHODs: Define mocks for process, describe, validate
    MOCK_CONST_METHOD1(process, int(int));
    MOCK_CONST_METHOD1(describe, std::string(int));
    MOCK_CONST_METHOD2(validate, bool(int, int));
};

// --- Test Fixture ---

// DataFlowTest: Google Test fixture for test setup and teardown
// Purpose: Manages TestTracker and ensures test isolation
class DataFlowTest : public ::testing::Test {
protected:
    // tracker: Thread-safe TestTracker instance
    // Purpose: Shared logging for all tests
    TestTracker tracker;

    // SetUp: Called before each test
    // Purpose: No static initialization needed due to instance-based TestProxy
    void SetUp() override {}

    // TearDown: Resets tracker after each test
    // Purpose: Ensures clean state between tests
    void TearDown() override {
        tracker.reset();
    }
};

// --- Tests ---

// NormalFlow: Tests full computation chain with default settings
// Purpose: Verifies control and data flow for the main computation
TEST_F(DataFlowTest, NormalFlow) {
    // Create instrumented Class1 with default multiplier=3
    auto c1 = Class1Factory::create(true, tracker);
    // Execute with input 2, expect ((2*2)+1)*3 = 15
    EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 3);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "Enter Class3::process",
        "Exit Class3::process",
        "Exit Class2::transform",
        "Exit Class1::execute"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
    // Verify data flow
    EXPECT_EQ(tracker.values["Class3::process_input"], "2");
    EXPECT_EQ(tracker.values["Class3::process_output"], "4");
    EXPECT_EQ(tracker.values["Class2::transform_output"], "5");
    EXPECT_EQ(tracker.values["Class1::execute_output"], "15");
}

// StringMethodFlow: Tests summarize method
// Purpose: Verifies string-based method interactions
TEST_F(DataFlowTest, StringMethodFlow) {
    // Create instrumented Class1
    auto c1 = Class1Factory::create(true, tracker);
    // Call summarize with input 2
    EXPECT_EQ(c1->summarize(2), "Class3 processed: 5");
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::summarize",
        "Enter Class2::transform",
        "Enter Class3::process",
        "Exit Class3::process",
        "Exit Class2::transform",
        "Enter Class3::describe",
        "Exit Class3::describe",
        "Exit Class1::summarize"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
    // Verify output
    EXPECT_EQ(tracker.values["Class3::describe_output"], "Class3 processed: 5");
}

// BoolMethodFlow: Tests validate method
// Purpose: Verifies boolean method with multiple parameters
TEST_F(DataFlowTest, BoolMethodFlow) {
    // Create instrumented Class3
    InstrumentedClass3NoExecuteNoSummarize c3(tracker);
    // Call validate with x=3, threshold=5
    EXPECT_TRUE(c3.validate(3, 5)); // 3*2 >= 5
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class3::validate",
        "Exit Class3::validate"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
    // Verify inputs and output
    EXPECT_EQ(tracker.values["Class3::validate_input_1"], "3");
    EXPECT_EQ(tracker.values["Class3::validate_input_2"], "5");
    EXPECT_EQ(tracker.values["Class3::validate_output"], "true");
}

// FaultInjectionException: Tests exception fault in Class3::process
// Purpose: Verifies error handling with exception injection
TEST_F(DataFlowTest, FaultInjectionException) {
    // Create instrumented Class3
    InstrumentedClass3NoExecuteNoSummarize c3(tracker);
    // Configure exception fault
    c3.proxy_.set_fault(FaultType::Exception, "Class3::process");
    // Create instrumented Class1
    auto c1 = Class1Factory::create(true, tracker);
    // Expect exception when executing
    EXPECT_THROW(c1->execute(2), std::runtime_error);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "Enter Class3::process",
        "FAULT INJECTED: 1"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
}

// FaultInjectionCustomReturn: Tests custom return fault in Class2::transform
// Purpose: Verifies behavior with simulated return values
TEST_F(DataFlowTest, FaultInjectionCustomReturn) {
    // Create instrumented Class2
    InstrumentedClass2NoSummarize c2(tracker);
    // Configure custom return fault (return 100)
    c2.proxy_.set_fault(FaultType::CustomReturn, "Class2::transform", 0, 100);
    // Create instrumented Class1
    auto c1 = Class1Factory::create(true, tracker);
    // Execute, expect 100*3 = 300
    EXPECT_EQ(c1->execute(2), 100 * 3);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "FAULT INJECTED: 2",
        "Exit Class1::execute"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
    // Verify output
    EXPECT_EQ(tracker.values["Class2::transform_output"], "100");
}

// FaultInjectionDelay: Tests delay fault in Class3::validate
// Purpose: Verifies behavior with simulated latency
TEST_F(DataFlowTest, FaultInjectionDelay) {
    // Create instrumented Class3
    InstrumentedClass3NoExecuteNoSummarize c3(tracker);
    // Configure delay fault (100ms)
    c3.proxy_.set_fault(FaultType::Delay, "Class3::validate", 100);
    // Measure execution time
    auto start = std::chrono::steady_clock::now();
    c3.validate(3, 5);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    // Expect duration >= 100ms
    EXPECT_GE(duration, 100);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class3::validate",
        "FAULT INJECTED: 3",
        "Exit Class3::validate"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
}

// MemberVariableImpact: Tests effect of custom member variables
// Purpose: Verifies configuration impacts computation
TEST_F(DataFlowTest, MemberVariableImpact) {
    // Create instrumented Class1 with multiplier=4
    auto c1 = Class1Factory::create(true, tracker, 4);
    // Execute, expect ((2*2)+1)*4 = 20
    EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 4);
    // Verify data flow
    EXPECT_EQ(tracker.values["Class3::process_input"], "2");
    EXPECT_EQ(tracker.values["Class3::process_output"], "4");
    EXPECT_EQ(tracker.values["Class2::transform_output"], "5");
    EXPECT_EQ(tracker.values["Class1::execute_output"], "20");
}

// MockDependency: Tests Class1 with mocked Class2
// Purpose: Verifies isolated testing of Class1
TEST_F(DataFlowTest, MockDependency) {
    // Create mock Class2
    MockClass2 mock_c2;
    // Configure mock to return 10 for transform(2)
    EXPECT_CALL(mock_c2, transform(2)).WillOnce(testing::Return(10));
    // Create instrumented Class1
    auto c1 = Class1Factory::create(true, tracker);
    // Execute with mock, expect 10*3 = 30
    int result = mock_c2.transform(2) * c1->multiplier;
    EXPECT_EQ(result, 30);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_output"], "30");
}

// MockFullChain: Tests Class1 with mocked Class2 and Class3
// Purpose: Verifies isolated testing of the full chain
TEST_F(DataFlowTest, MockFullChain) {
    // Create mocks
    MockClass2 mock_c2;
    MockClass3 mock_c3;
    // Configure mock behaviors
    EXPECT_CALL(mock_c3, process(2)).WillOnce(testing::Return(8));
    EXPECT_CALL(mock_c2, transform(2)).WillOnce(testing::Return(9));
    // Create instrumented Class1
    auto c1 = Class1Factory::create(true, tracker);
    // Execute with mock, expect 9*3 = 27
    int result = mock_c2.transform(2) * c1->multiplier;
    EXPECT_EQ(result, 27);
    // Define expected call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute"
    };
    // Verify call stack
    ASSERT_EQ(tracker.call_stack, expected);
}

// OriginalClassNoTracking: Tests original Class1 without instrumentation
// Purpose: Verifies production code runs without test overhead
TEST_F(DataFlowTest, OriginalClassNoTracking) {
    // Create original Class1
    auto c1 = Class1Factory::create(false, tracker);
    // Execute with input 2
    EXPECT_EQ(c1->execute(2), ((2 * 2) + 1) * 3);
    // Verify no tracking occurred
    EXPECT_TRUE(tracker.call_stack.empty());
}

// Main function: Initializes and runs Google Test
// Purpose: Entry point for test execution
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```