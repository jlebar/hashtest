#include "spooky.h"
#include "Murmurhash3.h"
#include "city.h"
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <sys/time.h>

using namespace std;

typedef struct timespec timespec;

class AutoTimer
{
public:
  AutoTimer() {
    gettimeofday(&startTime, NULL);
  }

  ~AutoTimer() {
    timeval endTime;
    gettimeofday(&endTime, NULL);

    double elapsed = endTime.tv_sec  - startTime.tv_sec +
                     (endTime.tv_usec - startTime.tv_usec) / 1e6;

    printf("%0.2fms elapsed\n", elapsed * 1000);
  }

private:
  timeval startTime;
};

enum hash_name {
  HASH_SPOOKYHASH,
  HASH_CITYHASH,
  HASH_MURMURHASH,
  HASH_NSCRT
};

#define PR_ROTATE_LEFT32(a, bits) (((a) << (bits)) | ((a) >> (32 - (bits))))
#define ADD_TO_HASHVAL(hashval, c) \
    hashval = PR_ROTATE_LEFT32(hashval, 4) ^ (c);

uint32_t NSCrtHash(const char* start, size_t length)
{
  uint32_t h = 0;
  const char* s = start;
  const char* end = start + length;

  unsigned char c;
  while ( s < end ) {
    c = *s++;
    ADD_TO_HASHVAL(h, c);
  }

  return h;
}

template<hash_name hashfn>
inline uint32_t hash(const string& line)
{
  if (hashfn == HASH_SPOOKYHASH) {
    return SpookyHash::Hash32(line.c_str(), line.length(), 0);
  }
  else if (hashfn == HASH_CITYHASH) {
    uint64_t result = CityHash64(line.c_str(), line.length());
    return *(uint32_t*)(&result);
  }
  else if (hashfn == HASH_MURMURHASH) {
    uint32_t result[4];
    MurmurHash3_x64_128(line.c_str(), line.length(), 0, &result);
    return result[0];
  }
  else if (hashfn == HASH_NSCRT) {
    return NSCrtHash(line.c_str(), line.length());
  }

  return -1;
}

template<hash_name hashfn>
inline uint32_t wrappedhash(const string& line)
{
  switch(line.length()) {
  case 1:
    return line[0];
  case 2:
    return line[0] << 8 | line[1];
  case 3:
    return line[0] << 16 | line[1] << 8 | line[2];
  case 4:
    return line[0] << 24 | line [1] << 16 | line[2] << 8 | line[3];
  default:
    return hash<hashfn>(line);
  }
}

template<hash_name hashfn>
void test(const vector<string>& lines, const char* testname)
{
  printf("Testing %s\n", testname);

  {
    AutoTimer timer;
    for (size_t i = 0; i < 1000; i++) {
      for (size_t j = 0; j < lines.size(); j++) {
        wrappedhash<hashfn>(lines[i]);
      }
    }
  }

  map<uint32_t, uint32_t> hashes;
  for (size_t i = 0; i < lines.size(); i++) {
    uint32_t h = wrappedhash<hashfn>(lines[i]);
    hashes[h]++;
  }

  bool foundCollision = false;
  map<uint32_t, uint32_t> collisions;
  for (map<uint32_t, uint32_t>::iterator i = hashes.begin(); i != hashes.end(); ++i) {
    collisions[i->second]++;
    if (i->second > 1) {
      foundCollision = true;
    }
  }

  if (!foundCollision) {
    printf("No collisions\n");
  }
  else {
    for (map<uint32_t, uint32_t>::iterator i = collisions.begin(); i != collisions.end(); ++i) {
      if (i->first == 1) {
        printf("        unique: %6d\n", i->second);
      }
      else {
        printf("%3d collisions: %6d\n", i->first, i->second);
      }
    }
  }

  printf("\n");
}

int main(int argc, const char **argv)
{
  if (argc != 2) {
    printf("Usage: %s FILE\n", argv[0]);
    return 1;
  }

  const char* filename = argv[1];
  ifstream fin(filename);
  vector<string> lines;

  while (!fin.eof()) {
    string line;
    getline(fin, line);

    if (line.length() > 0) {
      lines.push_back(line);
    }
  }

  test<HASH_SPOOKYHASH>(lines, "spookyhash");
  test<HASH_CITYHASH>(lines, "cityhash");
  test<HASH_MURMURHASH>(lines, "murmurhash");
  test<HASH_NSCRT>(lines, "nscrt");

  return 0;
}
