void simple_uaf() {
    int* ptr = new int(42);
    delete ptr;
    *ptr = 10;
}

void double_free_bug() {
    int* p = new int(5);
    delete p;
    delete p;
}

int* return_local() {
    int x = 10;
    return &x;
}

void loop_uaf() {
    int* arr = new int[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = i;
    }
    delete[] arr;
    arr[5] = 100;
}

int main() {
    simple_uaf();
    double_free_bug();
    loop_uaf();
    return 0;
}
