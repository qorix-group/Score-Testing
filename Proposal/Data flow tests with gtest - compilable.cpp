#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <utility>
#include <iostream>
#include <iomanip>
#include <sstream>

// Forward declarations
class Class1;
class Class2;
class Class3;
class InstrumentedClass1;
class InstrumentedClass2;
class InstrumentedClass3;

// Test Infrastructure
class TestTracker {
public:
    std::vector<std::string> call_stack;
    std::map<std::string, std::string> values;

    void reset() {
        call_stack.clear();
        values.clear();
    }

    void push_call(const std::string& call) {
        call_stack.push_back(call);
    }

    void insert_value(const std::string& key, int value) {
        values[key] = std::to_string(value);
    }

    void insert_value(const std::string& key, double value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << value;
        std::string str = oss.str();
        // Remove trailing zeros and decimal point if unnecessary
        str.erase(str.find_last_not_of('0') + 1);
        if (str.back() == '.') str.pop_back();
        values[key] = str;
    }

    void insert_value(const std::string& key, const std::string& value) {
        values[key] = value;
    }
};

// Global tracker for all proxy classes
static TestTracker* global_tracker = nullptr;

// Factory for runtime switching
class Factory {
public:
    static bool use_instrumented;

    static std::unique_ptr<Class1> create_class1();
    static std::unique_ptr<Class2> create_class2();
    static std::unique_ptr<Class3> create_class3();
};

// Static member initialization
bool Factory::use_instrumented = false;

// Original Classes (no virtual methods)
class Class3 {
public:
    Class3() : offset_(5), name_("Class3") {}

    int process(int x) const {
        return x * 2 + offset_; // Step 3: x*2 + offset
    }

    double scale(double factor, int count) const {
        return factor * count + offset_; // Additional method
    }

    std::string describe() const {
        return name_ + ": Processing unit"; // Uses attribute
    }

private:
    int offset_; // Attribute affecting logic
    std::string name_;
};

class Class2 {
public:
    Class2() : multiplier_(2), name_("Class2") {}

    int transform(int x) const {
        auto c3 = Factory::create_class3();
        return c3->process(x) * multiplier_; // Step 2: (x*2 + offset) * multiplier
    }

    std::string combine(int x, const std::string& label) const {
        auto c3 = Factory::create_class3();
        return c3->describe() + " | " + label + " | " + std::to_string(x); // Combines attribute and input
    }

    std::string get_name() const {
        return name_;
    }

private:
    int multiplier_; // Attribute affecting logic
    std::string name_;
};

class Class1 {
public:
    Class1() : factor_(3), counter_(0) {}

    int execute(int x) const {
        auto c2 = Factory::create_class2();
        counter_++; // Modify attribute
        return c2->transform(x) * factor_; // Step 1: ((x*2 + offset) * multiplier) * factor
    }

    double compute(double value, int count) const {
        auto c2 = Factory::create_class2();
        auto c3 = Factory::create_class3();
        counter_++; // Modify attribute
        return c3->scale(value, count) * c2->get_name().length(); // Combines results and attribute
    }

    int get_counter() const {
        return counter_;
    }

protected:
    int factor_; // Attribute affecting logic
    mutable int counter_; // Tracks method calls
};

// Proxy Layer
template<typename T>
class TestProxy {
public:
    static bool inject_fault;
    static std::string fault_target;

    template<typename Method, typename... Args>
    static auto wrap(Method method, const T* obj, const std::string& name, Args... args) {
        if (!global_tracker) {
            std::cerr << "Error: global_tracker is null in " << name << std::endl;
            return (obj->*method)(args...);
        }
        std::cout << "Wrapping " << name << ", inject_fault=" << inject_fault << ", fault_target=" << fault_target << std::endl;
        global_tracker->push_call("Enter " + name);
        // Store first argument if available
        if constexpr (sizeof...(args) > 0) {
            std::apply([&](auto first, auto...) { global_tracker->insert_value(name + "_input", first); }, std::tuple(args...));
        }
        if (inject_fault && fault_target == name) {
            std::cout << "Triggering fault injection for " << name << std::endl;
            global_tracker->push_call("FAULT INJECTED");
            throw std::runtime_error("Fault injected in " + name);
        }
        auto result = (obj->*method)(args...);
        global_tracker->push_call("Exit " + name);
        global_tracker->insert_value(name + "_output", result);
        return result;
    }
};

