use std::env::args;
use std::error::Error;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

fn main() -> Result<(), Box<dyn Error>> {
	let args: Vec<_> = args().collect();

	let path = Path::new(&args[1]);
	let file_minimap2 = File::open(&path)?;

	let path = Path::new(&args[2]);
	let file_seedfarm = File::open(&path)?;

	let lines = BufReader::new(&file_minimap2).lines();

	let minimap2_perf: Result<Vec<_>, _> = lines
		.map(|l| {
			let l = l?;
			let field: Vec<_> = l.split('\t').collect();
			let res = (field[4].parse::<f32>()?, field[5].parse::<u32>()?);
			Ok::<(f32, u32), Box<dyn Error>>(res)
		})
		.collect();

	let minimap2_perf = minimap2_perf?;

	let lines = BufReader::new(&file_seedfarm).lines();

	let mut best_vtd = 0;
	let mut cur_vtd = 0;
	let mut best_score = std::i32::MIN;
	let mut max_occ = 0;
	let mut perf: Vec<(f32, u32)> = vec![];

	for l in lines {
		let l = l?;
		let field: Vec<_> = l.split('\t').collect();
		if field[0].eq("P") {
			// Avoid first case
			if max_occ != 0 {
				let score = compute_score(&minimap2_perf, &perf);
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
				best_score = std::i32::MIN;
				max_occ = cur_max_occ;
				perf = vec![];
			}
		} else {
			perf.push((field[4].parse()?, field[5].parse()?));
		}
	}

	if max_occ != 0 {
		let score = compute_score(&minimap2_perf, &perf);
		if score > best_score {
			best_vtd = cur_vtd;
		}
		println!("{}\t{}", max_occ, best_vtd);
	}
	Ok(())
}

fn compute_score(minimap2_perf: &Vec<(f32, u32)>, perf: &Vec<(f32, u32)>) -> i32 {
	perf.iter().fold(0, |score, (error, nb_mapped)| {
		for i in 0..minimap2_perf.len() - 1 {
			let x0 = minimap2_perf[i].0;
			let x1 = minimap2_perf[i + 1].0;
			if error >= &x0 && error <= &x1 {
				let y0 = minimap2_perf[i].1;
				let y1 = minimap2_perf[i + 1].1;
				let a: f32 = (y1 - y0) as f32 / (x1 - x0);
				let b: f32 = y0 as f32 - x0 * a;
				return score + *nb_mapped as i32 - (a * error + b) as i32;
			}
		}
		score
	})
}
