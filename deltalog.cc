#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <memory>
#include "base_x.hh"

#define CHUNK_SIZE 1016
struct Chunk {
    uint16_t tail;
		uint32_t start;
    uint8_t values[CHUNK_SIZE] = {0};
};

bool try_add(Chunk &c, uint32_t diff) {
    uint16_t &p = c.tail;
		if (diff < 0xC0) {
      if (p >= CHUNK_SIZE - 1) {
        return false;
			}
      c.values[++p] = diff;
		} else if (diff < 0x3F00) {
      if (p >= CHUNK_SIZE - 2) {
        return false;
			}
      c.values[++p] = ((diff >> 8) & 0xFF) | 0xC0;
      c.values[++p] = diff & 0xFF;
		} else {
      if (p >= CHUNK_SIZE - 4) {
        return false;
			}
      c.values[++p] = 0xFF;
      c.values[++p] = (diff >> 16) & 0xFF;
      c.values[++p] = (diff >> 8) & 0xFF;
      c.values[++p] = diff & 0xFF;
		}
    return true;
}

uint32_t read_chunk(const Chunk &c, uint32_t *k) {
    uint16_t p = c.tail;
		uint32_t s = c.start;
		uint32_t i = 0;
    for (size_t j = 0; j <= p;) {
			uint8_t start_byte = c.values[j++];
			if (start_byte < 0xC0) {
				s = s + start_byte;
			} else if (start_byte < 0xFF) {
        s = s + ((start_byte & 0x3F) << 8) + c.values[j++];
			} else {
				s = s + ((c.values[j] << 16) | (c.values[j+1] << 8) | (c.values[j+2]));
				j += 3;
			}
      k[i++] = s;
    }
		return i;
}

class Container {
private:
    std::vector<std::unique_ptr<Chunk>> chunks;
    std::vector<uint32_t> min_values;
    uint32_t max_value = 0; // Maximum value in the container

public:
    void add(uint32_t x) {
        if (x <= max_value) return; // Only allow strictly increasing values
				uint32_t diff = x - max_value;	
        max_value = x;
        if (chunks.empty() || !try_add(*chunks.back(), diff)) {
						std::cout << "new chunk #" << chunks.size() << ": " << x << std::endl;
            Chunk* c = new Chunk();
						c->tail = 0;
						c->start = x;
            chunks.emplace_back(c);
						min_values.emplace_back(x);
        }
    }

    void get(uint32_t a, uint32_t *k) {
				auto it = std::lower_bound(min_values.begin(), min_values.end(), a);
        size_t i = (it == min_values.begin()) ? 0 : (it - min_values.begin() - 1);
        read_chunk(*chunks[i], k);
    }
};


int main() {
    Container c;
		int x = 10;
		for (int i = 0; i < 1300; i++) { 
						x += i;
						c.add(x);
		}

    uint32_t result[CHUNK_SIZE] = {0};
		int a;
		scanf("%d", &a);
    c.get(a, result);
    for (size_t i = 0; i < CHUNK_SIZE; i++) {
				auto s = Base36::base36().encode(((uint64_t) result[i]) << 27);
				auto t = Base36::base36().encode(((uint64_t) result[i] + 1) << 27);
        std::cout << s << ":" << t << "  ";
    }
    std::cout << std::endl;
    return 0;
}
