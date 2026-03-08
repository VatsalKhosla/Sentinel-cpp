void test_null_deref() {
    int* ptr = nullptr;
    *ptr = 42;  // Should detect: null pointer dereference
}

void test_null_arrow() {
    struct MyStruct {
        int value;
    };
    
    MyStruct* obj = nullptr;
    obj->value = 10;  // Should detect: null pointer dereference via arrow
}

void test_safe_null_check() {
    int* ptr = nullptr;
    if (ptr != nullptr) {
        *ptr = 42;  // This is safe
    }
}

void test_null_assign() {
    int* x;
    x = 0;
    *x = 100;  // null deref via 0
}
