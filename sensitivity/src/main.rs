use std::fs::File;
use std::io::Write;
use std::io::{BufRead, BufReader};
use std::path::Path;
use std::process::Command;
use std::time::Instant;
mod offset;

const BLAST_FILT: f64 = 0.0;

fn main() {
	let gold = get_gold();
	let path = Path::new("sensitivity.dat");
	let mut file = match File::create(&path) {
		Err(why) => panic!("open {}: {}", path.display(), why),
		Ok(file) => file,
	};

	(12..40).for_each(|w_ref| {
		build_index(w_ref);
		// If we want to test different window sizes for the reference genome and the reads
		(w_ref..w_ref+1).for_each(|w_read| {
			let time_si = seed_index(w_read);
			let threshold = get_threshold(&gold);
			let time_sia = seed_index_adja(w_read, threshold);
			let (fp_n, fn_n) = get_false(&gold);
			writeln!(&mut file, "{}\t{}\t{}\t{}\t{}\t{}", w_ref, time_si, time_sia, threshold, fp_n, fn_n).unwrap();
			println!("w: {}\ttime_si: {}\ttime_sia: {}\tthreshold: {}\tfalse_positive: {}\tfalse_negative: {}", w_ref, time_si, time_sia, threshold, fp_n, fn_n);
		});
	});
}

fn build_index(w_ref: u32) {
	let w = format!("-w{}", w_ref);
	let status = Command::new("./indexdna")
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
	assert!(status.success());
}

fn seed_index_adja(w_read: u32, threshold: u32) -> u64 {
	let env = format!(
		"-DADJACENCY_FILTERING_THRESHOLD={} -DADJACENCY_FILTERING -DW={}",
		threshold, w_read
	);
	let status = Command::new("make")
		.arg("clean")
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	assert!(status.success());
	let status = Command::new("make")
		.env("CFLAGS", env)
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	let now = Instant::now();
	assert!(status.success());
	let status = Command::new("./map-pacbio")
		.args(["-i/tmp/idx.bin", "../res/pacbio_1000.bin", "-o/tmp/locs"])
		.current_dir("../cpu_impl")
		.status()
		.expect("map-pacbio");
	assert!(status.success());
	now.elapsed().as_secs()
}

fn seed_index(w_read: u32) -> u64 {
	let env = format!("-DW={}", w_read);
	let status = Command::new("make")
		.arg("clean")
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	assert!(status.success());
	let status = Command::new("make")
		.env("CFLAGS", env)
		.current_dir("../cpu_impl")
		.status()
		.expect("make");
	let now = Instant::now();
	assert!(status.success());
	let status = Command::new("./map-pacbio")
		.args(["-i/tmp/idx.bin", "../res/pacbio_1000.bin", "-o/tmp/locs"])
		.current_dir("../cpu_impl")
		.status()
		.expect("map-pacbio");
	assert!(status.success());
	now.elapsed().as_secs()
}

fn get_gold() -> Vec<Gold> {
	let path = Path::new("../res/gold_pacbio_1000.paf");
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
		})
		.collect()
}

fn get_threshold(gold: &Vec<Gold>) -> u32 {
	let path = Path::new("/tmp/locs.dat");
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
		.unwrap()
}

fn get_false(gold: &Vec<Gold>) -> (u32, u32) {
	let mut fn_n = 0;
	let path = Path::new("/tmp/locs.dat");
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
