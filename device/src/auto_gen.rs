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
	let map_size: u32 = line.expect("REASON").parse().unwrap();

	let path = Path::new(&args[2]);
	let mut file = File::create(&path).unwrap();
	writeln!(file, "TEST");
}
