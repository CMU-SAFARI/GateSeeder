use std::env::args;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

fn main() {
	let args: Vec<_> = args().collect();
	let nb_reads: u32 = args[1].parse().unwrap();
	let path = Path::new(&args[2]);
	let file = File::open(&path).unwrap();
	let lines = BufReader::new(&file).lines();
	let mut max_occ: u32 = 0;
	let mut vt_distance: u32 = 0;
	let mut t_mapped: u32 = 0;
	let mut f_mapped: u32 = 0;
	let mut unmapped: u32 = nb_reads;

	let mut cur_vt_distance: u32 = 0;
	let mut cur_mapped: u32 = 0;
	let mut cur_f_mapped: u32 = 0;

	for line in lines {
		let line = line.unwrap();
		let field: Vec<_> = line.split('\t').collect();
		if field[0].eq("P") {
			let new_max_occ: u32 = field[1].parse().unwrap();
			// First case
			if max_occ == 0 {
				max_occ = new_max_occ;
				cur_vt_distance = field[2].parse().unwrap();
			}
			// New max occ
			else if max_occ != new_max_occ {
				println!(
					"{}\t{}\t{}\t{}\t{}",
					max_occ, vt_distance, t_mapped, f_mapped, unmapped
				);
				max_occ = new_max_occ;
				cur_vt_distance = field[2].parse().unwrap();
				t_mapped = 0;
				f_mapped = 0;
			}
			// New vt_distance
			else {
				let cur_t_mapped = cur_mapped - cur_f_mapped;
				if cur_t_mapped > t_mapped {
					let cur_unmapped = nb_reads - cur_mapped;
					vt_distance = cur_vt_distance;
					t_mapped = cur_t_mapped;
					f_mapped = cur_f_mapped;
					unmapped = cur_unmapped;
				}
				cur_vt_distance = field[2].parse().unwrap();
				cur_f_mapped = 0;
			}
		} else {
			cur_f_mapped += field[3].parse::<u32>().unwrap();
			cur_mapped = field[5].parse().unwrap();
		}
	}
	println!(
		"{}\t{}\t{}\t{}\t{}",
		max_occ, vt_distance, t_mapped, f_mapped, unmapped
	);
}
