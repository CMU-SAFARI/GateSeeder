use std::env::args;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;
extern crate getopts;
use getopts::Options;
mod offset;

const BLAST_FILT: f64 = 0.0;

fn main() {
	let args: Vec<_> = args().collect();
	let mut opts = Options::new();
	opts.optflag("s", "stats", "");
	opts.optflag("t", "threshold", "");
	let matches = match opts.parse(&args[1..]) {
		Ok(m) => m,
		Err(f) => {
			panic!("{}", f)
		}
	};
	if matches.free.len() != 2 {
		panic!("USAGE:\tsensitivity [option]* <GOLDEN PAF FILE> <LOCS DAT FILE>");
	}
	let gold = get_gold(&matches.free[0]);
	if matches.opt_present("s") {
		let (fp_n, fn_n) = get_stats(&gold, &matches.free[1]);
		println!("{}\t{}", fp_n, fn_n);
	} else if matches.opt_present("t") {
		println!("{}", get_threshold(&gold, &matches.free[1]));
	} else {
		eprintln!("Threshold: {}", get_nb_locations(&gold, &matches.free[1]));
	}
}

fn get_gold(path: &String) -> Vec<Gold> {
	let path = Path::new(&path);
	let file_gold = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	let offset_map = offset::get_offset();

	let reader = BufReader::new(&file_gold);
	reader
		.lines()
		.filter_map(|line| {
			let line_buf = line.unwrap();
			let sec: Vec<_> = line_buf.split('\t').collect();
			let offset: u32 = *offset_map.get(sec[5]).unwrap();
			let tb = sec[10].parse().unwrap();
			//Filtering
			if sec[12].eq("tp:A:S") && tb >= 10000 {
				Some(Gold {
					id: sec[0].split('.').collect::<Vec<_>>()[1].parse().unwrap(),
					strand: match sec[4].chars().collect::<Vec<_>>()[0] {
						'+' => true,
						'-' => false,
						_ => panic!("parse"),
					},
					start: sec[7].parse::<u32>().unwrap() + offset,
					end: sec[8].parse::<u32>().unwrap() + offset,
					mb: sec[9].parse().unwrap(),
					tb: tb,
				})
			} else {
				None
			}
		})
		.collect()
}

fn get_nb_locations(gold: &Vec<Gold>, path: &String) -> u32 {
	let path = Path::new(path);
	let file_res = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};
	let reader = BufReader::new(&file_res);
	let res: Vec<_> = reader.lines().collect();
	gold.iter()
		.filter_map(|x| {
			// FILTERING
			if (x.mb as f64) / (x.tb as f64) >= BLAST_FILT {
				let line_buf = res[(x.id - 1) as usize].as_ref().unwrap();
				let mut counter: u32 = 0;
				if !line_buf.is_empty() {
					line_buf.split('\t').for_each(|loc_stra| {
						let mut loc_stra = loc_stra.split('.');
						let stra = match loc_stra.next().unwrap().chars().collect::<Vec<_>>()[0] {
							'+' => true,
							'-' => false,
							_ => panic!("parse"),
						};
						let loc: u32 = loc_stra.next().unwrap().parse().unwrap();
						if stra == x.strand && loc >= x.start && loc <= x.end {
							counter += 1;
						}
					});
				}
				println!("{}", counter);
				Some(counter)
			} else {
				None
			}
		})
		.min()
		.unwrap()
}

fn get_threshold(gold: &Vec<Gold>, path: &String) -> u32 {
	let path = Path::new(path);
	let file_res = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};
	let reader = BufReader::new(&file_res);
	let res: Vec<_> = reader.lines().collect();
	gold.iter()
		.filter_map(|x| {
			// FILTERING
			if (x.mb as f64) / (x.tb as f64) >= BLAST_FILT {
				let line_buf = res[(x.id - 1) as usize].as_ref().unwrap();
				let mut counter: u32 = 0;
				if !line_buf.is_empty() {
					line_buf.split('\t').for_each(|loc_stra| {
						let mut loc_stra = loc_stra.split('.');
						let stra = match loc_stra.next().unwrap().chars().collect::<Vec<_>>()[0] {
							'+' => true,
							'-' => false,
							_ => panic!("parse"),
						};
						let loc: u32 = loc_stra.next().unwrap().parse().unwrap();
						if stra == x.strand && loc >= x.start && loc <= x.end {
							counter += 1;
						}
					});
				}
				Some(counter)
			} else {
				None
			}
		})
		.min()
		.unwrap()
}

fn get_stats(gold: &Vec<Gold>, path: &String) -> (u32, u32) {
	let mut fn_n = 0;
	let path = Path::new(path);
	let file_res = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};
	let reader = BufReader::new(&file_res);
	let res: Vec<_> = reader.lines().collect();
	gold.iter().for_each(|x| {
		// FILTERING
		if (x.mb as f64) / (x.tb as f64) >= BLAST_FILT {
			let line_buf = res[(x.id - 1) as usize].as_ref().unwrap();
			let mut found = false;
			if !line_buf.is_empty() {
				line_buf.split('\t').for_each(|loc_stra| {
					let mut loc_stra = loc_stra.split('.');
					let stra = match loc_stra.next().unwrap().chars().collect::<Vec<_>>()[0] {
						'+' => true,
						'-' => false,
						_ => panic!("parse"),
					};
					let loc: u32 = loc_stra.next().unwrap().parse().unwrap();
					if stra == x.strand && loc >= x.start && loc <= x.end {
						found = true;
					}
				});
			}
			if !found {
				fn_n += 1;
			}
		}
	});
	let mut map_n: u32 = 0;
	res.iter().for_each(|x| {
		map_n += x.as_ref().unwrap().split('\t').collect::<Vec<_>>().len() as u32;
	});
	(map_n + fn_n - gold.len() as u32, fn_n)
}

struct Gold {
	id: u32,
	strand: bool,
	start: u32,
	end: u32,
	mb: u32,
	tb: u32,
}
