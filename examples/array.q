
extern proc puts(str : [i8; 5]) => ()

proc modify(ptr: ^i8, idx: u32, val: i8) => () {
	ptr[idx] = val;
	return 0;
}

proc main() => (val: i8) {
	list : [u32; 5] = {4, 4, 5, 6, 9};
	str : [i8; 5] = {65, 65, 65, 65, 0};
	list[4] = 0;

	//apple : ^i8 = &str[0];
	//modify(apple, 1, 66);
	
	sum : u32 = 0;
	index : u32 = 0;
	while (index < 5) {
		sum = sum + list[index];
		index = index + 1;
		puts(str);
	}

	return sum;
}
