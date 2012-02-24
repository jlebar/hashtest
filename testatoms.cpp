#include "spooky.h"
#include "Murmurhash3.h"
#include "city.h"
#include <vector>
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

    fprintf(stderr, "%0.2fms elapsed\n", elapsed * 1000);
  }

private:
  timeval startTime;
};

enum hash_name {
  HASH_SPOOKYHASH,
  HASH_CITYHASH,
  HASH_MURMURHASH
};

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

  return -1;
}

template<hash_name hashfn>
void test(const char* filename)
{
  ifstream fin(filename);
  vector<string> lines;

  while (!fin.eof()) {
    string line;
    getline(fin, line);

    if (line.length() > 0) {
      lines.push_back(line);
    }
  }

  {
    AutoTimer timer;
    for (size_t i = 0; i < 1000; i++) {
      for (size_t j = 0; j < lines.size(); j++) {
        hash<hashfn>(lines[i]);
      }
    }
  }

  for (size_t i = 0; i < lines.size(); i++) {
    printf("'%s': %08x\n", lines[i].c_str(), hash<hashfn>(lines[i]));
  }
}

int main(int argc, const char **argv)
{
  if (argc != 3) {
    printf("Usage: %s (spookyhash|murmurhash|cityhash) FILE\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "spookyhash") == 0) {
    test<HASH_SPOOKYHASH>(argv[2]);
  }
  else if (strcmp(argv[1], "cityhash") == 0) {
    test<HASH_CITYHASH>(argv[2]);
  }
  else if (strcmp(argv[1], "murmurhash") == 0) {
    test<HASH_MURMURHASH>(argv[2]);
  }
  else {
    printf("Unknown hash name.\n");
    return 1;
  }

  return 0;
}
