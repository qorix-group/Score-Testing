#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <map>
 
// Original Classes (unchanged)
class Class3 {
public:
    int process(int x) const { return x * 2; } // Step 3: x*2
};
 
class Class2 {
public:
    int transform(int x) const {
        Class3 c3;
        return c3.process(x) + 1; // Step 2: (x*2) + 1
    }
};
 
class Class1 {
public:
    int execute(int x) const {
        Class2 c2;
        return c2.transform(x) * 3; // Step 1: ((x*2) + 1) * 3
    }
};
 
// Test Infrastructure
class TestTracker {
public:
    std::vector<std::string> call_stack;
    std::map<std::string, int> values;
    void reset() {
        call_stack.clear();
        values.clear();
    }
};
 
// Proxy Layer
template<typename T>
class TestProxy {
public:
    static TestTracker* tracker;
    static bool inject_fault;
    static std::string fault_target;
 
    template<typename Method>
    static int wrap(Method method, const T* obj, int x, const std::string& name) {
        // Track call
        tracker->call_stack.push_back("Enter " + name);
        tracker->values[name + "_input"] = x;
 
        // Fault injection
        if(inject_fault && fault_target == name) {
            tracker->call_stack.push_back("FAULT INJECTED");
            throw std::runtime_error("Fault injected in " + name);
        }
 
        // Execute original method
        int result = (obj->*method)(x);
 
        // Track result
        tracker->call_stack.push_back("Exit " + name);
        tracker->values[name + "_output"] = result;
        return result;
    }
};
 
// Static members initialization
template<typename T> TestTracker* TestProxy<T>::tracker = nullptr;
template<typename T> bool TestProxy<T>::inject_fault = false;
template<typename T> std::string TestProxy<T>::fault_target = "";
 
// Proxied Methods
#define PROXY_METHOD(Class, Method) \
    int Method(int x) const { \
        return TestProxy<Class>::wrap(&Class::Method, this, x, #Class "::" #Method); \
    }
 
// Instrumented Classes
class InstrumentedClass1 : public Class1 {
public:
    PROXY_METHOD(Class1, execute)
};
 
class InstrumentedClass2 : public Class2 {
public:
    PROXY_METHOD(Class2, transform)
};
 
class InstrumentedClass3 : public Class3 {
public:
    PROXY_METHOD(Class3, process)
};
 
// Test Fixture
class DataFlowTest : public ::testing::Test {
protected:
    TestTracker tracker;
    void SetUp() override {
        TestProxy<InstrumentedClass1>::tracker = &tracker;
        TestProxy<InstrumentedClass2>::tracker = &tracker;
        TestProxy<InstrumentedClass3>::tracker = &tracker;
    }
 
    void TearDown() override {
        TestProxy<InstrumentedClass1>::inject_fault = false;
        TestProxy<InstrumentedClass2>::inject_fault = false;
        TestProxy<InstrumentedClass3>::inject_fault = false;
        tracker.reset();
    }
};
 
// Tests
TEST_F(DataFlowTest, NormalFlow) {
    InstrumentedClass1 c1;
    EXPECT_EQ(c1.execute(2), ((2 * 2) + 1) * 3); // (2*2=4) +1=5 *3=15
    // Verify call sequence
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "Enter Class3::process",
        "Exit Class3::process",
        "Exit Class2::transform",
        "Exit Class1::execute"
    };
    ASSERT_EQ(tracker.call_stack, expected);
    // Verify data flow
    EXPECT_EQ(tracker.values["Class3::process_input"], 2);
    EXPECT_EQ(tracker.values["Class3::process_output"], 4);
    EXPECT_EQ(tracker.values["Class2::transform_output"], 5);
    EXPECT_EQ(tracker.values["Class1::execute_output"], 15);
}
 
TEST_F(DataFlowTest, FaultInjectionClass3) {
    TestProxy<InstrumentedClass3>::inject_fault = true;
    TestProxy<InstrumentedClass3>::fault_target = "Class3::process";
 
    InstrumentedClass1 c1;
    EXPECT_THROW(c1.execute(2), std::runtime_error);
    // Verify partial execution
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "Enter Class3::process",
        "FAULT INJECTED"
    };
    ASSERT_EQ(tracker.call_stack, expected);
}
 
TEST_F(DataFlowTest, FaultInjectionClass2) {
    TestProxy<InstrumentedClass2>::inject_fault = true;
    TestProxy<InstrumentedClass2>::fault_target = "Class2::transform";
 
    InstrumentedClass1 c1;
    EXPECT_THROW(c1.execute(2), std::runtime_error);
    // Verify partial execution
    std::vector<std::string> expected = {
        "Enter Class1::execute",
        "Enter Class2::transform",
        "FAULT INJECTED"
    };
    ASSERT_EQ(tracker.call_stack, expected);
}
 
TEST_F(DataFlowTest, ValuePropagation) {
    InstrumentedClass1 c1;
    c1.execute(5);
    // Verify value propagation through classes
    EXPECT_EQ(tracker.values["Class3::process_input"], 5);
    EXPECT_EQ(tracker.values["Class2::transform_input"], 5);
    EXPECT_EQ(tracker.values["Class1::execute_input"], 5);
    EXPECT_EQ(tracker.values["Class3::process_output"], 10);  // 5*2
    EXPECT_EQ(tracker.values["Class2::transform_output"], 11); // 10+1
    EXPECT_EQ(tracker.values["Class1::execute_output"], 33);   // 11*3
}
 
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}