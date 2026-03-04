int main() {
    print("----------------------------
");
    print("   T-OS INTERACTIVE APP   
");
    print("----------------------------
");
    int i = 0;
    while (i < 3) {
        print("Loop iteration: ");
        print(i);
        i = i + 1;
    }
    print("Press any key to exit...
");
    int key = getchar();
    print("You pressed a key!
");
    return 42;
}
