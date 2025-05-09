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
#include <climits>
#include <cfloat>
#include <limits>

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

// Factory for creating instrumented classes
class Factory {
public:
    static std::unique_ptr<Class1> create_class1();
    static std::unique_ptr<Class2> create_class2();
    static std::unique_ptr<Class3> create_class3();
};

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
    std::cout << "Creating InstrumentedClass1" << std::endl;
    return std::make_unique<InstrumentedClass1>();
}

std::unique_ptr<Class2> Factory::create_class2() {
    std::cout << "Creating InstrumentedClass2" << std::endl;
    return std::make_unique<InstrumentedClass2>();
}

std::unique_ptr<Class3> Factory::create_class3() {
    std::cout << "Creating InstrumentedClass3" << std::endl;
    return std::make_unique<InstrumentedClass3>();
};

// Test Fixture for Unit Tests
class UnitTests : public ::testing::Test {
protected:
    TestTracker tracker;

    void SetUp() override {
        std::cout << "UnitTests SetUp: global_tracker initialized at " << &tracker << std::endl;
        global_tracker = &tracker;
    }

    void TearDown() override {
        TestProxy<Class1>::inject_fault = false;
        TestProxy<Class2>::inject_fault = false;
        TestProxy<Class3>::inject_fault = false;
        TestProxy<Class1>::fault_target = "";
        TestProxy<Class2>::fault_target = "";
        TestProxy<Class3>::fault_target = "";
        global_tracker = nullptr;
        tracker.reset();
    }
};

// Test Fixture for Data Flow Tests
class DataFlowTests : public ::testing::Test {
protected:
    TestTracker tracker;

    void SetUp() override {
        std::cout << "DataFlowTests SetUp: global_tracker initialized at " << &tracker << std::endl;
        global_tracker = &tracker;
    }

    void TearDown() override {
        TestProxy<Class1>::inject_fault = false;
        TestProxy<Class2>::inject_fault = false;
        TestProxy<Class3>::inject_fault = false;
        TestProxy<Class1>::fault_target = "";
        TestProxy<Class2>::fault_target = "";
        TestProxy<Class3>::fault_target = "";
        global_tracker = nullptr;
        tracker.reset();
    }
};

