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
class TestClass1;
class TestClass2;
class TestClass3;

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

// Original Classes (unchanged)
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
        Class3 c3;
        return c3.process(x) * multiplier_; // Step 2: (x*2 + offset) * multiplier
    }

    std::string combine(int x, const std::string& label) const {
        Class3 c3;
        return c3.describe() + " | " + label + " | " + std::to_string(x); // Combines attribute and input
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
    Class1() : c2_(), factor_(3), counter_(0) {}

    int execute(int x) const {
        counter_++;
        return c2_.transform(x) * factor_; // Step 1: ((x*2 + offset) * multiplier) * factor
    }

    double compute(double value, int count) const {
        Class3 c3;
        counter_++;
        return c3.scale(value, count) * c2_.get_name().length(); // Combines results and attribute
    }

    int get_counter() const {
        return counter_;
    }

protected:
    Class2 c2_; // Direct member dependency
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

// Test Wrappers
class TestClass1 {
public:
    TestClass1(std::unique_ptr<Class1> c1, std::unique_ptr<InstrumentedClass1> ic1, std::unique_ptr<InstrumentedClass2> ic2, std::unique_ptr<InstrumentedClass3> ic3)
        : c1_(std::move(c1)), ic1_(std::move(ic1)), ic2_(std::move(ic2)), ic3_(std::move(ic3)) {}

    int execute(int x) const { return c1_->execute(x); }
    int proxy_execute(int x) const { return ic1_->proxy_execute(x); }
    double compute(double value, int count) const { return c1_->compute(value, count); }
    double proxy_compute(double value, int count) const { return ic1_->proxy_compute(value, count); }
    int get_counter() const { return c1_->get_counter(); }
    int proxy_get_counter() const {
        global_tracker->push_call("Enter Class1::get_counter");
        if (TestProxy<Class1>::inject_fault && TestProxy<Class1>::fault_target == "Class1::get_counter") {
            std::cout << "Triggering fault injection for Class1::get_counter" << std::endl;
            global_tracker->push_call("FAULT INJECTED");
            throw std::runtime_error("Fault injected in Class1::get_counter");
        }
        auto result = c1_->get_counter();
        global_tracker->push_call("Exit Class1::get_counter");
        global_tracker->insert_value("Class1::get_counter_output", result);
        return result;
    }

private:
    std::unique_ptr<Class1> c1_;
    std::unique_ptr<InstrumentedClass1> ic1_;
    std::unique_ptr<InstrumentedClass2> ic2_;
    std::unique_ptr<InstrumentedClass3> ic3_;
};

class TestClass2 {
public:
    TestClass2(std::unique_ptr<Class2> c2, std::unique_ptr<InstrumentedClass2> ic2, std::unique_ptr<InstrumentedClass3> ic3)
        : c2_(std::move(c2)), ic2_(std::move(ic2)), ic3_(std::move(ic3)) {}

    int transform(int x) const { return c2_->transform(x); }
    int proxy_transform(int x) const { return ic2_->proxy_transform(x); }
    std::string combine(int x, const std::string& label) const { return c2_->combine(x, label); }
    std::string proxy_combine(int x, const std::string& label) const { return ic2_->proxy_combine(x, label); }
    std::string get_name() const { return c2_->get_name(); }
    std::string proxy_get_name() const { return ic2_->proxy_get_name(); }

private:
    std::unique_ptr<Class2> c2_;
    std::unique_ptr<InstrumentedClass2> ic2_;
    std::unique_ptr<InstrumentedClass3> ic3_;
};

class TestClass3 {
public:
    TestClass3(std::unique_ptr<Class3> c3, std::unique_ptr<InstrumentedClass3> ic3)
        : c3_(std::move(c3)), ic3_(std::move(ic3)) {}

    int process(int x) const { return c3_->process(x); }
    int proxy_process(int x) const { return ic3_->proxy_process(x); }
    double scale(double factor, int count) const { return c3_->scale(factor, count); }
    double proxy_scale(double factor, int count) const { return ic3_->proxy_scale(factor, count); }
    std::string describe() const { return c3_->describe(); }
    std::string proxy_describe() const { return ic3_->proxy_describe(); }

private:
    std::unique_ptr<Class3> c3_;
    std::unique_ptr<InstrumentedClass3> ic3_;
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc1.execute(2);
    EXPECT_EQ(result, ((2 * 2 + 5) * 2) * 3); // (2*2+5=9)*2=18*3=54
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteNegative) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc1.execute(-2);
    EXPECT_EQ(result, ((-2 * 2 + 5) * 2) * 3); // (-2*2+5=1)*2=2*3=6
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteZero) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc1.execute(0);
    EXPECT_EQ(result, ((0 * 2 + 5) * 2) * 3); // (0*2+5=5)*2=10*3=30
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitExecuteMax) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc1.execute(INT_MAX);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(static_cast<long long>(INT_MAX) * 2 + 5) * 2) * 3); // Overflow expected, modulo 2^32
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionExecuteTypical) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::execute";
    EXPECT_THROW(tc1.proxy_execute(2), std::runtime_error);
    EXPECT_EQ(tc1.get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class1::execute_input"], "2");
}

