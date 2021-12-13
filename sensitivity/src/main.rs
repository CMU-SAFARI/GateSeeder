use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

fn main() {
	let path = Path::new("../res/gold_pacbio_10000.paf");
	let file_gold = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	let path = Path::new("../res/res_pacbio_10000.paf");
	let file_res = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	let reader = BufReader::new(&file_gold);
	let gold = reader.lines().map(|line| {
		let line_buf = line.unwrap();
		let sec: Vec<_> = line_buf.split('\t').collect();
		Gold {
			id: sec[0].split('.').collect::<Vec<_>>()[1].parse().unwrap(),
			strand: match sec[4].chars().collect::<Vec<_>>()[0] {
				'+' => true,
				'-' => false,
				_ => panic!("parse"),
			},
			start: sec[7].parse().unwrap(),
			end: sec[8].parse().unwrap(),
			mb: sec[9].parse().unwrap(),
			tb: sec[10].parse().unwrap(),
		}
	});

	let reader = BufReader::new(&file_res);
	let res: Vec<_> = reader.lines().collect();
	gold.for_each(|x| {
		let line_buf = res[(x.id - 1) as usize].as_ref().unwrap();
		let mut counter: u32 = 0;
		line_buf.split('\t').for_each(|loc_stra| {
			let mut loc_stra = loc_stra.split('.');
			let stra = match loc_stra.next().unwrap().chars().collect::<Vec<_>>()[0] {
				'+' => true,
				'-' => false,
				_ => panic!("parse"),
			};
			let loc: u32 = loc_stra.next().unwrap().parse().unwrap();
			if stra == x.strand && loc > x.start && loc < x.end {
				counter += 1;
			}
		});
		println!("{}", counter);
	});
}

struct Gold {
	id: u32,
	strand: bool,
	start: u32,
	end: u32,
	mb: u32,
	tb: u32,
}