// Unit Tests for Class1
TEST_F(UnitTests, UnitExecutePositive) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(2);
    EXPECT_EQ(result, ((2 * 2 + 5) * 2) * 3); // (2*2+5=9)*2=18*3=54
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteNegative) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(-2);
    EXPECT_EQ(result, ((-2 * 2 + 5) * 2) * 3); // (-2*2+5=1)*2=2*3=6
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteZero) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(0);
    EXPECT_EQ(result, ((0 * 2 + 5) * 2) * 3); // (0*2+5=5)*2=10*3=30
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteMax) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MAX);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MAX) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteMin) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MIN);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MIN) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteNearMax) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MAX - 1);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MAX - 1) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteNearMin) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MIN + 1);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MIN + 1) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteLargePositive) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MAX / 2);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MAX / 2) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteLargeNegative) {
    auto c1 = Factory::create_class1();
    int result = c1->execute(INT_MIN / 2);
    EXPECT_EQ(result, static_cast<int>((static_cast<long long>(INT_MIN / 2) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionExecuteTypical) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    ASSERT_NE(ic1, nullptr);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::execute";
    EXPECT_THROW(ic1->proxy_execute(2), std::runtime_error);
    EXPECT_EQ(c1->get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
}

TEST_F(UnitTests, UnitFaultInjectionExecuteMax) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    ASSERT_NE(ic1, nullptr);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::execute";
    EXPECT_THROW(ic1->proxy_execute(INT_MAX), std::runtime_error);
    EXPECT_EQ(c1->get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], std::to_string(INT_MAX));
}

TEST_F(UnitTests, UnitComputePositive) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(2.5, 3);
    EXPECT_DOUBLE_EQ(result, (2.5 * 3 + 5) * 6); // (2.5*3+5=12.5)*6=75
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeNegative) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(-2.5, -3);
    EXPECT_DOUBLE_EQ(result, (-2.5 * -3 + 5) * 6); // (-2.5*-3+5=12.5)*6=75
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeZero) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(0.0, 0);
    EXPECT_DOUBLE_EQ(result, (0.0 * 0 + 5) * 6); // (0*0+5=5)*6=30
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeMaxValue) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(DBL_MAX, 1);
    EXPECT_DOUBLE_EQ(result, (DBL_MAX * 1 + 5) * 6); // Large value
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeMinValue) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(DBL_MIN, 1);
    EXPECT_DOUBLE_EQ(result, (DBL_MIN * 1 + 5) * 6);
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeSmallValue) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(1e-308, 1);
    EXPECT_DOUBLE_EQ(result, (1e-308 * 1 + 5) * 6);
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeNegativeCount) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(2.5, -3);
    EXPECT_DOUBLE_EQ(result, (2.5 * -3 + 5) * 6); // (2.5*-3+5=-2.5)*6=-15
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeZeroCount) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(2.5, 0);
    EXPECT_DOUBLE_EQ(result, (2.5 * 0 + 5) * 6); // (2.5*0+5=5)*6=30
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeMaxCount) {
    auto c1 = Factory::create_class1();
    double result = c1->compute(1.0, INT_MAX);
    EXPECT_DOUBLE_EQ(result, (1.0 * INT_MAX + 5) * 6);
    EXPECT_EQ(c1->get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionComputeTypical) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    ASSERT_NE(ic1, nullptr);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::compute";
    EXPECT_THROW(ic1->proxy_compute(2.5, 3), std::runtime_error);
    EXPECT_EQ(c1->get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(std::stod(tracker.values["Class1::compute_input"]), 2.5);
}

TEST_F(UnitTests, UnitFaultInjectionComputeMax) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    ASSERT_NE(ic1, nullptr);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::compute";
    EXPECT_THROW(ic1->proxy_compute(DBL_MAX, 1), std::runtime_error);
    EXPECT_EQ(c1->get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_DOUBLE_EQ(std::stod(tracker.values["Class1::compute_input"]), DBL_MAX);
}

TEST_F(UnitTests, UnitGetCounterNonZero) {
    auto c1 = Factory::create_class1();
    c1->execute(2);
    int result = c1->get_counter();
    EXPECT_EQ(result, 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitGetCounterAfterCompute) {
    auto c1 = Factory::create_class1();
    c1->compute(2.5, 3);
    int result = c1->get_counter();
    EXPECT_EQ(result, 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitGetCounterZero) {
    auto c1 = Factory::create_class1();
    int result = c1->get_counter();
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionGetCounter) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    ASSERT_NE(ic1, nullptr);
    c1->execute(2);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::get_counter";
    EXPECT_THROW(ic1->proxy_get_counter(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class1::get_counter",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Unit Tests for Class2
TEST_F(UnitTests, UnitTransformPositive) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(2);
    EXPECT_EQ(result, (2 * 2 + 5) * 2); // (2*2+5=9)*2=18
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformNegative) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(-2);
    EXPECT_EQ(result, (-2 * 2 + 5) * 2); // (-2*2+5=1)*2=2
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformZero) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(0);
    EXPECT_EQ(result, (0 * 2 + 5) * 2); // (0*2+5=5)*2=10
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformMax) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MAX);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX) * 2 + 5) * 2); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformMin) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MIN);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN) * 2 + 5) * 2); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformNearMax) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MAX - 1);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX - 1) * 2 + 5) * 2);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformNearMin) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MIN + 1);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN + 1) * 2 + 5) * 2);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformLargePositive) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MAX / 2);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX / 2) * 2 + 5) * 2);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformLargeNegative) {
    auto c2 = Factory::create_class2();
    int result = c2->transform(INT_MIN / 2);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN / 2) * 2 + 5) * 2);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionTransformTypical) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic2, nullptr);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(ic2->proxy_transform(2), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
}

TEST_F(UnitTests, UnitFaultInjectionTransformMax) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic2, nullptr);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(ic2->proxy_transform(INT_MAX), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], std::to_string(INT_MAX));
}

