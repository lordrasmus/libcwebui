
windowBits = 8
memLevel = 8

compressor = (1 << (windowBits + 2)) + (1 << (memLevel + 9))
decompressor = (1 << windowBits) + 1440 * 2 * 4
total = compressor + decompressor

print( total )
