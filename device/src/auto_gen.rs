use std::env::args;
use std::fs::File;
use std::io::{BufRead, BufReader, Write};
use std::path::Path;

fn main() {
	let args: Vec<_> = args().collect();
	let path = Path::new(&args[1]);
	let file = File::open(&path).unwrap();
	let mut lines = BufReader::new(&file).lines();
	let line = lines.next().unwrap();
	let w: u32 = line.expect("REASON").parse().unwrap();
	let line = lines.next().unwrap();
	let k: u32 = line.expect("REASON").parse().unwrap();
	let line = lines.next().unwrap();
	let mut map_size: u32 = line.expect("REASON").parse().unwrap();

	if map_size > 2 * k {
		map_size = 2 * k;
	}

	let path = Path::new("src/auto_gen.hpp");
	let mut file = File::create(&path).unwrap();

	writeln!(
		file,
		"#ifndef AUTO_GEN_HPP
#define AUTO_GEN_HPP"
	)
	.unwrap();
	writeln!(
		file,
		"#include \"ap_int.h\"
#include <cstdint>"
	)
	.unwrap();
	writeln!(
		file,
		"#define SE_W {}
#define SE_K {}
#define MAP_SIZE {}
#define READ_SIZE 22",
		w, k, map_size
	)
	.unwrap();

	if 2 * k > map_size {
	} else {
		writeln!(
			file,
			"struct ms_pos_t {{
	uint32_t start_pos;
	uint32_t end_pos;
	ap_uint<READ_SIZE> query_loc;
	ap_uint<1> str;
	ap_uint<1> EOR;
}};"
		)
		.unwrap();
		writeln!(file, "#define PUSH_EOR(stream) stream << ms_pos_t{{.start_pos = 0, .end_pos = 0, .query_loc = 0, .str = 0, .EOR = 1}};").unwrap();
		writeln!(file,"#define PUSH_EOF(stream) stream << ms_pos_t{{.start_pos = 0, .end_pos = 0, .query_loc = 0, .str = 1, .EOR = 1}};").unwrap();
		writeln!(file, "#define PUSH_POS(stream, start, end, seed) stream << ms_pos_t{{.start_pos = start, .end_pos = end, .query_loc = seed.loc, .str = seed.str, .EOR = 0}};").unwrap();
		writeln!(
			file,
			"#define PUSH_LOC(stream, key, pos) \\
	{{ \\
		const uint64_t loc = key_2_loc(key, pos.str, pos.query_loc); \\
		loc_o << loc; \\
	}}"
		)
		.unwrap();
	}

	writeln!(file, "#endif").unwrap();
}
