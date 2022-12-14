
# {0xF1F1F1F1F1F1F1B0,0xF0F0F0F0F0F0F0F0},
#     {0xF1F1F1F1F1F1B0B0,0xF0F0F0F0F0F0F0F0},
#     {0xF1F1F1F1B0B0B0B0,0xF0F0F0F0F0F0F0F0},
#     {0xF1F1F1B0B0B0B0FB,0xB0B0B0B0B0B0B0F0},
#     {0xB0B0B0B0B0B0B0B0,0xB0B0FBB0B0B0B0F0},
#     {0xB0B0B0B0FBB0B0B0,0xB0B0B0B0B0B0FBF0},
#     {0xFBFBB0B0B0B0B0FB,0xB0B0B0B0B0FBFBF0},
#     {0xFBB0B0B0B0B0B0B0,0xB0F3F3F3F3F3F3F3},
#     {0xFBB0B0FBB0B0B0B0,0xF3F3F3F3F3F3F3F3},
#     {0xFBB0B0B0B0B0B0F3,0xF3F3F3F3F3F3F3F3},
#     {0xFBFBFBFBFBFBF3F3,0xF3F3F3F3F3F3F3F3}

# first char after x denotes monotone or not.
# 0xE(index of design) for design
# monotone: 0x(desired color)(anything)
# 16 col. 11 row


# mx indicates monotone color x
# dy indicates design y
# block_map[] = { &WALL_TILE[0][0], &TREE_TILE[0][0], &GRASS_TILE[0][0], &WATER_TILE[0][0] }
# design indices: 0=wall, 1=tree, 2=grass, 3=water, 4=bed, 5=stairs, 6=ladder
# 7=cave tile, 8=cave obj, 9=cave wall, A=red wall, B=tree2, C=boardwalk
levelDesign = [
    "d3d3d3d3d3mEm0mEmEmEmEmEmEmEmEmE",
    "d3d3d3d3d3d3mEmEmEmEmEmEmEmEmEmE",
    "d3d3d3d3d3d3d3mEmEmEmEmEmEmEmEmE",
    "d3d3d3d3d3d3d3d3mEmEmEmEmEmEmEmE",
    "d3d3d3d3d3d3d3d3mEmEmEmEmEmEmEmE",
    "d3d3d3d3d3d3d3d3d3mEmEmEmEmEmEmE",
    "mBd3d3d3d3d3d3d3d3d3mEmEmEmEmEmE",
    "mBmBdCdCdCdCdCdCdCdCmEmEmEmEmEmE",
    "mBmBdCdCdCdCdCdCdCdCmEmEmEmEmEmE",
    "mBd3d3d3d3d3d3d3d3mEmEmEmEmEmEmE",
    "d3d3d3d3d3d3d3d3d3d3mEmEmEmEmEmE"]

level_output = []

# init array to hold end result
for i in range(11):
    level_output.append("")


for row in range(len(levelDesign)):
    level_output[row] += "{0x"

    # first half of row
    for col in range(0, len(levelDesign[row]) // 2, 2):
        if levelDesign[row][col] == 'm':  # monotone ==> color + 0. use E for white mono
            level_output[row] += levelDesign[row][col+1] + "0"
        else:  # design ==> F + index
            level_output[row] += "F" + levelDesign[row][col+1]

    level_output[row] += ",0x"

    # second half of row
    for col in range(len(levelDesign[row])//2, len(levelDesign[row]), 2):
        if levelDesign[row][col] == 'm':
            level_output[row] += levelDesign[row][col+1] + "0"
        else:
            level_output[row] += "F" + levelDesign[row][col+1]

    level_output[row] += "},"

# print
for row in range(len(level_output)):
    print(level_output[row])