TEST_F(UnitTests, UnitComputePositive) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    double result = tc1.compute(2.5, 3);
    EXPECT_DOUBLE_EQ(result, (2.5 * 3 + 5) * 6); // (2.5*3+5=12.5)*6=75
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeNegative) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    double result = tc1.compute(-2.5, -3);
    EXPECT_DOUBLE_EQ(result, (-2.5 * -3 + 5) * 6); // (-2.5*-3+5=12.5)*6=75
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeZero) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    double result = tc1.compute(0.0, 0);
    EXPECT_DOUBLE_EQ(result, (0.0 * 0 + 5) * 6); // (0*0+5=5)*6=30
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeMaxValue) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    double result = tc1.compute(DBL_MAX, 1);
    EXPECT_DOUBLE_EQ(result, (DBL_MAX * 1 + 5) * 6);
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitComputeNegativeCount) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    double result = tc1.compute(2.5, -3);
    EXPECT_DOUBLE_EQ(result, (2.5 * -3 + 5) * 6); // (2.5*-3+5=-2.5)*6=-15
    EXPECT_EQ(tc1.get_counter(), 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionComputeTypical) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::compute";
    EXPECT_THROW(tc1.proxy_compute(2.5, 3), std::runtime_error);
    EXPECT_EQ(tc1.get_counter(), 0);
    std::vector<std::string> expected = {
        "Enter Class1::compute",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(std::stod(tracker.values["Class1::compute_input"]), 2.5);
}

TEST_F(UnitTests, UnitGetCounterNonZero) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.execute(2);
    int result = tc1.get_counter();
    EXPECT_EQ(result, 1);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitGetCounterZero) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc1.get_counter();
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionGetCounter) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.execute(2);
    TestProxy<Class1>::inject_fault = true;
    TestProxy<Class1>::fault_target = "Class1::get_counter";
    EXPECT_THROW(tc1.proxy_get_counter(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class1::get_counter",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Unit Tests for Class2
TEST_F(UnitTests, UnitTransformPositive) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc2.transform(2);
    EXPECT_EQ(result, (2 * 2 + 5) * 2); // (2*2+5=9)*2=18
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformNegative) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc2.transform(-2);
    EXPECT_EQ(result, (-2 * 2 + 5) * 2); // (-2*2+5=1)*2=2
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformZero) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc2.transform(0);
    EXPECT_EQ(result, (0 * 2 + 5) * 2); // (0*2+5=5)*2=10
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitTransformMax) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    int result = tc2.transform(INT_MAX);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX) * 2 + 5) * 2); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionTransformTypical) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(tc2.proxy_transform(2), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::transform_input"], "2");
}

TEST_F(UnitTests, UnitCombinePositive) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    std::string result = tc2.combine(10, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineZero) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    std::string result = tc2.combine(0, "TestLabel");
    EXPECT_EQ(result, "Class3: Processing unit | TestLabel | 0");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineEmptyLabel) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    std::string result = tc2.combine(10, "");
    EXPECT_EQ(result, "Class3: Processing unit |  | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitCombineLongLabel) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    std::string long_label(1000, 'A');
    std::string result = tc2.combine(10, long_label);
    EXPECT_EQ(result, "Class3: Processing unit | " + long_label + " | 10");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionCombineTypical) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::combine";
    EXPECT_THROW(tc2.proxy_combine(10, "TestLabel"), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::combine",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class2::combine_input"], "10");
}

TEST_F(UnitTests, UnitGetNameDefault) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    std::string result = tc2.get_name();
    EXPECT_EQ(result, "Class2");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionGetName) {
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::get_name";
    EXPECT_THROW(tc2.proxy_get_name(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class2::get_name",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Unit Tests for Class3
TEST_F(UnitTests, UnitProcessPositive) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    int result = tc3.process(2);
    EXPECT_EQ(result, 2 * 2 + 5); // 2*2+5=9
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessNegative) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    int result = tc3.process(-2);
    EXPECT_EQ(result, -2 * 2 + 5); // -2*2+5=1
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessZero) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    int result = tc3.process(0);
    EXPECT_EQ(result, 0 * 2 + 5); // 0*2+5=5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitProcessMax) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    int result = tc3.process(INT_MAX);
    EXPECT_EQ(result, static_cast<int>(static_cast<long long>(INT_MAX) * 2 + 5)); // Overflow expected, modulo 2^32
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionProcessTypical) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::process";
    EXPECT_THROW(tc3.proxy_process(2), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::process",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(tracker.values["Class3::process_input"], "2");
}

