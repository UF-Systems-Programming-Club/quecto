
extern proc puts(str : i8[5]) => ()

proc main() => () {
	list : u32[5] = {4, 4, 5, 6, 9};
	list[4] = 0;
	sum : u32 = 0;
	index : u32 = 0;
	while (index < 5) {
		sum = sum + list[index];
		index = index + 1;
	}

	str : i8[5] = {65, 65, 65, 65, 0};

	puts(str);
	
	return sum;
}
