#include "querying.hpp"

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_0_o,
                     hls::stream<ms_pos_t> &ms_pos_1_o) {
	seed_t seed = seed_i.read();

	while (seed.str == 0 || seed.EOR == 0) {
		if (seed.EOR == 1) {
			ms_pos_0_o << ms_pos_t{.str = 0, .EOR = 1};
			ms_pos_1_o << ms_pos_t{.str = 0, .EOR = 1};
		} else {
			const uint32_t bucket_id = seed.hash.range(bucket_id_msb, 0).to_uint();
			// access the map
			const ap_uint<32> start = ap_uint<32>((bucket_id == 0) ? 0 : map_i[bucket_id - 1]);
			const ap_uint<32> end   = ap_uint<32>(map_i[bucket_id]);

			// std::cout << std::hex << "start: " << start << " end: " << end << std::endl;

			// If there are some locations
			if (start != end) {

				// find out in which MS are the locations
				const ap_uint<ms_id_size> ms_id_start = start.range(31, ms_id_lsb);
				const ap_uint<ms_id_size> ms_id       = end.range(31, ms_id_lsb);

				ms_pos_t ms_pos;
				ms_pos.end_pos   = end.range(ms_pos_msb, 0);
				ms_pos.seed_id   = seed.hash.range(seed_id_msb, seed_id_lsb);
				ms_pos.query_loc = seed.loc.to_uint();
				ms_pos.str       = seed.str;
				ms_pos.EOR       = 0;

				// In case we are at the begining of a MS
				if (ms_id != ms_id_start) {
					ms_pos.start_pos = 0;
				} else {
					ms_pos.start_pos = start.range(ms_pos_msb, 0);
				}

				switch (ms_id) {
					case 0:
						ms_pos_0_o << ms_pos;
						break;
					default:
						ms_pos_1_o << ms_pos;
						break;
				}
			}
		}
		seed = seed_i.read();
	}
	ms_pos_0_o << ms_pos_t{.str = 1, .EOR = 1};
	ms_pos_1_o << ms_pos_t{.str = 1, .EOR = 1};
}

inline loc_t extract_key_loc(const uint64_t key_i, const ap_uint<1> query_str_i, const uint32_t query_loc_i) {
	ap_uint<64> key(key_i);
	return loc_t{
	    .target_loc = key.range(29, 0),
	    .query_loc  = ap_uint<21>(query_loc_i),
	    .chrom_id   = key.range(41, 32),
	    .str        = key.range(42, 42) ^ query_str_i,
	    .EOR        = ap_uint<1>(0),
	};
}

inline uint64_t loc_to_uint64(loc_t loc_i) {
	ap_uint<64> loc = (loc_i.EOR, loc_i.str, loc_i.chrom_id, loc_i.query_loc, loc_i.target_loc);
	return loc.to_uint64();
}

inline loc_t uint64_to_loc(uint64_t loc_i) {
	ap_uint<64> loc = ap_uint<64>(loc_i);
	return loc_t{
	    .target_loc = loc.range(29, 0),
	    .query_loc  = loc.range(50, 30),
	    .chrom_id   = loc.range(60, 51),
	    .str        = loc.range(62, 61),
	    .EOR        = loc.range(63, 63),
	};
}

void copy_key_value(const uint64_t *key_i, loc_t &loc_key, ap_uint<1> &enable, uint64_t *buf_o, const uint32_t buf_j,
                    uint32_t &key_j, const uint32_t end_pos, const uint64_t key_i_i, const ap_uint<1> query_str_i,
                    const uint32_t query_loc_i, const ap_uint<seed_id_size> seed_id_i) {
	// Copy loc into the buffer
	buf_o[buf_j] = loc_to_uint64(loc_key);
	// Check that we are not at the end
	if (key_j == end_pos) {
		enable = 0;
	} else {
		// Check if we have the good seed_id
		const uint64_t key = key_i[key_j];
		key_j++;
		const ap_uint<64> key_uint          = ap_uint<64>(key);
		const ap_uint<seed_id_size> seed_id = key_uint.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);
		if (seed_id != seed_id_i) {
			enable = 0;
		} else {
			loc_key = extract_key_loc(key_i_i, query_str_i, query_loc_i);
		}
	}
}