TEST_F(UnitTests, UnitScalePositive) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    double result = tc3.scale(2.5, 3);
    EXPECT_DOUBLE_EQ(result, 2.5 * 3 + 5); // 2.5*3+5=12.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleNegative) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    double result = tc3.scale(-2.5, -3);
    EXPECT_DOUBLE_EQ(result, -2.5 * -3 + 5); // -2.5*-3+5=12.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleZero) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    double result = tc3.scale(0.0, 0);
    EXPECT_DOUBLE_EQ(result, 0.0 * 0 + 5); // 0*0+5=5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleMaxFactor) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    double result = tc3.scale(DBL_MAX, 1);
    EXPECT_DOUBLE_EQ(result, DBL_MAX * 1 + 5);
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitScaleNegativeCount) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    double result = tc3.scale(2.5, -3);
    EXPECT_DOUBLE_EQ(result, 2.5 * -3 + 5); // 2.5*-3+5=-2.5
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionScaleTypical) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(tc3.proxy_scale(2.5, 3), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::scale",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
    EXPECT_EQ(std::stod(tracker.values["Class3::scale_input"]), 2.5);
}

TEST_F(UnitTests, UnitDescribeDefault) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    std::string result = tc3.describe();
    EXPECT_EQ(result, "Class3: Processing unit");
    EXPECT_TRUE(tracker.call_stack.empty());
}

TEST_F(UnitTests, UnitFaultInjectionDescribe) {
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::describe";
    EXPECT_THROW(tc3.proxy_describe(), std::runtime_error);
    std::vector<std::string> expected = {
        "Enter Class3::describe",
        "FAULT INJECTED"
    };
    EXPECT_EQ(tracker.call_stack, expected);
}

// Data Flow Tests for Execute Call Chain
TEST_F(DataFlowTests, ExecuteToTransform) {
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.proxy_execute(2);
    tc1.execute(2);
    int result = tc2.transform(2);
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.proxy_execute(2);
    tc1.execute(2);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::transform";
    EXPECT_THROW(tc2.proxy_transform(2), std::runtime_error);
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
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc2.proxy_transform(2);
    int result = tc3.process(2);
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
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc2.proxy_transform(2);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::process";
    EXPECT_THROW(tc3.proxy_process(2), std::runtime_error);
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc1.proxy_compute(2.5, 3);
    tc1.compute(2.5, 3);
    double result = tc3.scale(2.5, 3);
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc1.proxy_compute(2.5, 3);
    tc1.compute(2.5, 3);
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::scale";
    EXPECT_THROW(tc3.proxy_scale(2.5, 3), std::runtime_error);
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.proxy_compute(2.5, 3);
    tc1.compute(2.5, 3);
    std::string result = tc2.get_name();
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    tc1.proxy_compute(2.5, 3);
    tc1.compute(2.5, 3);
    TestProxy<Class2>::inject_fault = true;
    TestProxy<Class2>::fault_target = "Class2::get_name";
    EXPECT_THROW(tc2.proxy_get_name(), std::runtime_error);
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
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc2.proxy_combine(10, "TestLabel");
    std::string result = tc3.describe();
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
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc2.proxy_combine(10, "TestLabel");
    TestProxy<Class3>::inject_fault = true;
    TestProxy<Class3>::fault_target = "Class3::describe";
    EXPECT_THROW(tc3.proxy_describe(), std::runtime_error);
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
    auto c1 = std::make_unique<Class1>();
    auto tc1 = TestClass1(std::move(c1), std::make_unique<InstrumentedClass1>(), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c2 = std::make_unique<Class2>();
    auto tc2 = TestClass2(std::move(c2), std::make_unique<InstrumentedClass2>(), std::make_unique<InstrumentedClass3>());
    auto c3 = std::make_unique<Class3>();
    auto tc3 = TestClass3(std::move(c3), std::make_unique<InstrumentedClass3>());
    tc1.execute(5);
    tc2.transform(5);
    tc1.compute(1.0, 2);
    tc3.scale(1.0, 2);
    tc2.get_name();
    int result = tc1.proxy_get_counter();
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}