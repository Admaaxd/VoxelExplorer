#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <cstdint>
#include <cmath>

class Chunk;
class World;

class Structure {
public:
    static void generateBaseTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z);

    static void generateBaseProceduralTree(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z);

    static void generateProceduralTreeOrangeLeaves(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z);

    static void generateProceduralTreeYellowLeaves(Chunk& chunk, uint8_t x, uint8_t y, uint8_t z);

};

#endif // STRUCTURES_H