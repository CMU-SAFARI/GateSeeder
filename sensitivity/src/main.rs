use std::fs::File;
use std::io::Write;
use std::io::{BufRead, BufReader};
use std::path::Path;
use std::process::Command;
use std::time::Instant;
mod offset;

fn main() {
	let path = Path::new("plot.dat");
	let mut file = match File::create(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	(10..20).for_each(|w_ref| {
		make_index(w_ref);
		(w_ref..31).for_each(|w_read| {
			let time = seed_index(w_read);
			let threshold = get_stats();
			writeln!(&mut file, "{}\t{}\t{}\t{}", w_ref, w_read, time, threshold).unwrap();
			println!("{}\t{}\t{}\t{}", w_ref, w_read, time, threshold);
		});
	});
}

fn make_index(w_ref: u32) {
	let w = format!("-w{}", w_ref);
	Command::new("./indexdna")
		.args([
			"-k19",
			&w,
			"-b26",
			"-f2000",
			"../res/GCF_000001405.39_GRCh38.p13_genomic.fna",
			"/tmp/idx",
		])
		.current_dir("../dna_indexing")
		.status()
		.expect("indexdna");
}

fn seed_index(w_read: u32) -> u64 {
	let w = format!("CFLAGS+='-DW={}'", w_read);
	Command::new("make")
		.arg("clean")
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	Command::new("make")
		.arg(&w)
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	let now = Instant::now();
	Command::new("./map-pacbio")
		.args(["-i/tmp/idx.bin", "../res/pacbio_10000.bin", "-o/tmp/locs"])
		.current_dir("../cpu_impl")
		.status()
		.expect("map-pacbio");
	now.elapsed().as_secs()
}

fn get_stats() -> u32 {
	let path = Path::new("../res/gold_pacbio_10000.paf");
	let file_gold = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	let path = Path::new("/tmp/locs.dat");
	let file_res = match File::open(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	let offset_map = offset::get_offset();

	let reader = BufReader::new(&file_gold);
	let gold = reader.lines().filter_map(|line| {
		let line_buf = line.unwrap();
		let sec: Vec<_> = line_buf.split('\t').collect();
		let offset: u32 = *offset_map.get(sec[5]).unwrap();
		let tb = sec[10].parse().unwrap();
		//Filtering
		if sec[12].eq("tp:A:P") && tb >= 10000 {
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
	});

	let reader = BufReader::new(&file_res);
	let res: Vec<_> = reader.lines().collect();
	let min = gold
		.filter_map(|x| {
			let line_buf = res[(x.id - 1) as usize].as_ref().unwrap();
			let mut counter: u32 = 0;
			if (x.mb as f64) / (x.tb as f64) >= 0.5 {
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
				Some(counter)
			} else {
				None
			}
		})
		.min()
		.unwrap();
	min
}

struct Gold {
	id: u32,
	strand: bool,
	start: u32,
	end: u32,
	mb: u32,
	tb: u32,
}
