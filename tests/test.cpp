#include <iostream>
#include <utility> // Include for std::pair and std::make_pair

class MyObject {
public:
    MyObject(const std::string& name, int value) : name_(name), value_(value) {}

    void printInfo() const {
        std::cout << "Name: " << name_ << ", Value: " << value_ << std::endl;
    }

private:
    std::string name_;
    int value_;
};

int main() {
    MyObject obj1("Object1", 10);
    MyObject obj2("Object2", 20);

    // Using std::make_pair to create a pair of MyObject instances
    std::pair<MyObject, MyObject> pairOfObjects = std::make_pair(obj1, obj2);

    // Accessing and printing information of the objects in the pair
    std::cout << "First object: ";
    pairOfObjects.first.printInfo();

    std::cout << "Second object: ";
    pairOfObjects.second.printInfo();

    return 0;
}
