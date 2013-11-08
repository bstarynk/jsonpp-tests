// (C) Copyright Basile Starynkevitch 2013
// this program is available and distributed under
// the GNU Lesser General Public License version 3 or more
// see LICENSE or COPYING3.LGPL files
// or http://www.gnu.org/licenses/lgpl-3.0.en.html

// test the jsoncpp library from http://jsoncpp.sourceforge.net/
// coded in C++11

#include <string>
#include <fstream>
#include <random>
#include <functional>
#include <list>
#include <vector>
#include <cassert>
#include <climits>
#include <argp.h>
#include <time.h>
#include <string.h>
#include <json/json.h>


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

std::string
my_random_attr(unsigned sz)
{
  static const char* atstrings[] = {
    "zero", "one", "two", "three", "four", "five", "six", "seven",
    "eight", "nine", "ten", "blue", "green", "yellow", "red",
    "pink", "black", "sad", "happy", "big", "tiny", "left", "right"
  };
  const unsigned nbat = sizeof(atstrings)/sizeof(atstrings[0]);
  assert (sz>2);
  unsigned r = my_random() % sz;
  if (r<nbat) return std::string(atstrings[r]);
  else {
    std::string s="_";
    while (r>0) {
      s.push_back('A' + r % 26);
      r = r/26;
    };
    return s;
  }
}

typedef std::function<bool(const Json::Value&)> action_t;

bool
do_random_action(std::vector<action_t>& vectact, const Json::Value&jv)
{
  unsigned width = vectact.size();
  for (unsigned cnt = width/4 + (my_random() % (width/2+1));
       cnt > 0;
       cnt--)
  { auto f = vectact[my_random() % width];
    if (f(jv)) return true;
  }
  return false;
}

Json::Value
my_random_json(unsigned sz, unsigned width)
{
  assert(my_randomgen != nullptr);
  assert (sz < INT_MAX);
  assert (width < SHRT_MAX && width >= 2);
  Json::Value jroot(Json::objectValue);
  // an action may change a value and return true, or do nothing and return false
#warning we should also keep the pointer on which we are doing the action...
  std::vector<action_t> actionvect;
  actionvect.reserve(width);
  /// build a random jroot JSON object
  jroot["@build"] = __DATE__"-" __TIME__;
  jroot["@size"] = sz;
  jroot["@width"] = width;
  jroot["@random"] = my_random() % 100000;
  // initialize the action vector to fill the root
  for (unsigned cnt=0; cnt<width; cnt++) {
    auto at = my_random_attr((5*width/4+1)|7);
    jroot[at] = Json::Value::null;
    actionvect.push_back([=,&jroot](const Json::Value& jv)
    { if (jroot[at].isNull())
      {
        jroot[at]=jv;
        return true;
      }
      else return false;
    });
  }
  while (sz > 0) {
    unsigned r = my_random() % 9;
    Json::Value jv;
    DEBUG("sz=" << sz << " r=" << r);
    switch (r) {
    case 0: // random number
      jv = Json::Value((int)my_random() % (10000+5*width));
      DEBUG("number jv=" << jv);
      if (do_random_action(actionvect,jv))
        sz--;
      break;
    case 1: // random small string
    {
      char buf[4] = {0,0,0,0};
      buf[0] = 'a' + (my_random() % 26);
      buf[1] = 'a' + (my_random() % 26);
      if (my_random() % 3)
        buf[2] = '0' + (my_random() % 10);
      jv = Json::Value(buf);
      DEBUG("string jv=" << jv);
      if (do_random_action(actionvect,jv))
        sz--;
    }
    break;
    case 2: // bigger random string, attribute-like
    {
      char nbuf[24];
      memset (nbuf, 0, sizeof(nbuf));
      snprintf (nbuf, sizeof(nbuf), "+%u", sz);
      std::string sv = "_" + my_random_attr(width+4) + nbuf;
      jv = Json::Value(sv);
      DEBUG("nice string jv=" << jv);
      if (do_random_action(actionvect,jv))
        sz--;
    }
    break;
    // more often, a random object
    case 3:
    case 4:
    case 5:
    {
      jv = Json::Value(Json::objectValue);
      jv["@num"] = Json::Value (Json::Int64(-((long)sz+1)));
      DEBUG("object jv=" << jv);
      std::list<std::pair<unsigned,action_t>> actlist;
      for (unsigned cnt = width/2 + 1 + my_random() % (width/4+1);
           cnt > 0;
           cnt--) {
        auto at = my_random_attr(4*width/3+1);
        jv[at] = Json::Value::null;
        actlist.push_back( {my_random() % width,
                            [=,&jv](const Json::Value& jcomp)
        { if (jv[at].isNull())
          {
            jv[at]=jcomp;
            DEBUG("objaction at=" << at << "; jv=" << jv);
            return true;
          }
          else return false;
        }
                           });
      }
      if (do_random_action(actionvect,jv))
        sz--;
      for (auto pr : actlist)
      {
        unsigned ix = pr.first;
        action_t act = pr.second;
        actionvect[ix] = act;
      };
    }
    break;
    // quite often, an array
    case 6:
    case 7:
    {
      char buf [32];
      memset (buf, 0, sizeof(buf));
      snprintf(buf, sizeof(buf), "_%d_", sz);
      jv = Json::Value(Json::arrayValue);
      jv.append(Json::Value(buf));
      DEBUG("array jv=" << jv);
      std::list<std::pair<unsigned,action_t>> actlist;
      for (unsigned cnt = width/2 + 1 + my_random() % (width/4+1);
           cnt > 0;
           cnt--) {
        actlist.push_back( {my_random() % width,
                            [=,&jv](const Json::Value& jcomp)
        {
          jv.append(jcomp);
          DEBUG("array action jv=" << jv);
          return true;
        }
                           });
      }
      if (do_random_action(actionvect,jv))
        sz--;
      for (auto pr : actlist)
      {
        unsigned ix = pr.first;
        action_t act = pr.second;
        actionvect[ix] = act;
      };
    }
    break;
    // perhaps a boolean or a null
    case 8:
      if (my_random() % 2)
        jv = Json::Value((bool) (my_random() % 2));
      DEBUG("atomic jv=" << jv);
      if (do_random_action(actionvect,jv))
        sz--;
      break;
    }
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
