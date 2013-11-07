// (C) Copyright Basile Starynkevitch 2013
// this program is available and distributed under
// the GNU Lesser General Public License version 3 or more
// see LICENSE or COPYING3.LGPL files
// or http://www.gnu.org/licenses/lgpl-3.0.en.html

// test the jsoncpp library from http://jsoncpp.sourceforge.net/
// coded in C++11

#include <json/json.h>
#include <string>
#include <fstream>
#include <random>
#include <cassert>
#include <climits>
#include <argp.h>
#include <time.h>
#include <string.h>


std::ranlux24* my_randomgen;

static inline unsigned
my_random () {
  return ((*my_randomgen)() % (UINT_MAX/2));
}

bool my_debug = false;

#define DEBUG_AT(Fil,Lin,Out) do {if(my_debug) { std::clog << Fil \
	<< ":" << Lin << " " << Out << std::endl; }}while(0)
#define DEBUG(Out) DEBUG_AT(__FILE__,__LINE__,Out)

static const struct argp_option my_options[] =
{
  {
    .name = "random-seed",.key = 'R',.arg = "<seed-number>",
    .flags = 0,
    .doc = "seed for pseudo-random number generator, default is random"
  },
  {
    .name = "width",.key = 'W',.arg = "<width>",
    .flags = 0,
    .doc = "threshold for emission of random JSON"
  },
  {
    .name = "size",.key = 's',.arg = "<size>",
    .flags = 0,
    .doc = "size, i.e. number of generated JSON internal components"
  },
  {
    .name = "output",.key = 'o',.arg = "<output-file>",
    .flags = 0,
    .doc = "output file, or - for stdout"
  },
  {
    .name = "input",.key = 'i',.arg = "<input-file>",
    .flags = 0,
    .doc = "input file, or - for stdin"
  },
  {
    .name = "debug",.key = 'D',.arg = nullptr,
    .flags = 0,
    .doc = "debugging"
  },
  {
    .name = 0,.key = 0,.arg = 0,
    .flags = 0,.doc = "testing the JsonCPP library"
  },
  {0}
};

error_t my_parse_options(int key, char* arg, struct argp_state*state);

static const struct  argp my_argp =
{
  /*.options= */ my_options,
  /* .parser= */ my_parse_options,
  /* .args_doc= */ "# test-jsoncpp by Basile Starynkevitch\n ...",
  /* .children= */ NULL,
  /* .help_filter= */ NULL,
  /* .argp_domain= */ NULL
};


class Myargs {
public:
  unsigned my_width;
  unsigned my_size;
  std::string my_input;
  std::string my_output;
  Myargs() : my_width(2), my_size(100), my_input(), my_output() {};
  ~Myargs() {};
};

Json::Value
my_random_json(unsigned sz, unsigned width)
{
  assert (sz < INT_MAX);
  assert (width < SHRT_MAX && width >= 2);
  static const char* atstrings[] = {
    "one", "two", "three", "four", "five", "six", "seven",
    "eight", "nine", "ten", "blue", "green", "yellow", "red",
    "pink", "black", "sad", "happy", "big", "tiny"
  };
  Json::Value jroot(Json::objectValue);
  std::vector<Json::Value> jvect;
  jvect.resize(width);
  assert(my_randomgen != nullptr);
  /// build a random jroot JSON object
  jroot["build"] = __DATE__;
  jroot["size"] = sz;
  jroot["random"] = my_random() % 100000;
  while (sz-- > 0) {
    unsigned r = my_random() % 7;
    Json::Value jv;
    bool composite = false;
    DEBUG("sz=" << sz << " r=" << r);
    switch (r) {
    case 0:
      jv = Json::Value((int)my_random() % 10000);
      break;
    case 1:
    {
      char buf[4] = {0,0,0,0};
      buf[0] = 'a' + (my_random() % 26);
      buf[1] = 'a' + (my_random() % 26);
      if (my_random() % 3)
        buf[2] = '0' + (my_random() % 10);
      jv = Json::Value(buf);
    }
    break;
    case 2:
    case 3:
      jv = Json::Value(Json::objectValue);
      jv["num"] = Json::Value (Json::Int64(-((long)sz+1)));
      composite = true;
      break;
    case 4:
    case 5:
    {
      char buf [32];
      memset (buf, 0, sizeof(buf));
      snprintf(buf, sizeof(buf), "_%d_", sz);
      jv = Json::Value(Json::arrayValue);
      jv.append(Json::Value(buf));
      composite = true;
    }
    break;
    case 6:
      if (my_random() % 2)
        jv = Json::Value((bool) (my_random() % 2));
      break;
    }
    DEBUG("jv=" << jv);
    unsigned rix = (my_random())%width;
    Json::Value &jcont = jvect[rix];
    DEBUG("rix=" << rix << " old jcont=" << jcont);
    if (jcont.isArray())
      jcont.append(jv);
    else if (jcont.isObject())
    {
      char atbuf[8];
      snprintf(atbuf, sizeof(atbuf),"_%c%c%d",
               (int) ('A' + my_random() % 26),
               (int) ('A' + my_random() % 26),
               (int) my_random() % 100);
      jcont[atbuf] = jv;
    }
    else if (jcont.isNull()) {
      std::string ats = atstrings[my_random()
                                  % (sizeof(atstrings)/sizeof(atstrings[0]))];
      if (!jroot.isMember(ats))
        jroot[ats] = jv;
      DEBUG("ats=" << ats << " jv=" << jv);
    };
    if (composite) {
      std::string atr = atstrings[my_random()
                                  % (sizeof(atstrings)/sizeof(atstrings[0]))];
      if (!jroot.isMember(atr))
        jroot[atr] = jv;
      else if (jvect[rix].isNull() || my_random() % (4*width/3+1) == 0)
        jvect[rix] = jv;
    }
    if (my_random() % (2*width+1) == 0)
      jvect[my_random () % width] = Json::Value::null;
  }
  return jroot;
}