TEST_F(UnitTests, UnitCombinePositive) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(10, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineNegative) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(-10, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | -10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineZero) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(0, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | 0");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineEmptyLabel) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(10, "");
    EXPECT_EQ(result, "Class3: Processing unit |  | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineLongLabel) {
    auto c2 = Factory::create_class2();
    std::string long_label(1000, 'A');
    std::string result = c2->combine(10, long_label);
    EXPECT_EQ(result, "Class3: Processing unit | " + long_label + " | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineMax) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(INT_MAX, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | " + std::to_string(INT_MAX));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineMin) {
    auto c2 = Factory::create_class2();
    std::string result = c2->combine(INT_MIN, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | " + std::to_string(INT_MIN));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionCombineTypical) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic2, nullptr);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::combine";
    EXPECT_THROW(ic2->proxy_combine(10, "TestLabel"), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
}

TEST_F(UnitTests, UnitFaultInjectionCombineLongLabel) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic2, nullptr);
    std::string long_label(1000, 'A');
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::combine";
    EXPECT_THROW(ic2->proxy_combine(10, long_label), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
}

TEST_F(UnitTests, UnitGetNameDefault) {
    auto c2 = Factory::create_class2();
    std::string result = c2->get_name();
    EXPECT_EQ(result, "Class2");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitGetNameAfterCombine) {
    auto c2 = Factory::create_class2();
    c2->combine(10, "TestLabel");
    std::string result = c2->get_name();
    EXPECT_EQ(result, "Class2");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionGetName) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic2, nullptr);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::get_name";
    EXPECT_THROW(ic2->proxy_get_name(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::get_name",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Unit Tests for Class3
TEST_F(UnitTests, UnitProcessPositive) {
    auto c3 = Factory::create_class3();
    int result = c3->process(2);
    EXPECT_EQ(result, 2 * 2 + 5); // 2*2+5=9
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessNegative) {
    auto c3 = Factory::create_class3();
    int result = c3->process(-2);
    EXPECT_EQ(result, -2 * 2 + 5); // -2*2+5=1
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessZero) {
    auto c3 = Factory::create_class3();
    int result = c3->process(0);
    EXPECT_EQ(result, 0 * 2 + 5); // 0*2+5=5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessMax) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MAX);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX) * 2 + 5)); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessMin) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MIN);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN) * 2 + 5)); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessNearMax) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MAX - 1);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX - 1) * 2 + 5));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessNearMin) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MIN + 1);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN + 1) * 2 + 5));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessLargePositive) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MAX / 2);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX / 2) * 2 + 5));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessLargeNegative) {
    auto c3 = Factory::create_class3();
    int result = c3->process(INT_MIN / 2);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MIN / 2) * 2 + 5));
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionProcessTypical) {
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic3, nullptr);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::process";
    EXPECT_THROW(ic3->proxy_process(2), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::process",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class3::process_input"], "2");
}

TEST_F(UnitTests, UnitFaultInjectionProcessMax) {
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic3, nullptr);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::process";
    EXPECT_THROW(ic3->proxy_process(INT_MAX), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::process",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class3::process_input"], std::to_string(INT_MAX));
}

TEST_F(UnitTests, UnitScalePositive) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(2.5, 3);
    EXPECT_DOUBLE_EQ(result, 2.5 * 3 + 5); // 2.5*3+5=12.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleNegative) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(-2.5, -3);
    EXPECT_DOUBLE_EQ(result, -2.5 * -3 + 5); // -2.5*-3+5=12.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleZero) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(0.0, 0);
    EXPECT_DOUBLE_EQ(result, 0.0 * 0 + 5); // 0*0+5=5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleMaxFactor) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(DBL_MAX, 1);
    EXPECT_DOUBLE_EQ(result, DBL_MAX * 1 + 5);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleMinFactor) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(DBL_MIN, 1);
    EXPECT_DOUBLE_EQ(result, DBL_MIN * 1 + 5);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleSmallFactor) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(1e-308, 1);
    EXPECT_DOUBLE_EQ(result, 1e-308 * 1 + 5);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleNegativeCount) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(2.5, -3);
    EXPECT_DOUBLE_EQ(result, 2.5 * -3 + 5); // 2.5*-3+5=-2.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleZeroCount) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(2.5, 0);
    EXPECT_DOUBLE_EQ(result, 2.5 * 0 + 5); // 2.5*0+5=5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleMaxCount) {
    auto c3 = Factory::create_class3();
    double result = c3->scale(1.0, INT_MAX);
    EXPECT_DOUBLE_EQ(result, 1.0 * INT_MAX + 5);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionScaleTypical) {
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic3, nullptr);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(ic3->proxy_scale(2.5, 3), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::scale",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(std::stod(tracker.values["Class3::scale_input"]), 2.5);
}

TEST_F(UnitTests, UnitFaultInjectionScaleMax) {
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic3, nullptr);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(ic3->proxy_scale(DBL_MAX, 1), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::scale",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_DOUBLE_EQ(std::stod(tracker.values["Class3::scale_input"]), DBL_MAX);
}

TEST_F(UnitTests, UnitDescribeDefault) {
    auto c3 = Factory::create_class3();
    std::string result = c3->describe();
    EXPECT_EQ(result, "Class3: Processing unit");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitDescribeAfterProcess) {
    auto c3 = Factory::create_class3();
    c3->process(2);
    std::string result = c3->describe();
    EXPECT_EQ(result, "Class3: Processing unit");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionDescribe) {
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic3, nullptr);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::describe";
    EXPECT_THROW(ic3->proxy_describe(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::describe",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Data Flow Tests for Execute Call Chain
TEST_F(DataFlowTests, ExecuteToTransform) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_execute(2);
    ic1->execute(2);
    int result = c2->transform(2);
    EXPECT_EQ(result, (2 * 2 + 5) * 2); // (2*2+5=9)*2=18
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
    EXPECT_EQ(tracker.values["Class1::execute_output"], "54");
}

TEST_F(DataFlowTests, FaultInjectionTransform) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_execute(2);
    ic1->execute(2);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(ic2->proxy_transform(2), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Exit Class1::execute",
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
}

TEST_F(DataFlowTests, TransformToProcess) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_transform(2);
    int result = c3->process(2);
    EXPECT_EQ(result, 2 * 2 + 5); // 2*2+5=9
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "Exit Class2::transform"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
    EXPECT_EQ(tracker.values["Class2::transform_output"], "18");
}

