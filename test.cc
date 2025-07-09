template<typename T> void foo(T) {}
void foo(int) {}
void foo(double);
template<> void foo(int) {}

int main() {
    foo(1);
    foo(1.0);
    return 0;
}

void foo(double) {
    // This function is defined to handle double arguments.
    // It can be used to demonstrate the specialization of the template function.
}