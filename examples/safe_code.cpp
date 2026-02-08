// Safe examples - should pass analysis
void safe_example1() {
    int* ptr = new int(42);
    int value = *ptr;  // OK: use before free
    delete ptr;
}

void safe_example2() {
    int* p = new int(5);
    delete p;
    p = new int(10);  // OK: reassignment after free
    delete p;
}

void safe_example3() {
    int* arr = new int[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = i;  // OK: use while alive
    }
    delete[] arr;
}

int main() {
    safe_example1();
    safe_example2();
    safe_example3();
    return 0;
}
