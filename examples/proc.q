proc factorial (n: i32) => (r: i32) {
    if n <= 1 return 1;
    return n * factorial(n - 1);
}

proc sum_to (n: i32) => (r: i32) {
    a := 0;
    i := 0;
    while i < n {
        a = a + i;
        i = i + 1;
    }
    return a;
}

proc string () => (ret: i8[20]) {
    return 0;
}

proc main () => () {
    a : i32 = factorial(5);
    b : i32 = sum_to(8);
    return a + b;
}
