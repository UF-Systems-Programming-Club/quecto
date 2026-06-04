
extern proc putchar (char : i32) => ()
extern proc getchar () => (char : i32)

proc main () => () {
	chr : i32 = 0;
	while chr >= 0 {
		chr = getchar();
		putchar(chr);
	}

	return 0;
}
