use std::env::args;
use std::error::Error;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

fn main() -> Result<(), Box<dyn Error>> {
	let args: Vec<_> = args().collect();

	let path = Path::new(&args[1]);
	let file_seedfarm = File::open(&path)?;

	let lines = BufReader::new(&file_seedfarm).lines();

	let mut best_vtd = 0;
	let mut cur_vtd = 0;
	let mut best_score = std::f32::NEG_INFINITY;
	let mut max_occ = 0;
	let mut perf: Vec<(f32, u32)> = vec![];

	for l in lines {
		let l = l?;
		let field: Vec<_> = l.split('\t').collect();
		if field[0].eq("P") {
			// Avoid first case
			if max_occ != 0 {
				let score = compute_score(&perf);
				println!("{}", score);
				if score > best_score {
					best_score = score;
					best_vtd = cur_vtd;
				}
			}

			let cur_max_occ = field[1].parse()?;
			cur_vtd = field[2].parse()?;

			if cur_max_occ != max_occ {
				//print the values with the best score
				if max_occ != 0 {
					println!("{}\t{}", max_occ, best_vtd);
				}
				best_score = std::f32::NEG_INFINITY;
				max_occ = cur_max_occ;
				perf = vec![];
			}
		} else {
			perf.push((field[4].parse()?, field[5].parse()?));
		}
	}

	if max_occ != 0 {
		let score = compute_score(&perf);
		if score > best_score {
			best_vtd = cur_vtd;
		}
		println!("{}\t{}", max_occ, best_vtd);
	}
	Ok(())
}

fn compute_score(perf: &Vec<(f32, u32)>) -> f32 {
	let mut score: f32 = 0.0;
	for i in 0..perf.len() - 1 {
		let x0 = perf[i].0;
		let x1 = perf[i + 1].0;

		let y0 = perf[i].1;
		let y1 = perf[i + 1].1;

		score += (x1 - x0) * (y1 + y0) as f32 / 2.0;
	}
	score / (perf[perf.len() - 1].0 - perf[0].0)
}
