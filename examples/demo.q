extern proc getchar() => (chr: i32)
extern proc puts(str: ^i8) => ()

proc main () => () {
	buf : [i8; 32];
	len : i32 = 0;
	chr : i32 = 0;
	while chr > 0 {
		chr = getchar();
		if (chr ==  10) {
			buf[len] = 0;
			// chr = -2; // does not work bc negate isn't in yet
		} else {
			buf[len] = chr;
		}
		len = len + 1;
	}

	puts(buf);
	return len;
}