double my_time(clockid_t id)
{
  double r=0.0;
  struct timespec ts = {0,0};
  if (!clock_gettime(id, &ts)) {
    r = (double) ts.tv_sec + 1.0e-9 * ts.tv_nsec;
    return r;
  }
  else return 0.0;
}

int main(int argc, char**argv)
{
  static std::random_device _random_dev_;
  Myargs args;
  my_randomgen = new std::ranlux24(_random_dev_());
  argp_parse(&my_argp, argc, argv, 0, nullptr, &args);
  Json::Value jroot;
  if (args.my_size>0 && args.my_width>0) {
    fprintf(stderr, "computing random Json of size %d and width %d\n",
            (int)args.my_size, (int)args.my_width);
    double startreal = my_time(CLOCK_MONOTONIC);
    double startcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    jroot = my_random_json(args.my_size, args.my_width);
    double endreal = my_time(CLOCK_MONOTONIC);
    double endcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    fprintf(stderr, "computed random Json (size %d) in %.4f real %.4f cpu sec\n",
            args.my_size, endreal - startreal, endcpu - startcpu);
  }
  else if (!args.my_input.empty()) {
    Json::Reader rd;
    double startreal = my_time(CLOCK_MONOTONIC);
    double startcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    if (args.my_input == "-") {
      fprintf(stderr, "reading from standard input...\n");
      if (!rd.parse(std::cin, jroot, false))
        fprintf(stderr, "failed to read from stdin... %s\n",
                rd.getFormattedErrorMessages().c_str());
    }
    else {
      std::ifstream inp(args.my_input);
      fprintf(stderr, "reading from file %s...\n",
              args.my_input.c_str());
      if (!rd.parse(inp, jroot, false))
        fprintf(stderr, "failed to read from file %s... %s\n",
                args.my_input.c_str(),
                rd.getFormattedErrorMessages().c_str());
    };
    double endreal = my_time(CLOCK_MONOTONIC);
    double endcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    fprintf(stderr, "read input in %.4f real %.4f cpu sec\n",
            endreal - startreal, endcpu - startcpu);
  }
  if (!args.my_output.empty()) {
    Json::StyledStreamWriter wr(" ");
    double startreal = my_time(CLOCK_MONOTONIC);
    double startcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    if (args.my_output == "-") {
      fprintf(stderr, "writing to standard output...\n");
      wr.write(std::cout, jroot);
      std::cout << std::endl << std::flush;
    } else {
      fprintf(stderr, "writing to file %s...\n", args.my_output.c_str());
      std::ofstream out(args.my_output);
      wr.write(out, jroot);
      out << std::endl << std::flush;
    }
    double endreal = my_time(CLOCK_MONOTONIC);
    double endcpu = my_time(CLOCK_PROCESS_CPUTIME_ID);
    fprintf(stderr, "wrote output in %.4f real %.4f cpu sec\n",
            endreal - startreal, endcpu - startcpu);

  }

}


error_t
my_parse_options(int key, char* arg, struct argp_state*state)
{
  Myargs* myargs = (Myargs*)state->input;
  switch(key) {
  case 'D':
    my_debug = true;
    fprintf(stderr, "Debugging output...\n");
    fflush(NULL);
    break;
  case 'R': // random seed
    if (arg) {
      unsigned s = atoi(arg);
      fprintf(stderr, "seeding with %ud\n", s);
      delete my_randomgen;
      my_randomgen = new std::ranlux24(s);
    }
    break;
  case 'W': // width
    if (arg)
      myargs->my_width = atoi(arg);
    break;
  case 's': // size
    if (arg)
      myargs->my_size = atoi(arg);
    break;
  case 'i': // input
    if (arg)
      myargs->my_input = arg;
    break;
  case 'o': // output
    if (arg)
      myargs->my_output = arg;
    break;
  }
  return 0;
}

// Compile command on my Linux/Debian/Sid/x86-64
////////////////////////////////////////////////////////////////
/**** for emacs
  ++ Local Variables: ++
  ++ compile-command: "g++ -std=c++11 -Wall -O -g  -I/usr/include/jsoncpp test-jsoncpp.cc -o test-jsoncpp -ljsoncpp" ++
  ++ End: ++
*/