TEST_F(DataFlowTests, FaultInjectionProcess) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_transform(2);
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
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
}

// Data Flow Tests for Compute Call Chain
TEST_F(DataFlowTests, ComputeToScale) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic1->proxy_compute(2.5, 3);
    ic1->compute(2.5, 3);
    double result = c3->scale(2.5, 3);
    EXPECT_DOUBLE_EQ(result, 2.5 * 3 + 5); // 2.5*3+5=12.5
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
    EXPECT_EQ(tracker.values["Class1::compute_output"], "75");
}

TEST_F(DataFlowTests, FaultInjectionScale) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic1->proxy_compute(2.5, 3);
    ic1->compute(2.5, 3);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(ic3->proxy_scale(2.5, 3), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute",
        "Enter Class3::scale",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
}

TEST_F(DataFlowTests, ComputeToGetName) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_compute(2.5, 3);
    ic1->compute(2.5, 3);
    std::string result = c2->get_name();
    EXPECT_EQ(result, "Class2");
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::compute_input"], "2.5");
    EXPECT_EQ(tracker.values["Class1::compute_output"], "75");
}

TEST_F(DataFlowTests, FaultInjectionGetName) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ic1->proxy_compute(2.5, 3);
    ic1->compute(2.5, 3);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::get_name";
    EXPECT_THROW(ic2->proxy_get_name(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "Exit Class1::compute",
        "Enter Class2::get_name",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Data Flow Test for Combine Call Chain
TEST_F(DataFlowTests, CombineToDescribe) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_combine(10, "TestLabel");
    std::string result = c3->describe();
    EXPECT_EQ(result, "Class3: Processing unit");
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "Exit Class2::combine"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
    EXPECT_EQ(tracker.values["Class2::combine_output"], "Class3: Processing unit | TestLabel | 10");
}

TEST_F(DataFlowTests, FaultInjectionDescribe) {
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    ic2->proxy_combine(10, "TestLabel");
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::describe";
    EXPECT_THROW(ic3->proxy_describe(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "Exit Class2::combine",
        "Enter Class3::describe",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
}

// Additional Data Flow Tests
TEST_F(DataFlowTests, AttributePropagation) {
    auto c1 = Factory::create_class1();
    auto ic1 = static_cast<InstrumentedClass1*>(c1.get());
    auto c2 = Factory::create_class2();
    auto ic2 = static_cast<InstrumentedClass2*>(c2.get());
    auto c3 = Factory::create_class3();
    auto ic3 = static_cast<InstrumentedClass3*>(c3.get());
    ASSERT_NE(ic1, nullptr);
    ASSERT_NE(ic2, nullptr);
    ASSERT_NE(ic3, nullptr);
    c1->execute(5);
    c2->transform(5);
    c1->compute(1.0, 2);
    c3->scale(1.0, 2);
    c2->get_name();
    int result = ic1->proxy_get_counter();
    EXPECT_EQ(result, 2);
    std::vector<std::string> expected = {
        "Enter Class1::get_counter",
        "Exit Class1::get_counter"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::get_counter_output"], "2");
}

TEST_F(DataFlowTests, NonInstrumentedExecution) {
    auto c1 = std::make_unique<Class1>();
    EXPECT_EQ(c1->execute(2), ((2 * 2 + 5) * 2) * 3); // 54
    EXPECT_TRUE(tracker.call_stack.empty());
    EXPECT_EQ(c1->get_counter(), 1);
}

TEST_F(DataFlowTests, TestTrackerFunctionality) {
    TestTracker tracker;
    tracker.push_call("TestCall1");
    tracker.insert_value("key1", 42);
    tracker.insert_value("key2", 3.14);
    tracker.insert_value("key3", "test");
    EXPECT_EQ(tracker.call_stack, std::vector<std::string>{"TestCall1"});
    EXPECT_EQ(tracker.values["key1"], "42");
    EXPECT_EQ(tracker.values["key2"], "3.1");
    EXPECT_EQ(tracker.values["key3"], "test");
    tracker.reset();
    EXPECT_TRUE(tracker.call_stack.empty());
    EXPECT_TRUE(tracker.values.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}