void* malloc(unsigned long size);
void free(void* ptr);

void leak1() {
    int* ptr = new int(42);
}

void leak2() {
    char* buffer = (char*)malloc(1024);
    if (buffer) {
    }
}

void conditional_leak(bool condition) {
    int* data = new int[100];
    if (condition) {
        delete[] data;
        return;
    }
}

int* return_leak() {
    int* temp = new int(10);
    return temp;
}

int main() {
    leak1();
    leak2();
    conditional_leak(false);
    int* ptr = return_leak();
    return 0;
}
