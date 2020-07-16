#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace stacktrace {

  struct entry {
    entry () : line(0) {}

    std::string file;     // filename
    size_t line;          // line number
    std::string function; // name of function or method

    // Serialize entry into a text string
    std::string to_string () const {
      std::ostringstream os;
      os << file << " (" << line << "): " << function;
      return os.str();
      }
    };

  // Stack-trace base class, for retrieving the current call-stack. */
  class call_stack {
  public:
    // param num_discard - number of stack entries to discard at the top
    call_stack (const size_t num_discard = 0);

    virtual ~call_stack () throw();

    // Serializes the entire call-stack into a text string
    std::string to_string () const {
      std::ostringstream os;
      for (size_t i = 0; i < stack.size(); i++)
        os << stack[i].to_string() << std::endl;
      return os.str();
      }

    // Call stack
    std::vector<entry> stack;
    };

  }
