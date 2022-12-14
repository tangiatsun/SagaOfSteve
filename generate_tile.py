# input list of colors as single dig numbers and letters
# array is one row of a list of colors (columns) at a time
# script doubles columns manually and mirrors each half (left/right) 

# 0: BLACK,     1: DARK_GREEN,  2: MED_GREEN,   3: GREEN,
# 4: DARK_BLUE, 5: BLUE,        6: LIGHT_BLUE,  7: CYAN,
# 8: RED,       9: DARK_ORANGE, A: ORANGE,      B: YELLOW, 
# C: NAGENTA,   D: PINK,        E: LIGHT_PINK,  F: WHITE
# Light pink has been removed in favor of a "transparent" color - which will do 
# nothing for now but can be used for different backgrounds for when the 
# character model doesn't take up the whole tile

tileDesign = [\
#   "xxxxxxxxxxxxxxxx"
    "9999999499999994",
    "9099909090999090",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9999999099999990",
    "9099909090999090",
    "9999999099999990",
    "4000000040000000"
 

]

# {0x11FFFFFFFFFFFFFF, 0xFFFFFFFF11111111},
#     {0x11FFEEFFFFFFFFFF, 0xFFFFFF1111111111},
#     {0x44FFEEFFFFFFFFFF, 0xFFFFFF4444444444},
#     {0x44EEEEFFFFFFFFFF, 0xFFFFFFFFEE77EEEE},
#     {0x44EE44FFFFFFFFFF, 0xFFFFFFFFEE00EEEE},
#     {0x444444FFFFFFFFFF, 0xDDFFFFFFEE00EEEE},
#     {0xEE111111FFFFFFFF, 0xDDFFFFEEEEEEEEEE},
#     {0x1111111111FFFFFF, 0xDDFFFFFFEEEEEE11},
#     {0x444444111111FFFF, 0xDDDDDDEEDDDD4444},
#     {0x444444111111FFFF, 0xDDDDDDEEEEEE4444},
#     {0x444444111111FFFF, 0xDDFFFFFF11111111},
#     {0x11111111111111FF, 0xDDFFFFFF44111111},
#     {0x11111111111111FF, 0xFFFFFFFFFF444444},
#     {0x1111111111114444, 0xFFFFFFFF44441111},
#     {0xFFFFFFFF444444FF, 0xFFFFFF444444FFFF},
#     {0xFFFFFF444444FFFF, 0xFFFF44444444FFFF}

# {0x8888888888FFFFFF,0xFFFFFF8888888888},
#     {0x88EEEEEEEE88FFFF,0xFFFF88EEEEEEEEEE},
#     {0xEEFFFFFFEEEE88FF,0xFF88EEEEEEEEEEFF},
#     {0xEEEEEEFFFFEEEE88,0x88EEEEEE88EEEEEE},
#     {0xEEEEEEFFFFEEEE88,0x8888EEEE88EEEEEE},
#     {0xEEEEEEEEFFFFEE88,0x8888EEEEEEEEEEEE},
#     {0xEEFFFFEEEEFFEE88,0x88EEEEEEEEEEEEEE},
#     {0xEEEEEEEEEEEEEE88,0x8888EEEEEEEEEEEE},
#     {0xEEEEEEEEEEEEEE88,0x8888EEEEEEEEEEEE},
#     {0xEEEEEEEEEEEEEE88,0x8888EEEEEEEEEEEE},
#     {0xEEEEEEEEEEEE88FF,0x8888EEEEEEEE88EE},
#     {0xEEEEEE88EEEE88FF,0xFF8888EEEEEE8888},
#     {0xEEEEEEEEEEEE88FF,0xFF8888EEEEEEEEEE},
#     {0xEEEEEEEEEEEE88FF,0xFF888888EEEEEEEE},
#     {0xEE8888EEEE88FFFF,0xFFFF888888888888},
#     {0x8888888888FFFFFF,0xFFFFFF8888888888}

# new_tile stores the array in C format row by row
new_tile= []

for i in range(16):
    new_tile.append("")

for row in range(len(tileDesign)):
    new_tile[row] += "{0x"
    this_row= [] # array stores current row duplicated data
    for col in range(len(tileDesign[row])//2 ):
        # duplicate the color
        # append to front of new row
        # paste
        # print(tileDesign[row][col], ", row: ", row, "col: ", col)
        this_row.append(tileDesign[row][col])
        this_row.append(tileDesign[row][col])

    this_row.reverse() # reverse the row's data
    # transfer data into output
    for char in this_row:
        new_tile[row] += char
    new_tile[row] += ",0x"
    this_row= []
    for col in range(len(tileDesign[row])//2 , len(tileDesign[row])):
        this_row.append(tileDesign[row][col])
        this_row.append(tileDesign[row][col])

    this_row.reverse()
    for char in this_row:
        new_tile[row] += char

    new_tile[row] += "},"

print("\n")

for row in range(len(new_tile)):
    print(new_tile[row])

print("\n -------------------\n")
mask= []

# GENERATE MASK
# use F for transparent

for i in range(16):
    mask.append("")

for row in range(len(tileDesign)):
    mask[row] += "{0b"
    this_row= []
    # first half
    for col in range(len(tileDesign[row]) //2):
        #if its F put 1. anything else put 0
        if tileDesign[row][col] == 'F':
            this_row.append("1")
            # mask[row] += "1"
        else:
            this_row.append("0")
            # mask[row] += "0"
    this_row.reverse()
    for char in this_row:
        mask[row] += char
    mask[row] += ",0b"
    this_row= []
    # second half
    for col in range(len(tileDesign[row]) //2, len(tileDesign[row])):
        #if its F put 1. anything else put 0
        if tileDesign[row][col] == 'F':
            this_row.append("1")
            # mask[row] += "1"
        else:
            this_row.append("0")
            # mask[row] += "0"
    this_row.reverse()
    for char in this_row:
        mask[row] += char
    mask[row] += "},"

# print
for row in range(len(mask)):
    print(mask[row])