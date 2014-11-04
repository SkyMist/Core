#ifndef DEF_THRONEOFTHEFOURWINDS_H
#define DEF_THRONEOFTHEFOURWINDS_H

#define TotFWScriptName "instance_throne_of_the_four_winds"

enum Data
{
    DATA_CONCLAVE_OF_WIND,
    DATA_ALAKIR,
    DATA_ANSHAL,
    DATA_NEZIR,
    DATA_ROHASH,
};

enum Creatures
{
    NPC_ANSHAL  = 45870,
    NPC_NEZIR   = 45871,
    NPC_ROHASH  = 45872,
    NPC_ALAKIR  = 46753,
};

const Position startPos = {-266.834991f, 816.938049f, 200.0f, 0.0f};
const Position rohashPos = {-51.4635f, 576.25f, 200.1f, 1.51f}; // for tornado

#endif