buf_metadata_t query_index_key_MS(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *key_i, uint64_t *buf) {
	ms_pos_t pos = ms_pos_i.read();
	buf_metadata_t metadata{.pos = 0, .len = 0, .EOS = 0};

	while (pos.EOR != 1) {
		std::cout << "start_pos: " << std::hex << pos.start_pos << " end_pos: " << pos.end_pos
		          << " seed_id: " << pos.seed_id << " query_loc: " << pos.query_loc << std::endl;
		// Eliminate the false-positive
		const uint32_t start_pos   = pos.start_pos.to_uint();
		const uint32_t end_pos     = pos.end_pos.to_uint();
		const uint32_t query_loc   = pos.query_loc;
		const ap_uint<1> query_str = pos.str;

		uint32_t key_j = start_pos;
		uint64_t key;

		ap_uint<1> found              = 0;
		ap_uint<1> continue_searching = 1;

		// Check if we find locations with the same seed_id
		while (continue_searching) {
			key = key_i[key_j];
			key_j++;

			const ap_uint<64> uint_key = ap_uint<64>(key);
			const ap_uint<seed_id_size> seed_id =
			    uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);
			if (seed_id == pos.seed_id) {
				found = 1;
			}
			if (key_j == end_pos || seed_id >= pos.seed_id) {
				continue_searching = 0;
			}
		}

		// DEBUG
		if (found) {
			unsigned counter              = 1;
			uint32_t debug_j              = key_j;
			const ap_uint<64> uint_key    = ap_uint<64>(key);
			ap_uint<seed_id_size> seed_id = uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);

			// std::cout << "key: " << std::hex << key << " seed_id: " << seed_id << std::endl;

			while (debug_j != end_pos && seed_id == pos.seed_id) {
				key = key_i[debug_j];
				debug_j++;

				const ap_uint<64> uint_key = ap_uint<64>(key);
				seed_id = uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);
				if (seed_id == pos.seed_id) {
					// std::cout << "key: " << std::hex << key << " seed_id: " << seed_id <<
					// std::endl;
					counter++;
				}
			}
			std::cout << "new_val: " << std::dec << counter << std::endl;
		}

		// If there are seed hits
		if (found) {
			uint32_t buf_o_j = (metadata.pos + metadata.len) % ms_buf_size;
			loc_t loc_key    = extract_key_loc(key, query_str, query_loc);

			// First case: need to merge
			if (metadata.len != 0) {
				uint32_t buf_i_j         = metadata.pos;
				const uint32_t buf_i_end = buf_o_j;

				loc_t loc_buf    = uint64_to_loc(buf[buf_i_j]);
				ap_uint<1> merge = 1;
				while (merge) {
					// First compare the str, then the chrom_id, then the target_loc
					ap_uint<40> cmp_key = (loc_key.chrom_id, loc_key.target_loc);
					ap_uint<40> cmp_buf = (loc_buf.chrom_id, loc_buf.target_loc);
					// write the buf value
					if (cmp_buf <= cmp_key) {
						// std::cout << "Write buf_value" << std::endl;
						buf[buf_o_j] = loc_to_uint64(loc_buf);
						buf_i_j      = (buf_i_j + 1) % ms_buf_size;
						if (buf_i_j == buf_i_end) {
							merge = 0;
						} else {
							loc_buf = uint64_to_loc(buf[buf_i_j]);
						}
					}
					// write the key value
					else {
						// std::cout << "Write key_value" << std::endl;
						copy_key_value(key_i, loc_key, merge, buf, buf_o_j, key_j, end_pos, key,
						               query_str, query_loc, pos.seed_id);
					}
					buf_o_j = (buf_o_j + 1) % ms_buf_size;
				}
				/*
				// DEBUG
				const uint32_t new_pos = (metadata.pos + metadata.len) % ms_buf_size;
				const uint32_t new_len =
				    (buf_o_j < new_pos) ? ms_buf_size + buf_o_j - new_pos : buf_o_j - new_pos;
				std::cout << "intermediate_len " << new_len << std::endl;
				*/

				// Copy the last values
				if (buf_i_j != buf_i_end) {
					while (buf_i_j != buf_i_end) {
						buf[buf_o_j] = buf[buf_i_j];
						buf_i_j      = (buf_i_j + 1) % ms_buf_size;
						buf_o_j      = (buf_o_j + 1) % ms_buf_size;
						/*
						const uint32_t new_pos = (metadata.pos + metadata.len) % ms_buf_size;
						const uint32_t new_len = (buf_o_j < new_pos)
						                             ? ms_buf_size + buf_o_j - new_pos
						                             : buf_o_j - new_pos;
						std::cout << "intermediate_len " << new_len << std::endl;
						*/
					}
				} else {
					ap_uint<1> copy = 1;
					while (copy) {
						copy_key_value(key_i, loc_key, copy, buf, buf_o_j, key_j, end_pos, key,
						               query_str, query_loc, pos.seed_id);
						buf_o_j = (buf_o_j + 1) % ms_buf_size;
					}
				}
			}

			// Second case: Initialization
			else {
				ap_uint<1> copy = 1;
				while (copy) {
					copy_key_value(key_i, loc_key, copy, buf, buf_o_j, key_j, end_pos, key,
					               query_str, query_loc, pos.seed_id);
					buf_o_j = (buf_o_j + 1) % ms_buf_size;
				}
			}

			// Update buffer metadata
			const uint32_t new_pos = (metadata.pos + metadata.len) % ms_buf_size;
			const uint32_t new_len =
			    (buf_o_j < new_pos) ? ms_buf_size + buf_o_j - new_pos : buf_o_j - new_pos;
			metadata.pos = new_pos;
			metadata.len = new_len;

			std::cout << std::dec << "out_len :" << metadata.len << std::endl;
		}
		pos = ms_pos_i.read();
		/*
		std::cout << "EOR: " << pos.EOR << " start_pos: " << std::hex << pos.start_pos
		          << " end_pos: " << pos.end_pos << " seed_id: " << pos.seed_id << std::dec
		          << " query_loc: " << pos.query_loc << std::endl;
		          */
	}

	// End of sequence
	if (pos.str == 1) {
		metadata.EOS = 1;
	}
	return metadata;
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_0_i, hls::stream<ms_pos_t> &ms_pos_1_i, const uint64_t *key_0_i,
                     const uint64_t *key_1_i, uint64_t *buf_0_i, uint64_t *buf_1_i, hls::stream<loc_t> &location_o) {
	buf_metadata_t metadata0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
	std::cout << "MS0: " << std::dec << metadata0.len << std::endl;
	buf_metadata_t metadata1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);
	std::cout << "MS1: " << metadata1.len << std::endl;

	while (!metadata0.EOS && !metadata1.EOS) {
		// merge
		metadata0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
		std::cout << "MS0: " << metadata0.len << std::endl;
		metadata1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);
		std::cout << "MS1: " << metadata1.len << std::endl;
	}
	location_o << loc_t{.target_loc = 0, .query_loc = 0, .chrom_id = 0, .str = 1, .EOR = 1};
}