// Static member initialization
template<typename T> bool TestProxy<T>::inject_fault = false;
template<typename T> std::string TestProxy<T>::fault_target = "";

// Proxied Methods
#define PROXY_METHOD(Class, Method, ReturnType, Params, ...) \
    ReturnType proxy_##Method Params const { \
        return TestProxy<Class>::wrap(&Class::Method, this, #Class "::" #Method, ##__VA_ARGS__); \
    }

#define PROXY_METHOD_NO_ARGS(Class, Method, ReturnType) \
    ReturnType proxy_##Method() const { \
        return TestProxy<Class>::wrap(&Class::Method, this, #Class "::" #Method); \
    }

// Instrumented Classes
class InstrumentedClass1 : public Class1 {
public:
    InstrumentedClass1() = default;

    PROXY_METHOD(Class1, execute, int, (int x), x)
    PROXY_METHOD(Class1, compute, double, (double value, int count), value, count)
    PROXY_METHOD_NO_ARGS(Class1, get_counter, int)
};

class InstrumentedClass2 : public Class2 {
public:
    InstrumentedClass2() = default;

    PROXY_METHOD(Class2, transform, int, (int x), x)
    PROXY_METHOD(Class2, combine, std::string, (int x, const std::string& label), x, label)
    PROXY_METHOD_NO_ARGS(Class2, get_name, std::string)
};

class InstrumentedClass3 : public Class3 {
public:
    InstrumentedClass3() = default;

    PROXY_METHOD(Class3, process, int, (int x), x)
    PROXY_METHOD(Class3, scale, double, (double factor, int count), factor, count)
    PROXY_METHOD_NO_ARGS(Class3, describe, std::string)
};

// Factory Implementations
std::unique_ptr<Class1> Factory::create_class1() {
    if (use_instrumented) {
        std::cout << "Creating InstrumentedClass1" << std::endl;
        return std::make_unique<InstrumentedClass1>();
    }
    std::cout << "Creating Class1" << std::endl;
    return std::make_unique<Class1>();
}

std::unique_ptr<Class2> Factory::create_class2() {
    if (use_instrumented) {
        std::cout << "Creating InstrumentedClass2" << std::endl;
        return std::make_unique<InstrumentedClass2>();
    }
    std::cout << "Creating Class2" << std::endl;
    return std::make_unique<Class2>();
}

std::unique_ptr<Class3> Factory::create_class3() {
    if (use_instrumented) {
        std::cout << "Creating InstrumentedClass3" << std::endl;
        return std::make_unique<InstrumentedClass3>();
    }
    std::cout << "Creating Class3" << std::endl;
    return std::make_unique<Class3>();
};

// Test Fixture
class DataFlowTest : public ::testing::Test {
protected:
    TestTracker tracker;

    void SetUp() override {
        Factory::use_instrumented = true;
        std::cout << "SetUp: Factory::use_instrumented = " << Factory::use_instrumented << std::endl;
        global_tracker = &tracker;
        std::cout << "SetUp: global_tracker initialized at " << global_tracker << std::endl;
    }

    void TearDown() override {
        TestProxy<Class1>::inject_fault = false;
        TestProxy<Class2>::inject_fault = false;
        TestProxy<Class3>::inject_fault = false;
        TestProxy<Class1>::fault_target = "";
        TestProxy<Class2>::fault_target = "";
        TestProxy<Class3>::fault_target = "";
        Factory::use_instrumented = false;
        global_tracker = nullptr;
        tracker.reset();
    }
};

// Tests for Execute Call Chain Transitions
TEST_F(DataFlowTest, ExecuteToTransform) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_execute(2); // Instrument up to Class1::execute
    ic1->execute(2); // Call regular Class1::execute to increment counter
    int result = ic2->transform(2); // Call regular Class2::transform
    EXPECT_EQ(result, (2 * 2 + 5) * 2); // (2*2+5=9)*2=18
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
    EXPECT_EQ(tracker.values["Class1::execute_output"], "54");
}

