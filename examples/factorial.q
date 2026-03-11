proc factorial(n: i32) => (r: i32) {
    if n <= 1 return 1;

    return n * factorial(n - 1);
}

proc main() => () {
    return factorial(5);
}
