extern proc malloc (n: u32) => (out: ^i8)
extern proc free (p: ^i8) => ()

proc main() => () {
	buf : ^i8 = malloc(8);
	i : i8 = 0;
	while i < 8 {
		buf[i] = i;
		i = i + 1;
	}
	sum : i8 = 0;
	i = 0;
	while i < 8 {
		sum = sum + buf[i];
		i = i + 1;
	}
	free(buf);
	return sum;
}
