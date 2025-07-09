template<typename T>
class A {
public:
    void AAAAAAA(typename T::something xd) {}
};

template<typename T>
class B : public A<T> {
public:
    void BBBBBBBBB() {}
};

struct TT {
    using something = int;
};

int main() {
    B<TT> a;
    a.BBBBBBBBB();
    // a.A::AAAAAAA();
    return 0;
}
