
proc main() => () {
    a := 5;
    b := 6;
    ex := 3;

    if a > b {
        c := 4;
        d := 3;
        ex = 4;
        if c < d {
            ex = 5;
        } else {
            ex = c;
        }
    } elif a == b {
        ex = b;
    } else {
        ex = 0;
    }

    return a;
}
