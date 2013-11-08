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

// an action may change a container value and return true, or do nothing and return false
typedef std::function<bool(Json::Value&jcontainer, const Json::Value&jcomponent)> action_t;

class ToDo {
  static unsigned td_counter_;
  unsigned td_serial;
  Json::Value* td_cont;
  action_t td_action;
  friend class SetDo;
  ToDo(unsigned serial, Json::Value* cont, action_t act) :
    td_serial(serial), td_cont(cont), td_action(act) {};
public:
  ToDo (Json::Value* cont, action_t act) :
    td_serial(++td_counter_), td_cont(cont), td_action(act) {};
  ~ToDo () {
    td_serial=0;
    td_cont= nullptr;
  };
  ToDo (const ToDo&) = default;
  bool operator < (const ToDo &r) const
  {
    return td_serial < r.td_serial;
  };
  bool do_it(const Json::Value&jcomp) {
    if (td_cont) {
      bool done= td_action(*td_cont, jcomp);
      td_cont = nullptr;
      return done;
    } else return false;
  }
  unsigned serial() const {
    return td_serial;
  };
}; // end class ToDo

unsigned ToDo::td_counter_;

class SetDo {
  std::map<unsigned,ToDo> ts_map;
public:
  size_t size() const {
    return ts_map.size();
  };
  bool empty() const {
    return ts_map.empty();
  };
  bool has(unsigned ser) const {
    return ts_map.find(ser) != ts_map.end();
  };
  unsigned add_todo(Json::Value* val, action_t act)
  {
    unsigned ser = 1+my_random() % (size()+3);
    if (!has(ser)) {
      ToDo td(ser,val,act);
      ts_map.insert({ser,td});
      return ser;
    } else {
      ToDo td(val,act);
      unsigned serial = td.serial();
      ts_map.insert({serial,td});
      return serial;
    };
  };
  unsigned add_todo(ToDo td) {
    unsigned serial = td.serial();
    ts_map.insert({serial,td});
    return serial;
  };
  bool do_random(const Json::Value& v)
  {
    if (empty()) return false;
    unsigned lastser = ts_map.rbegin()->second.serial();
    assert (lastser>0);
    unsigned randser = my_random() % lastser;
    auto it = ts_map.lower_bound(randser);
    if (it == ts_map.end()) return false;
    ToDo td = it->second;
    ts_map.erase(it);
    return td.do_it(v);
  }
}; // end class SetDo

Json::Value
my_random_json(unsigned sz, unsigned width)
{
#error we should use pointers in this function .. ie new Json::Value everywhere....
  assert(my_randomgen != nullptr);
  assert (sz < INT_MAX);
  assert (width < SHRT_MAX && width >= 2);
  Json::Value jroot(Json::objectValue);
  SetDo tds;
  /// build a random jroot JSON object
  jroot["@build"] = __DATE__"-" __TIME__;
  jroot["@size"] = sz;
  jroot["@width"] = width;
  jroot["@random"] = my_random() % 100000;
  // initialize the todo set to fill the root
  for (unsigned cnt=0; cnt<=width; cnt++) {
    auto at = my_random_attr((5*width/4+1)|7);
    jroot[at] = Json::Value::null;
    tds.add_todo(&jroot,[=](Json::Value&job, const Json::Value&jcomp)
    { if (job[at].isNull())
      {
        job[at]=jcomp;
        return true;
      }
      else return false;
    });
  }
  while (sz > 0) {
    if (tds.empty()) {
      fprintf(stderr, "prematurely empty todo set, with %u remaining\n", sz);
      break;
    }
    unsigned r = my_random() % 9;
    DEBUG("sz=" << sz << " r=" << r);
    switch (r) {
    case 0: // random number
    {
      Json::Value jint((int)my_random() % (10000+5*width));
      DEBUG("number jint=" << jint);
      if (tds.do_random(jint))
        sz--;
    }
    break;
    case 1: // random small string
    {
      char buf[4] = {0,0,0,0};
      buf[0] = 'a' + (my_random() % 26);
      buf[1] = 'a' + (my_random() % 26);
      if (my_random() % 3)
        buf[2] = '0' + (my_random() % 10);
      Json::Value jnewstr(buf);
      DEBUG("string jnewstr=" << jnewstr);
      if (tds.do_random(jnewstr))
        sz--;
    }
    break;
    case 2: // bigger random string, attribute-like
    {
      char nbuf[24];
      memset (nbuf, 0, sizeof(nbuf));
      snprintf (nbuf, sizeof(nbuf), "+%u", sz);
      std::string sv = "_" + my_random_attr(width+4) + nbuf;
      Json::Value jstr(sv);
      DEBUG("nice string jstr=" << jstr);
      if (tds.do_random(jstr))
        sz--;
    }
    break;
    // more often, a random object
    case 3:
    case 4:
    case 5:
    {
      Json::Value jnewob(Json::objectValue);
      jnewob["@num"] = Json::Value (Json::Int64(-((long)sz+1)));
      DEBUG("object jnewob=" << jnewob);
      std::list<ToDo> todolist;
      for (unsigned cnt = width/2 + 1 + my_random() % (width/4+1);
           cnt > 0;
           cnt--) {
        auto at = my_random_attr(4*width/3+1);
        jnewob[at] = Json::Value::null;
        ToDo td(&jnewob,[=](Json::Value&job, const Json::Value&jcomp)
        { if (job[at].isNull())
          {
            job[at]=jcomp;
            DEBUG("objaction at=" << at << "; job=" << job);
            return true;
          }
          else return false;
        });
        todolist.push_back(td);
      }
      if (tds.do_random(jnewob))
        sz--;
      for (auto td : todolist)
        tds.add_todo(td);
    }
    break;
    // quite often, an array
    case 6:
    case 7:
    {
      char buf [32];
      memset (buf, 0, sizeof(buf));
      snprintf(buf, sizeof(buf), "_%d_", sz);
      Json::Value jnewarr(Json::arrayValue);
      jnewarr.append(Json::Value(buf));
      DEBUG("array jnewarr=" << jnewarr);
      std::list<ToDo> todolist;
      for (unsigned cnt = width/2 + 1 + my_random() % (width/4+1);
           cnt > 0;
           cnt--) {
        ToDo td(&jnewarr,[=](Json::Value&jarr, const Json::Value&jcomp)
        {
          jarr.append(jcomp);
          DEBUG("array action jarr=" << jarr);
          return true;
        }
               );
        todolist.push_back(td);
      }
      if (tds.do_random(jnewarr))
        sz--;
      for (auto td : todolist)
        tds.add_todo(td);
    }
    break;
    // perhaps a boolean or a null
    case 8:
    {
      Json::Value jatom;
      if (my_random() % 2)
        jatom = Json::Value((bool) (my_random() % 2));
      DEBUG("atomic jatom=" << jatom);
      if (tds.do_random(jatom))
        sz--;
    }
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
