
proc five () => () {
    return 5;
}

proc six () => () {
    return 6;
}

proc seven () => () {
    return 7;
}

proc main () => () {
    a := five() + six() + seven();
    b := 1;

    five();

    while a < 5 {
        b = b * 2;
        a = a + 1;
    }

    return a;
}
