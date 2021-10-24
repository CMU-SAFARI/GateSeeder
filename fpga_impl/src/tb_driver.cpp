#include "tb_driver.hpp"
#include "seeding.hpp"
#define READ_BUFFER_SIZE 2147483648

using namespace std;

static base_t seq_nt4_table[256] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 1, 4, 4, 4, 2,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

vector<out_loc_t> drive_sim(ifstream &ifs_read, ifstream &ifs_idx) {

    char *read_buffer = new char[READ_BUFFER_SIZE];
    ifs_read.read(read_buffer, READ_BUFFER_SIZE);
    if (!ifs_read.eof()) {
        fputs("Reading error: buffer too small\n", stderr);
        ifs_read.close();
        exit(3);
    }

    vector<out_loc_t> out;
    out_loc_t read{{0}, 0, NULL};

    index_t idx = parse_index(ifs_idx);

    size_t i = 0;
    base_t base_buffer[READ_LEN] = {0};
    unsigned char name_i = 0;

    for (;;) {
        char c = read_buffer[i];

        if (c == '@') {
            i++;
            c = read_buffer[i];
            name_i = 0;
            while (c != ' ') {
                read.name[name_i] = c;
                name_i++;
                i++;
                c = read_buffer[i];
            }
            read.name[name_i] = '\0';

            while (c != '\n') {
                i++;
                c = read_buffer[i];
            }

            for (size_t j = 0; j < READ_LEN; j++) {
                i++;
                base_buffer[j] = seq_nt4_table[(uint8_t)read_buffer[i]];
            }

            // DUT
            ap_uint<32> locs_buffer[OUT];
            ap_uint<OUT_LOG> locs_len;
            seeding(idx.h, idx.location, base_buffer, locs_buffer, locs_len);

            read.n = locs_len.to_uint();
            read.locs = new uint32_t[read.n];
            for (size_t j = 0; j < read.n; j++) {
                read.locs[j] = locs_buffer[j].to_uint();
            }

            out.push_back(read);

            i++;
            assert(read_buffer[i] == '\n');
            i++;
            assert(read_buffer[i] == '+');
            i = READ_LEN + i;
        } else if (c == 0) {
            break;
        }
        i++;
    }
    ifs_read.close();
    delete[] read_buffer;
    delete[] idx.h;
    delete[] idx.location;
    return out;
}

index_t parse_index(ifstream &ifs) {
    uint32_t n;
    ifs.read((char *)&n, sizeof(uint32_t));
    fprintf(stderr, "Info: Number of minimizers: %u\n", n);

    index_t idx;
    idx.h = new ap_uint<32>[n];
    uint32_t *h_buff = new uint32_t[n];
    ifs.read((char *)h_buff, n * sizeof(uint32_t));
    for (size_t i = 0; i < n; i++) {
        idx.h[i] = h_buff[i];
    }
    delete[] h_buff;
    size_t m = idx.h[n - 1];
    fprintf(stderr, "Info: Size of the location & strand arrays: %u\n", m);

    idx.location = new ap_uint<32>[m];
    uint32_t *l_buff = new uint32_t[m];
    ifs.read((char *)l_buff, m * sizeof(uint32_t));
    for (size_t i = 0; i < m; i++) {
        idx.location[i] = l_buff[i];
    }
    delete[] l_buff;

    // Check if we reached the EOF
    char eof;
    ifs.read(&eof, sizeof(char));
    if (!ifs.eof()) {
        fputs("Reading error: wrong file format\n", stderr);
        ifs.close();
        exit(3);
    }

    ifs.close();
    return idx;
}
