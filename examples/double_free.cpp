void* malloc(unsigned long size);
void free(void* ptr);

void example1() {
    int* ptr = (int*)malloc(sizeof(int));
    free(ptr);
    free(ptr);
}

void example2() {
    char* buffer = (char*)malloc(100);
    if (buffer) {
        free(buffer);
        buffer = nullptr;
        free(buffer);
    }
}

void example3() {
    int* data = new int[10];
    delete[] data;
    delete[] data;
}

int main() {
    example1();
    example2();
    example3();
    return 0;
}