TEST_F(DataFlowTest, TransformToProcess) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_transform(2); // Instrument up to Class2::transform
    int result = ic3->process(2); // Call regular Class3::process
    EXPECT_EQ(result, 2 * 2 + 5); // 2*2+5=9
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "Exit Class2::transform"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
    EXPECT_EQ(tracker.values["Class2::transform_output"], "18");
}

// Tests for Compute Call Chain Transitions
TEST_F(DataFlowTest, ComputeToScale) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic1->proxy_compute(2.5, 3); // Instrument up to Class1::compute
    ic1->compute(2.5, 3); // Call regular Class1::compute to increment counter
    double result = ic3->scale(2.5, 3); // Call regular Class3::scale
    EXPECT_DOUBLE_EQ(result, 2.5 * 3 + 5); // 2.5*3+5=12.5
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
    EXPECT_EQ(tracker.values["Class1::compute_output"], "75");
}

TEST_F(DataFlowTest, ComputeToGetName) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_compute(2.5, 3); // Instrument up to Class1::compute
    ic1->compute(2.5, 3); // Call regular Class1::compute to increment counter
    std::string result = ic2->get_name(); // Call regular Class2::get_name
    EXPECT_EQ(result, "Class2");
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
    EXPECT_EQ(tracker.values["Class1::compute_output"], "75");
}

// Test for Combine Call Chain Transition
TEST_F(DataFlowTest, CombineToDescribe) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_combine(10, "TestLabel"); // Instrument up to Class2::combine
    std::string result = ic3->describe(); // Call regular Class3::describe
    EXPECT_EQ(result, "Class3: Processing unit");
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "Exit Class2::combine"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
    EXPECT_EQ(tracker.values["Class2::combine_output"], "Class3: Processing unit | TestLabel | 10");
}

// Test for Attribute Propagation
TEST_F(DataFlowTest, AttributePropagation) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic1->execute(5); // Call regular Class1::execute to increment counter
    ic2->transform(5); // Call regular Class2::transform
    ic1->compute(1.0, 2); // Call regular Class1::compute to increment counter
    ic3->scale(1.0, 2); // Call regular Class3::scale
    ic2->get_name(); // Call regular Class2::get_name
    int result = ic1->proxy_get_counter();
    EXPECT_EQ(result, 2);
    std::vector<std::string> expected = {
        "Enter Class1::get_counter",
        "Exit Class1::get_counter"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::get_counter_output"], "2");
}

// Fault Injection Tests
TEST_F(DataFlowTest, FaultInjectionClass3) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_transform(2); // Instrument up to Class2::transform
    TestProxy<Class3>::inject_fault = true; // Set fault injection for correct template
    TestProxy<Class3>::fault_target = "Class3::process";
    EXPECT_THROW(ic3->proxy_process(2), std::runtime_error); // Call proxy Class3::process with fault
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "Exit Class2::transform",
        "Enter Class3::process",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
}

TEST_F(DataFlowTest, FaultInjectionClass2) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_execute(2); // Instrument up to Class1::execute
    ic1->execute(2); // Call regular Class1::execute to increment counter
    TestProxy<Class2>::inject_fault = true; // Set fault injection for correct template
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(ic2->proxy_transform(2), std::runtime_error); // Call proxy Class2::transform with fault
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute",
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
}

TEST_F(DataFlowTest, FaultInjectionScale) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic1->proxy_compute(2.5, 3); // Instrument up to Class1::compute
    ic1->compute(2.5, 3); // Call regular Class1::compute to increment counter
    TestProxy<Class3>::inject_fault = true; // Set fault injection for correct template
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(ic3->proxy_scale(2.5, 3), std::runtime_error); // Call proxy Class3::scale with fault
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute",
        "Enter Class3::scale",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
}

// Test for Non-Instrumented Execution
TEST_F(DataFlowTest, NonInstrumentedExecution) {
    Factory::use_instrumented = false;
    auto c1 = Factory::create_class1();
    EXPECT_EQ(c1->execute(2), ((2 * 2 + 5) * 2) * 3); // 54
    EXPECT_TRUE(tracker.call_stack.empty()); // No tracking
    EXPECT_EQ(c1->get_counter(), 1);
}

int main(int argc, char** argv) {
    // Set use_instrumented globally before tests run
    Factory::use_instrumented = true;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}