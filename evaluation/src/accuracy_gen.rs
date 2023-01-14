use std::env::args;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

fn main() {
	let args: Vec<_> = args().collect();
	let max_error_rate: f32 = args[1].parse().unwrap();
	let path = Path::new(&args[2]);
	let file = File::open(&path).unwrap();
	let lines = BufReader::new(&file).lines();
	let mut max_occ: u32 = 0;
	let mut mapped: u32 = 0;
	let mut error_rate: f32 = 1.0;

	let mut cur_mapped: u32 = 0;
	let mut cur_error_rate: f32 = 1.0;

	for line in lines {
		let line = line.unwrap();
		let field: Vec<_> = line.split('\t').collect();
		if field[0].eq("P") {
			let new_max_occ: u32 = field[1].parse().unwrap();
			// First case
			if max_occ == 0 {
				max_occ = new_max_occ;
				error_rate = 1.0;
				mapped = 0;
			}
			// New max occ
			else if max_occ != new_max_occ {
				println!("{}\t{}\t{}", max_occ, mapped, error_rate);
				max_occ = new_max_occ;
				error_rate = 1.0;
				mapped = 0;
			}
			// New vt_distance
			else {
				if (error_rate > max_error_rate && cur_error_rate < error_rate)
					|| (cur_error_rate <= max_error_rate && cur_mapped > mapped)
				{
					error_rate = cur_error_rate;
					mapped = cur_mapped;
				}
			}
		} else {
			cur_error_rate = field[4].parse().unwrap();
			cur_mapped = field[5].parse().unwrap();
		}
	}
	println!("{}\t{}\t{}", max_occ, mapped, error_rate);
}
