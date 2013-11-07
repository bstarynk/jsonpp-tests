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
#include <argp.h>

#warning this code is very incomplete

std::ranlux24* my_randomg;


static const struct argp_option my_options[] =
{
  {
    .name = "random-seed",.key = 'r',.arg = "<seed-number>",
    .flags = 0,
    .doc = "seed for pseudo-random number generator, default is random"
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

void testwrite(std::string filename, unsigned sz)
{
	Json::Value jroot(Json::objectValue);
	const unsigned tablen = 16;
	Json::Value jtab[tablen];
	assert(my_randomg != nullptr);
	jroot["build"] = __DATE__;
	jroot["size"] = sz;
	while (sz-- > 0) {
		unsigned r = (*my_randomg)();
		Json::Value jv;
		bool composite = false;
		switch (r % 4) {
		case 0:
			jv = (int)(*my_randomg)() % 10000;
		    break;
		case 1:
			{
				char buf[4] = {0,0,0,0};
				buf[0] = 'a' + ((*my_randomg)() % 26);
				buf[1] = 'a' + ((*my_randomg)() % 26);
				if ((*my_randomg)() % 3) 
					buf[2] = '0' + ((*my_randomg)() % 10);
				jv = buf;
			}
			break;
		case 2:
			jv = Json::Value(Json::objectValue);
			jv["num"] = - (sz+1);
			composite = true;
			break;
		case 3:
			jv = Json::Value(Json::arrayValue);
			composite = true;
			break;
		}
		Json::Value &jcont = jtab[((*my_randomg)())%tablen];
		if (jcont.isArray()) 
			jcont.append(jv);
		else if (jcont.isObject()) 
		{
			char atbuf[8];
			snprintf(atbuf, sizeof(atbuf),"_%c%c%d",
					'A' + (*my_randomg)() % 26),
					'A' + (*my_randomg)() % 26),
					(int) (*my_randomg)() % 100));
			jcont[atbuf] = jv;
		}
		else {
		}
	}
	
}

int main(int argc, char**argv)
{
	static std::random_device _random_dev_;
}

error_t 
my_parse_options(int key, char* arg, struct argp_state*state)
{
	switch(key) {
		case 'R':
		break;
	}
}

// Compile command on my Linux/Debian/Sid/x86-64
////////////////////////////////////////////////////////////////
/**** for emacs
  ++ Local Variables: ++
  ++ compile-command: "g++ -std=c++11 -Wall -O -g  -I/usr/include/jsoncpp test-jsoncpp.cc -o test-jsoncpp -ljsoncpp" ++
  ++ End: ++
*/
