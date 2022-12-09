#include "querying.hpp"
#include <string.h>

void query_index_map(hls::stream<seed_t> &seed_i, const uint32_t *map_i, hls::stream<ms_pos_t> &ms_pos_0_o,
                     hls::stream<ms_pos_t> &ms_pos_1_o) {
	seed_t seed = seed_i.read();

query_index_map_loop:
	while (seed.str == 0 || seed.EOR == 0) {
#pragma HLS pipeline II = 2
		if (seed.EOR == 1) {
			ms_pos_0_o << ms_pos_t{
			    .start_pos = 0, .end_pos = 0, .seed_id = 0, .query_loc = 0, .str = 0, .EOR = 1};
			ms_pos_1_o << ms_pos_t{
			    .start_pos = 0, .end_pos = 0, .seed_id = 0, .query_loc = 0, .str = 0, .EOR = 1};
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
	ms_pos_0_o << ms_pos_t{.start_pos = 0, .end_pos = 0, .seed_id = 0, .query_loc = 0, .str = 1, .EOR = 1};
	ms_pos_1_o << ms_pos_t{.start_pos = 0, .end_pos = 0, .seed_id = 0, .query_loc = 0, .str = 1, .EOR = 1};
}

#define LOC_OFFSET (1 << 21)
inline loc_t extract_key_loc(const uint64_t key_i, const ap_uint<1> query_str_i, const uint32_t query_loc_i) {
	ap_uint<64> key(key_i);
	// std::cout << "extract_key_loc: " << std::hex << key_i << std::endl;
	const ap_uint<1> str = key.range(42, 42) ^ query_str_i;
	ap_uint<30> target_loc =
	    key.range(29, 0) + (str ? ap_uint<30>(query_loc_i - SE_K + 1) : ap_uint<30>(LOC_OFFSET - query_loc_i));
	// std::cout << "target_loc: " << std::hex << target_loc << std::endl;
	return loc_t{
	    .target_loc = target_loc,
	    .query_loc  = ap_uint<21>(query_loc_i),
	    .chrom_id   = key.range(41, 32),
	    .str        = str,
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
                    uint32_t &key_j, const uint32_t end_pos, const ap_uint<1> query_str_i, const uint32_t query_loc_i,
                    const ap_uint<seed_id_size> seed_id_i) {
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
			loc_key = extract_key_loc(key, query_str_i, query_loc_i);
		}
	}
}

buf_metadata_t query_index_key_MS(hls::stream<ms_pos_t> &ms_pos_i, const uint64_t *key_i, uint64_t *buf) {
	ms_pos_t pos = ms_pos_i.read();
	buf_metadata_t metadata{.pos = 0, .len = 0, .EOS = 0};

query_index_key_MS_loop:
	while (pos.EOR == 0) {
		/*
		std::cout << "start_pos: " << std::hex << pos.start_pos << " end_pos: " << pos.end_pos
		          << " seed_id: " << pos.seed_id << " query_loc: " << pos.query_loc << std::endl;
		          */
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
	search_key_loop:
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

		/*
		// DEBUG
		if (found) {
		        uint64_t key_debug            = key;
		        unsigned counter              = 1;
		        uint32_t debug_j              = key_j;
		        const ap_uint<64> uint_key    = ap_uint<64>(key_debug);
		        ap_uint<seed_id_size> seed_id = uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);

		        std::cout << "key: " << std::hex << key_debug << " seed_id: " << seed_id << std::endl;

		        while (debug_j != end_pos && seed_id == pos.seed_id) {
		                key_debug = key_i[debug_j];
		                debug_j++;

		                const ap_uint<64> uint_key = ap_uint<64>(key_debug);
		                seed_id = uint_key.range(LOC_SHIFT + 1 + seed_id_size, LOC_SHIFT + 1);
		                if (seed_id == pos.seed_id) {
		                        std::cout << "key: " << std::hex << key_debug << " seed_id: " << seed_id
		                                  << std::endl;
		                        counter++;
		                }
		        }
		        std::cout << "new_val: " << std::dec << counter << std::endl;
		}
		*/

		// If there are seed hits
		if (found) {
			uint32_t buf_o_j = (metadata.pos + metadata.len) % MS_BUF_LEN;
			loc_t loc_key    = extract_key_loc(key, query_str, query_loc);
			// std::cout << "key: " << std::hex << key << std::endl;

			// First case: need to merge
			if (metadata.len != 0) {
				uint32_t buf_i_j         = metadata.pos;
				const uint32_t buf_i_end = buf_o_j;

				loc_t loc_buf    = uint64_to_loc(buf[buf_i_j]);
				ap_uint<1> merge = 1;
			query_index_key_MS_merge_loop:
				while (merge) {
					// First compare the str, then the chrom_id, then the target_loc
					const ap_uint<40> cmp_key = (loc_key.chrom_id, loc_key.target_loc);
					const ap_uint<40> cmp_buf = (loc_buf.chrom_id, loc_buf.target_loc);
					// write the buf value
					if (cmp_buf <= cmp_key) {
						// std::cout << "Write buf_value" << std::endl;
						buf[buf_o_j] = loc_to_uint64(loc_buf);
						buf_i_j      = (buf_i_j + 1) % MS_BUF_LEN;
						if (buf_i_j == buf_i_end) {
							merge = 0;
						} else {
							loc_buf = uint64_to_loc(buf[buf_i_j]);
						}
					}
					// write the key value
					else {
						// std::cout << "Write key_value" << std::endl;
						copy_key_value(key_i, loc_key, merge, buf, buf_o_j, key_j, end_pos,
						               query_str, query_loc, pos.seed_id);
					}
					buf_o_j = (buf_o_j + 1) % MS_BUF_LEN;
				}
				/*
				// DEBUG
				const uint32_t new_pos = (metadata.pos + metadata.len) % MS_BUF_LEN;
				const uint32_t new_len =
				    (buf_o_j < new_pos) ? MS_BUF_LEN + buf_o_j - new_pos : buf_o_j - new_pos;
				std::cout << "intermediate_len " << new_len << std::endl;
				*/

				// Copy the last values
				if (buf_i_j != buf_i_end) {
				query_index_key_MS_copy0_loop:
					while (buf_i_j != buf_i_end) {
						buf[buf_o_j] = buf[buf_i_j];
						buf_i_j      = (buf_i_j + 1) % MS_BUF_LEN;
						buf_o_j      = (buf_o_j + 1) % MS_BUF_LEN;
						/*
						const uint32_t new_pos = (metadata.pos + metadata.len) % MS_BUF_LEN;
						const uint32_t new_len = (buf_o_j < new_pos)
						                             ? MS_BUF_LEN + buf_o_j - new_pos
						                             : buf_o_j - new_pos;
						std::cout << "intermediate_len " << new_len << std::endl;
						*/
					}
				} else {
					ap_uint<1> copy = 1;
				query_index_key_MS_copy1_loop:
					while (copy) {
						copy_key_value(key_i, loc_key, copy, buf, buf_o_j, key_j, end_pos,
						               query_str, query_loc, pos.seed_id);
						buf_o_j = (buf_o_j + 1) % MS_BUF_LEN;
					}
				}
			}

			// Second case: Initialization
			else {
				ap_uint<1> copy = 1;
			query_index_key_MS_init_loop:
				while (copy) {
					// std::cout << "key: " << std::hex << loc_to_uint64(loc_key) << std::endl;
					copy_key_value(key_i, loc_key, copy, buf, buf_o_j, key_j, end_pos, query_str,
					               query_loc, pos.seed_id);
					buf_o_j = (buf_o_j + 1) % MS_BUF_LEN;
				}
			}

			// Update buffer metadata
			const uint32_t new_pos = (metadata.pos + metadata.len) % MS_BUF_LEN;
			const uint32_t new_len =
			    (buf_o_j < new_pos) ? MS_BUF_LEN + buf_o_j - new_pos : buf_o_j - new_pos;
			metadata.pos = new_pos;
			metadata.len = new_len;

			/*
			std::cout << std::dec << "out_len :" << metadata.len << std::endl;
			for (unsigned i = new_pos; i < new_pos + new_len; i++) {
			        loc_t loc = uint64_to_loc(buf[i]);
			        std::cout << std::hex << "target_loc: " << loc.target_loc
			                  << " query_loc: " << loc.query_loc << " chrom_id: " << loc.chrom_id
			                  << std::endl;
			}
			*/
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

void merge_buf(const uint64_t *buf_0_i, const uint64_t *buf_1_i, const buf_metadata_t metadata_0_i,
               const buf_metadata_t metadata_1_i, hls::stream<loc_t> &location_o) {
	uint32_t pos0_j = metadata_0_i.pos;
	uint32_t pos1_j = metadata_1_i.pos;

	const uint32_t end0 = (metadata_0_i.pos + metadata_0_i.len) % MS_BUF_LEN;
	const uint32_t end1 = (metadata_1_i.pos + metadata_1_i.len) % MS_BUF_LEN;
	/*
	std::cout << "pos0: " << std::hex << metadata_0_i.pos << std::dec << " len0: " << metadata_0_i.len << std::endl;
	std::cout << "pos1: " << std::hex << metadata_1_i.pos << std::dec << " len1: " << metadata_1_i.len << std::endl;
	*/

	// If locations
	if (pos0_j != end0 || pos1_j != end1) {
		loc_t loc0 = uint64_to_loc(buf_0_i[pos0_j]);
		loc_t loc1 = uint64_to_loc(buf_1_i[pos1_j]);

		// If we need to merge
	merge_buf_merge_loop:
		while (pos0_j != end0 && pos1_j != end1) {
			const ap_uint<40> cmp0 = (loc0.chrom_id, loc0.target_loc);
			const ap_uint<40> cmp1 = (loc1.chrom_id, loc1.target_loc);
			if (cmp0 <= cmp1) {
				location_o << loc0;
				pos0_j = (pos0_j + 1) % MS_BUF_LEN;
				loc0   = uint64_to_loc(buf_0_i[pos0_j]);
			} else {
				location_o << loc1;
				pos1_j = (pos1_j + 1) % MS_BUF_LEN;
				loc1   = uint64_to_loc(buf_1_i[pos1_j]);
			}
		}

		// Copy the remaining values
		if (pos0_j == end0) {
		merge_buf_copy0_loop:
			while (pos1_j != end1) {
				location_o << loc1;
				pos1_j = (pos1_j + 1) % MS_BUF_LEN;
				loc1   = uint64_to_loc(buf_1_i[pos1_j]);
			}
		} else {
		merge_buf_copy1_loop:
			while (pos0_j != end0) {
				location_o << loc0;
				pos0_j = (pos0_j + 1) % MS_BUF_LEN;
				loc0   = uint64_to_loc(buf_0_i[pos0_j]);
			}
		}
	}
	location_o << loc_t{.target_loc = 0, .query_loc = 0, .chrom_id = 0, .str = 0, .EOR = 1};
}

void query_index_key(hls::stream<ms_pos_t> &ms_pos_0_i, hls::stream<ms_pos_t> &ms_pos_1_i, const uint64_t *key_0_i,
                     const uint64_t *key_1_i, uint64_t *buf_0_i, uint64_t *buf_1_i, hls::stream<loc_t> &location_o) {
	buf_metadata_t metadata0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
	// DEBUG
	/*
	std::cout << "MS0: " << std::dec << metadata0.len << std::endl;
	for (unsigned i = 0; i < metadata0.len; i++) {
	        loc_t loc = uint64_to_loc(buf_0_i[i + metadata0.pos]);
	        std::cout << std::hex << "target_loc: " << loc.target_loc << " query_loc: " << loc.query_loc
	                  << " chrom_id: " << loc.chrom_id << " str: " << loc.str << std::endl;
	}
	*/

	buf_metadata_t metadata1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);

	// DEBUG
	/*
	std::cout << "MS1: " << std::dec << metadata1.len << std::endl;
	for (unsigned i = 0; i < metadata1.len; i++) {
	        loc_t loc = uint64_to_loc(buf_1_i[i + metadata1.pos]);
	        std::cout << std::hex << "target_loc: " << loc.target_loc << " query_loc: " << loc.query_loc
	                  << " chrom_id: " << loc.chrom_id << std::endl;
	}
	*/

query_index_key_loop:
	while (!metadata0.EOS && !metadata1.EOS) {
		// merge
		merge_buf(buf_0_i, buf_1_i, metadata0, metadata1, location_o);
		metadata0 = query_index_key_MS(ms_pos_0_i, key_0_i, buf_0_i);
		metadata1 = query_index_key_MS(ms_pos_1_i, key_1_i, buf_1_i);
	}
	location_o << loc_t{.target_loc = 0, .query_loc = 0, .chrom_id = 0, .str = 1, .EOR = 1};
}

void write_locations(hls::stream<loc_t> &location_i, uint64_t *location_o) {
	loc_t loc  = location_i.read();
	uint32_t i = 0;
write_locations_loop:
	while (loc.EOR != 1 || loc.str != 1) {
		location_o[i] = loc_to_uint64(loc);
		i++;
		loc = location_i.read();
	}
	location_o[i] = UINT64_MAX;
}
