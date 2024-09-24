#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <cstdint>

class Chunk;
class World;

class Structure {
public:
    static void generateBaseTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z);
};

#endif // STRUCTURES_H